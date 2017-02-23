import web
import os
import subprocess
import shutil
from web import form
import time
import shlex
import re
from subprocess import Popen, PIPE

configpath="/home/pi/canpiserver/canpi.cfg"
messageFile="/home/pi/canpiserver/webserver/msg"
portlimit = 65535
invalid_ports = [21,22,80]


render = web.template.render('templates/')
urls = ('/', 'index',
        '/upload' , 'upload'
        )
app = web.application(urls, globals(),autoreload=True)

def readMessage():
    msg=""
    if os.path.isfile(messageFile) and os.access(messageFile, os.R_OK):
        f = open(messageFile)
        msg=f.read()
        f.close()
    return msg

def writeMessage(msg):
    try:
        f = open(messageFile, 'w+')
        f.write(msg)
        f.close()
    except:
        print("Failed to open the message file\n");
        return

class configManager:
    config={}
    def readConfig(self):
        self.config.clear()
        with open(configpath) as f:
            for line in f:
                #print(line + "\n")
                if ((line.strip().find("#") >= 0) or (len(line.strip())<1)):
                    continue
                strval = line.strip().replace('"','')
                strval = strval.strip().replace(';','')
                strval = strval.strip().replace(' = ','=')
                (key, val) = strval.split('=')
                self.config[key.strip()] = val
            f.close()
            #print("\n########## CONFIG ##########\n")
            #print(self.config)

    def getValue(self,key):
        if key in self.config:
            return self.config[key]
        return ""
    #insert a default value if does not exist
    def getValueInsert(self,key,value):
        if key in self.config:
            return self.config[key]
        else:
            self.setValue(key,value)
        return value

    def setValue(self,key,value):
        self.config[key] = value

    def saveFile(self):
        #create a copy of the file
        print("Saving file")
        tmpfile = configpath + ".tmp"
        with open(tmpfile,'w+') as f:
            for k,v in self.config.iteritems():
                line = ""
                #dont put quotes on numbers
                if k in ["cangrid_port","canid","button_pin","green_led_pin","yellow_led_pin","red_led_pin","start_event_id","shutdown_code"]:
                     line = k + "=" + v + "\n"
                else:
                     line = k + "=\"" + v + "\"\n"
                #print(line)
                f.write(line)
        f.close()
        #move the file
        if os.path.exists(tmpfile):
           print("Moving the file")
           shutil.move(tmpfile,configpath)

cm = configManager()
cm.readConfig()

id_grid_port="cangrid_port"
id_logfile="logfile"
id_loglevel="loglevel"
id_logappend="logappend"
id_canid="canid"
id_create_logfile="create_log_file"
id_start_event_id="start_event_id"
id_btn_save="btnSave"
id_btn_restart="btnRestart"
id_btn_stop="btnStop"
id_btn_restart_all="btnRestartAll"
id_btn_update_file="btnUpdateFile"

desc_grid_port="Grid connect Service port"
desc_logfile="Logfile"
desc_logappend="Append log"
desc_loglevel="Log Level"
desc_canid="Can ID"
desc_create_logfile="Creates log file"
desc_start_event="Start event id"
desc_btn_save="btnSave"
desc_btn_restart="btnRestart"
desc_btn_stop="btnStop"
desc_btn_restart_all="btnRestartAll"
desc_btn_update_file="btnUpdateFile"

def restartAll():
    #time.sleep(3)
    os.system("/etc/init.d/start_canpi.sh restart")

def shutdownPi():
    #time.sleep(3)
    os.system("/etc/init.d/start_canpi.sh shutdown")

