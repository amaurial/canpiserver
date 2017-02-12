__author__ = 'amaurial'


import socket
import struct
import sys
import logging
import select
import time

def dissect_can_frame(frame):
    can_id, can_dlc, data = struct.unpack(can_frame_fmt, frame)
    return (can_id, can_dlc, data[:can_dlc])
cantimeout = 0.01 #seconds
recbuffer = 16
incan = []
canid = 110

can_frame_fmt = "=IB3x8s"
device = "can0"

can = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
can.bind((device, ))
while True:
    ready = select.select([can],[],[],cantimeout)
    if ready[0]:
           cf,addr = can.recvfrom(recbuffer)
           incan.append(cf)
           canid,candlc,data= dissect_can_frame(cf)
           datahex=":".join("{:02x}".format(c) for c in data)
           print('Received: can_id=%x, size=%x, datahex=%s' %(canid, candlc, datahex) )

