#!/bin/sh

[ -f /var/run/cam ] && exit
touch /var/run/cam

# 
cam_enabled=`nvram_get cam_enabled`

if [ "$cam_enabled" == "y" ] ; then
    # enable camera module
    gpio c 1

    # wait for camera usb init
    sleep 5

    # check if ir led enabled
    cam_ir_enabled=`nvram_get cam_ir_enabled`
    if [ "$cam_ir_enabled" == "y" ] ; then
        gpio i 1
    fi

    # init ait control
    v4l2aitcontrol --init

    # load framerate
    cam_framerate=`nvram_get cam_framerate`
    if [ -z "$cam_framerate" ] ; then
        cam_framerate=5
    fi

    # load bitrate
    cam_bitrate=`nvram_get cam_bitrate`
    if [ -z "$cam_bitrate" ] ; then
        cam_bitrate=2048
    fi

    # load quality
    cam_quality=`nvram_get cam_quality`
    if [ -z "$cam_quality" ] ; then
        cam_quality=75
    fi

    # load mirror flip
    cam_mirror_flip=`nvram_get cam_mirror_flip`
    if [ -z "$cam_mirror_flip" ] ; then
        cam_mirror_flip=0
    fi

    # load ir mode
    cam_ir_mode=`nvram_get cam_ir_mode`
    if [ -z "$cam_ir_mode" ] ; then
        cam_ir_mode=0
    fi

    # configure device
    v4l2aitcontrol --framerate $cam_framerate --quality $cam_quality --mirror $cam_mirror_flip
    v4l2aitcontrol --irmode $cam_ir_mode --bitrate $cam_bitrate

    # load cam stream port
    cam_port=`nvram_get cam_port`
    if [ -z "$cam_port" ] ; then
        cam_port=8080
    fi

    # load cam resolution
    cam_resolution=`nvram_get cam_resolution`
    if [ -z "$cam_resolution" ] ; then
        cam_resolution=1280x720
    fi

    # start uvc stream
    uvc_stream -b -r $cam_resolution -f $cam_framerate -p $cam_port --disable_control

    # store pid for watchdog
    echo "pidfile = /var/run/uvc_stream.pid" >> /etc/watchdog.conf
fi
