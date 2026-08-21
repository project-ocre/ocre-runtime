/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/fs/fs.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/http/client.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>

#include <ocre/ocre.h>

#include "wifi_credentials.h"

/* Hardcoded for now; see also wifi_credentials.h. */
#define FETCH_SERVER_ADDR "192.168.50.56"
#define FETCH_SERVER_PORT 8080
#define FETCH_URL         "/getCode"
#define FETCH_HASH_URL    "/getHash"

#define FETCH_INTERVAL       K_SECONDS(20)
#define FETCH_RECV_BUF_LEN   2048
#define FETCH_MAX_BODY_LEN   (512 * 1024)
#define FETCH_TIMEOUT_MS     5000
#define FETCH_HASH_BUF_LEN   80 /* sha256 hex digest (64 chars) plus slack */

/* This board's WiFi intermittently fails an outbound connection or times out
 * mid-request -- confirmed to not be a lost IP/interface-down condition (the
 * interface stays up with a valid address throughout) and not limited to any
 * particular operation, so it looks like general RF-level flakiness.
 * Retrying rides it out instead of giving up on the first hiccup. */
#define FETCH_CODE_MAX_ATTEMPTS   5
#define FETCH_CODE_RETRY_DELAY_MS 2000

#define FETCH_HASH_MAX_ATTEMPTS   3
#define FETCH_HASH_RETRY_DELAY_MS 1000

#define CONTAINER_IMAGE     "current.wasm"
#define CONTAINER_IMAGE_TMP "current.wasm.new"
#define CONTAINER_HASH_FILE "current.hash"
#define CONTAINER_ID        "fetched"

/* Board's built-in LED, driven directly here (not through ocre_gpio) so it's
 * available before any container exists. No-op on a board that doesn't
 * define a "led0" alias. */
#if DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)
#define HAVE_BUILTIN_LED 1
static const struct gpio_dt_spec led0_spec = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#else
#define HAVE_BUILTIN_LED 0
#endif

static struct net_mgmt_event_callback wifi_mgmt_cb;
static volatile bool wifi_connected;

static struct ocre_context *g_ctx;
static struct ocre_container *g_container;

/* The hash of whatever code is currently installed/running, so an update
 * check only needs to compare a short string (fetched from /getHash)
 * instead of holding the whole image in RAM just for comparison -- see
 * check_for_update(). Empty until either a cached image's hash file is
 * read at boot, or a fetch successfully installs one. */
static char g_current_hash[FETCH_HASH_BUF_LEN];

/* Used by fetch_hash(): the digest is tiny, so this just fills a fixed
 * buffer directly -- no heap growth needed. */
struct hash_fetch_state {
	char buf[FETCH_HASH_BUF_LEN];
	size_t len;
	bool failed;
};

/* Used by fetch_code_to_file(): each fragment is written straight to an
 * already-open file as it arrives, so downloading a new image needs no
 * heap buffer at all -- see the comment on fetch_code_to_file(). */
struct code_fetch_state {
	int fd;
	size_t len;
	bool failed;
};

/* Blinks the built-in LED a few times as a purely visual "the fetch sample
 * just (re)started" signal -- driven directly, independent of whatever the
 * running container's own AssemblyScript code is doing with the same pin via
 * ocre_gpio, so it works even before any container exists. Left off
 * afterward: from that point on the pin is the container's to control (see
 * gpio-demo's onRequest(), which drives it through ocre_gpio_set_by_name()).
 * No-op on a board without a "led0" alias. */
static void native_led_blink(int times)
{
#if HAVE_BUILTIN_LED
	if (!gpio_is_ready_dt(&led0_spec)) {
		return;
	}

	gpio_pin_configure_dt(&led0_spec, GPIO_OUTPUT_INACTIVE);

	for (int i = 0; i < times; i++) {
		gpio_pin_set_dt(&led0_spec, 1);
		k_sleep(K_MSEC(150));
		gpio_pin_set_dt(&led0_spec, 0);
		k_sleep(K_MSEC(150));
	}
#else
	ARG_UNUSED(times);
#endif
}

/* WiFi power-save (modem sleep) periodically puts the radio to sleep between
 * beacons; if it ever fails to wake/resync correctly, the station stays
 * associated (no disconnect event fires) but stops passing traffic in either
 * direction until the device is reset. Explicitly disabling it removes that
 * failure mode. Re-issued on every successful connect (not just the first),
 * since a reconnect could otherwise come back up with power-save re-enabled. */
