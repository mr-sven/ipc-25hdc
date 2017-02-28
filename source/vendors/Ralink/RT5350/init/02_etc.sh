#!/bin/sh

[ -f /var/run/etc ] && exit
touch /var/run/etc

# link profile file
ln -s /etc_ro/profile /etc/profile

# generate default config for factory reset
mkdir -p /etc/default
cp /etc_ro/default/FACTORY.dat /etc/default/FACTORY.dat
cat etc_ro/default/RT2860.dat >> /etc/default/FACTORY.dat
echo factory_mac=`nvram_get factory_mac` >> /etc/default/FACTORY.dat
echo factory_wifimac=`nvram_get factory_wifimac` >> /etc/default/FACTORY.dat
echo WAN_MAC_ADDR=`nvram_get WAN_MAC_ADDR` >> /etc/default/FACTORY.dat
