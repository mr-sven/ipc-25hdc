#!/bin/sh

[ -f /var/run/dropbear ] && exit
touch /var/run/dropbear

# create dropbear folder
mkdir -p /etc/dropbear

# load ssh rsa key from nvram
nvram_get ssh_rsa_key | uudecode

# check if ssh rsa key could be loaded
if [ $? -ne 0 ] ; then

    SSH_RSA_KEY=/etc/dropbear/dropbear_rsa_host_key

    #Check for the Dropbear RSA key
    if [ ! -f $SSH_RSA_KEY ] ; then
        echo Generating RSA Key...
        dropbearkey -t rsa -f $SSH_RSA_KEY
        nvram_set ssh_rsa_key "`uuencode $SSH_RSA_KEY < $SSH_RSA_KEY`"
    fi
fi

# load ssh dss key from nvram
nvram_get ssh_dss_key | uudecode

# check if ssh dss key could be loaded
if [ $? -ne 0 ] ; then

    SSH_DSS_KEY=/etc/dropbear/dropbear_dss_host_key

    #Check for the Dropbear DSS key
    if [ ! -f $SSH_DSS_KEY ] ; then
        echo Generating DSS Key...
        dropbearkey -t dss -f $SSH_DSS_KEY
        nvram_set ssh_dss_key "`uuencode $SSH_DSS_KEY < $SSH_DSS_KEY`"
    fi
fi

dropbear