static void wifi_disable_power_save(struct net_if *iface)
{
	struct wifi_ps_params params = {
		.enabled = WIFI_PS_DISABLED,
	};

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		printk("Failed to disable WiFi power save\n");
	} else {
		printk("WiFi power save disabled\n");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				     struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		const struct wifi_status *status = (const struct wifi_status *)cb->info;

		if (status->status) {
			printk("WiFi connect request failed (%d)\n", status->status);
		} else {
			printk("WiFi connected\n");
			wifi_connected = true;
			wifi_disable_power_save(iface);
		}
		break;
	}
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		printk("WiFi disconnected\n");
		wifi_connected = false;
		break;
	default:
		break;
	}
}

static void wifi_connect(void)
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params params;

	if (!iface) {
		printk("No default network interface yet\n");
		return;
	}

	memset(&params, 0, sizeof(params));
	params.ssid = FETCH_WIFI_SSID;
	params.ssid_length = strlen(FETCH_WIFI_SSID);
	params.psk = FETCH_WIFI_PSK;
	params.psk_length = strlen(FETCH_WIFI_PSK);
	params.security = WIFI_SECURITY_TYPE_PSK;
	params.channel = WIFI_CHANNEL_ANY;

	printk("Connecting to WiFi SSID '%s'...\n", FETCH_WIFI_SSID);

	int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));

	if (ret) {
		printk("WiFi connect request failed: %d\n", ret);
	}
}

/* Opens a socket, issues a GET for url, and hands response fragments to cb.
 * Shared by fetch_hash() and fetch_code_to_file() -- they differ only in
 * how they handle those fragments (accumulate a few bytes vs. stream
 * straight to flash). Returns 0 on success (check user_data's own
 * failed/len fields), a negative errno on a connection-level failure. */
static int do_http_get(const char *url, http_response_cb_t cb, void *user_data)
{
	struct sockaddr_in addr;
	struct http_request req;
	static uint8_t recv_buf[FETCH_RECV_BUF_LEN];
	int sock;
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(FETCH_SERVER_PORT);

	if (inet_pton(AF_INET, FETCH_SERVER_ADDR, &addr.sin_addr) != 1) {
		return -EINVAL;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		printk("do_http_get(%s): socket() failed: errno=%d\n", url, errno);
		return -errno;
	}

	ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		ret = -errno;
		printk("do_http_get(%s): connect() failed: errno=%d\n", url, -ret);

		/* Temporary diagnostic: check whether the interface itself
		 * still thinks it's up and holds an IPv4 address at the
		 * moment connect() fails, to tell a real interface-level
		 * outage apart from a purely socket/routing-layer issue. */
		struct net_if *diag_iface = net_if_get_default();

		if (diag_iface) {
			struct net_in_addr *diag_ip =
				net_if_ipv4_get_global_addr(diag_iface, NET_ADDR_PREFERRED);
			char diag_ip_buf[NET_IPV4_ADDR_LEN];

			printk("  iface %p: up=%d, ipv4=%s\n", diag_iface,
			       net_if_flag_is_set(diag_iface, NET_IF_UP),
			       diag_ip ? net_addr_ntop(AF_INET, diag_ip, diag_ip_buf,
							sizeof(diag_ip_buf))
				       : "none");
		} else {
			printk("  no default iface\n");
		}

		close(sock);
		return ret;
	}

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = url;
	req.host = FETCH_SERVER_ADDR;
	req.protocol = "HTTP/1.1";
	req.response = cb;
	req.recv_buf = recv_buf;
	req.recv_buf_len = sizeof(recv_buf);

	ret = http_client_req(sock, &req, FETCH_TIMEOUT_MS, user_data);
	if (ret < 0) {
		printk("do_http_get(%s): http_client_req() failed: %d\n", url, ret);
	}

	close(sock);

	return ret;
}

static int hash_response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
	struct hash_fetch_state *st = user_data;

	ARG_UNUSED(final_data);

	if (rsp->http_status_code != 0 && rsp->http_status_code != 200) {
		printk("Unexpected HTTP status %u\n", rsp->http_status_code);
		st->failed = true;
		return -1;
	}

	if (rsp->body_frag_len == 0) {
		return 0;
	}

	/* The digest is a handful of bytes; just take what fits and drop the
	 * rest rather than growing anything. */
	size_t space = sizeof(st->buf) - 1 - st->len;
	size_t copy_len = rsp->body_frag_len < space ? rsp->body_frag_len : space;

	memcpy(st->buf + st->len, rsp->body_frag_start, copy_len);
	st->len += copy_len;
	st->buf[st->len] = '\0';

	return 0;
}

