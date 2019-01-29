#!/bin/sh

[ -f /var/run/ntpclient ] && exit
touch /var/run/ntpclient

ntp_server=`nvram_get ntp_server`
if [ -n "$ntp_server" ] ; then
    ntpclient -h $ntp_server &
fi
