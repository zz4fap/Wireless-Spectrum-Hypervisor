#!/bin/bash
# NEED to be a sudoer in your system to run this without prompt for password
# execute "sudo visudo" and add the following line to become a sudoer without need for password:
# username ALL=(ALL) NOPASSWD: ALL


if [ "$EUID" -ne 0 ]
    then echo "Please run as root"
    exit
fi

IP=${1:-192.168.2.1/24}

exec > /dev/null 2>&1
mkdir -p build
cd build
cmake ..
make
cd ..

sleep 2

sudo build/phy/srslte/examples/phy -v -f 1000000000 -B 20000000 > phy_log.txt 2>&1 &
sleep 10