/* Fetches the sha256 hex digest of the code currently served at FETCH_URL.
 * Used instead of fetching the code itself for the once-a-minute check --
 * a few dozen bytes instead of the whole image, so containers only ever
 * get stopped and re-fetched when something actually changed. */
static int fetch_hash(char *out_hash, size_t out_hash_size)
{
	struct hash_fetch_state st;
	int ret;

	memset(&st, 0, sizeof(st));

	ret = do_http_get(FETCH_HASH_URL, hash_response_cb, &st);
	if (ret < 0) {
		return ret;
	}

	if (st.failed || st.len == 0) {
		return -EIO;
	}

	/* Trim any trailing newline/whitespace a server might send. */
	while (st.len > 0 && (st.buf[st.len - 1] == '\n' || st.buf[st.len - 1] == '\r' ||
			      st.buf[st.len - 1] == ' ')) {
		st.buf[--st.len] = '\0';
	}

	strncpy(out_hash, st.buf, out_hash_size - 1);
	out_hash[out_hash_size - 1] = '\0';
	return 0;
}

static int code_response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
	struct code_fetch_state *st = user_data;

	ARG_UNUSED(final_data);

	if (rsp->http_status_code != 0 && rsp->http_status_code != 200) {
		printk("Unexpected HTTP status %u\n", rsp->http_status_code);
		st->failed = true;
		return -1;
	}

	if (rsp->body_frag_len == 0) {
		return 0;
	}

	if (st->len + rsp->body_frag_len > FETCH_MAX_BODY_LEN) {
		printk("Fetched body exceeds max size (%d bytes)\n", FETCH_MAX_BODY_LEN);
		st->failed = true;
		return -1;
	}

	/* Written straight to flash as each fragment arrives -- no heap
	 * buffer to grow, and nothing here scales with the image size. This
	 * is plain POSIX file I/O and Zephyr's own streaming HTTP client
	 * callback, so it's portable to any board with a filesystem, not an
	 * ESP32-specific trick. */
	ssize_t written = write(st->fd, rsp->body_frag_start, rsp->body_frag_len);

	if (written != (ssize_t)rsp->body_frag_len) {
		printk("Failed to write fetched data to flash: errno=%d\n", errno);
		st->failed = true;
		return -1;
	}

	st->len += rsp->body_frag_len;

	return 0;
}

/* Streams the code currently served at FETCH_URL directly into tmp_path on
 * flash, without ever holding the full image in RAM. Returns 0 on success
 * (with *out_len set), a negative errno otherwise -- the caller is
 * responsible for removing a partial tmp_path on failure. */
static int fetch_code_to_file(const char *tmp_path, size_t *out_len)
{
	struct code_fetch_state st;
	int ret;

	memset(&st, 0, sizeof(st));

	st.fd = open(tmp_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (st.fd < 0) {
		printk("Failed to open '%s' for the incoming image: errno=%d\n", tmp_path, errno);
		return -errno;
	}

	ret = do_http_get(FETCH_URL, code_response_cb, &st);

	close(st.fd);

	if (ret < 0) {
		printk("do_http_get(%s) failed: %d\n", FETCH_URL, ret);
		return ret;
	}

	if (st.failed || st.len == 0) {
		return -EIO;
	}

	*out_len = st.len;
	return 0;
}

static void save_hash_file(const char *workdir)
{
	char path[128];
	int fd;

	snprintf(path, sizeof(path), "%s/images/%s", workdir, CONTAINER_HASH_FILE);

	fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0) {
		printk("Failed to persist hash file: errno=%d\n", errno);
		return;
	}

	write(fd, g_current_hash, strlen(g_current_hash));
	close(fd);
}

/* Populates g_current_hash from a previous run's hash file, if any. Left
 * empty (meaning "unknown") if the file doesn't exist yet -- the first
 * update check will then always treat the fetched hash as different and
 * self-heal by installing whatever is currently served, recording its
 * hash for next time. */
static void load_hash_file(const char *workdir)
{
	char path[128];
	int fd;

	snprintf(path, sizeof(path), "%s/images/%s", workdir, CONTAINER_HASH_FILE);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return;
	}

	ssize_t r = read(fd, g_current_hash, sizeof(g_current_hash) - 1);

	close(fd);

	if (r > 0) {
		g_current_hash[r] = '\0';
	}
}

