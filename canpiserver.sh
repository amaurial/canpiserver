#!/bin/bash
### BEGIN INIT INFO
# Provides: canpiserver
# Required-Start:    $remote_fs $syslog $network
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start Merg canpiserver
# Description:       Enable service provided by Merg canpiserver daemon.
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
. /lib/lsb/init-functions

dir="/home/pi/canpiserver"
cmd="/home/pi/canpiserver/canpiserver"
webdir="$dir/webserver"
upgradedir="$dir/upgrade"
webcmd="/usr/bin/python $webdir/canpiconfig.py 80"
config="${dir}/canpi.cfg"
user=""

name=`basename $0`
pid_file="/var/run/$name.pid"
stdout_log="/var/log/$name.log"
stderr_log="/var/log/$name.err"

webname="canpiconfig"
web_pid_file="/var/run/$webname.pid"
web_stdout_log="/var/log/$webname.log"
web_stderr_log="/var/log/$webname.err"

#clean spaces and ';' from config file
sed -i 's/ = /=/g' $config
sed -i 's/;//g' $config
source $config

#set the permission for pi user
chown -R pi.pi $dir
chmod +x $cmd

if [[ -d "${upgradedir}" && ! -L "${upgradedir}" ]] ; then
    echo "'upgrade' directory exists"
else
    echo "'upgrade' directory does not exists. Creating"
    mkdir $upgradedir
    if [[ -d "${upgradedir}" && ! -L "${upgradedir}" ]] ; then
        echo "'upgrade' directory created successfully"
    else
        echo "Failed to create 'upgrade' directory"
    fi
fi

get_push_button(){
   pb=`grep button_pin ${config} |cut -d "=" -f 2`
   if [[ ${pb} > 1 && ${pb} < 30 ]];then
      echo $pb
   else
      pb=17
      echo $pb
   fi
}

set_push_button(){
   p=$(get_push_button)
   echo "pb pin is ${p}"
   echo ${p} > /sys/class/gpio/export
   echo "in" > /sys/class/gpio/gpio${p}/direction
}

is_pb_pressed(){
   p=$(get_push_button)
   v=`cat /sys/class/gpio/gpio${p}/value`
   #the pressed value is 0
   if [[ $v -eq "0" ]]; then
      sleep 1
      v=`cat /sys/class/gpio/gpio${p}/value`
   fi
   echo $v
}

blink_red_led(){
   set_red_led
   sleep 1
   unset_red_led
   sleep 1
   set_red_led
   sleep 1
   unset_red_led
   sleep 1
   set_red_led
}

get_red_led_pin(){
   grep red_led_pin ${config}
   if [[ $? -eq 0 ]];then
      ledpin=`grep red_led_pin ${config} |cut -d "=" -f 2`
   else
      ledpin=22
      echo "red_led_pin=${ledpin}" >> ${config}
   fi

   if [[ $ledpin > 1 && $ledpin < 30 ]];then
      echo ${ledpin}
   else
      ledpin=22
      #echo "red_led_pin=${ledpin}" >> ${config}
      echo ${ledpin}
   fi
}

setup_red_led(){
   redled=$(get_red_led_pin)
   echo "${redled}" > /sys/class/gpio/export
   echo "out" > /sys/class/gpio/gpio${redled}/direction
}

set_red_led(){
   redled=$(get_red_led_pin)
   echo "1" > /sys/class/gpio/gpio${redled}/value
}

unset_red_led(){
   redled=$(get_red_led_pin)
   echo "0" > /sys/class/gpio/gpio${redled}/value
}

get_pid() {
    cat "$pid_file"
}

is_running() {
    [ -f "$pid_file" ] && ps `get_pid` > /dev/null 2>&1
}

get_web_pid() {
    cat "$web_pid_file"
}

is_web_running() {
    [ -f "$web_pid_file" ] && ps `get_web_pid` > /dev/null 2>&1
}


kill_all_processes(){
    p="$1"
    list=`ps -A -o pid,cmd |grep "$p" | grep -v color | grep -v canpiserver | cut -d " " -f 2`
    for pid in $list;
    do
        echo "Killing process $pid"
        kill -9 $pid
    done
}



start_led_watchdog(){
   echo "watchdog"
}

stop_led_watchdog(){
   echo "watchdog"
}

bootstrap(){
   echo "bootstrap"
}

