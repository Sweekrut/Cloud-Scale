#Author: Suhaskrishna Gopalkrishna
import socket
import time
import os

#Specify the address and port on Web Server which listens to control messages for handling load balance in the system
UDP_IP = "192.168.122.37"
UDP_PORT = 7100

#Specify the different load balancing scenarios
STR1 = "Load PHPSer2"
STR2 = "Balance"
STR3 = "Load PHPSer1"
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
while True:
    #Start listening to control messages and take action as per the message received
    data, addr = sock.recvfrom(1024)
    dataStr = str(data,'utf-8')
    print(dataStr)
    if STR1 in dataStr:
        os.system("cp /etc/haproxy/LoadPHPSer2.cfg /etc/haproxy/haproxy.cfg")
        os.system("service haproxy restart")
    if STR2 in dataStr:
        os.system("cp /etc/haproxy/Balance.cfg /etc/haproxy/haproxy.cfg")
        os.system("service haproxy restart")
    if STR3 in dataStr:
        os.system("cp /etc/haproxy/LoadPHPSer1.cfg /etc/haproxy/haproxy.cfg")
        os.system("service haproxy restart")
