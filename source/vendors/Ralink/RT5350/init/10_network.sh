#!/bin/sh

[ -f /var/run/network ] && exit
touch /var/run/network

# wifi config
wifi_enabled=`nvram_get wifi_enabled`

if [ "$wifi_enabled" == "y" ] ; then

    # load nvram data
    wifi_mode=`nvram_get wifi_mode`
    SSID=`nvram_get SSID`
    WPAPSK=`nvram_get WPAPSK`
    WirelessMode=`nvram_get WirelessMode`
    EncrypType=`nvram_get EncrypType`
    CountryRegion=`nvram_get CountryRegion`
    CountryCode=`nvram_get CountryCode`
    WIFI_INT=ra0

    # create config folder
    mkdir -p "/etc/Wireless/RT2860"

    # create base config
    cat > "/etc/Wireless/RT2860/RT2860.dat" <<EOL
# The word of "Default" must not be removed
Default
NetworkType=Infra
Channel=1
AuthMode=WPA2PSK
CountryRegionABand=7
ChannelGeography=1
BeaconPeriod=100
TxPower=100
RxPower=100
TxBurst=1
HT_BW=1
SSID=$SSID
WPAPSK=$WPAPSK
WirelessMode=$WirelessMode
EncrypType=$EncrypType
CountryRegion=$CountryRegion
CountryCode=$CountryCode
EOL

    # enable wifi interface
    ifconfig $WIFI_INT up

    # select network mode
    case "$wifi_mode" in
        dhcp)
            udhcpc -i $WIFI_INT -n
            ;;
        static)
            
            ;;
    esac
fi
