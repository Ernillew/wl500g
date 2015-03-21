Originally from http://oleg.wl500g.info/sdram.html

This pages focus on memory controller configuration values, stored in the nvram and used for BCM3302 memory controller configuration found in the BCM4712, BCM53xx series of SoC and BCM5836 CPU

There are several references, which was used to make this summary page, including GPL source code tarballs, distributed by Linksys and ASUS and CFE distributed by Broadcom.

Memory configuration is perfromed by code, assembled from shared/sbsdram.S file. Depending on version it's either uses embedded nvram header values (found in PMON/CFE loader at offset 0x100 or 0x400) or values, from current nvram. These values are generated from sdram`_*` values either during bootloader flashing or via nvram update using nvram commit command.

The exact sdram configuration variables are: **sdram\_init**, **sdram\_config**, **sdram\_refresh** and **sdram\_ncdl**. They are used as following:

  * **sdram\_init** determines the memory controller operation mode, i.e. DDR/SDR cycles, 16/32 bit memory interface, number of bits in CAS signal, etc.:
|Field|Name|Description|
|:----|:---|:----------|
|0 |MemType|Memory type in use.<br> 0: SDR SDRAM<br> 1: DDR SDRAM<br>
<tr><td>1 </td><td>16BitMem</td><td>Memory interface.<br> 0: 32-bit interface<br> 1: 16-bit interface</td></tr>
<tr><td>4:2</td><td>ColWidth</td><td>Column width in use.<br> 000: 8-bit column<br> 010: 9-bit column<br> 100: 10-bit column</td></tr>
<tr><td>13</td><td>Clock</td><td>Clock source?<br> 0: External clock<br> 1: Internal clock</td></tr></li></ul></tbody></table>

<ul><li><b>sdram_config</b>  is used to initialize the mode register (during MRS cycle) of the SDRAM, it contains things defined by JEDEC: burst type, burst length, CAS latency;<br>
<table><thead><th>Field</th><th>Name</th><th>Description</th></thead><tbody>
<tr><td>0:2</td><td>BurstLength</td><td>Burst Length value.<br> 000: 1<br> 001: 2<br> 010: 4<br> 011: 8<br> 111: full page</td></tr>
<tr><td>3 </td><td>BurstType</td><td>Burst Type value.<br> 0: Sequential<br> 1: Interleave</td></tr>
<tr><td>6:4</td><td>CASLatency</td><td>CAS Latency.<br> 001: 1<br> 010: 2<br> 011: 3<br> 101: 1.5<br> 110: 2.5<br> 111: 3.5</td></tr></li></ul></tbody></table>

<ul><li><b>sdram_refresh</b>  seems to be leaved for compatibility, and not used anymore;<br>
</li><li><b>sdram_ncdl</b> contains various memory controller timing information, this could be either 0, which indicates to perform auto-tuning during memory initialzation (it will be updated to real value once initialization completed by CFE) or predefined value.</li></ul>

Sample memory configurations which could be found in the sources provided by Broadcom:<br>
<table><thead><th>Size</th><th>Type</th><th>Organization</th><th>sdram_init</th><th>bits</th><th>RAS</th><th>CAS</th><th>Technology</th></thead><tbody>
<tr><td>8MB</td><td>SDR</td><td>4 Meg x 16</td><td>0x0002</td><td>000 1 0</td><td>A0-A11</td><td>A0-A7</td><td>64Mbit</td></tr>
<tr><td>16MB</td><td>SDR</td><td>4 Meg x 16 x 2</td><td>0x0000</td><td>000 0 0</td><td>A0-A11</td><td>A0-A7</td><td>64Mbit</td></tr>
<tr><td>16MB</td><td>SDR</td><td>8 Meg x 16</td><td>0x000a</td><td>010 1 0</td><td>A0-A11</td><td>A0-A8</td><td>128Mbit</td></tr>
<tr><td>32MB</td><td>DDR</td><td>16 Meg x 16</td><td>0x000b</td><td>010 1 1</td><td>A0-A12</td><td>A0-A8</td><td>256Mbit</td></tr>
<tr><td>32MB</td><td>SDR</td><td>16 Meg x 16</td><td>0x000a</td><td>010 1 0</td><td>A0-A12</td><td>A0-A8</td><td>256Mbit</td></tr>
<tr><td>32MB</td><td>SDR</td><td>8 Meg x 16 x 2</td><td>0x0008</td><td>010 0 0</td><td>A0-A11</td><td>A0-A8</td><td>128Mbit</td></tr>
<tr><td>64MB</td><td>DDR</td><td>16 Meg x 16 x 2</td><td>0x0009</td><td>010 0 1</td><td>A0-A12</td><td>A0-A8</td><td>256Mbit</td></tr>
<tr><td>64MB</td><td>SDR</td><td>16 Meg x 16 x 2</td><td>0x0008</td><td>010 0 0</td><td>A0-A12</td><td>A0-A8</td><td>256Mbit</td></tr>
<tr><td>128MB</td><td>DDR</td><td>32 Meg x 16 x 2</td><td>0x0011</td><td>100 0 1</td><td>A0-A12</td><td>A0-A9</td><td>512Mbit</td></tr>
<tr><td>128MB</td><td>SDR</td><td>32 Meg x 16 x 2</td><td>0x0010</td><td>100 0 0</td><td>A0-A12</td><td>A0-A9</td><td>512Mbit</td></tr>
<tr><td>256MB</td><td>DDR</td><td>64 Meg x 16 x 2</td><td>0x0011</td><td>100 0 1</td><td>A0-A13</td><td>A0-A9</td><td>1Gbit</td></tr>