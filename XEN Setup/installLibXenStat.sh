#!/bin/sh

#Author: Bharath Banglaore Veeranna

#Description: Installs libxenstat library in Xen hypervisor

sudo apt-get install alien dpkg-dev debhelper build-essential
sudo alien -c --to-tgz *.rpm
sudo alien -c *.tgz

dpkg -i *.deb

rm -rf libxen*.tgz
rm -rf libxen*.deb

sudo apt-get install -y  python-numpy
sudo apt-get install -y python-scipy
sudo apt-get install -y python-pandas
sudo apt-get install -y python-setuptools
sudo apt-get install -y python-dev
easy_install statsmodels
