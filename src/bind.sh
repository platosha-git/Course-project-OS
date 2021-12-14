#!/bin/sh

# Usage:
# bash bind.sh <device_id>

echo "Rebinding $1..."
echo -n "$1" > /sys/bus/usb/drivers/usbhid/unbind
echo -n "$1" > /sys/bus/usb/drivers/usbmouse/bind