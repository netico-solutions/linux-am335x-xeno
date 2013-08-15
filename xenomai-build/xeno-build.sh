#!/bin/sh

cflags="-march=armv7-a -mfpu=vfp3"
ldflags="-march=armv7-a -mfpu=vfp3"
host_compiler="arm-linux-gnueabihf"
staging_path="/srv/nfs/testing"

../xenomai-2.6.2.1/configure CFLAGS=$cflags LDFLAGS=$ldflags --host=$host_compiler
make DESTDIR=$staging_path
