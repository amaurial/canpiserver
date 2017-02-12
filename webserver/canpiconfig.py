import web
import os
import subprocess
import shutil
from web import form
import time
import shlex
import re
from subprocess import Popen, PIPE

configpath="/home/pi/canpi/canpi.cfg"
messageFile="/home/pi/canpi/webserver/msg"
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
                if k in ["tcpport","cangrid_port","ap_channel","node_number","canid","button_pin","green_led_pin","yellow_led_pin","red_led_pin","start_event_id","orphan_timeout","shutdown_code"]:
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

id_apmode="apmode"
id_apmode_no_passwd="ap_no_password"
id_ssid="ap_ssid"
id_password="ap_password"
id_channel="ap_channel"
id_router_ssid="router_ssid"
id_router_password="router_password"
id_bonjour_name="service_name"
id_ed_tcpport="tcpport"
id_grid_enable="gridenable"
id_grid_port="cangrid_port"
id_logfile="logfile"
id_loglevel="loglevel"
id_logappend="logappend"
id_canid="canid"
id_turnout_file="turnout_file"
id_fns_momentary="fn_momentary"
id_create_logfile="create_log_file"
id_start_event_id="start_event_id"
id_btn_save="btnSave"
id_btn_apply="btnApply"
id_btn_restart="btnRestart"
id_btn_stop="btnStop"
id_btn_restart_all="btnRestartAll"
id_btn_update_file="btnUpdateFile"
id_btn_shutdown="btnShutdown"
id_orphan_timeout="orphan_timeout"
id_shutdown_code="shutdown_code"

desc_apmode="AP mode"
desc_apmode_no_passwd="No password in AP mode"
desc_ssid="SSID"
desc_password="Password"
desc_channel="Channel"
desc_router_ssid="Router SSID"
desc_router_password="Router Password"
desc_bonjour_name="Bonjour service name"
desc_ed_tcpport="ED Throttle Service port"
desc_grid_enable="Cangrid enable"
desc_grid_port="Grid connect Service port"
desc_logfile="Logfile"
desc_logappend="Append log"
desc_loglevel="Log Level"
desc_canid="Can ID"
desc_turnout_file="Turnouts"
desc_fns_momentary="Fns momentary"
desc_create_logfile="Creates log file"
desc_start_event="Start event id"
desc_btn_save="btnSave"
desc_btn_apply="btnApply"
desc_btn_restart="btnRestart"
desc_btn_stop="btnStop"
desc_btn_restart_all="btnRestartAll"
desc_btn_update_file="btnUpdateFile"
desc_btn_shutdown="btnShutdown"
desc_orphan_timeout="Orphan sessions timeout seconds"
desc_shutdown_code="Shutdown code"

#validators
ssid_length = form.Validator("SSID length should be between 1 and 8 characters", lambda p: p is None or len(p)>8)
passwd_length = form.Validator("Password length should be 8 characters", lambda p: len(p)!= 8)
router_ssid_length = form.Validator("Router SSID length should be between 1 and 12 characters", lambda p: p is None or len(p)>12)
router_passwd_length = form.Validator("Router password length should be less then 12 characters", lambda p: len(p)> 12)
turnout_length = form.Validator("Turnout name length should less than 11 characters", lambda p: p is None or len(p)>11)
service_name_length = form.Validator("Service name length should less than 8 characters", lambda p: p is None or len(p)>8)