/* Temporary diagnostic: not declared in a public header. */
extern int malloc_runtime_stats_get(struct sys_memory_stats *stats);

static void print_heap_stats(const char *label)
{
	struct sys_memory_stats stats;

	if (malloc_runtime_stats_get(&stats) == 0) {
		printk("[heap %s] free=%zu allocated=%zu max_allocated=%zu\n", label, stats.free_bytes,
		       stats.allocated_bytes, stats.max_allocated_bytes);
	}
}

/* Feeds `ocre-as stats`: a machine-readable line per memory type, easy to
 * pick out of the interleaved log/container output on the same serial
 * connection. Runs continuously (its own thread, independent of WiFi/
 * container state) so it's useful for watching memory even before the first
 * container starts. */
static void print_ocre_stats_line(const char *type, size_t total, size_t used)
{
	printk("OCRE_STATS,%s,%zu,%zu\n", type, total, used);
}

static void stats_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		struct sys_memory_stats heap_stats;

		if (malloc_runtime_stats_get(&heap_stats) == 0) {
			print_ocre_stats_line("heap", heap_stats.free_bytes + heap_stats.allocated_bytes,
					      heap_stats.allocated_bytes);
		}

		struct fs_statvfs flash_stats;

		/* "/lfs" is this board's fixed LittleFS mount point (see
		 * fstab.overlay) -- independent of ocre_context_get_working_directory(),
		 * which is just a subdirectory under it, so this works even
		 * before ocre_create_context() has run. */
		if (fs_statvfs("/lfs", &flash_stats) == 0) {
			size_t total = (size_t)flash_stats.f_blocks * flash_stats.f_frsize;
			size_t free_bytes = (size_t)flash_stats.f_bfree * flash_stats.f_frsize;

			print_ocre_stats_line("flash", total, total - free_bytes);
		}

		k_sleep(K_SECONDS(1));
	}
}

K_THREAD_DEFINE(ocre_stats_tid, 2048, stats_thread_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, 0);

static void start_container(void)
{
	print_heap_stats("before create");

	/* "ocre:api" is what makes wamr.c register this module's custom data
	 * and, on teardown, actually run ocre_cleanup_module_resources() --
	 * without it, per-container resource cleanup handlers (e.g. GPIO's,
	 * which frees any pin registrations still owned by the outgoing
	 * module) never run at all. */
	static const char *capabilities[] = {"ocre:api", NULL};
	static const struct ocre_container_args args = {.capabilities = capabilities};

	g_container = ocre_context_create_container(g_ctx, CONTAINER_IMAGE, "wamr/wasip1", CONTAINER_ID, true, &args,
						     STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);
	if (!g_container) {
		printk("Failed to create container from fetched image\n");
		return;
	}

	if (ocre_container_start(g_container)) {
		printk("Failed to start container\n");
		ocre_context_remove_container(g_ctx, g_container);
		g_container = NULL;
	}
}

/* Checks whether the code served at FETCH_URL has changed, and if so,
 * installs it and reboots the whole board to run it. Only ever fetches the
 * (small) hash first; the (potentially much larger) code itself is fetched
 * -- streamed straight to flash, never fully buffered in RAM -- only when
 * that hash differs from g_current_hash.
 *
 * The currently running container is left alone throughout: it isn't
 * stopped before the download (streaming to flash needs only a couple KB of
 * scratch, not the whole image) and it isn't restarted in place afterward
 * either -- once the new image is safely installed, the board reboots, and
 * the normal boot-time path (load_cached_image() + start_container() in
 * main()) picks up the new code fresh, with a clean WiFi/network stack.
 * That sidesteps needing any in-process container-swap machinery here at
 * all, at the cost of a brief full restart for every update. */
