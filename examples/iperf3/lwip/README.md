# How to Test iPerf Example



## Step 1: Prepare software

The following serial terminal programs are required for iPerf example test, download and install from below links.

- [**Tera Term**][link-tera_term]
- [**iPerf3**][link-iPerf]



## Step 2: Prepare hardware

If you are using W5100S-EVB-Pico, W5500-EVB-Pico, W55RP20-EVB-Pico, W5100S-EVB-Pico2 or W5500-EVB-Pico2, you can skip '1. Combine...'

1. Combine WIZnet Ethernet HAT with Raspberry Pi Pico.

2. Connect ethernet cable to WIZnet Ethernet HAT, W5100S-EVB-Pico, W5500-EVB-Pico, W55RP20-EVB-Pico, W5100S-EVB-Pico2 or W5500-EVB-Pico2 ethernet port.

3. Connect Raspberry Pi Pico, W5100S-EVB-Pico or W5500-EVB-Pico to desktop or laptop using 5 pin micro USB cable. W55RP20-EVB-Pico, W5100S-EVB-Pico2 or W5500-EVB-Pico2 require a USB Type-C cable.



## Step 3: Setup iPerf Example

To test the iPerf example, minor settings shall be done in code.

1. Setup SPI port and pin in 'w5x00_spi.h' in 'WIZnet-PICO-IPERF-C/port/ioLibrary_Driver/' directory.

Setup the SPI interface you use.
- If you use the W5100S-EVB-Pico, W5500-EVB-Pico, W5100S-EVB-Pico2 or W5500-EVB-Pico2,

```cpp
/* SPI */
#define SPI_PORT spi0

#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_MISO 16
#define PIN_CS 17
#define PIN_RST 20
```

If you want to test with the iPerf example using SPI DMA, uncomment USE_SPI_DMA.

```cpp
/* Use SPI DMA */
//#define USE_SPI_DMA // if you want to use SPI DMA, uncomment.
```
- If you use the W55RP20-EVB-Pico,
```cpp
/* SPI */
#define USE_SPI_PIO

#define PIN_SCK 21
#define PIN_MOSI 23
#define PIN_MISO 22
#define PIN_CS 20
#define PIN_RST 25
```

2. Setup network configuration such as IP in 'w5x00_iperf_lwip.c' which is the iPerf example in 'WIZnet-PICO-IPERF-C/examples/iperf3/lwip/' directory.

Setup IP and other network settings to suit your network environment.

```cpp
/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 11, 2},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 11, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};
```

3. Setup iPerf configuration in 'w5x00_iperf_lwip.c' in 'WIZnet-PICO-IPERF-C/examples/iperf3/lwip/' directory.

```cpp
/* Port */
#define PORT_IPERF 5201
```

4. Setup LwIP default port in 'lwiperf.h' in 'WIZnet-PICO-IPERF-C/libraries/pico-sdk/lib/lwip/src/include/lwip/apps/' directory.

```cpp
#define LWIPERF_TCP_PORT_DEFAULT  5201
```


## Step 4: Build

1. After completing the iPerf example configuration, click 'build' in the status bar at the bottom of Visual Studio Code or press the 'F7' button on the keyboard to build.

2. When the build is completed, 'w5x00_iperf_lwip.uf2' is generated in 'WIZnet-PICO-IPERF-C/build/examples/iperf3/lwip/' directory.



## Step 5: Upload and Run

1. While pressing the BOOTSEL button of Raspberry Pi Pico, W5100S-EVB-Pico, W5500-EVB-Pico, W55RP20-EVB-Pico, W5100S-EVB-Pico2 or W5500-EVB-Pico2 power on the board, the USB mass storage 'RPI-RP2' is automatically mounted.

![][link-raspberry_pi_pico_usb_mass_storage]

2. Drag and drop 'w5x00_iperf_lwip.uf2' onto the USB mass storage device 'RPI-RP2'.

3. Connect to the serial COM port of Raspberry Pi Pico, W5100S-EVB-Pico, W5500-EVB-Pico, W55RP20-EVB-Pico, W5100S-EVB-Pico2 or W5500-EVB-Pico2 with Tera Term.

![][link-connect_to_serial_com_port]

4. Reset your board.

5. If the iPerf3 example works normally on Raspberry Pi Pico, W5100S-EVB-Pico, W5500-EVB-Pico, W55RP20-EVB-Pico, W5100S-EVB-Pico2 or W5500-EVB-Pico2, you can see the network information of Raspberry Pi Pico, W5100S-EVB-Pico, W5500-EVB-Pico, W55RP20-EVB-Pico, W5100S-EVB-Pico2 or W5500-EVB-Pico2 and the TCP server is open.

![][link-network_information]

6. Run command prompt to enter the iPerf command and move to iPerf path.

```cpp
/* Change directory */
// change to the 'iperf-x.x.x-winxx' directory.
cd [user path]/iperf3.xx_xx

// e.g.
cd D:/iperf3.16_64
```
![][link-move_to_iperf3_path]

7. In the command prompt, enter the following command to connect to Raspberry Pi Pico, W5100S-EVB-Pico or W5500-EVB-Pico running as a TCP server and test.

```cpp
/* Network performance measurement test */
.\iperf3 -c [connecting to] -t [time in seconds to transmit for] -i [seconds between periodic bandwidth reports]

// e.g.
.\iperf -c 192.168.11.2 -t 10 -t 1
```
![][link-iperf3_lwip_result]



<!--
Link
-->

[link-tera_term]: https://osdn.net/projects/ttssh2/releases/
[link-iPerf]: https://iperf.fr/iperf-download.php
[link-raspberry_pi_pico_usb_mass_storage]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-C/blob/main/static/images/loopback/raspberry_pi_pico_usb_mass_storage.png
[link-connect_to_serial_com_port]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-C/blob/main/static/images/loopback/connect_to_serial_com_port.png
[link-network_information]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-IPERF-C/blob/main/static/images/iperf/network_information.png
[link-move_to_iperf3_path]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-IPERF-C/blob/main/static/images/iperf/move_to_iperf3_path.png
[link-iperf3_lwip_result]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-IPERF-C/blob/main/static/images/iperf/iperf3_lwip_result.png