def configurePi():
    #time.sleep(3)
    os.system("/etc/init.d/start_canpi.sh configure")

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

            cm.setValue("ap_mode", str(form[id_apmode].checked))
            cm.setValue("ap_no_password", str(form[id_apmode_no_passwd].checked))
            cm.setValue("ap_ssid", str(form[id_ssid].value))
            cm.setValue("ap_password", str(form[id_password].value))
            cm.setValue("ap_channel", str(form[id_channel].value))
            cm.setValue("router_ssid", str(form[id_router_ssid].value))
            cm.setValue("router_password", str(form[id_router_password].value))
            cm.setValue("can_grid", str(form[id_grid_enable].checked))
            cm.setValue("cangrid_port", str(form[id_grid_port].value))
            cm.setValue("service_name", str(form[id_bonjour_name].value))
            cm.setValue("tcpport", str(form[id_ed_tcpport].value))
            cm.setValue("loglevel", str(form[id_loglevel].value))
            cm.setValue(id_create_logfile, str(form[id_create_logfile].checked))
            cm.setValue(id_logappend, str(form[id_logappend].checked))
            cm.setValue("canid", str(form[id_canid].value))
            cm.setValue("fn_momentary", str(form[id_fns_momentary].value))
            cm.setValue("turnout_file", str(form[id_turnout_file].value))
            cm.setValue(id_start_event_id, str(form[id_start_event_id].value))
            cm.setValue(id_orphan_timeout, str(form[id_orphan_timeout].value))
            cm.setValue(id_shutdown_code, str(form[id_shutdown_code].value))
            cm.saveFile()
            writeMessage("Save successful")
            raise web.seeother('/')

        if id_btn_apply in userData:
            print("Apply button pressed")
            writeMessage("")
            cpid = os.fork()
            if cpid == 0:
                configurePi()
            return render.reboot("Applying changes and rebooting",myuri)

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
            if "/home/pi/canpi/canpi" in out:
                msg="Service restarted"
            else:
                msg="Restart failed"
            return render.index(form,"Canpi Configuration",msg)

        if id_btn_stop in userData:
            print("Stop button pressed")
            writeMessage("")
            os.system("/etc/init.d/start_canpi.sh stopcanpi")
            exitcode,out,err = self.get_exitcode_stdout_stderr("ps -ef")
            msg=""
            if "/home/pi/canpi/canpi" in out:
                msg="Stop failed"
            else:
                msg="Stop successfull"
            return render.index(form,"Canpi Configuration",msg)

        if id_btn_restart_all in userData:
            print("Restart all button pressed")
            writeMessage("")
            cpid = os.fork()
            if cpid == 0:
                restartAll()
            return render.reboot("Restarting the webpage and canpi",myuri)

        if id_btn_shutdown in userData:
            print("Shutdown button pressed")
            writeMessage("")
            cpid = os.fork()
            if cpid == 0:
                shutdownPi()
            return render.reboot("Shuting down the pi",myuri)

        return render.index(form,"Canpi Configuration","")

    def validate_form(self,form):
        global desc_ed_tcpport
        global desc_grid_port

        if form[id_apmode].checked == True :
            if len(form[id_ssid].value) > 8 or len(form[id_ssid].value) == 0 :
                return False, "SSID length should be between 1 and 8 characters"

            if len(form[id_password].value) != 8 and form[id_apmode_no_passwd].checked == False:
                return False, "Password length should be 8 characters"
        else:
            if len(form[id_router_ssid].value) > 12 or len(form[id_router_ssid].value) == 0 :
                return False, "Router SSID length should be between 1 and 12 characters"
            if len(form[id_router_password].value) == 0 or len(form[id_router_password].value) > 12 :
                return False, "Router password length should be less then 12 characters"

        if self.isint(form[id_canid].value):
            v = int(form[id_canid].value)
            if v <= 0 or v > 110:
                return False, "The CANID should be between 1 and 110"
        else:
            return False, "The CANID should be between 1 and 110"

        if self.isint(form[id_orphan_timeout].value):
            v = int(form[id_orphan_timeout].value)
            if v <= 0 or v > 3600:
                return False, "The orphan timeout should be between 1 and 3600 seconds"
        else:
            return False, "The orphan should be between 1 and 3600 seconds"

        if self.isint(form[id_ed_tcpport].value):
            v = int(form[id_ed_tcpport].value)
            if v <= 0 or v > portlimit or v in invalid_ports:
                return False, desc_ed_tcpport + "[" + str(v) + "] should be between 1 and " + str(portlimit) + " and not be " + str(invalid_ports)
        else:
            return False, desc_ed_tcpport + "[" + str(v) + "] should be between 1 and " + str(portlimit) + " and not be " + str(invalid_ports)

        if self.isint(form[id_grid_port].value):
            v1 = int(form[id_grid_port].value)
            if v1 <= 0 or v1 > portlimit or v1 in invalid_ports:
                return False, desc_grid_port + " should be between 1 and " + str(portlimit) + " and not be " + str(invalid_ports)
        else:
            return False, desc_grid_port + " should be between 1 and " + str(portlimit) + " and not be " + str(invalid_ports)

        if v == v1:
            return False, desc_ed_tcpport + " and " + desc_grid_port + " should be different"

        if self.isint(form[id_start_event_id].value):
            v = int(form[id_start_event_id].value)
            if v <= 0 or v > portlimit:
                print("Run validation\n")
                return False, desc_start_event + "[" + str(v) + "] should be between 1 and " + str(portlimit)
        else:
            return False, desc_start_event + "[" + str(v) + "] should be between 1 and " + str(portlimit)
  
        if self.isint(form[id_shutdown_code].value):
            v = int(form[id_shutdown_code].value)
            if v <= -1 or v > portlimit:
                print("Run validation\n")
                return False, desc_shutdown_code + "[" + str(v) + "] should be between -1 and " + str(portlimit)
            return False, desc_shutdown_code + "[" + form[id_shutdown_code].value + "] should be between -1 and " + str(portlimit)


        if self.isint(form[id_orphan_timeout].value):
            v = int(form[id_orphan_timeout].value)
            if v <= 0 or v > 120:
                print("Run validation\n")
                return False, desc_orphan_timeout + "[" + str(v) + "] should be between 1 and 120 seconds"
        else:
            return False, desc_orphan_timeout + "[" + str(v) + "] should be between 1 and and 120 seconds"

        if len(form[id_turnout_file].value) == 0 or len(form[id_turnout_file].value) > 11 :
            return False, "Turnout file name length should be between 1 and 11"

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

        if cm.getValueInsert("ap_mode", "True").lower() != "true":
            apmode = False
        if cm.getValueInsert("can_grid", "True").lower() != "true":
            gridenable = False
        if cm.getValueInsert(id_apmode_no_passwd, "True").lower() != "true":
            apmode_no_passwd = False
        if cm.getValueInsert(id_create_logfile, "True").lower() != "true":
            create_logfile = False
        if cm.getValueInsert(id_logappend, "True").lower() != "true":
            logappend = False

        myform = form.Form(
            form.Button(desc_btn_shutdown, id=id_btn_shutdown, value="shutdown", html="Shutdown"),
            form.Checkbox(id_apmode,description=desc_apmode,checked=apmode,value="apmode",id="tapmode"),
            form.Checkbox(id_apmode_no_passwd,description=desc_apmode_no_passwd,checked=apmode_no_passwd,value="apmode_no_passwd",id="tapmode_no_passwd"),
            form.Textbox(id_ssid,ssid_length,description=desc_ssid,value=cm.getValueInsert("ap_ssid","canpiwi"),id="apssid"),
            form.Textbox(id_password,passwd_length,description=desc_password, value=cm.getValueInsert("ap_password","12345678"),id="appasswd"),
            form.Dropdown(id_channel, ['1', '2', '3','4','5','6','7','8','9','10','11','12','13'],value=cm.getValueInsert("ap_channel",6)),
            form.Textbox(id_router_ssid,router_ssid_length,description=desc_router_ssid,value=cm.getValueInsert("router_ssid",""),id="routerssid"),
            form.Textbox(id_router_password,router_passwd_length, description=desc_router_password,value=cm.getValueInsert("router_password",""),id="routerpasswd"),
            form.Textbox(id_bonjour_name,service_name_length,description=desc_bonjour_name,value=cm.getValueInsert("service_name","MyLayout"),id="servicename"),
            form.Textbox(id_ed_tcpport,description=desc_ed_tcpport,value=cm.getValueInsert("tcpport",5555),id="tcpport"),
            form.Checkbox(id_grid_enable,description=desc_grid_enable,checked=gridenable,value="gridenable",id="cangrid"),
            form.Textbox(id_grid_port,description=desc_grid_port,value=cm.getValueInsert("cangrid_port",4444),id="cangripport"),
            form.Textbox(id_canid,description=desc_canid,value=cm.getValueInsert("canid",100)),
            form.Textbox(id_start_event_id,description=desc_start_event,value=cm.getValueInsert(id_start_event_id,1)),
            form.Textbox(id_orphan_timeout,description=desc_orphan_timeout,value=cm.getValueInsert(id_orphan_timeout,30)),
            form.Textbox(id_shutdown_code,description=desc_shutdown_code,value=cm.getValueInsert(id_shutdown_code,-1)),
            form.Textbox(id_fns_momentary,description=desc_fns_momentary,value=cm.getValueInsert("fn_momentary","2")),
            form.Textbox(id_turnout_file,turnout_length,description=desc_turnout_file,value=cm.getValueInsert("turnout_file","turnout.txt")),
            form.Dropdown(id_loglevel,  ['INFO', 'WARN', 'DEBUG'],value=cm.getValueInsert("loglevel","INFO")),
            form.Checkbox(id_create_logfile,description=desc_create_logfile,checked=create_logfile,value=id_create_logfile,id="tcreate_logfile"),
            form.Checkbox(id_logappend,description=desc_logappend,checked=logappend,value= id_logappend,id="tlogappend"),
            #form.Textbox(id_logfile,description=,value=cm.getValue("logfile"),id="logfile"),
            form.Button(desc_btn_save, id=id_btn_save, value="save", html="Save changes"),
            form.Button(desc_btn_apply, id=id_btn_apply, value="apply", html="Apply changes and reboot"),
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
