#!/bin/bash

for num in {0..9}
do
    echo "on $num" > /dev/berkelder-driver
    dmesg | grep "berkelder-driver" | tail -2
    cat /dev/berkelder-driver
    sleep 1
done

echo "off" > /dev/berkelder-driver
dmesg | grep "berkelder-driver" | tail -2
cat /dev/berkelder-driver