stop_canpi(){
    if is_running; then
        echo -n "Stopping $name.."
        kill `get_pid`
        for i in {1..10}
        do
            if ! is_running; then
                break
            fi

            echo -n "."
            sleep 1
        done
        echo

        if is_running; then
            echo "Not stopped; may still be shutting down or shutdown may have failed"
            return 1
        else
            echo "Stopped"
            if [ -f "$pid_file" ]; then
                rm "$pid_file"
            fi
        fi
    else
        echo "Not running"
    fi
    return 0
}

start_canpi(){

    if is_running; then
        echo "Already started"
    else
        echo "Starting $name"
        cd "$dir"

        #restart can interface
        #sudo /sbin/ip link set can0 down
        #sudo /sbin/ip link set can0 up type can bitrate 125000 restart-ms 1000

        if [ -z "$user" ]; then
            sudo $cmd >> "$stdout_log" 2>> "$stderr_log" &
        else
            sudo -u "$user" $cmd >> "$stdout_log" 2>> "$stderr_log" &
        fi
        echo $! > "$pid_file"
        if ! is_running; then
            echo "Unable to start, see $stdout_log and $stderr_log"
            return 1
        fi
    fi
    return 0
}

upgrade_canpi(){
    echo "Checking for file 'canpiserver'"
    if [[ -f canpiserver ]];then

        echo "'canpiserver' file present. Stopping the service"

        stop_canpi
        if [[ $? -eq 1 ]]; then
            exit 1
        fi

        echo "Service stopped. Backing up the file"
        cd "${upgradedir}"
        mv -f ../canpiserver ../canpiserver.bkp
        cp -f canpiserver ../
        chmod +x ../canpiserver

        echo "Starting the service after upgrade"
        start_canpi

        if [[ $? -eq 1 ]]; then
            echo "Failed to restart the service. Reversing the upgrade"
            cd "${upgradedir}"
            mv -f ../canpiserver.bkp ../canpiserver
            chmod +x ../canpiserver
            start_canpi
            if [[ $? -eq 1 ]]; then
                echo "Failed to start the service after recover"
                return 1
            fi
            echo "Service restarted after unsuccessesful upgrade"
            return 1
        fi
        echo "Service restarted after upgrade."
    else
        echo "No 'canpiserver' file on upgrade files"
    fi
    return 0
}

copy_webserver_file(){
    wpath=$1
    wfile=$2
    echo "checking '${wpath}/${wfile}'"
    if [[ -f "${wpath}/${wfile}" ]]; then
        echo "Backing up ${wfile}"
        cp -f ../$wpath/$wfile ../$wpath/${wfile}.bak
        echo "Applying changes to ${wfile}"
        cp -f $wpath/$wfile ../$wpath/
        echo "Changes to ${wfile} applied"
    fi
}

upgrade_webserver(){
    #check the directory
    oldpath=`pwd`
    cd "${upgradedir}"
    p=`pwd`
    echo "Actual path ${p}"
    if [[ ! -d webserver && -L webserver ]] ; then
        echo "No webserver path. Skipping"
        cd $oldpath
        return 0
    fi

    copy_webserver_file "webserver" "canpiconfig.py"
    #webserver/templates path
    if [[ -d webserver/templates && ! -L webserver/templates ]] ; then
        copy_webserver_file "webserver/templates" "index.html"
        copy_webserver_file "webserver/templates" "reboot.html"
    else
        echo "No webserver/templates path. Skipping"
    fi

    #webserver/static path
    if [[ -d webserver/static && ! -L webserver/static ]] ; then
        copy_webserver_file "webserver/static" "bootstrap.css"
        copy_webserver_file "webserver/static" "main.css"
        copy_webserver_file "webserver/static" "merg_logo.png"
        copy_webserver_file "webserver/static" "rpi.jpg"
    else
        echo "No webserver/static path. Skipping"
    fi

    cd $oldpath
    return 0

}

copy_config_file(){
    wfile=$1
    echo "checking '${wfile}'"
    if [[ -f $wfile ]]; then
        echo "Backing up ${wfile}"
        cp -f ../$wfile ../$wfile.bak
        echo "Applying changes to ${wfile}"
        cp -f $wfile ../
        echo "Changes to ${wfile} applied"
    fi
}

