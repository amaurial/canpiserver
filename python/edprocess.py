
import logging
import threading
import select
from opc import *
import re
import time
import sys

#class that deals with the ED messages, basically this class holds the client socket and the major code

SOFT_VERSION = "VN2.0"
ROASTER_INFO = "RL0]|["  # no locos
START_INFO = "VN2.0\n\rRL0]|[\n\rPPA1\n\rPTT]|[\n\rPRT]|[\n\rRCC0\n\rPW12080\n\r"
DELIM_BRACET = "]|["
DELIM_BTLT = "<;>"
DELIM_KEY = "}|{"
EMPTY_LABELS = "<;>]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[\n" #MTS3<......MTLS1<;>]\[]\[]

RE_SPEED = re.compile('M[TA]+[SL\*]([0-9]+)?<;>[VX]([0-9]+)?') #regex to identify a set speed message
RE_SESSION = re.compile('M[TA]+\+[SL][0-9]+<;>\\w') #regex to identify a create session message
RE_REL_SESSION = re.compile('M[TA]+\-[SL*]([0-9]+)?<;>\\w') #regex to identify a release session message
RE_DIR = re.compile('M[TA]+[SL\*]([0-9]+)?<;>R[0-1]') #regex to identify a create session message
RE_QRY_SPEED = re.compile('M[TA]+[SL\*]([0-9]+)?<;>qV') #regex to identify a query speed
RE_QRY_DIRECTION = re.compile('M[TA]+[SL\*]([0-9]+)?<;>qR') #regex to identify a query direction
RE_FUNC = re.compile('M[TA]+[SL\*]([0-9]+)?<;>F[0-9]+') #regex to identify a query direction

CBUS_KEEP_ALIVE = 4000
ED_KEEP_ALIVE = 10000
BS = 128
ST = 0.003