class index:

    myform=form.Form()

    def GET(self):
        cm.readConfig()
        form = self.reloadMyForm()
        self.myform = form()
        # make sure you create a copy of the form by calling it (line above)
        # Otherwise changes will appear globally
        msg = readMessage()
        writeMessage("")
        return render.index(form,"Canpi Configuration",msg)

    def POST(self):
        userData = web.input()
        ctx=web.ctx
        myuri = ctx.realhome
        form = self.reloadMyForm()
        form.fill()

        if id_btn_save in userData:
            #get all the values and update
            r = True
            msg = ""
            r,msg = self.validate_form(form)
            if not r:
                writeMessage(msg)
                raise web.seeother('/')

            cm.setValue("cangrid_port", str(form[id_grid_port].value))
            cm.setValue("loglevel", str(form[id_loglevel].value))
            cm.setValue(id_create_logfile, str(form[id_create_logfile].checked))
            cm.setValue(id_logappend, str(form[id_logappend].checked))
            cm.setValue("canid", str(form[id_canid].value))
            cm.setValue(id_start_event_id, str(form[id_start_event_id].value))
            cm.saveFile()
            writeMessage("Save successful")
            raise web.seeother('/')

        if id_btn_update_file in userData:
            print("Upgrade button pressed")
            writeMessage("")
            raise web.seeother('/upload')

        if id_btn_restart in userData:
            print("Restart button pressed")
            writeMessage("")
            os.system("/etc/init.d/start_canpi.sh restartcanpi")
            exitcode,out,err = self.get_exitcode_stdout_stderr("ps -ef")
            msg=""
            if "/home/pi/canpiserver/canpiserver" in out:
                msg="Service restarted"
            else:
                msg="Restart failed"
            return render.index(form,"Canpiserver Configuration",msg)

        if id_btn_stop in userData:
            print("Stop button pressed")
            writeMessage("")
            os.system("/etc/init.d/start_canpi.sh stopcanpi")
            exitcode,out,err = self.get_exitcode_stdout_stderr("ps -ef")
            msg=""
            if "/home/pi/canpiserver/canpiserver" in out:
                msg="Stop failed"
            else:
                msg="Stop successfull"
            return render.index(form,"Canpiserver Configuration",msg)

        if id_btn_restart_all in userData:
            print("Restart all button pressed")
            writeMessage("")
            cpid = os.fork()
            if cpid == 0:
                restartAll()
            return render.reboot("Restarting the webpage and canpiserver",myuri)

        return render.index(form,"Canpiserver Configuration","")

    def validate_form(self,form):
        global desc_ed_tcpport
        global desc_grid_port

        if self.isint(form[id_canid].value):
            v = int(form[id_canid].value)
            if v <= 0 or v > 110:
                return False, "The CANID should be between 1 and 110"
        else:
            return False, "The CANID should be between 1 and 110"

        if self.isint(form[id_grid_port].value):
            v1 = int(form[id_grid_port].value)
            if v1 <= 0 or v1 > portlimit or v1 in invalid_ports:
                return False, desc_grid_port + " should be between 1 and " + str(portlimit) + " and not be " + str(invalid_ports)
        else:
            return False, desc_grid_port + " should be between 1 and " + str(portlimit) + " and not be " + str(invalid_ports)


        if self.isint(form[id_start_event_id].value):
            v = int(form[id_start_event_id].value)
            if v <= 0 or v > portlimit:
                print("Run validation\n")
                return False, desc_start_event + "[" + str(v) + "] should be between 1 and " + str(portlimit)
        else:
            return False, desc_start_event + "[" + str(v) + "] should be between 1 and " + str(portlimit)
  
        return True,"Saved success"

    def isint(self, value):
        try:
            int(value)
            return True
        except:
            return False

    def get_exitcode_stdout_stderr(self,cmd):
        """
        Execute the external command and get its exitcode, stdout and stderr.
        """
        args = shlex.split(cmd)

        proc = Popen(args, stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate()
        exitcode = proc.returncode
        return exitcode, out, err

    def reloadMyForm(self):

        apmode = True
        gridenable = True
        apmode_no_passwd = True
        create_logfile = True
        logappend = True

        if cm.getValueInsert(id_create_logfile, "True").lower() != "true":
            create_logfile = False
        if cm.getValueInsert(id_logappend, "True").lower() != "true":
            logappend = False

        myform = form.Form(
            form.Textbox(id_grid_port,description=desc_grid_port,value=cm.getValueInsert("cangrid_port",4444),id="cangripport"),
            form.Textbox(id_canid,description=desc_canid,value=cm.getValueInsert("canid",100)),
            form.Textbox(id_start_event_id,description=desc_start_event,value=cm.getValueInsert(id_start_event_id,1)),
            form.Dropdown(id_loglevel,  ['INFO', 'WARN', 'DEBUG'],value=cm.getValueInsert("loglevel","INFO")),
            form.Checkbox(id_create_logfile,description=desc_create_logfile,checked=create_logfile,value=id_create_logfile,id="tcreate_logfile"),
            form.Checkbox(id_logappend,description=desc_logappend,checked=logappend,value= id_logappend,id="tlogappend"),
            form.Button(desc_btn_save, id=id_btn_save, value="save", html="Save changes"),
            form.Button(desc_btn_stop, id=id_btn_stop, value="stop", html="Stop throttle service"),
            form.Button(desc_btn_restart_all, id=id_btn_restart_all, value="stop", html="Restart throttle and configuration page"),
            form.Button(desc_btn_restart, id=id_btn_restart, value="restart", html="Restart throttle service"),
            form.Button(desc_btn_update_file, id=id_btn_update_file, value="upgrade", html="Upload upgrade file"))

        return myform()

class upload:
    def GET(self):
        msg=readMessage()
        web.header("Content-Type","text/html; charset=utf-8")
        return """<html><head></head><body>
<header><h4>""" + msg + """</br></h4></header>
<form method="POST" enctype="multipart/form-data" action="">
<input type="file" name="myfile" />
<br/>
<input type="submit" />
</form>
</body></html>"""

    def POST(self):
        x = web.input(myfile={})
        filedir = '/home/pi/canpi/upgrade/' # change this to the directory you want to store the file in.
        if 'myfile' in x: # to check if the file-object is created
            filepath=x.myfile.filename.replace('\\','/') # replaces the windows-style slashes with linux ones.
            filename=filepath.split('/')[-1] # splits the and chooses the last part (the filename with extension)
            filepattern=re.compile(r"canpiwi-upgrade-\d+\.zip")

	    if (filepattern.match(filename)):
                fout = open(filedir +'/'+ filename,'w') # creates the file where the uploaded file should be stored
                fout.write(x.myfile.file.read()) # writes the uploaded file to the newly created file.
                fout.close() # closes the file, upload complete.
                os.system("/etc/init.d/start_canpi.sh upgrade")
                writeMessage("Upgrade file copied and applied.");
                #subprocess.call(['chmod', '+x', filedir +'/'+ filename])
	    else:
                writeMessage("File name incorrect. The expected format is canpiwi-upgrade-number.zip<br>Example: canpiwi-upgrade-20160720.zip");
                raise web.seeother('/upload')

        raise web.seeother('/')

if __name__=="__main__":
    web.internalerror = web.debugerror
    web.config.debug = True
    app.run()
