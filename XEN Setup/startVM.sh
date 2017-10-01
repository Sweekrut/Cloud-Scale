#!/bin/sh

#Author: Bharath Banglaore Veeranna

#Description: Script to start a new VM. Pass the VM name to create. Name could be any string 

vmName=$1

mirror="http://ubuntu.mirrors.wvstateu.edu"
distr="trusty"

sudo mkdir -p /var/lib/xen/images/ubuntu-netboot/trusty14LTS
cd /var/lib/xen/images/ubuntu-netboot/trusty14LTS

sudo rm -rf *

wget $mirror/dists/$distr/main/installer-amd64/current/images/netboot/xen/vmlinuz
wget $mirror/dists/$distr/main/installer-amd64/current/images/netboot/xen/initrd.gz

xen-create-image --hostname=$vmName   --memory=512mb  --size=500M  --vcpus=2   --lvm=ubuntu-vg   --dhcp  --dist=$distr --mirror=$mirror --bridge=virbr0

xl create /etc/xen/$vmName.cfg

