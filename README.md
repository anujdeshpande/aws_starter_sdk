# Marvell's Starter SDK for AWS IoT Service

[![Join the chat at https://gitter.im/marvell-iot/aws_starter_sdk](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/marvell-iot/aws_starter_sdk?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Thanks for trying out Marvell's AWS IoT Starter Kit. The AWS IoT Starter SDK in conjunction with the AWS IoT Starter Kit provide embedded developers a way to quickly develop and prototype Wi-Fi connected embedded products that interface with peripherals over the UART, I2C, SPI, GPIO and other such interfaces at one end, and talk to the AWS IoT service at the other.

---

## Contents
1. [Development Host Setup](#Development-Host-Setup)
  - [Optional Eclipse IDE setup guide](./Development-Host-Setup-with-Eclipse.md)
2. [Build and flash the hardware](#Build-and-flash-the-hardware)
3. [Pin Map](#Pin-Map)
4. [Getting started with AWS IoT](#Getting-started-with-AWS-IoT)
5. [Developing your own applications](#Developing-your-own-applications)

[Download PDF]()

---

## 1. Development Host Setup
The first step for developing your applications with the SDK is to cross-compile the application firmware, along with the SDK, on your host computer. Once the application firmware is generated it can be loaded on the development board (starter kit) using the tools provided by the SDK. Once the application starts running, it can be debugged using the serial console on the development host.

### Cross-compiler Toolchain
Before installing development host specific drivers and packages as mentioned below, an ARM cross-compiler toolchain should be installed on the development host. The SDK supports the GCC Compiler Toolchain. The toolchain for GNU ARM is available from: https://launchpad.net/gcc-arm-embedded
* Download the toolchain for your development host from https://launchpad.net/gcc-arm-embedded
* Make sure that the toolchain is added to your environment path.

### Development Hosts
Windows, Linux and Mac are supported as development hosts. This is the host that will be used to build the firmware and then load/execute the generated firmware on the development board. The setup instructions that install the various packages on the development host are as follows :

#### Step 1 : Install dependencies

##### Linux
The SDK needs 'binutils' package to run the SDK tools. Please run the following command to install the package:
```
$ sudo yum install binutils (for fedora)
$ sudo apt-get install binutils (for ubuntu)
```
Additionally, 64-bit Linux distributions require '32-bit libc' package to run 32-bit host tools. Please run the following command to install the package:
```
$ sudo yum install glibc.i686 (for fedora)
$ sudo apt-get install libc6:i386 (for ubuntu)
```
##### Mac OS X
OS X releases Yosemite and El Capitan can be used to develop with the SDK.

###### Installing OpenOCD (using ports)
* Install macports from [Macports Site](http://www.macports.org/install.php). If you prefer brew, please refer to the end of this page for the instructions. Restart your terminal window so that it finds the port utility in its execution path. Then execute following commands in the terminal window
```
$ which port
$ sudo port selfupdate
```
* Install OpenOCD version 0.9.0 for Mac (Make sure the starter kit is still connected to the Mac)
```
$ sudo port install openocd @0.9.0
```

###### Installing OpenOCD (using brew)
You may skip this section, if you have already installed OpenOCD using the ports tool above.

* Install brew
```
$ ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```
* Install OpenOCD version 0.9.0 for Mac (Make sure the starter kit is still connected to the Mac)
```
$ brew info openocd
$ brew install openocd
```

##### Windows
* Install the following GnuWin32 packages from http://gnuwin32.sourceforge.net/packages.html:
 * coreutils
 * make
 * which

  Add GnuWin32 installation path to the system path.
* Install Python 2.7 for Windows from https://www.python.org/. Make sure python is added to the system path during installation.


#### Step 2 - Set up USB

##### Linux
No special commands required.

##### Mac OS X: Fix Driver Conflict

* Check the usbserial ports using the following command
```
$ ls -l  /dev/tty.usbserial*
crw-rw-rw-  1 root  wheel   18,   6 Nov 17 09:53 /dev/tty.usbserial-142A
crw-rw-rw-  1 root  wheel   18,   8 Nov 17 09:53 /dev/tty.usbserial-142B
```
* To use OpenOCD, we have to ensure that Apple's FTDI drivers are available for the USB-serial profile, while OpenOCD continues to use the JTAG interface. This can be done in the following way
```
$ sudo kextunload -p -b com.apple.driver.AppleUSBFTDI
$ sudo kextutil -b com.apple.driver.AppleUSBFTDI -p AppleUSBEFTDI-6010-1
```
* To verify that the unload has worked, rerun the ```ls``` command. You should now see only one ```usbserial``` device
```
$ ls -l  /dev/tty.usbserial*
crw-rw-rw-  1 root  wheel   18,  10 Nov 17 09:56 /dev/tty.usbserial-142B
```

##### Windows: Install WinUSB
* Download WinUSB driver installer from http://zadig.akeo.ie/.
* Connect the starter kit to the PC, let default Windows driver install first. After this,
   * Launch Zadig WinUSB driver installer
   * Click on options->List all Devices
   * Select Dual RS232 (Interface 0) in the dropdown list
   * Select the driver WinUSB for this device
   * Install the driver


#### Step 3 : Get the SDK

##### For all platforms
Download the latest Marvell's Starter SDK for AWS IoT Service from https://github.com/marvell-iot/aws_starter_sdk/archive/master.zip

##### Additional steps only for Windows
* Get OpenOCD 0.9.0 for Windows from http://www.freddiechopin.info/en/download/category/4-openocd
* Unzip the OpenOCD tarball and copy the contents of bin/ folder (for 32-bit Windows machines) or bin-x64/ folder (for 64-bit Windows machines) to _aws_starter_sdk/wmsdk/tools/OpenOCD/Windows_ folder.
````
copy \path\to\OpenOCD-0.9.0\bin\* \path\to\aws_starter_sdk\wmsdk\tools\OpenOCD\Windows
OR
copy \path\to\OpenOCD-0.9.0\bin-x64\* \path\to\aws_starter_sdk\wmsdk\tools\OpenOCD\Windows
````

#### Step 4 : Serial console
##### Linux
* Once the Starter Kit is connected to your Linux host, two new ttyUSB devices will be detected. This can be verified as follows:
```
$ dmesg | grep ttyUSB
[7337563.575701] usb 3-4: FTDI USB Serial Device converter now attached to ttyUSB0
[7337563.576290] usb 3-4: FTDI USB Serial Device converter now attached to ttyUSB1
```
The second of these devices, in this case the ttyUSB1, is the device for the serial console.
* We will use the _minicom_ program for accessing the serial console.
* For initial configuration of minicom, execute the program in setup mode as (minicom -s).
* Go to Serial Port Setup
* Perform the following settings:
```
    | A -    Serial Device      : /dev/ttyUSB1
    | B – Lockfile Location     : /var/lock
    | C -   Callin Program      :
    | D -  Callout Program      :
    | E -    Bps/Par/Bits       : 115200 8N1
    | F – Hardware Flow Control : No
    | G – Software Flow Control : No
```
where Serial Device is the second of the ttyUSB device that was detected on your system.
* Save the settings as default, and then relaunch minicom with the '-s' parameter.
* One pressing reset button on the starter kit, you should start seeing console messages that are generated by the firmware.

##### Mac OS X
- Once the above settings are done, and the board is plugged in to your development host, a virtual USB device will be created for you. Please lookup the name of this device. The device will be of the form /dev/tty.usbserial-XXXX.
- Install any application that can read serial console (e.g. minicom) as:
```
$ sudo port install minicom
```
- Execute minicom in setup mode (minicom –s)
- Go to Serial Port Setup
- Perform the following settings:
```
    | A -    Serial Device      : /dev/tty.usbserial-XXXX
    | B – Lockfile Location     : /var/lock
    | C -   Callin Program      :
    | D -  Callout Program      :
    | E -    Bps/Par/Bits       : 115200 8N1
    | F – Hardware Flow Control : No
    | G – Software Flow Control : No
```
You can save these settings in minicom for future use. The minicom window will now show messages from the serial console.

##### Windows
Once the appropriate drivers are installed and the development board is connected to your Windows host, virtual COM port devices are added to your systems. Depending upon your Windows configuration, you may see one or two COM ports. These can be verified as follows:
- Right click on My Computer and go to Properties
- Hardware --> Device Manager
- Under "Ports (COM & LPT), you may find the entries of the form "USB Serial Port (COMx)"

This can be used to access the serial console of the development board.

The serial port can be accessed using any serial communication application like HyperTerminal or Putty. Please perform the following settings for your serial console application:
```
Device: "USB Serial Port (COMx)"
Bits per Second: 115200
Data Bits: 8
Parity: N
Stop Bits: 1
Flow Control: None
```
If two COM ports are listed on your system, please select the second of the COM ports for the serial console.


---

## 2. Build and flash the binary

### Build Instructions
The following command builds the entire SDK including all the libraries and sample applications:
```
$ cd aws_starter_sdk
$ make
```

You could compile individual sample application by passing the _APP=_ parameter to make.
```
$ make APP=sample_apps/hello_world
```

### Firmware Variants
Whenever an application firmware is built, two kinds of artifacts are created
- **hello_world.axf:** This is a firmware image that can directly be loaded into the SRAM of the micro-controller using the _ramload.py_ program. Since the firmware image is not written to flash, this operation is faster. This is commonly used for iterative development.
- **hello_world.bin:** This is a firmware image that can be flashed on to the starter kit. This is commonly used as the default firmware that gets executed on boot up.

### Flashing

The Starter Kit comes pre-programmed with the aws_starter demo firmware. If you wish to re-program the starter kit with the aws_starter demo, this can be done as follows:

* The aws_starter demo is available as an image in the [_releases_](https://github.com/marvell-iot/aws_starter_sdk/releases) section of the repository. Download this flash blob to some location, say, _/path/to/flash.blob_.
* Connect the starter kit to your development host using a USB to mini-USB cable
* Perform the following steps for flashing the aws_starter demo:
```
$ cd aws_starter_sdk
$ sudo wmsdk/tools/OpenOCD/flash.py -f /path/to/flash.blob
$ sudo wmsdk/tools/OpenOCD/flash.py -r
```
* The aws_starter demo blob consists of application firmware and other components like bootloader, Wi-Fi firmware etc.
* When application developer needs to flash the new or modified application, he can directly flash application firmware using following steps:
```
$ cd aws_starter_sdk
$ sudo wmsdk/tools/OpenOCD/flash.py --mcufw /path/to/<application name>.bin
$ sudo wmsdk/tools/OpenOCD/flash.py -r
```
* Also, one can directly load application firmware into the SRAM of the micro-controller using following steps:
```
$ cd aws_starter_sdk
$ sudo wmsdk/tools/OpenOCD/ramload.py /path/to/<application name>.axf
```
---

## 3. Pin Map

**NOTE** : The Starter Kit Board is 3.3v tolerant. If you are using a 5v sensor or module, please make sure that you shift down the voltage levels using a resistances or a level-shifter IC. **5v input can damage the module**.

<a href = "https://raw.githubusercontent.com/marvell-iot/aws_starter_sdk_wiki_images/master/PinMap.png"> ![PinMap](https://raw.githubusercontent.com/marvell-iot/aws_starter_sdk_wiki_images/master/PinMap.png) </a>

(click image to zoom)
### Header 2
To the left of the SoC

| GPIO_#  | Function 0  | Function 1 | Function 2 |
| :------------ |:---------------:| :-----:| :-----:|
| 11      | UART1_CTS | SSP1_CLK|-
| 12      | UART1_RTS |SSP1_FRM |-
| 04      | I2C0_SDA | CAN_TX|GPT0_CH4
| 05      | I2C0_SCL | CAN_RX|GPT0_CH5
| 13      | UART1_TXD| SSP1_TXD|-
| 14      | UART1_RXD | SSP1_RXD| -
| 22      | WAKE0 | - | -
| 26      | PUSH_SW0 |I2C1_SCL|CAN_RX
| 25      | 32KHz_CLK_IN | I2C1_SDA|CAN_TX
| 39      | I2C0_RESET | - | -
| 23      | WAKE1 |- |-
| 02      | UART0_TXD | SSP0_TXD|GPT0_CH2
| 03      | UART0_RXD | SSP0_RXD|GPT0_CH3
| 16      | STRAP1 |- |-
| 17      | GPT3_CH0 |I2C1_SCL |-
| 18      | GPT3_CH1 | I2C1_SDA|-
| 24      | PUSH_SW1 | GPT1_CH5|-

### Header 4
To the right of the SoC

| GPIO_#  | Function 0  | Function 1 | Function 2 |
| :------------ |:---------------:| :-----:| :-----:|
| 0      | UART0_CTS | SSP0_CLK| GPT0_CH0
| 01      |  UART0_RTS|SSP0_FRM |GPT0_CH1
| 40      |  LED1|- |-
| 41      | LED2 | -|-
| 27      | STRAP0 |USB_DRVBUS|-
| 48      | SSP2_TXD | UART2_TXD |ADC0_6
| 49      | SSP2_RXD | UART2_RXD|ADC0_7
| 47      | SSP2_FRM | UART2_RTS|ADC0_5
| 42      | ADC0_0 | ACOMP0|-
| 43      |  ADC0_1| ACOMP1|-
| 46      |  SSP2_CLK| UART2_CTS|ADC0_4


---

## 4. Getting started with AWS IoT

For instructions on how to create a new thing and provisioning your device, see
this [page](Getting-Started-Guide.md)

---

## 5. Developing your own applications

Let's quickly look at how you can develop application firmware with the SDK.

### Hello World!
Let's begin with the customary Hello World! The sample code for Hello World is shown below:

```
int main(void)
{
        int count = 0;

        /* Initialize console on uart0 */
        wmstdio_init(UART0_ID, 0);

        wmprintf("Hello World application Started\r\n");

        while (1) {
                count++;
                wmprintf("Hello World: iteration %d\r\n", count);

                /* Sleep  5 seconds */
                os_thread_sleep(os_msec_to_ticks(5000));
        }
        return 0;
}
```

The `main()` function is the entry point for any application. This function is expected to perform any application specific initializations and launch any application threads.

The `wmstdio` facility can be used to print log messages on the serial console over UART. Before using this though, we will have to initialize the UART. This is done using the `wmstdio_init()` call. This function identifies the UART that should be used for this communication and the baud rate that should be used.

The `wmprintf()` function is then used to print messages to the console.

### Wi-Fi
The first thing that you typically do with an IoT Kit is to get it on a network. The easiest way to connect to a known Wi-Fi network is to use the function wm_wlan_connect(). This function takes a Wi-Fi network name and a passphrase as an argument. It then attempts to connect to this Wi-Fi network.

The wm_wlan_connect() is simple and useful, but what if you don't want to hard-code the network credentials in your application firmware's code? The SDK also offers another more powerful Wi-Fi API, wm_wlan_start(). This API does a number of things,
- Firstly, it starts a Wireless Network of its own, a Micro-AP/uAP
- On this uAP network, it hosts a web-app based wizard that guides the end-user through the provisioning of the home Wi-Fi network
- The end-user can launch this web-app by connecting with the uAP network, and then launching a browser that points to http://192.168.10.1
- The end goal of this web-app is to retrieve a Wi-Fi network name and passphrase from the end-user
- Once this configuration is received, then the kit makes connection attempts to the configured network. Once a network is configured, the firmware remembers this configuration across power resets. Subsequent boot-ups of the kit, will skip the setup of the initial configuration wizard, but instead directly start making connection attempts with this network.

#### Connectivity Callbacks
The application firmware gets to know the status of the Wi-Fi connectivity using connectivity callbacks. The following connectivity callbacks have been defined:
- wlan_event_normal_connected(): This function is called whenever the Wi-Fi station successfully associates with the destined Wireless Access Point.
- wlan_event_connect_failed(): This function is called whenever the Wi-Fi station fails to associate with the destined
- wlan_event_normal_link_lost(): This function is called whenever an existing association with the Wireless Access Point is lost

An application firmware can define these functions to take the required action as is applicable to them.

### AWS IoT
The AWS IoT APIs is a collection of APIs that can be used to connect to the AWS IoT service. The detailed documentation of the AWS IoT API can be found here: http://aws-iot-device-sdk-embedded-c-docs.s3-website-us-east-1.amazonaws.com/index.html

Typically for using AWS IoT Shadows the following sequence of calls will be useful:
- aws_iot_mqtt_init() to initialize the MQTT module
- aws_iot_shadow_init() to initialize the AWS Shadows
- aws_iot_shadow_connect() to connect to an AWS Shadow
- Now, create a thread that keeps calling aws_iot_shadow_yield(). This keeps looking for incoming messages from the AWS IoT server
- Once such a setup is done, the application firmware can
   - subscribe to changes from the AWS IoT service using the aws_iot_shadow_register_delta() function. The callback registered will be called whenever the designated JSON objects are updated at the server
   - publish updates to states using the aws_publish_property_state() function


***
