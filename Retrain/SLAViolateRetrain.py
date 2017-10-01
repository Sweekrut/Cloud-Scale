#Author Suhaskrishna Gopalkrishna
import socket
import time
import os

#Listening port details of the SLO Violation notification listening server - Here it is the machine running CloudScale
UDP_IP = "152.46.18.191"
UDP_PORT = 7102

#A packet is received for each SLO violation. Each time a packet is received, check for the time since last retrain and initiate new retrain if within time bounds. To retrain, just remove the "*mm.txt" files
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
tstamp_old = time.time()
while True:
    data, addr = sock.recvfrom(1024)
    print data
    if data:
        tstamp_new = time.time()
        if (tstamp_new - tstamp_old) > 1000:
            tstamp_old = tstamp_new
            lists = os.listdir("/home/bbangla")
            for files in lists:
                if files.endswith("mm.txt"):
                    os.remove(os.path.join("/home/bbangla/program/result",files))

