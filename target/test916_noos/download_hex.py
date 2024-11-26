#! /usr/bin/env python3

import pylink
import time

jlink = pylink.JLink()
jlink.open()
rst = jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)

if (rst == True):
	print("Set Jlink SWD success\r")
else:
	print("Set Jlink SWD fail\r")
	jlink.close()

jlink.connect('ING9168xx')

rst = jlink.target_connected()

if (rst == True):
	print("Target connect success\r")
else:
	print("Target connect fail\r")
	jlink.close()

file_path = 'ing916.hex'
# file_path = '../ING918XX_SDK_SOURCE/bundles/noos_typical/ING9168xx/platform.hex'
jlink.flash_file(file_path, 0x2027000)
jlink.reset()

print(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()), end=" ")
print(file_path, "Flash done, close Jlink")
jlink.close()