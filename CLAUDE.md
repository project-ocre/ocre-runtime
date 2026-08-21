# ocre-runtime — notes for Claude

**This repo is a fork of upstream [project-ocre/ocre-runtime](https://github.com/project-ocre/ocre-runtime).**
`README.md` is mostly upstream's own (kept intact for attribution/links) —
this fork's own additions are called out in a note near its top and detailed
in `docs/samples/fetch.md`, which is the canonical, up-to-date reference for
everything below (feature list, build/flash instructions, full AssemblyScript
API table). It's the Ocre container runtime itself: a WAMR-based WASM runtime
on Zephyr, plus the `src/samples/fetch` sample firmware that's been the main
thing developed and tested against real hardware (Seeed XIAO ESP32-C6).

This repo is meant to be paired with two separate repos developed alongside
it: **`ocre-as`** (a Node CLI: builds AssemblyScript to WASM, hosts it over
HTTP for this repo's `fetch` sample to pull, and live-monitors device memory
over serial) and **`gpio-demo`** (an example AssemblyScript project using
`ocre-as` and this fork's GPIO/ADC/HTTP API). If working across more than one
of these at once, check whether a shared local workspace-level `CLAUDE.md`
exists one level up — it may have cross-repo operational notes (hardcoded
WiFi/IP config that must agree across repos, serial port gotchas, etc.) that
don't belong in any single repo.

## Architecture in one paragraph

`src/ocre/` is the container lifecycle layer (context/container create,
start, destroy). `src/runtime/wamr-wasip1/wamr.c` is the WAMR-specific
runtime vtable implementation — this is where a module gets instantiated,
executed, and torn down. `src/runtime/wamr-wasip1/ocre_api/` holds every
native function exposed to WASM containers, one subdirectory per capability
(`ocre_gpio`, `ocre_adc`, `ocre_http` [inbound server], `ocre_http_client`
[outbound client], `ocre_timers`, `ocre_sensors`, `ocre_messaging`,
`ocre_print`), all registered in `ocre_api/ocre_api.c`'s `ocre_api_table[]`.
Each entry there needs a matching mirror in the separate `ocre-as` repo's
`lib/bindings/ocre.ts` (see that repo's own CLAUDE.md) — nothing enforces
this automatically.

**Reactive execution model**: a container's `main()` runs once. If it also
exports `loop` and/or `onRequest`, the native side (`wamr.c` +
`ocre_api/ocre_dispatch/`) keeps the instance alive afterward instead of
tearing it down, calling those exports on its own schedule — `loop()` on a
timer (`CONFIG_OCRE_LOOP_INTERVAL_MS`), `onRequest()` per HTTP request — all
serialized through a mutex since only one thread may execute inside a given
WASM instance at a time. The thread that actually does this (`loop_scheduler_thread`
in `ocre_dispatch.c`) is the one real consumer of
`CONFIG_DYNAMIC_THREAD_STACK_SIZE` in this whole app — see below.

## Building/flashing

```sh
source ~/ocre/.venv/bin/activate
west build -d <build-dir> -b xiao_esp32c6/esp32c6/hpcore -s src/samples/fetch/zephyr
west flash -d <build-dir>
```

Always pass `-b`/`-s` explicitly for a fresh build dir — West can otherwise
misconfigure against the top-level library CMakeLists instead of the sample.

## Feature history (what's actually been built here)

Chronologically, on top of upstream Ocre:

1. **Reactive execution model** (`wamr.c`, `ocre_api/ocre_dispatch/`) — see
   above. Replaced the old "container loops forever in `main()`" model.
2. **Generic HTTP request/response bridge** (`ocre_api/ocre_http/`) —
   replaced fixed `/`+`/button` endpoints with a single fallback resource
   that hands every request to the container's `onRequest()`.
3. **ADC support** (`ocre_api/ocre_adc/`) — named channels via the same
   `zephyr,user` io-channels devicetree convention used for GPIO aliases.
4. **`ocre_print`** (`ocre_api/ocre_print/`) — serial print + AssemblyScript's
   required `env.abort` hook.
5. **Hash-based update checks streamed to flash** (`main.c`, `ocre-as/lib/host.js`) —
   `/getHash` (sha256 digest) checked first, `/getCode` only fetched on
   change, written straight to a temp file rather than buffered in RAM.
6. **WiFi power-save disabled on connect** (`main.c`, `wifi_disable_power_save()`) —
   mitigates (but does not fully explain) an intermittent full network stall.
7. **`"ocre:api"` capability now passed on container create** (`main.c`,
   `start_container()`) — makes per-container resource cleanup (GPIO release
   etc.) actually run on teardown; previously silently never ran.
8. **Updates apply via full reboot, not in-place container swap** (`main.c`,
   `check_for_update()`) — see the WiFi-stall bullet below for why.
9. **Native LED blink** (`main.c`, `native_led_blink()`) — blinks the board's
   built-in LED (devicetree `led0` alias, driven directly via `gpio_dt_spec`,
   independent of any container's own GPIO usage) 3x on every boot and 3x
   right before the reboot that follows a successful update, purely as a
   visual "something just happened" signal. Left off afterward for the
   running container's own AssemblyScript to control.
10. **Outbound HTTP client API** (`ocre_api/ocre_http_client/`, new
    `CONFIG_OCRE_HTTP_CLIENT` Kconfig) — `ocre_http_get(host, port, path)`,
    the counterpart to the inbound server bridge, letting container code make
    its own blocking HTTP GET requests. Demonstrated in `gpio-demo` by
    polling `/getHash` from `loop()` every ~10s and showing it on the page.
11. **Device-side memory stats for `ocre-as stats`** (`main.c`,
    `stats_thread_fn`, always-on via `K_THREAD_DEFINE`) — prints
    `OCRE_STATS,<type>,<total>,<used>` once a second for `heap` (libc heap via
    `malloc_runtime_stats_get`) and `flash` (LittleFS via `fs_statvfs("/lfs", ...)`).
12. **`heap_size=0` fix in `wasm_runtime_instantiate()`** (`wamr.c`) — see
    below; recovered ~64KB of native heap per container.

### Tried and reverted — don't redo without re-reading why

- **In-process container hot-swap + an AssemblyScript `onDestroy()` handler.**
  Implemented following a 5-step recipe (terminate → deinstantiate → unload →
  load new bytecode → instantiate) plus vtable/dispatch plumbing for
  `onDestroy`. Hit a real bug: the implementation loaded the new image into a
  heap buffer *before* freeing the old (still-resident) instance, causing
  `ENOMEM` on the very first swap — this directly contradicted the article's
  own step ordering and was about to be fixed when the user reconsidered the
  whole approach and asked for a full revert instead. Fully reverted (verified
  via grep for `hot_swap`/`onDestroy` returning zero hits); updates now use
  the full-reboot approach (item 8 above) instead. If hot-swap is revisited,
  fix the load-before-free ordering bug first.
- **A WAMR-based debug server for VS Code source-level debugging.** Explored
  in depth: `WASM_ENABLE_DEBUG_INTERP` requires `WAMR_BUILD_THREAD_MGR=1` and
  auto-disables the fast interpreter; the Zephyr platform port already
  implements every `os_socket_*` primitive `debug-engine`/`gdbserver.c` need,
  so it's plausible to wire up. Not implemented — the user said it was "too
  expensive right now" mid-investigation. No code changes were made for this;
  don't assume any debug-server plumbing exists.

### Known open issue, deliberately not chased further

A small heap leak of ~60 bytes per container create/destroy cycle exists,
confirmed independent of the `"ocre:api"` capability fix (measured identical
magnitude with and without it). Root cause not found; most likely inside
WAMR's own `wasm_runtime_instantiate`/`deinstantiate` internals. Too small to
matter at any realistic swap frequency (1000+ swaps to become significant) —
noted here so it isn't mistaken for something introduced by later changes.

## Hard-won facts about this specific board/runtime combo

- **`wasm_runtime_instantiate()`'s `heap_size` param (`wamr.c`) must stay 0
  for AssemblyScript containers.** That parameter is WAMR's own "app heap"
  for languages needing `malloc()` inside the sandbox (C/Rust via WASI) — AS
  never uses it. Worse than just wasting the requested amount: WAMR appends
  it onto the end of linear memory and rounds up to the next 64KB page
  boundary, so even a small nonzero value (e.g. 8192) can silently cost a
  **full extra 64KB page** of native heap per container. Confirmed by
  measurement: dropping 8192→0 took heap usage from 99.5% to 67.7% on this
  board, not the ~4% a naive estimate would predict.
- **`CONFIG_DYNAMIC_THREAD_STACK_SIZE` (`src/samples/fetch/zephyr/prj.conf`)
  only governs one thread in this app**: `ocre_dispatch.c`'s
  `loop_scheduler_thread` (created via plain `pthread_create()`). Everything
  else — WiFi, the HTTP server, the net stack — has its own dedicated,
  separately-Kconfig'd stack. Measured that one thread hitting ~35% usage
  under light HTTP testing; not safe to trim aggressively without real
  profiling first (enable `CONFIG_THREAD_ANALYZER` + `CONFIG_THREAD_ANALYZER_AUTO`
  + `CONFIG_THREAD_NAME` temporarily to get real per-thread high-water-marks —
  removed from this tree after use, re-add if investigating further).
- **printk over this board's console can genuinely busy-wait.** The ESP32-C6
  native USB-Serial/JTAG driver (`zephyr/drivers/serial/serial_esp32_usb.c`,
  `serial_esp32_usb_poll_out`) spins for up to 50ms per byte if a host was
  recently draining the TX FIFO but it's momentarily full. Frequent/heavy
  logging while something is actively reading (e.g. a monitor tool that
  clears and redraws the whole screen on every line) can cost real device
  CPU time; if nothing is attached at all it degrades gracefully instead
  (near-instant no-op, data just dropped).
- **`"ocre:api"` must be passed via `ocre_container_args.capabilities`** for
  per-container resource cleanup (e.g. GPIO pin release) to actually run on
  teardown — easy to silently omit, and the failure mode (leaked
  registrations across container swaps) is quiet.
- **The recurring full bidirectional WiFi stall was never conclusively
  root-caused** despite extensive investigation (interface/IP loss, power-save
  alone, container-teardown CPU cost, and connection-pool exhaustion were all
  ruled out with real hardware evidence). The shipped mitigation:
  `check_for_update()` in `main.c` reboots the whole board
  (`sys_reboot(SYS_REBOOT_COLD)`) after installing a new image rather than
  swapping the container in-process, which self-heals a stuck WiFi stack as
  a side effect of the normal update path. If this stall resurfaces and
  someone wants to chase the actual root cause again, that investigation
  history (power-save, connection pools, teardown timing) doesn't need to be
  repeated — it's already ruled out.
- **The device's serial port path changes across reflashes/resets** — always
  `ls /dev/cu.usbmodem*` fresh rather than reusing a previously-known path.
- **The separate `ocre-as` repo's `host` command must be running** for this
  firmware's update-fetch cycle to succeed — it's a long-running Node process
  on the dev machine and dies silently sometimes.
- **WiFi credentials and the fetch server address/port are hardcoded** in
  this repo (`wifi_credentials.h`, `FETCH_SERVER_ADDR`/`FETCH_SERVER_PORT` in
  `main.c`) and must agree with wherever the `ocre-as` repo's `host` command
  is actually running (its own machine's LAN IP, port 8080 by default).
