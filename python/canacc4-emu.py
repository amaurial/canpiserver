__author__ = 'amaurial'


import socket
import struct
import sys
import logging
import select
import time
from opc import *


can_frame_fmt = "=IB3x8s"
recbuffer = struct.calcsize(can_frame_fmt)#16

def dissect_can_frame(frame):
    can_id, can_dlc, data = struct.unpack(can_frame_fmt, frame)
    return (can_id, can_dlc, data[:can_dlc])

def build_can_frame(canid, data):
    can_dlc = len(data)
    #data = data.ljust(8, b'\x00')
    return struct.pack(can_frame_fmt, canid, can_dlc, data)

cantimeout = 0.01 #seconds
incan = []

can_frame_fmt = "=IB3x8s"
device = "can0"

can = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
can.bind((device, ))
counter=0
while True:
    ready = select.select([can],[],[],cantimeout)
    if ready[0]:
           cf,addr = can.recvfrom(recbuffer)
           incan.append(cf)
           canid,candlc,data= dissect_can_frame(cf)
           datahex=":".join("{:02x}".format(c) for c in data)
           #print('Received: can_id=%x, size=%x, datahex=%s' %(canid, candlc, datahex) )
           datahex.encode('ascii')
           msg=datahex.split(':')
           counter=counter + 1
           if (msg[0] == '73' and msg[1] == '01' and msg[2] == '00' and msg[3] == '09' ):
              #print('send PARAM')
              d0=1
              d1=0
              d2=9
              d3=1 
              msgsend=OPC_PARAN + b'\x01\x00\x09\x01'
              frame=build_can_frame(24,msgsend)
              can.send(frame)
              counter=1
           
           if (msg[0] == '5c' and msg[1] == '01' and msg[2] == '00'):
              print('BOOT')

           if (canid == 0x80000004 ):
              print("extended frame")

           if (canid == 0x80000004 and msg[0] == '00' and msg[1] == '00' and msg[2] == '00' and msg[3]=='00' and msg[4]=='0d' and msg[5] == '04' and msg[6] == '00' and msg[7] == '00'):
              #print("message 1")
              id=0x10000004 | 0x80000000
              msgsend=bytes([2])
              frame=build_can_frame(id,msgsend)
              can.send(frame)
           if (canid == 0x80000004 and msg[0] == '00' and msg[1] == '00' and msg[2] == '00' and msg[3]=='00' and msg[4]=='0d' and msg[5] == '03' and msg[6] == '38' and msg[7] == '15'):
              #print("message 22")
              id=0x10000004 | 0x80000000
              msgsend=bytes([1])
              frame=build_can_frame(id,msgsend)
              can.send(frame)
              print ('pacotes %d' %(counter))

