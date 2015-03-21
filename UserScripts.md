All pre/post scripts must conform following rules:
  * Stored in /usr/local/sbin directory
  * Has `#!/bin/sh` in first line in case of it shell script (not ELF executable)

List of scripts:
  1. **pre-boot** - launched before main initialization, when no kernel modules are loaded, but flashfs is expanded & all files from /usr/etc copied to /etc.<br><br>
<ol><li><b>post-boot</b> - executed after all services started, except webcam & usb disc<br><br>
</li><li><b>pre-shutdown</b> - launched before reboot.<br><br>
</li><li><b>pre-mount</b> - launched on hotplug usb disc before mount, suitable to manual mounts, disk checks, etc. Arguments:<br>  $1 - product ID (example: "58f/6387/142")<br><br>
</li><li><b>post-mount</b> - launched, when disc mount is finished, to achieve custom user actions. Arguments:<br>   $1 - product ID<br>  $2 - mounted directory<br><br>
</li><li><b>post-firewall</b> - executed every time, after router changes routing & firewall rules (for example, after new IP assigned to WAN), to allow add custom rules in firewall tables. Arguments:<br>   $1 / $2 - WAN interface name / WAN IP<br>  $3 / $4 - LAN interface name / LAN IP<br> $5 / $6 - DMZ interface name / DMZ IP<br><br>
</li><li><b>ez-setup</b> - executed on EZ-SETUP button pressed<br><br></li></ol>

<b>WARNING:</b> <i>Incorrect files could prevent your wl500g from booting. Double check your changes before committing. If you wl500g failed to boot flash it with other firmware version and execute these commands:</i>
<pre><code> nvram set boot_local<br>
 nvram commit<br>
</code></pre>
<i>After that you can reflash your wl500g back to the latest firmware and boot normal</i>