static void check_for_update(const char *workdir)
{
	char fetched_hash[FETCH_HASH_BUF_LEN];
	char tmp_path[128];
	char final_path[128];
	size_t new_len = 0;
	int ret;

	if (!wifi_connected) {
		printk("Skipping update check: WiFi not connected\n");
		return;
	}

	for (int attempt = 1; attempt <= FETCH_HASH_MAX_ATTEMPTS; attempt++) {
		ret = fetch_hash(fetched_hash, sizeof(fetched_hash));
		if (ret == 0) {
			break;
		}

		printk("Fetch from " FETCH_SERVER_ADDR ":%d" FETCH_HASH_URL " failed (%d), attempt %d/%d\n",
		       FETCH_SERVER_PORT, ret, attempt, FETCH_HASH_MAX_ATTEMPTS);

		if (attempt < FETCH_HASH_MAX_ATTEMPTS) {
			k_sleep(K_MSEC(FETCH_HASH_RETRY_DELAY_MS));
		}
	}

	if (ret) {
		printk("Giving up on hash check after %d attempt(s); keeping current code\n",
		       FETCH_HASH_MAX_ATTEMPTS);
		return;
	}

	if (g_current_hash[0] != '\0' && strcmp(g_current_hash, fetched_hash) == 0) {
		/* Unchanged -- nothing to do, and nothing worth logging every
		 * minute. */
		return;
	}

	printk("New code hash detected (%s); fetching update\n", fetched_hash);

	snprintf(tmp_path, sizeof(tmp_path), "%s/images/%s", workdir, CONTAINER_IMAGE_TMP);

	for (int attempt = 1; attempt <= FETCH_CODE_MAX_ATTEMPTS; attempt++) {
		ret = fetch_code_to_file(tmp_path, &new_len);
		if (ret == 0) {
			break;
		}

		printk("Fetch from " FETCH_SERVER_ADDR ":%d" FETCH_URL " failed (%d), attempt %d/%d\n",
		       FETCH_SERVER_PORT, ret, attempt, FETCH_CODE_MAX_ATTEMPTS);

		if (attempt < FETCH_CODE_MAX_ATTEMPTS) {
			k_sleep(K_MSEC(FETCH_CODE_RETRY_DELAY_MS));
		}
	}

	if (ret) {
		printk("Giving up after %d attempt(s); keeping current code\n", FETCH_CODE_MAX_ATTEMPTS);
		unlink(tmp_path);
		return;
	}

	snprintf(final_path, sizeof(final_path), "%s/images/%s", workdir, CONTAINER_IMAGE);

	if (rename(tmp_path, final_path) != 0) {
		printk("Failed to install fetched image: errno=%d; keeping current code\n", errno);
		unlink(tmp_path);
		return;
	}

	strncpy(g_current_hash, fetched_hash, sizeof(g_current_hash) - 1);
	g_current_hash[sizeof(g_current_hash) - 1] = '\0';
	save_hash_file(workdir);

	printk("New code installed (%zu bytes); rebooting to run it\n", new_len);

	/* Give the log line above (and the hash file write) a moment to
	 * actually flush before the reset. */
	k_sleep(K_MSEC(100));

	native_led_blink(3);

	sys_reboot(SYS_REBOOT_COLD);
}

/* If a previously fetched image is cached on flash, records its known hash
 * (if any) and reports that it's ready to run -- the caller starts it.
 * Nothing here loads the image itself into RAM; ocre_context_create_container()
 * reads it directly from flash when actually instantiating the container. */
static bool load_cached_image(const char *workdir)
{
	char path[128];
	struct stat st;

	snprintf(path, sizeof(path), "%s/images/%s", workdir, CONTAINER_IMAGE);

	if (stat(path, &st) != 0) {
		return false;
	}

	load_hash_file(workdir);

	printk("Found cached image from a previous fetch (%zu bytes); starting it\n", (size_t)st.st_size);
	return true;
}

int main(void)
{
	int rc = ocre_initialize(NULL);

	if (rc) {
		fprintf(stderr, "Failed to initialize runtimes\n");
		return 1;
	}

	g_ctx = ocre_create_context(NULL);
	if (!g_ctx) {
		fprintf(stderr, "Failed to create ocre context\n");
		return 1;
	}

	const char *workdir = ocre_context_get_working_directory(g_ctx);

	if (!workdir) {
		fprintf(stderr, "Failed to get working directory\n");
		return 1;
	}

	/* Visual "the fetch sample just started" signal -- same on a fresh
	 * power-on as after an update-triggered reboot. Left off afterward
	 * for the container's own AssemblyScript to control. */
	native_led_blink(3);

	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler,
				      NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_mgmt_cb);

	wifi_connect();

	/* Run whatever code we already have cached from a previous successful
	 * fetch, without waiting for the network to come up. */
	if (load_cached_image(workdir)) {
		start_container();
	} else {
		printk("No cached image yet; waiting for the first successful fetch\n");
	}

	/* Give WiFi association + DHCP a head start before the first fetch. */
	k_sleep(K_SECONDS(5));

	check_for_update(workdir);

	while (1) {
		k_sleep(FETCH_INTERVAL);

		if (!wifi_connected) {
			wifi_connect();
		}

		check_for_update(workdir);
	}

	return 0;
}
