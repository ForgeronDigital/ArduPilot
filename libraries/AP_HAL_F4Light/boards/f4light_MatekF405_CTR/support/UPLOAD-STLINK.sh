#!/bin/sh

#binary with bootloader
/usr/local/stlink/st-flash  --reset write ../../../../../ArduCopter/f4light_MatekF405_CTR.bin 0x08010000 && \
/usr/local/stlink/st-util -m


