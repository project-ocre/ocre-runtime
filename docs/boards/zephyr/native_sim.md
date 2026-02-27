# Zephyr Board: `native_sim`

The **native_sim** board is a simulated Zephyr target that runs on your host Linux machine. It allows you to develop and test Zephyr applications without needing physical hardware. The native_sim board is particularly useful for rapid iteration, testing, and debugging.

For detailed information, see the [Zephyr Native Simulator Documentation](https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html).

## Architecture Support

Both **native_sim/native/64** (64-bit) and **native_sim** (32-bit) variants are supported for the following Ocre samples:

- **mini**
- **demo**
- **supervisor**

**Note:** The 32-bit variant requires `gcc-multilib` to be installed on your host machine (see [Prerequisites](#prerequisites)) if you are running on a 64-bit system.

## Networking

For **supervisor** sample, native_sim board supports two distinct networking modes, configurable at build time:

### 1. TAP Network Interface (Default)

The TAP (TAP Virtual Network Interface) mode creates a virtual Ethernet network interface on your host machine, allowing the simulated board to communicate with the actual network.

**Configuration:**

- Uses `CONFIG_ETH_NATIVE_TAP=y` (enabled by default)
- No additional configuration file needed

### 2. NSOS (Native Sockets) Mode

The NSOS (Native Sockets) mode uses the host machine's native socket implementation, allowing the simulated board to use the host's network stack directly.

**Characteristics:**

- Uses host native sockets API
- Based on `CONFIG_NET_SOCKETS` and `CONFIG_NET_SOCKETS_OFFLOAD`
- Uses separate `overlay-nsos.conf` configuration

**Configuration File:**
The NSOS mode uses board overlay configuration at:

```
boards/overlay-nsos.conf
```

## Build Instructions

### Prerequisites

Start with [Getting Started with Zephyr](../../GetStartedZephyr.md) to get a working build environment.

To build for the 32-bit `native_sim` variant on a 64-bit system, install `gcc-multilib` on your host machine:

```bash
sudo apt install gcc-multilib
```
**Note:** this is not required for 32-bit systems.

### Building with TAP Networking (Default)

To build native_sim with TAP networking:

```bash
west build -p always -b native_sim/native/64 src/samples/supervisor/zephyr/
```
Alternatively, the `native_sim` board could be used for supported 32-bit platforms.

### Building with NSOS Networking

To build native_sim with Native Sockets (NSOS):

```bash
west build -p always -b native_sim/native/64 src/samples/supervisor/zephyr/ -- -DEXTRA_CONF_FILE=boards/overlay-nsos.conf
```
Alternatively, the `native_sim` board could be used for supported 32-bit platforms.

## Running the Simulation

After building, regardless of which network method was used, run the native_sim application with the following command:

```bash
west build -t run
```

## Serial Console

Unlike physical boards, native_sim outputs to your terminal directly. You will see output similar to:

```
*** Booting Zephyr OS build <version> ***

ocre:~$
```

To interact with the Ocre shell, type commands directly in the same terminal.

## Test Network Connectivity

Once running, check network configuration with:

```bash
ocre:~$ net iface
```

**Example TAP mode output:**

```bash
Interface eth0 (0x4ba390) (Ethernet) [1]
===================================
Link addr : 02:00:5E:00:53:31
MTU       : 1500
Flags     : AUTO_START,IPv4,IPv6
Device    : zeth0 (0x4b8b50)
Status    : oper=UP, admin=UP, carrier=ON
```

**Example NSOS mode output:**

```bash
Interface net0 (0x4c1988) (<unknown type>) [1]
=========================================
MTU       : 1500
Flags     : AUTO_START,IPv4,IPv6
Device    : nsos_socket (0x4c0008)
Status    : oper=UP, admin=UP, carrier=ON
IPv4 unicast addresses (max 1):
    192.0.2.1/255.255.255.0 overridable preferred infinite
```

## Memory Configuration

Both `native_sim/native/64` and `native_sim` variant includes enhanced memory configuration:

```
CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE=10485760  # 10 MB malloc arena
```

Flash and storage partitions are sized appropriately for simulation:

```
flash0:     32 MB (simulated)
storage:    16 MB (for container images)
```

## Advantages for Development

- **Fast iteration**: No hardware flashing required
- **Easy debugging**: Direct terminal access and debugging tools
- **Network simulation**: Test networking without physical network setup
- **CI/CD compatible**: Can run in automated testing pipelines

## Limitations

- **32-bit requires gcc-multilib on 64-bit systems**: Install `gcc-multilib` on your machine before building the 32-bit variant
- **Hardware peripherals**: No hardware-specific peripherals available (GPIO, sensors, etc.)

## References

- [Zephyr Native Simulator Documentation](https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html)
- [Zephyr Networking Documentation](https://docs.zephyrproject.org/latest/connectivity/networking/index.html)
- [Supervisor Sample Documentation](../../samples/supervisor.md)

See the main documentation for:

- [Zephyr Build System](../../BuildSystemZephyr.md)
- [Get started Zephyr](../../GetStartedZephyr.md)
