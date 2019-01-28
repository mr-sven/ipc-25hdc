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
    if ["$cam_ir_enabled" == "y"] ; then
        gpio i 1
    fi

    # init ait control
    v4l2aitcontrol --init

    # load framerate
    cam_framerate=`nvram_get cam_framerate`
    if [ -z "$cam_framerate" ] ; then
        cam_framerate=5
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
    v4l2aitcontrol --framerate $cam_framerate --quality $cam_quality --mirror $cal_mirror_flip --irmode $cam_ir_mode

fi
