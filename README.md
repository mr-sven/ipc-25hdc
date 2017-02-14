# Maginon IPC-25 HDC Custom Firmware

The base of this repository is the GPL Source published by Supra for the IPC-25 HDC.

The content of the Instruction TXT
-
This document describes build steps to rebuild the open source components in the Maginon IPC-25HDC device. These instructions are only for the open source components and not for the proprietary components.

Preparing your system
- ----------------

First install Ubuntu 11.04 (32 bits) with following packages:

* ncurses-dev
* bison
* zlib1g-dev
* flex

It might be that some other packages need to be installed as well. Please note that Ubuntu 11.04 is no longer supported by Canonical, so you might need to update the configuration for "apt" to point to the right repositories on http://old-releases.ubuntu.com/

Please note that the build of the source code might fail on other versions of Ubuntu! We have verified that the build works on Ubuntu 11.04.

As a second step create a directory called /tftpboot, and possibly change ownership if you will not be building as "root".

As a third step unpack the precompiled toolchain in /opt. The toolchain can be found in the file buildroot-gcc342.tar.bz2 in the directory "toolchain". The reason that it has to be installed in /opt is because this is where the build configuration expects to find the toolchain. The toolchain comes in precompiled form. If you want to recompile the toolchain from scratch you can find the sources in the file "buildroot-gdb-src-pkt.tar.bz2" in the directory "toolchain". Building the toolchain is outside of the scope of this document.

Running the build
- ----------------

* run "make menuconfig", exit the menu and save the configuration
* run "make"

Depending on whether or not you are running the build as root you might, or might not, be asked for your password by "sudo" a number of times.

After compilation the firmware file can be found in the directory /tftboot

# Newer Ubuntu x64

In my case I was used to install `libc6-i386` and `lzma-alone` to run the Buildroot components and then link the lzma_alone to the Buildroot:
```bash

$ apt install libc6-i386 lzma-alone
$ ln -s /usr/bin/lzma_alone /opt/buildroot-gcc342/bin/lzma_alone

```

# RT5350 GPIO Pinout
| Pin | Direction | Function                               |
|----:|:---------:|----------------------------------------|
|   0 |     in    | Button on network connector socket     |
|  11 |    out    | IR-LED Enable                          |
|  12 |    out    | Enable Camera (Reset)                  |
|  13 |    out    | Yellow LED on network connector socket |
|  14 |     in    | IR-Filter active on Cam                |


