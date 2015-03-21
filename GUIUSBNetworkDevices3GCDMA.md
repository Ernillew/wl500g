#summary USB Network Devices - 3G/CDMA

In this menu you're able to tune up 3G/CDMA modems settings

# Index #
  * [Introduction](GUIUSBNetworkDevices3GCDMA#Introduction.md)
  * [General parameters](GUIUSBNetworkDevices3GCDMA#General_parameters.md)
    * [Set as WAN Connection Type by default](GUIUSBNetworkDevices3GCDMA#Set_as_WAN_Connection_Type_by_default.md)
    * [Zero CD Configuration](GUIUSBNetworkDevices3GCDMA#Zero_CD_Configuration.md)
  * [Modem options](GUIUSBNetworkDevices3GCDMA#Modem_options.md)
    * [Modem type](GUIUSBNetworkDevices3GCDMA#Modem_type.md)
    * [Username](GUIUSBNetworkDevices3GCDMA#Username.md)
    * [Password](GUIUSBNetworkDevices3GCDMA#Password.md)
    * [APN](GUIUSBNetworkDevices3GCDMA#APN.md)
    * [Dial Number](GUIUSBNetworkDevices3GCDMA#Dial_Number.md)
    * [Call on Demand](GUIUSBNetworkDevices3GCDMA#Call_on_Demand.md)
    * [Idle time before disconnect (sec)](GUIUSBNetworkDevices3GCDMA#Idle_time_before_disconnect_(sec).md)
    * [MTU](GUIUSBNetworkDevices3GCDMA#MTU.md)
    * [MRU](GUIUSBNetworkDevices3GCDMA#MRU.md)
  * [Custom USB device parameters](GUIUSBNetworkDevices3GCDMA#Custom_USB_device_parameters.md)
    * [Autodetect device](GUIUSBNetworkDevices3GCDMA#Autodetect_device.md)
    * [USB tts(ac) port](GUIUSBNetworkDevices3GCDMA#USB_tts(ac)_port.md)
    * [USB device Vendor ID (0xabcd)](GUIUSBNetworkDevices3GCDMA#USB_device_Vendor_ID_(0xabcd).md)
    * [USB device Product ID (0xefgh)](GUIUSBNetworkDevices3GCDMA#USB_device_Product_ID_(0xefgh).md)
    * [USB device packet size (0 for default)](GUIUSBNetworkDevices3GCDMA#USB_device_packet_size_(0_for_default).md)
    * [USB device serial speed (usually 921600)](GUIUSBNetworkDevices3GCDMA#USB_device_serial_speed_(usually_921600).md)
  * [Additional parameters](GUIUSBNetworkDevices3GCDMA#Additional_parameters.md)
    * [Additional AT commands](GUIUSBNetworkDevices3GCDMA#Additional_AT_commands.md)
    * [Additional pppd options](GUIUSBNetworkDevices3GCDMA#Additional_pppd_options.md)
    * [Enable USB-Serial drivers](GUIUSBNetworkDevices3GCDMA#Enable_USB-Serial_drivers.md)
    * [Failure event script name](GUIUSBNetworkDevices3GCDMA#Failure_event_script_name.md)
    * [Max number of reconnection attempts](GUIUSBNetworkDevices3GCDMA#Max_number_of_reconnection_attempts.md)

---

# Introduction #
This page contains parameters to setup WAN-connection using USB-modem or cellular phone with USB-cable. Allowed dialup, GPRS/EDGE/UMTS/CDMA etc. modems.


---

# General parameters #
## Set as WAN Connection Type by default ##
This checkbox allow to make 3G/CDMA connection as default WAN. Checked state indicate that parameter **WAN Connection Type:** at page **IP Config - WAN & LAN** is **USB-Modem**.  Unchecking of it will set parameter **WAN Connection Type:** to value **Auto**. Any changing of this checkbox requires router restart.

## Zero CD Configuration ##
Allow to switch USB-device with Zero CD mode to modem mode. Internally used usb\_modeswitch.

  * Not set – do not switch.
  * Auto – automatically switch device if it detected in usb-modeswitch-data used in the router.
  * Config at /usr/local/etc/usb\_modeswitch.conf – if switch rules does not included in current firmware but exist it is allowed to use user file **/usr/local/etc/usb\_modeswitch.conf** with any text available for usb\_modeswitch.

Use page Status & Log - Diagnostic Information to detect connected USB-devices and view devices that not switched in modem mode.

_The default value is: Auto_

---

# Modem options #

## Modem type ##
Modem type selection
  * CDMA/EVDO
  * GPRS/EDGE/UMTS/HSPDA
  * Dialup
  * User defined – run user dial script /usr/local/dial/dial.

_The default value is: GPRS/EDGE/UMTS/HSPDA_

## Username ##
User name. Examples:
  * SkyLink - mobile
  * MTS – mts
  * Beeline – beeline
  * Megafone - gdata

_The default value is: empty_

## Password ##
User password. Examples:
  * SkyLink - internet
  * MTS – mts
  * Beeline – beeline
  * Megafon - gdata

_The default value is: empty_
## APN ##
Access Point Name. Examples:
  * MTS – internet.mts.ru
  * Beeline – internet.beeline.ru
  * Megafon - internet

_The default value is: empty_

## Dial Number ##
For GPRS/EDGE/UMTS/HSPDA modems usually used `*99***1#` or `*99#` numbers.

For CDMA the only #777 value.

_The default value is: empty_

## Call on Demand ##
_The default value is: No_

## Idle time before disconnect (sec) ##
Idle time in seconds. Used together with Call on Demand parameter.

_The default value is: 60 sec_

## MTU ##
Maximum Transmission Unit

_The default value is: 1492_

## MRU ##
Maximum Receive Unit

_The default value is: 1492_

---

# Custom USB device parameters #
## Autodetect device ##
Detect VID:PID values of the USB device and fill appropriate parameters.

_The default value is: Yes_

## USB tts(ac) port ##
Port number that used for device configuration and data transmission. One USB device is able to use several channels for different purposes but the only channel used for data transmission.

Firmware with linux kernel 2.4 use following names:
  * /dev/usb/tts/$port - modems
  * /dev/usb/acm/$port – cellular phones

Firmware with linux kernel 2.6 use following names:
  * /dev/ttyUSB$port - modems
  * /dev/ttyACM$port - cellular phones

When drivers (usbserial, acm, option) was loaded, really used device channels may be viewed at page **Status & Log - Diagnostic Information**.

_The default value is: 0_

## USB device Vendor ID (0xabcd) ##
This parameter is optional.

_The default value is: empty_

## USB device Product ID (0xefgh) ##
This parameter is optional.

_The default value is: empty_

## USB device packet size (0 for default) ##
This parameter is optional.

_The default value is: 4096_

## USB device serial speed (usually 921600) ##
This parameter is optional. Some modem drivers ignore this value.

_The default value is: 921600_

---

# Additional parameters #
## Additional AT commands ##
This parameter may be used for prioritizing network type selection. E.g. use 3G net for ZTE 626 modem with value AT+ZSNT=0,0,2

_The default value is: empty_

## Additional pppd options ##
This parameter is optional.

_The default value is: empty_

## Enable USB-Serial drivers ##
Load USB-Serial drivers pl2303, ftdi\_sio before connection.

_The default value is: Yes_

## Failure event script name ##
Run this script if max number of reconnection attempts was used. It is allow informing an administrator about connection problem or restarting devices.

If this parameter is empty, **/usr/local/sbin/post-3g-failure** name will use.

Simples script name is reboot. More complex see http://wl500g.info/attachment.php?attachmentid=6704&d=1278335883

Script parameters:
  * ifnum=$1 - interface number, may be used for example as **ppp$ifnum**
  * usbdev=$2 -  usb device, for example **/dev/usb/tts/0**
  * pppd\_res=$3 - return code from pppd, for example 5 - pppd was exited with SIGINT, SIGTERM or SIGHUP.

_The default value is: empty_

## Max number of reconnection attempts ##
Zero value 0 indicates that number of reconnection attempts will infinite. For using Failure event script it is none zero value required.

_The default value is: 0_