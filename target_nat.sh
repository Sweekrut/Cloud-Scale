#!/usr/bin/expect -f
#Author: Suhaskrishna Gopalkrishna
#Expect script to configure NAT rules on the target host
#Command line arg1 - user password of target host
set pass [lindex $argv 0];
set timeout 30
#Create a thread to start session to add NAT rules on the target
spawn ssh sgopalk@152.46.20.57
expect "passw"
send "\r"
expect "sgopalk"
send "sudo iptables -t nat -A  PREROUTING -s 152.7.99.135 -j DNAT --to-destination 192.168.122.229\r"
expect "$"
send "sudo iptables -t nat -A POSTROUTING -s 152.7.99.135/32 -p tcp --sport 3306 -j SNAT --to-source 192.168.122.211\r"
expect "$"
send "sudo iptables -t nat -A POSTROUTING -s 152.7.99.135/32 -j SNAT --to-source 192.168.122.37\r"
expect "$"
send "exit\r"
expect "$"

