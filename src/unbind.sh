#!/bin/sh

# Usage:
# bash bind.sh <device_id>

echo "Rebinding $1..."
echo -n "$1" > /sys/bus/usb/drivers/usbmouse/unbind
echo -n "$1" > /sys/bus/usb/drivers/usbhid/bind
