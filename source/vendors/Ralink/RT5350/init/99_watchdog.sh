#!/bin/sh

[ -f /var/run/watchdog ] && exit
touch /var/run/watchdog

# copy config
cat /etc_ro/watchdog.conf >> /etc/watchdog.conf

# load watchdog module
insmod ralink_wdt

# start watchdog
watchdog
