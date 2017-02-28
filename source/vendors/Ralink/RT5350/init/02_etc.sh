#!/bin/sh

[ -f /var/run/etc ] && exit
touch /var/run/etc

# link profile file
ln -s /etc_ro/profile /etc/profile
