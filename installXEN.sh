#!/bin/sh

#Author: Bharath Banglaore Veeranna
# Description: Script which installs XEN hypervisor in a ubuntu host


sudo apt-get -y update

sudo apt-get install -y xen-hypervisor-4.4-amd64

dpkg-divert --divert /etc/grub.d/08_linux_xen --rename /etc/grub.d/20_linux_xen

update-grub

sudo reboot