class TcpClientHandler(threading.Thread):

    STATES = {'START': 255, 'WAITING_ED_RESP': 0, 'WAITING_SESS_RESP': 1, 'SESSION_ON': 2}
    STATE = ''

    # client is the tcp client
    # canwriter is the BufferWriter
    def __init__(self, client, address, canwriter, server, id):
        threading.Thread.__init__(self)
        self.client = client
        self.can = canwriter
        self.address = address
        self.canmsg = []
        self.sessions = {} #dict of sessions; key is the loco number
        self.running = True
        self.edsession = EdSession(0, "S")
        self.edname = ""
        self.hwinfo = ""
        self.server = server
        self.id = id

    def stop(self):
        self.running = False

    #main loop thread function
    def run(self):
        logging.info("Serving the tcp client %s" % self.address[0])

        #send the version software info and the throttle data
        logging.debug("Sending start info :%s" %START_INFO)
        self.sendClientMessage(START_INFO)

        size = 1024
        while self.running:
            try:
                ready = select.select([self.client], [], [], 1)
                if ready[0]:
                    data = self.client.recv(size)
                    if data:
                        response = data
                        self.handleEdMessages(data.decode("utf-8"))
                        #self.client.send(response)
                    else:
                        #raise Exception('Client disconnected')
                        logging.debug("Exception in client processing. Closing connection")
                        self.running = False
                #send keep alive in both directions
                self.sendKeepAlive()
            except BaseException as e:
                exc_type, exc_obj, exc_tb = sys.exc_info()
                logging.info("Exception in client processing in line %d\n %s" % (exc_tb.tb_lineno, str(e)))
                self.running = False

        logging.debug("Tcp server closing client socket for %s " % self.address[0])
        #del self.sessions
        self.server.removeClient(self.id)
        self.client.close()
        self.stop()


    def sendKeepAlive(self):
        try:
            for k,s in self.sessions.items():
                if s:
                    #10 seconds for tcp
                    #2 seconds for cbus
                    millis = int(round(time.time() * 1000))
                    if (millis - s.getClientTime()) > ED_KEEP_ALIVE:
                        #send enter
                        logging.debug("Sending keep alive to ED for loco %d" % s.getLoco())
                        self.sendClientMessage("\n")
                        m = int(round(time.time() * 1000))
                        self.sessions.get(s.getLoco()).setClientTime(m)

                    if (millis - s.getCbusTime()) > CBUS_KEEP_ALIVE:
                        #send keep alive
                        logging.debug("Sending keep alive to Cbus for loco %d" % s.getLoco())
                        self.can.put(OPC_DKEEP + bytes([s.getSessionID()]))
                        m = int(round(time.time() * 1000))
                        self.sessions.get(s.getLoco()).setCbusTime(m)
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception sending keep alive in line %d\n %s" % (exc_tb.tb_lineno, str(e)))


    #receive all the can message
    def canmessage(self, canid, size, data):
        try:
            logging.debug("Tcpclient received can msg: %s" % data)
            logging.debug("Opc: %s" % hex(data[0]))
            opc = data[0]

            if opc == OPC_PLOC[0]:

                logging.debug("Checking result of request session. OPC: PLOC %s %s" % (hex(data[2]) , hex(data[3])))

                session = int(data[1])
                Hb = data[2] & 0x3f
                Lb = data[3]

                loco = int.from_bytes(bytes([Hb, Lb]),byteorder='big')

                if self.edsession:
                    if self.edsession.getLoco() != loco:
                        logging.debug("PLOC %d not for this session %d. Discarding." % (loco, self.edsession.getLoco()))
                        return
                else:
                    logging.debug("No session request. Discarding.")
                    return

                speedir = int(data[4])
                f1 = data[5]
                f2 = data[6]
                f3 = data[7]

                speed = speedir & 0x7F #0111 1111
                direction = 0
                if (speedir & 0x80) > 127:
                    direction = 1
                #put session in array
                self.edsession.setDirection(direction)
                self.edsession.setSpeed(speed)
                self.edsession.setSessionID(session)
                logging.debug("Adding loco %d to sessions" % loco)
                self.sessions[loco] = self.edsession

                message = "MT+" + self.edsession.getAdType() + str(loco) + DELIM_BTLT + "\n"
                logging.debug("Ack client session created %d for loco %d :%s" % (session, loco, message))
                self.STATE = self.STATES['SESSION_ON']
                self.sendClientMessage(message)

                #set speed mode 128 to can
                self.can.put(OPC_STMOD + bytes([session]) + b'\x00')

                #send the labels to client
                labels = "MTLS" + str(loco) + EMPTY_LABELS + self.generateFunctionsLabel("S" + str(loco)) +  "\n"
                logging.debug("Sending labels")

                self.sendClientMessage(labels)

                logging.debug("Sending speed mode 128")
                msg = "MT" + self.edsession.getAdType() + str(loco) + DELIM_BTLT + "s0\n"
                self.sendClientMessage(msg)

                #logging.debug("Sending momentary for F0")
                #msg = "MT" + self.edsession.getAdType() + str(loco) + DELIM_BTLT + "F10\n"
                #self.sendClientMessage(msg)


            if opc == OPC_ERR[0]:
                logging.debug("OPC: ERR")

                Hb = data[2] & 0x3f
                Lb = data[3]
                loco = int.from_bytes(bytes([Hb, Lb]), byteorder='big')

                if self.edsession:
                    if loco != self.edsession.getLoco():
                        logging.debug("Error message for another client. Different loco number. Discarding.")
                        return
                else:
                    logging.debug("Error message for another client. No edsession set. Discarding.")
                    return

                err = int(data[3])
                if err == 1:
                    logging.debug("Can not create session. Reason: stack full")
                elif err == 2:
                    logging.debug("Err: Loco %d TAKEN" % loco)
                elif err == 3:
                    logging.debug("Err: No session %d" % loco)
                else:
                    logging.debug("Err code: %d" % err)
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception handling can messages in line %d\n %s"% (exc_tb.tb_lineno, str(e)))

        #self.client.send(str(canid).encode(encoding='ascii'))
        #self.client.send(b'-')
        #self.client.sendall(data.encode(encoding='ascii'))
        #self.client.send(b'\n')

    def handleEdMessages(self, message):

        try:

            logging.debug("Handling the client message :%s" % message)

            messages = message.split("\n")
            logging.debug("Split message: %s" % messages)

            for msg in messages:
                logging.debug("Processing message: %s" % msg)

                if len(msg) == 0:
                    continue

                #get the name
                if msg[0] == 'N':
                    self.edname = msg[1:]
                    logging.debug("ED name: %s" % self.edname)
                    self.sendClientMessage("\n")
                    continue

                #get hardware info
                if msg[0:1] == "HU":
                    self.hwinfo = msg[2:]
                    logging.debug("Received Hardware info: %s" % self.hwinfo)
                    logging.debug("Put session set timer in queue")
                    self.sendClientMessage("*10\n") #keep alive each 10 seconds
                    #TODO wait for confirmation: expected 0xa
                    continue

                #create session

                s = RE_SESSION.match(msg)
                if s:
                    logging.debug("Create session %s" % msg)
                    self.handleCreateSession(msg)
                    # wait until session is created
                    logging.debug("Waiting 2 secs for session to be created.")
                    time.sleep(2)
                    if len(self.sessions) > 0:
                        logging.debug("Session created after 2 secs.")
                        logging.debug(self.sessions)

                #set speed
                v = RE_SPEED.match(msg)
                if v:
                    self.handleSpeedDir(msg)
                    #time.sleep(ST)

                d = RE_DIR.match(msg)
                if d:
                    self.handleDirection(msg)

                q = RE_QRY_SPEED.match(msg)
                if q:
                    self.handleQuerySpeed(msg)

                q = RE_QRY_DIRECTION.match(msg)
                if q:
                    self.handleQueryDirection(msg)

                r = RE_REL_SESSION.match(msg)
                if r:
                    self.handleReleaseSession(msg)

                f = RE_FUNC.match(msg)
                if f:
                    self.handleSetFunction(msg)
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception handling ED messages in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    def getLoco(self,msg):
        i = msg.find("<")
        logging.debug("Extracted loco: %s" % msg[4:i])
        loco = int(msg[4:i])
        return loco

    def handleCreateSession(self, msg):
        try:
            logging.debug("Handle create session. MT+ found")
            #create session
            if msg[3] in ["S", "s", "L", "l"]:
                logging.debug("MT+S found")
                adtype = msg[3]
                logging.debug("Address type: %s" % adtype)
                #get loco
                loco = self.getLoco(msg)
                self.edsession.setLoco(loco)
                self.edsession.setAdType(adtype)
                #send the can data
                logging.debug("Put CAN session request in the queue for loco %d" % loco)
                self.STATE = self.STATES['WAITING_SESS_RESP']

                Hb = 0
                Lb = 0
                if (loco > 127) or (adtype in ["L", "l"]):
                    Hb = loco.to_bytes(2,byteorder='big')[0] | 0xC0
                    Lb = loco.to_bytes(2,byteorder='big')[1]
                else:
                    Lb = loco.to_bytes(2,byteorder='big')[1]

                self.can.put(OPC_RLOC + bytes([Hb]) + bytes([Lb]))
                return
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception while creating session in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    def handleReleaseSession(self, msg):
        try:

            logging.debug("Handle release session. MT- found")
            #release session
            self.sendClientMessage(msg + "\n")
            i = msg.find("*")
            relsessions = []
            #all sessions
            if i > 0:
                logging.debug("Releasing all sessions")
                for k , s in self.sessions.items():
                    if s:
                        #send the can data
                        sid = s.getSessionID()
                        logging.debug("Releasing session for loco KLOC %d" % s.getLoco())
                        self.can.put(OPC_KLOC + bytes([sid]))
                        time.sleep(1)
                        relsessions.append(k)
                #clear sessions
                for k in relsessions:
                    logging.debug("Delete session %s" % k)
                    del self.sessions[k]
                return

            loco = self.getLoco(msg)
            logging.debug("Releasing session for loco KLOC %d" % loco)

            session = self.sessions.get(loco)
            if session:
                #send the can data
                self.can.put(OPC_KLOC + bytes([session.getSessionID()]))
                time.sleep(1)
                #TODO check if it works
                del self.sessions[loco]
            else:
                logging.debug("No session found for loco %d" % loco)
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception while releasing session in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    #TODO
    def handleSpeedDir(self, msg):

        try:
            logging.debug("Handle speed request")
            self.sendClientMessage("\n")

            i = msg.find(">V")
            if i > 0:
                logging.debug("Extracted speed: %s" % msg[i+2:])
                speedString = msg[i+2:]
            else:
                i = msg.find(">X")
                if i > 0:
                    speedString = "X"
                else:
                    logging.debug("Bad speed message format. Discarding.")
                    return;

            speed = 0

            if speedString in ["X", "x"]:
                #stop
                speed = 1
            else:
                speed = int(speedString)
                if speed != 0:
                    speed = speed + 1

            i = msg.find("*")
            #all sessions
            if i > 0:
                logging.debug("Set speed for all sessions")
                for k, s in self.sessions.items():
                    if s:
                        logging.debug("Set speed %d for loco %d" % (speed, s.getLoco()))
                        self.sessions.get(s.getLoco()).setSpeed(speed)
                        sdir = s.getDirection() * BS + speed
                        self.can.put(OPC_DSPD + bytes([s.getSessionID()]) + bytes([sdir]))
                return

            #one session
            loco = self.getLoco(msg)
            session = self.sessions.get(loco)

            if session:
                #send the can data
                logging.debug("Set speed %d for loco %d" % (speed, loco))
                self.sessions.get(loco).setSpeed(speed)
                sdir = session.getDirection() * BS + speed
                self.can.put(OPC_DSPD + bytes([session.getSessionID()]) + bytes([sdir]))
            else:
                logging.debug("No session found for loco %d" % loco)
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception while handling speed message in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    def handleDirection(self, msg):
        try:
            logging.debug("Handle Direction request")
            self.sendClientMessage(msg + "\n")

            #get the direction
            i = msg.find(">R")
            logging.debug("Extracted direction: %s" % msg[i + 2:])
            d = int(msg[i + 2:])

            i = msg.find("*")
            #all sessions
            if i > 0:
                logging.debug("Set direction for all sessions")
                for k,s in self.sessions.items():
                    if s:
                        if d != s.getDirection():
                            #send the can data
                            self.sessions.get(s.getLoco()).setDirection(d)
                            logging.debug("Set direction %d for loco %d" % (d, s.getLoco()))
                            sdir = d * BS + s.getSpeed()
                            self.can.put(OPC_DSPD + bytes([s.getSessionID()]) + bytes([sdir]))
                return

            #one session
            loco = self.getLoco(msg)
            session = self.sessions.get(loco)
            if session:
                if d != session.getDirection():
                    #send the can data
                    self.sessions.get(loco).setDirection(d)
                    logging.debug("Set direction %d for loco %d" % (d, loco))
                    sdir = d * BS + session.getSpeed()
                    self.can.put(OPC_DSPD + bytes([session.getSessionID()]) + bytes([sdir]))
            else:
                logging.debug("No session found for loco %d" % loco)
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception while handling direction message in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    def handleQueryDirection(self, msg):
        try:
            logging.debug("Query Direction found")
            i = msg.find("*")
            if i > 0:
                logging.debug("Query direction for all locos")
                for k,s in self.sessions.items():
                    if s:
                        self.sendClientMessage("MTA" + s.getAdType() + str(s.getLoco()) + DELIM_BTLT + "R" + str(s.getDirection()) + "\n")
                return
            #get specific loco
            #TODO
            #get the direction
            loco = self.getLoco(msg)
            s = self.sessions.get(loco)
            if s:
                self.sendClientMessage("MTA" + s.getAdType() + str(s.getLoco()) + DELIM_BTLT + "R" + str(s.getDirection()) + "\n")
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception while handling query direction in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    def handleQuerySpeed(self, msg):
        try:
            logging.debug("Query speed found")
            i = msg.find("*")
            #all sessions
            if i > 0:
                logging.debug("Query direction for all locos")
                for k,s in self.sessions.items():
                    if s:
                        self.sendClientMessage("MTA" + s.getAdType() + str(s.getLoco()) + DELIM_BTLT + "V" + str(s.getSpeed()) + "\n")
                return

            #get specific loco
            loco = self.getLoco(msg)

            s = self.sessions.get(loco)
            if s:
                self.sendClientMessage("MTA" + s.getAdType() + str(s.getLoco()) + DELIM_BTLT + "R" + str(s.getSpeed()) + "\n")
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception while handling query speed in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    def handleSetFunction(self, msg):

        try:
            logging.debug("Set function request found")

            # the ED always sends an on and off we will consider just the on and toggle the function internally

            #get the function
            i = msg.find(">F")
            logging.debug("Extracted on/off: %s func: %s" % (msg[i + 2:i + 3], msg[i + 3:]))
            onoff = int(msg[i + 2:i + 3])



            fn = int(msg[i + 3:])
            i = msg.find("*")

            #all sessions
            if i > 0:
                for k,s in self.sessions.items():
                    if s:
                        if s.getFnType(fn) == 1 and onoff == 0:
                            logging.debug("Fn Message for toggle fn and for a off action. Discarding")
                        else:
                            self.sendFnMessages(s,fn,msg)
                return

            #one session
            loco = self.getLoco(msg)
            session = self.sessions.get(loco)
            if session:
                if session.getFnType(fn) == 1 and onoff == 0:
                    logging.debug("Fn Message for toggle fn and for a off action. Discarding")
                    return
                #send the can data
                self.sendFnMessages(session,fn,msg)
            else:
                logging.debug("No session found for loco %d" % loco)
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception while handling set functions in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    def sendFnMessages(self,session,fn, msg):

        try:

            logging.debug("Set function %d for loco %d" % (fn, session.getLoco()))

            fnbyte = 1

            #1 is F0(FL) to F4
            #2 is F5 to F8
            #3 is F9 to F12
            #4 is F13 to F19
            #5 is F20 to F28
            if 4 < fn and fn < 9 :
                fnbyte = 2
            if 8 < fn and fn < 13 :
                fnbyte = 3
            if 12 < fn and fn < 20 :
                fnbyte = 4
            if 19 < fn and fn < 29 :
                fnbyte = 5

            if session.getFnState(fn) == 1:
                session.setFnState(fn,0)
            else:
                session.setFnState(fn,1)

            #send status to ED
            i = msg.find(">F")
            logging.debug("message: %s" % msg[0:(i+2)])
            #logging.debug(str(session.getFnState(fn)))

            msgout = msg[0:(i+2)] + str(session.getFnState(fn)) + str(fn) + "\n"
            self.sendClientMessage(msgout)

            #send msg to CBUS
            fnbyte2 = session.getDccByte(fn)
            self.can.put(OPC_DFUN + bytes([session.getSessionID()]) + bytes([fnbyte]) + bytes([fnbyte2]))
        except BaseException as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            logging.info("Exception while sending fn messages in line %d\n%s" % (exc_tb.tb_lineno, str(e)))

    def sendClientMessage(self, message):
        logging.debug("Message to ED: %s" % message)
        self.client.sendall(message.encode('utf-8'))

    @staticmethod
    def generateFunctionsLabel(locoaddr):
        s = "MTA" + locoaddr + DELIM_BTLT #MTS1<;>
        a = ""
        for f in range(0, 29):
            a = a + s + "F0" + str(f) + "\n"
        a = a + s + "V0\n" + s + "R1\n" + s + "s0\n"
        return a

