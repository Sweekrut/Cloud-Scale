#!/bin/bash
#Author: Suhaskrishna Gopalkrishna
#Main script used for live migration

#Command line argument specifies the VM to be migrated
vmname=$1

#Specify password for the user of the target host
pass="password"

#Collect stats in file
date > mig_stat.txt

#Call the expect script which spawns thread to handle migration
./mig.sh $pass $vmname >> mig_stat.txt
date >> mig_stat.txt
sleep 1

#Check if migration succeeded. If yes, apply NAT rules on the client and the target hosts to keep connection to VM alive.
ret="$(grep -i success mig_time.txt)"
if [ $? == 0 ]; then
#Call script to take care of the NAT rules on the target host
./target_nat.sh $pass
#Apply NAT rules on the client
sudo iptables -t nat -A  PREROUTING -d 192.168.122.229 -j DNAT --to-destination 152.46.20.57
sudo iptables -t nat -A  PREROUTING -s 192.168.122.229 -p tcp --dport 3306 -j DNAT --to-destination 192.168.122.211
sudo iptables -t nat -A  PREROUTING -s 192.168.122.229 -j DNAT --to-destination 192.168.122.37
fi
