__author__ = 'amaurial'

import sys
import os
import canmodule
import tcpmodule
import logging
import time
import signal


logging.basicConfig(filename="canpi.log", filemode="w",level=logging.DEBUG, format='%(levelname)s - %(asctime)s - %(filename)s - %(funcName)s - %(message)s')

running=True

def receive_signal(signum,stack):
    logging.debug('Signal received. Stopping.')
    global running
    running = False

signal.signal(signal.SIGINT,receive_signal)
signal.signal(signal.SIGTERM,receive_signal)

device="can0"

logging.info('Starting CANPI')

#create the can manager
canThread = canmodule.CanManager(name="can",threadID=1,device=device)
#create the incomming can buffer consumer
bufferReader = canmodule.BufferReader(name="bufferReader",threadID=2)
#create the outgoing can buffer consumer
bufferWriter = canmodule.BufferWriter(name="bufferWriter", threadID=3, canManager=canThread)

#create the tcp server. can be more than one. each server can handle several clients
tcpServer = tcpmodule.TcpServer(host="pihost" ,port=4444, bufwriter=bufferWriter)
#register the tcp server as a can message consumer
bufferReader.register(tcpServer)

#start the incomming can buffer consumer
bufferReader.start()
#start the outgoing can buffer consumer
bufferWriter.start()
#start the can manager
canThread.start()
#start the tcp server
tcpServer.start()

while running:
    #do nothing
    time.sleep(3)

#stop all the components
canThread.stop()
tcpServer.stop()
bufferReader.stop()
bufferWriter.stop()

logging.info("Finishing %i" %os.getpid())
logging.shutdown()
#for some reason the tcp server is not dying gracefully. so we kill it
os.kill(os.getpid(), 9)
#sys.exit()
