#!/bin/sh

[ -f /var/run/etc ] && exit
touch /var/run/etc

# link profile file
ln -s /etc_ro/profile /etc/profile

# link passwd/group
ln -s /etc_ro/passwd /etc/passwd
ln -s /etc_ro/group /etc/group
cp /etc_ro/shadow /etc/shadow
chmod 600 /etc/shadow

# load root shadow from nvram
ROOTSHADOW=`nvram_get root_shadow`
if [ -n "$ROOTSHADOW" ] ; then
    nvram_get root_shadow | chpasswd -e
fi

# generate default config for factory reset
mkdir -p /etc/default
cp /etc_ro/default/FACTORY.dat /etc/default/FACTORY.dat

# preserve factory mac
echo factory_mac=`nvram_get factory_mac` >> /etc/default/FACTORY.dat
echo factory_wifimac=`nvram_get factory_wifimac` >> /etc/default/FACTORY.dat
