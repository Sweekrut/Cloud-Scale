#!/bin/sh

#Author: Bharath Banglaore Veeranna

#Description: Script which install necessary packages reuired for 
#starting a VM and configures the network settings.

sudo stop network-manager
echo "manual" | sudo tee /etc/init/network-manager.override

sudo apt-get install -y bridge-utils

echo "iface xenbr0 inet dhcp" >>/etc/network/interfaces 
echo "bridge_ports eth0" >>/etc/network/interfaces

echo "iface eth0 inet manual" >>/etc/network/interfaces

sudo ifdown eth0 && sudo ifup xenbr0 && sudo ifup eth0

echo "net.bridge.bridge-nf-call-ip6tables = 0" >> /etc/sysctl.conf
echo "net.bridge.bridge-nf-call-iptables = 0" >> /etc/sysctl.conf
echo "net.bridge.bridge-nf-call-arptables = 0" >> /etc/sysctl.conf

sudo sysctl -p /etc/sysctl.conf

lvreduce -L 100M /dev/ubuntu-vg/swap_1

sudo apt-get install -y xen-tools

sudo apt-get install -y virtinst

sudo mkdir /usr/share/xen-tools/vivid.d

cp /usr/share/xen-tools/debian.d/* vivid.d/
