#!/usr/bin/expect -f
#Author: Suhaskrishna Gopalkrishna
#Expect script to handle migration
#Command line arguments - arg1 specifies user password, arg2 specifies VM to be migrated
set pass [lindex $argv 0];
set vm [lindex $argv 1];
set timeout 100
#Create a thread to start the migration session
spawn sudo xl migrate $vm sgopalk@152.46.20.57 -l
expect "ssword: "
send "$pass\r"
set timeout 100
expect "successful."