class EdSession:
    def __init__(self, loco, adtype):
        self.sessionid = 0
        self.keepalivetime = 0
        self.loco = loco
        self.adtype = adtype #S =short address or L = long adress
        self.speed = 0
        self.direction = 1 #forward
        self.clientTime = 0
        self.cbusTime = 0
        self.fns = []
        self.fnstype = [] # 0 for momentary 1 for toggle
        for fn in range(0,29):
            self.fns.append(0)
            self.fnstype.append(0)
        self.fnstype[0] = 1 #light is toggle

    def getSessionID(self):
        return self.sessionid

    def setSessionID(self, sessionid):
        self.sessionid = sessionid

    def getLoco(self):
        return self.loco

    def setLoco(self, loco):
        self.loco = loco

    def getAdType(self):
        return self.adtype

    def setAdType(self, adtype):
        self.adtype = adtype

    def setSpeed(self, speed):
        self.speed = speed

    def getSpeed(self):
        return self.speed

    def setDirection(self, direction):
        self.direction = direction

    def getDirection(self):
        return self.direction

    def setClientTime(self, millis):
        self.clientTime = millis

    def getClientTime(self):
        return self.clientTime

    def setCbusTime(self, millis):
        self.cbusTime = millis

    def getCbusTime(self):
        return self.cbusTime

    def setFnType(self, fn , state):
        if state != 0 and state != 1:
            return
        self.fnstype[fn] = state

    def getFnType(self,fn):
        return self.fnstype[fn]

    def setFnState(self, fn , state):
        if state != 0 and state != 1:
            return
        self.fns[fn] = state

    def getFnState(self,fn):
        return self.fns[fn]

    def getDccByte(self,fn):
        # create the byte for the DCC
        #1 is F0(FL) to F4
        #2 is F5 to F8
        #3 is F9 to F12
        #4 is F13 to F19
        #5 is F20 to F28
        # see http://www.nmra.org/sites/default/files/s-9.2.1_2012_07.pdf
        fnbyte = 0x00
        i = 0
        j = 0
        if -1 < fn and fn < 5 :
            i = 0
            j = 5
        if 4 < fn and fn < 9 :
            i = 5
            j = 9
        if 8 < fn and fn < 13 :
            i = 9
            j = 13
        if 12 < fn and fn < 20 :
            i = 13
            j = 20
        if 19 < fn and fn < 29 :
            i = 20
            j = 29

        if i == j == 0:
            return -1
        k = 0
        for f in range(i,j):
            active = self.fns[f]
            if active == 1:
                if f == 0:#special case: light
                    fnbyte = self.set_bit(fnbyte, 4)
                else:
                    fnbyte = self.set_bit(fnbyte, k)
            if f != 0:
                k = k + 1

        return fnbyte


    def set_bit(self, value, bit):
        return value | (1<<bit)

    def clear_bit(self, value, bit):
        return value & ~(1<<bit)




class State:
    def __init__(self,name):
        self.name = name
    def __str__(self):
        return self.name

    def next(self,input):
        logging.debug("state")
