#!/bin/sh

[ -f /var/run/hardware ] && exit

touch /var/run/hardware

# load wireless module
insmod rt2860v2_sta

# setup mac addresses
ifconfig eth2 hw ether `nvram_get factory_mac`
ifconfig ra0 hw ether `nvram_get factory_wifimac`

