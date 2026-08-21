<!-- @copyright Copyright (c) contributors to Project Ocre,
which has been established as Project Ocre a Series of LF Projects, LLC

SPDX-License-Identifier: Apache-2.0 -->

# fetch sample

This sample connects to WiFi on startup, keeps a fetched container running
reactively (background `loop()` plus an HTTP server dispatching to
`onRequest()`), and periodically checks a URL on your machine for updated
code -- fetching and installing it, then rebooting to run it, only when it's
actually changed. If nothing is reachable (no WiFi, server down, no new
code), it just keeps running whatever it already has cached from a previous
successful fetch, including across reboots.

This sample -- and a set of runtime changes it depends on (below) -- is what
this fork adds on top of upstream [Ocre](https://github.com/project-ocre/ocre-runtime).
It's meant to be paired with the `ocre-as` CLI (a separate repo): write an
AssemblyScript container (see the `gpio-demo` repo for a full example),
`ocre-as build` it, `ocre-as host` it, and this sample fetches and runs it.

The application logic (`main.c`) is written entirely against portable Zephyr
networking APIs (`net_mgmt`, POSIX sockets, `http_client`, `http_server`) --
nothing ESP32-specific. Only the WiFi *driver* Kconfig differs per board, in
`zephyr/boards/<board>.conf`.

## Configuration

Currently hardcoded (see the source for where to change these before using
this sample outside of local development):

- WiFi SSID/password: `wifi_credentials.h` (create it if missing)
- Fetch server: `FETCH_SERVER_ADDR` / `FETCH_SERVER_PORT` in `main.c` --
  must be your dev machine's LAN IP and whatever port `ocre-as host` serves on
  (default 8080)

## Building and running

```sh
west build -p always -b xiao_esp32c6/esp32c6/hpcore src/samples/fetch/zephyr
west flash
```

Tested end to end (WiFi, fetch, GPIO, ADC, HTTP, outbound HTTP) on the Seeed
XIAO ESP32-C6. Only that board has a board-specific WiFi driver `.conf` today
(`zephyr/boards/xiao_esp32c6_esp32c6_hpcore.conf`, which just sets
`CONFIG_WIFI_ESP32=y`). Porting to another WiFi-capable board should only
require an equivalent `boards/<board>.conf` -- `main.c` itself needs no
changes. GPIO/ADC support on a new board additionally needs that board's
`led0`/`sw0`-style devicetree aliases (usually already defined) and a
`boards/<board>.overlay` with a `zephyr,user` `io-channels` mapping for ADC
(see `boards/xiao_esp32c6_esp32c6_hpcore.overlay`).

Watch it boot over serial (macOS port names look like `/dev/cu.usbmodem*`,
and change across reflashes/resets -- re-run `ls /dev/cu.usbmodem*` if a
previously-working path stops connecting):

```sh
ls /dev/cu.usbmodem*
screen /dev/cu.usbmodem101 115200   # Ctrl-A then k to exit
```

## Behavior

- On boot: if a previously fetched image is cached
  (`/lfs/ocre/images/current.wasm`), it starts running immediately, without
  waiting on the network. The board's built-in LED blinks 3x at this point
  (see "LED blink" below).
- Also on boot (after a short delay for WiFi association + DHCP) and then
  every ~20 seconds: fetches `/getHash` (a sha256 digest of whatever
  `/getCode` currently serves -- a few dozen bytes) and compares it to the
  hash of the code currently installed.
  - Unchanged: nothing happens, no log spam.
  - Changed: `/getCode` is fetched, streamed straight to a temp file on
    flash (never buffered whole in RAM), and renamed over the installed
    image on success. The LED blinks 3x, then the board reboots
    (`sys_reboot(SYS_REBOOT_COLD)`) to run it -- see "Why reboot" below.
  - Any failure (no WiFi, connection refused, timeout, non-200 status):
    the currently running container is left completely untouched, and a
    few retries with a short delay are attempted before giving up until the
    next periodic check.

## What this fork changes/adds on top of upstream Ocre

- **Reactive execution model** (`src/runtime/wamr-wasip1/wamr.c`,
  `ocre_api/ocre_dispatch/`): a container's `main()` runs once; if it also
  exports `loop` and/or `onRequest`, the native side keeps the instance alive
  afterward and calls those exports on its own schedule instead of tearing
  it down -- `loop()` on a timer (`CONFIG_OCRE_LOOP_INTERVAL_MS`, default
  100ms), `onRequest()` per HTTP request, serialized through a mutex since
  only one thread may execute inside a given WASM instance at a time.
- **Generic HTTP request/response bridge** (`ocre_api/ocre_http/`): a single
  native fallback HTTP resource catches every request regardless of
  path/method and dispatches it to the active container's
  `onRequest(requestId)` -- all routing lives in container code.
- **Outbound HTTP client** (`ocre_api/ocre_http_client/`, new
  `CONFIG_OCRE_HTTP_CLIENT`): `ocre_http_get(host, port, path)`, the
  counterpart to the inbound bridge, for container code that wants to make
  its own HTTP requests. Demonstrated in `gpio-demo` by polling `/getHash`
  from `loop()` every ~10s.
- **ADC support** (`ocre_api/ocre_adc/`, new): named channels ("a0".."a3")
  resolved via the same `zephyr,user` `io-channels` devicetree convention
  Zephyr's own `samples/drivers/adc/adc_dt` uses, pre-scaled to 0-255.
- **`ocre_print`** (`ocre_api/ocre_print/`, new): serial print + backs
  AssemblyScript's required `env.abort` runtime hook.
- **Hash-based update checks streamed to flash** (`main.c`,
  `ocre-as/lib/host.js`): see "Behavior" above -- this replaced fetching and
  diffing the whole image in RAM on every check.
- **Updates apply via a full reboot, not an in-place container swap**
  (`main.c`, `check_for_update()`): chosen after extensive investigation into
  an intermittent full bidirectional WiFi stall (station stays associated,
  no disconnect event fires, but no traffic passes in either direction until
  a reset) that recurred regardless of in-process container-swap machinery --
  interface/IP loss, WiFi power-save, container-teardown CPU cost, and
  connection-pool exhaustion were all ruled out as the direct cause via
  targeted hardware testing. Rather than continue chasing the root cause,
  updates now make a full restart part of the normal update path, which
  self-heals a stuck WiFi stack as a side effect. Needs `CONFIG_REBOOT=y`.
  An in-process hot-swap approach (terminate/deinstantiate/unload/reload/
  reinstantiate, plus an AssemblyScript `onDestroy()` handler) was
  implemented and then fully reverted in favor of this -- if revisiting
  hot-swap, note the implementation hit a real `ENOMEM` bug from loading the
  new image into RAM before freeing the old instance.
- **WiFi power-save disabled on connect** (`main.c`,
  `wifi_disable_power_save()`): mitigates (but does not fully explain) the
  stall mentioned above.
- **`"ocre:api"` capability passed on container create** (`main.c`,
  `start_container()`): makes per-container resource cleanup (e.g. GPIO pin
  release) actually run on teardown -- previously silently never ran. A
  separate, unrelated ~60 bytes/swap heap leak remains (confirmed unaffected
  by this fix, most likely inside WAMR's own instantiate/deinstantiate
  internals); too small to matter at any realistic swap frequency.
- **LED blink** (`main.c`, `native_led_blink()`): the board's built-in LED
  (devicetree `led0` alias, driven directly, independent of any container's
  own GPIO usage) blinks 3x on every boot and 3x right before an
  update-triggered reboot, as a visual "something just happened" signal.
  Left off afterward for the running container's own AssemblyScript to
  control via `ocre_gpio`.
- **`wasm_runtime_instantiate()`'s `heap_size` param set to 0** (`wamr.c`):
  that parameter is WAMR's own "app heap" for languages needing `malloc()`
  inside the sandbox (C/Rust via WASI) -- AssemblyScript never uses it, and
  WAMR appends it onto the end of linear memory rounded up to the next 64KB
  page boundary, so a nonzero value here can silently cost a full extra 64KB
  page of native heap per container. Recovered ~64KB per container on this
  board (heap usage dropped from 99.5% to 67.7% while a container was
  running).
- **Live device memory stats** (`main.c`, `stats_thread_fn`, always-on via
  `K_THREAD_DEFINE`): prints `OCRE_STATS,<type>,<total>,<used>` once a
  second for `heap` and `flash`, consumed by `ocre-as stats <serialPort>` for
  a live terminal table. A WAMR debug-server (source-level VS Code
  debugging via `WASM_ENABLE_DEBUG_INTERP`) was investigated -- the Zephyr
  platform port has every socket primitive it needs -- but not implemented;
  it was judged too expensive to pursue for now.
- **Memory tuning**: raised `CONFIG_HTTP_SERVER_MAX_CLIENTS`,
  `CONFIG_MAX_PTHREAD_MUTEX_COUNT`/`_COND_COUNT` (Zephyr's POSIX layer hands
  these out from small fixed pools; the dispatch/HTTP rework needs more than
  the default 5), and `CONFIG_NET_MAX_CONN`/`CONFIG_NET_MAX_CONTEXTS`.

### A note on `CONFIG_DYNAMIC_THREAD_STACK_SIZE`

If you're tempted to trim this (`prj.conf`, currently 8192) for more heap:
measure first. It turns out to govern only one thread in this whole app --
`ocre_dispatch.c`'s `loop_scheduler_thread`, the thread that actually calls
into WAMR to run a container's `loop()`/`onRequest()` -- everything else
(WiFi, the HTTP server, the net stack) has its own separately-Kconfig'd
stack. That one thread measured ~35% usage under light HTTP testing, so
there isn't much safe margin, and a stack overflow is a worse failure mode
than the heap pressure you'd be trying to fix. To re-measure, temporarily add
`CONFIG_THREAD_ANALYZER=y`, `CONFIG_THREAD_ANALYZER_AUTO=y`, and
`CONFIG_THREAD_NAME=y` to `prj.conf` for real per-thread high-water-marks.

## AssemblyScript API reference

Everything below is available via `import { ... } from "./ocre";` after
`ocre-as build` generates `assembly/ocre.ts`. Functions requiring a device
Kconfig option are noted; all of them need to be enabled in this sample's
`prj.conf` (they already are).

| Function | Description |
| --- | --- |
| `ocre_print(msg: string): i32` | Print a line to the device's serial console. |
| `ocre_sleep(ms: i32): i32` | Sleep for the given duration (blocking -- avoid in `loop()`/`onRequest()`). |
| `ocre_uname(): OcreUtsname \| null` | System/version info (sysname, release, machine, ...). |
| **HTTP server** (`CONFIG_OCRE_HTTP_SERVER`) | |
| `ocre_http_get_method(requestId: i32): i32` | Request method (see `OcreHttpMethod`). |
| `ocre_http_get_path(requestId: i32): string` | Request path, no query string. |
| `ocre_http_get_query(requestId: i32): string` | Raw query string (no leading `?`). |
| `ocre_http_get_header(requestId: i32, name: string): string` | Header value, or `""` (needs `CONFIG_HTTP_SERVER_CAPTURE_HEADERS`). |
| `ocre_http_get_body(requestId: i32): string` | Request body as text. |
| `ocre_http_respond(requestId: i32, status: i32, contentType?: string, body?: string): i32` | Answer a request captured by `onRequest()`. Call once per request, before returning. |
| **HTTP client** (`CONFIG_OCRE_HTTP_CLIENT`) | |
| `ocre_http_get(host: string, port: i32, path: string): string` | Blocking outbound GET to `http://host:port/path`. Returns the body on 200, `""` on any failure. Stalls this instance's `loop()`/`onRequest()` dispatch for the call's duration. |
| **GPIO** (`CONFIG_OCRE_GPIO`) | |
| `ocre_gpio_init(): i32` | Initialize the GPIO subsystem. Call once before use. |
| `ocre_gpio_configure_by_name(name: string, direction: i32): i32` | Configure a named pin (see `OcreGpioDirection`). |
| `ocre_gpio_set_by_name(name: string, state: i32): i32` | Drive a named output pin (see `OcreGpioState`). |
| `ocre_gpio_get_by_name(name: string): i32` | Read a named pin. |
| `ocre_gpio_toggle_by_name(name: string): i32` | Toggle a named output pin. |
| `ocre_gpio_register_callback_by_name` / `unregister...` | Interrupt-driven input; events arrive via `ocre_get_event()`. |
| `ocre_gpio_configure` / `_pin_set` / `_pin_get` / `_pin_toggle` / `_register_callback` / `_unregister_callback` | Same operations addressed by raw `(port, pin)` instead of a name. |
| **ADC** (`CONFIG_OCRE_ADC`) | |
| `ocre_adc_init(): i32` | Resolve the board's named analog channels. Call once before use. |
| `ocre_adc_read_by_name(name: string): i32` | Read a named channel, pre-scaled to 0-255. |
| **Timers** (`CONFIG_OCRE_TIMER`) | |
| `ocre_timer_create(id: i32): i32` / `_start(id, intervalMs, isPeriodic)` / `_stop(id)` / `_delete(id)` / `_get_remaining(id)` | Native timers; expiry arrives via `ocre_get_event()`. |
| **Sensors** (`CONFIG_OCRE_SENSORS`) | |
| `ocre_sensors_init()` / `_discover()` | Initialize and enumerate sensors. |
| `ocre_sensors_open_by_name` / `_get_handle_by_name` / `_get_channel_count_by_name` / `_get_channel_type_by_name` / `_read_by_name` | Named-sensor variants (and `..._open`, `_get_handle`, etc. by numeric ID). |
| **Container messaging** (`CONFIG_OCRE_CONTAINER_MESSAGING`) | |
| `ocre_publish_message(topic, contentType, payload: ArrayBuffer): i32` | Publish to a topic. |
| `ocre_subscribe_message(topic: string): i32` | Subscribe; messages arrive via `ocre_get_event()`. |
| `ocre_messaging_free_module_event_data(...)` | Free native buffers backing a messaging event after handling it. |
| **Events** (shared by GPIO/timers/sensors/messaging) | |
| `ocre_get_event(): OcreEvent \| null` | Pop the next queued event for this container, if any (see `OcreResourceType`). |
| `ocre_register_dispatcher(resourceType: i32, functionName: string): i32` | Register an exported function name as a resource type's dispatcher. |

HTTP does **not** use the event queue -- `onRequest()` is called directly,
synchronously, per request.

**Sync requirement**: every native function above has a hand-maintained
mirror in `ocre-as`'s `lib/bindings/ocre.ts` (the `@external` import + a
string-marshaling wrapper). Adding a new function to
`ocre_api/ocre_api.c`'s `ocre_api_table[]` without a matching entry there
means AssemblyScript code simply can't call it -- nothing enforces this
automatically.

## Monitoring

`ocre-as stats <serialPort>` (from the `ocre-as` repo) shows a live-updating
heap/flash usage table read from this sample's serial output. See that
repo's README for usage.
