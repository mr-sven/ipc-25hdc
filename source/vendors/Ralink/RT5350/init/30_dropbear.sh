#!/bin/sh

[ -f /var/run/dropbear ] && exit
touch /var/run/dropbear

# create dropbear folder
mkdir -p /etc/dropbear

# load ssh key from nvram
nvram_get ssh_key | uudecode

# check if ssh key could be loaded
if [ $? -ne 0 ] ; then

    SSH_KEY=/etc/dropbear/dropbear_rsa_host_key

    #Check for the Dropbear RSA key
    if [ ! -f $SSH_KEY ] ; then
        echo Generating RSA Key...
        dropbearkey -t rsa -f $SSH_KEY
        nvram_set ssh_key "`uuencode $SSH_KEY < $SSH_KEY`"
    fi
fi

dropbear