copy_start_script_file(){
    wfile=$1
    echo "checking '${wfile}'"
    if [[ -f $wfile ]]; then
        echo "Backing up ${wfile}"
        cp -f ../$wfile ../$wfile.bak
        echo "Applying changes to ${wfile}"
        cp -f $wfile /etc/init.d/
        echo "Changes to ${wfile} applied"
    fi
}

upgrade_config_files(){
    cd "${upgradedir}"
    copy_start_script_file canpiserver.sh
}

clean_upgrade_files()
{
    #do some backup
    cd $dir
    echo "Cleaning"
    if [[ -d "${upgradedir}" && ! -L "${upgradedir}" ]] ; then
        echo "Deleting ${upgradedir}/*"
        rm -rf ${upgradedir}/*
    fi
}

apply_upgrade(){
    #check if the dir exists
    if [[ -d "${upgradedir}" && ! -L "${upgradedir}" ]] ; then
        echo "'upgrade' directory exists. Checking for upgrade files."
        cd "${upgradedir}"
        listfiles=`ls -t | grep ".zip" | grep canpiserver-upgrade`
        upfile=(${listfiles[@]})
        echo "Upgrade zip files: ${upfile}"
        if [[ -f "${upfile}" ]] ; then
            echo "Unzip the file"
            unzip "${upfile}"
            upgrade_canpi
            upgrade_webserver
            upgrade_config_files
            clean_upgrade_files
        else
            echo "No upgrade file. Leaving."
        fi

    else
        echo "'upgrade' directory does not exist. Creating it."
        mkdir "${upgradedir}"
    fi
}

start_webserver(){
    if is_web_running; then
        echo "Web service already started"
    else
        echo "Starting $webname"
        cd "$webdir"

        if [ -z "$user" ]; then
            sudo $webcmd >> "$web_stdout_log" 2>> "$web_stderr_log" &
        else
            sudo -u "$user" $webcmd >> "$web_stdout_log" 2>> "$web_stderr_log" &
        fi
        echo $! > "$web_pid_file"
        if ! is_web_running; then
            echo "Unable to start, see $web_stdout_log and $web_stderr_log"
            return 1
        fi
    fi
    return 0
}

stop_webserver(){
    if is_web_running; then
        echo -n "Stopping $webname.."
        kill `get_web_pid`
        for i in {1..10}
        do
            if ! is_web_running; then
                break
            fi

            echo -n "."
            sleep 1
        done
        echo

        if is_web_running; then
            echo "$webname not stopped; may still be shutting down or shutdown may have failed"
            return 1
        else
            echo "Stopped"
            if [ -f "$web_pid_file" ]; then
                rm "$web_pid_file"
            fi
        fi
    else
        echo "$webname not running"
    fi
    return 0
}

#setup the push button
echo "Setting push button"
set_push_button

case "$1" in
    start)
        setup_red_led
        set_red_led
        start_webserver
        start_canpi
        if [[ $? -eq 1 ]]; then
            exit 1
        fi
    ;;
    stop)
        stop_webserver
        stop_canpi
        #kill the rest
        kill_all_processes "canpiserver"
    ;;
    startcanpi)
        setup_red_led
        start_canpi
        if [[ $? -eq 1 ]]; then
            exit 1
        fi
    ;;
    stopcanpi)
        stop_canpi
        if [[ $? -eq 1 ]]; then
            exit 1
        fi
    ;;
    restartcanpi)
        stop_canpi
        if [[ $? -eq 1 ]]; then
            exit 1
        fi
        if is_running; then
            echo "Unable to stop, will not attempt to start"
            exit 1
        fi
        start_canpi
        if [[ $? -eq 1 ]]; then
            exit 1
        fi
    ;;
    restart)
        $0 stop
        if is_running; then
            echo "Unable to stop, will not attempt to start"
            exit 1
        fi
        $0 start
    ;;
    upgrade)
        echo "Check for upgrade"
        apply_upgrade
    ;;
    status)
        if is_web_running; then
            echo "Config is Running"
        else
            echo "Config Stopped"
        fi

        if is_running; then
            echo "Canpiserver is Running"
        else
            echo "Canpiserver Stopped"
            exit 1
        fi
    ;;
    *)
    echo "Usage: $0 {start|stop|restart|status|startcanpi|stopcanpi|restartcanpi|upgrade}"
    exit 1
    ;;
esac

exit 0
