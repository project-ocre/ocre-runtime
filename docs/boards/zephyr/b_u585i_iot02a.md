# Zephyr Board: `b_u585i_iot02a`

The [Discovery Kit for IoT Node with STM32U5 Series](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) is a demonstration and development board featuring the **STM32U585AIUx** MCU.

Apart from the SoC, internal 2MB flash (which usually holds the application), there is an external 64MB flash chip (which is usually used for the storage_partition). And also features an 8MB PSRAM which we use for the container memory.

It is well supported by Zephyr, and more information can be found in Zephyr's [B-U585I-IOT02A Discovery kit](https://docs.zephyrproject.org/latest/boards/st/b_u585i_iot02a/doc/index.html) documentation.

## Networking

### Wi-Fi Module Not Supported

The onboard EMW3080 Wi-Fi module is **not supported** by Zephyr. For network connectivity, you **must use an Ethernet shield**.

### Supported Ethernet Shields

The board supports the following Ethernet shields, which connect via **SPI1**:

#### ENC28J60 (Microchip)

- **References**:
  - [Microchip ENC28J60 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/39662e.pdf)
  - [Zephyr ENC28J60 Driver Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/ethernet.html#microchip-enc28j60)

#### W5500 (WizNet)

- **References**:
  - [WizNet W5500 Datasheet](https://docs.wiznet.io/Product/iEthernet/W5500/Datasheet)
  - [Zephyr W5500 Driver Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/ethernet.html#wiznet-w5500)

### Hardware Connections

The Ethernet shields connect to the board via **SPI1** (Serial Peripheral Interface bus 1). This is the Ocre supported communication interface for these shields.

#### SPI1 Pin Configuration

All Ethernet shields use the same SPI1 pins on the b_u585i_iot02a board:

| Function  | Pin name     | STM32U5 pin | Connector |
| --------- | ------------ | ----------- | --------- |
| SPI1_NSS  | PWM/CS/D10   | PE12        | CN13      |
| SPI1_SCK  | SCK/D13      | PE13        | CN13      |
| SPI1_MISO | MISO/D12     | PE14        | CN13      |
| SPI1_MOSI | PWM/MOSI/D11 | PE15        | CN13      |
| GND       | GND          | -           | CN13      |
| VCC       | 5V           | -           | CN17      |

#### Important:

Each shield type uses a different interrupt GPIO pin:

- **ENC28J60 Shield**: Interrupt on **PC1** (Pin name: **D8**) on **CN13**
- **W5500 Shield**: Interrupt on **PD15** (Pin name: **D2**) on **CN14**

### Detailed Connection Instructions

#### Power and Ground

1. Connect shield **GND** pin to any **GND** pin on the board
2. Connect shield **5V** pin to any **5V** pin on the board

#### SPI1 Connections

3. Connect shield **CS** pin to board **PE12** (SPI1 Chip Select)
4. Connect shield **CLK** pin to board **PE13** (SPI1 Serial Clock)
5. Connect shield **MISO**/**SO** pin to board **PE14** (SPI1 Data In)
6. Connect shield **MOSI**/**SI** pin to board **PE15** (SPI1 Data Out)

#### Interrupt Connection

For **ENC28J60 shields**: 7. Connect shield **INT** pin to board **PC1**

For **W5500 shields**: 7. Connect shield **INT** pin to board **PD15**

More detailed information can be found in [User Manual](https://www.st.com/resource/en/user_manual/um2839-discovery-kit-for-iot-node-with-stm32u5-series-stmicroelectronics.pdf).

## Build Instructions

### Prerequisites

Start with [Getting Started with Zephyr](../../GetStartedZephyr.md) to get a working build environment.

### Building Without Shield (No Networking)

For development or testing without networking:

```bash
west build -p always -b b_u585i_iot02a src/samples/supervisor/zephyr/ 
```

### Building with ENC28J60 Ethernet Shield

To build with the ENC28J60 shield:

```bash
west build -p always -b b_u585i_iot02a --shield enc28j60 src/samples/supervisor/zephyr/
```

Replace `enc28j60` with `w5500` if WIZnet W5500 ethernet shield is used.

## Flashing

### First Flash (with Preloaded Images)

To flash the application and preloaded container images in the storage partition:

```bash
west flash --hex-file build/zephyr/merged.hex
```

This writes both:

- Application code to internal flash (2 MB)
- 16MB of storage partition to external flash (64 MB total) with preloaded WASM images

**Time**: May take 1-2 minutes depending on image size.

For more information about preloaded images, see [Build System Zephyr](../../BuildSystemZephyr.md).

### Subsequent Flashes (Development)

After the first flash with `merged.hex`, you can flash just the application code:

```bash
west flash
```

This only updates the application, leaving the storage partition intact.

## Serial Console

The serial console can be accessed using `picocom`.
This works both on native Linux and WSL, with a small setup.

### Native Linux

The board usually appears as:

```bash
/dev/ttyACM0
```

Check with:

```bash
ls /dev/ttyACM*
```

### WSL (Windows Subsystem for Linux)

WSL does not automatically expose USB serial devices.
You must attach the ST-Link device using `usbipd`.

**1. Install usbipd on Windows**

From PowerShell (Admin):

```bash
winget install usbipd
```

**2. List USB devices**

```bash
usbipd list
```

**3. Attach device to WSL (look for ST-Link / STMicroelectronics)**

```bash
usbipd attach --busid <BUS_ID> --wsl
```

Example:

```bash
usbipd attach --busid 1-4 --wsl
```

**4. Verify in WSL**

```bash
ls /dev/ttyACM*
```

You should now see:

```bash
/dev/ttyACM0
```

### Connect to the serial console

Open the serial console
Run picocom with 115200 baud (default Zephyr):

```bash
picocom -b 115200 /dev/ttyACM0
```

Exit with:

```bash
Ctrl + A, then Ctrl + X
```

## Test Network Connectivity

If the steps above were followed, you should see in the console:

```bash
ethernet@0: Link up
```

The board automatically gets an IPv4 address, to see it type in the console:

```bash
ocre:~$ net iface
```

**Output:**

```bash
Interface eth0 (index 1)
===========================
Device    : ethernet@0
Link state: UP
IPv4 unicast addresses (1):
  192.168.18.17/255.255.255.0 DHCP preferred
```

## References

- [Zephyr B-U585I-IOT02A Documentation](https://docs.zephyrproject.org/latest/boards/st/b_u585i_iot02a/doc/index.html)
- [Board Schematics and Datasheet](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) (available from ST)
- [Supervisor Sample Documentation](../../samples/supervisor.md)

See the main documentation for:

- [Zephyr Build System](../../BuildSystemZephyr.md)
- [Get started Zephyr](../../GetStartedZephyr.md)
