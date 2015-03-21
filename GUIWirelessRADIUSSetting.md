# Index #
  * [Introduction](GUIWirelessRADIUSSetting#Introduction.md)
  * [Server IP Address](GUIWirelessRADIUSSetting#Server_IP_Address.md)
  * [Server Port](GUIWirelessRADIUSSetting#Server_Port.md)
  * [Connection Secret](GUIWirelessRADIUSSetting#Connection_Secret.md)


---


# Introduction #

This section allows you to set up additional parameters for authorizing wireless clients through RADIUS server.

For RADIUS to work on your wireless network you need to set the "Authentication Method" in the [Wireless - Interface](GUIWirelessInterface.md) page to "WPA-Enterprise", "WPA2-Enterprise" or "Radius with 802.1x"


---

## Server IP Address ##

The IP Address of the RADIUS server.

This should be a IPv4 address.


---

## Server Port ##

The udp port the RADIUS server is using.

default: 1812


---

## Connection Secret ##

This is the shared secret configured on the RADIUS server.

The secret is used to encrypt the data between the server and the router