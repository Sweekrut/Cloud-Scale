#Author: Suhaskrishna Gopalkrishna
import socket
import time
import os

#Listening port details of the Handler
UDP_IP = "127.0.0.1"
UDP_PORT = 7000

#Listen to migration requests. Send control message to load balancer to transfer load onto the other server. Finally, call the main migration script - live.sh

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
tstamp_old = time.time()
tstamp_new = tstamp_old
while True:
    data, addr = sock.recvfrom(1024)
    dataStr = str(data,'utf-8')
    if "PHPSer1" in dataStr:
        tstamp_new = time.time()
        if (tstamp_new - tstamp_old) > 500:
            tstamp_old = tstamp_new
            Web_IP = "192.168.122.37"
            Web_port = 7100
            MESSAGE = "Load PHPSer2"
            udpsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            udpsock.sendto(MESSAGE, (Web_IP, Web_port))
            cmd = "./live.sh" + " PHPSer1"
            if(!os.system(cmd)):
                MESSAGE = "Balance"
                udpsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                udpsock.sendto(MESSAGE, (Web_IP, Web_port))

    if "PHPSer2" in dataStr:
        tstamp_new = time.time()
        if (tstamp_new - tstamp_old) > 500:
            Web_IP = "192.168.122.37"
            Web_port = 7100
            MESSAGE = "Load PHPSer1"
            udpsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            udpsock.sendto(MESSAGE, (Web_IP, Web_port))
            cmd = "./live.sh" + " PHPSer2"
            if(!os.system(cmd)):
                MESSAGE = "Balance"
                udpsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                udpsock.sendto(MESSAGE, (Web_IP, Web_port))

