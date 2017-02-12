#!/bin/bash
### BEGIN INIT INFO
# Provides: canpi
# Required-Start:    $remote_fs $syslog $network
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start Merg canpi
# Description:       Enable service provided by Merg canpi daemon.
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
. /lib/lsb/init-functions

dir="/home/pi/canpi"
cmd="/home/pi/canpi/canpi"
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

#Bonjour files
bonjour_file="/etc/avahi/services/multiple.service"
bonjour_template="$dir/multiple.service"

#wpa supplicant files
wpa_file="/etc/wpa_supplicant/wpa_supplicant.conf"
wpa_template="$dir/wpa_supplicant.conf"

#hostapd files
hostapd_file="/etc/hostapd/hostapd.conf"
hostapd_template="$dir/hostapd.conf"
default_hostapd_file="/etc/default/hostapd"
safe_hostapd_file="$dir/hostapd"

#dhcp files
default_dhcp_file="/etc/default/isc-dhcp-server"
safe_dhcp_file="$dir/isc-dhcp-server"

#interface files
iface_file="/etc/network/interfaces"
iface_file_wifi="$dir/interfaces.wifi"
iface_file_ap="$dir/interfaces.ap"

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

reconfigure_if_pb_pressed(){
   echo "Checking if the push button is pressed"
   p=$(is_pb_pressed)
   if [[ $p -eq "0" ]];then
      #we reconfigure
      setup_red_led
      blink_red_led
      unset_red_led
      echo "Push button pressed. Reconfiguring"
      ap_no_password="true"
      sed -i 's/ap_mode="false"/ap_mode="true"/Ig' $config
      sed -i 's/ap_no_password=false/ap_no_password="true"/Ig' $config
      setup_ap_mode
      setup_bonjour
      sleep 1
      echo "Rebooting"
      sudo reboot
      exit 0
   fi
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

is_wifi_running(){
   #the wifi is running if we have and ip address
   ipaddr=`ip addr | grep -A2 'wlan0:' |grep -A2 'state UP' -A2 | tail -n1 | awk '{print $2}'`
   [[ $ipaddr =~ [0-9]+\.[0-9]+\.[0-9]+\.[0-9]+\/[0-9]+ ]]
   echo $?
}

is_web_running() {
    [ -f "$web_pid_file" ] && ps `get_web_pid` > /dev/null 2>&1
}

is_avahi_running(){
    r=`/etc/init.d/avahi-daemon status`
    echo $r | grep "active (running)"
}

is_dhcp_running(){
    r=`/etc/init.d/isc_dhcp-server status`
    echo $r | grep "active (running)"
}

is_hostapd_running(){
    r=`/etc/init.d/hostapd status`
    echo $r | grep "active (running)"
}

kill_all_processes(){
    p="$1"
    list=`ps -A -o pid,cmd |grep "$p" | grep -v color | grep -v start_canpi | cut -d " " -f 2`
    for pid in $list;
    do
        echo "Killing process $pid"
        kill -9 $pid
    done
}

setup_bonjour() {
    echo "Configuring the bonjour service"
    #back the old file
    #mv $bonjour_file "${bonjour_file}.bak"
    cp -f $bonjour_template "${bonjour_template}.tmp"

    #change the service name
    sed -i 's/SERVICENAME/'"$service_name"'/' "${bonjour_template}.tmp"

    #change the port
    sed -i 's/PORT/'"$tcpport"'/' "${bonjour_template}.tmp"
    mv -f "${bonjour_template}.tmp" $bonjour_file

    #restart the service
    #echo "Restarting the bonjour service"
    #/etc/init.d/avahi-daemon restart
    #sleep 1
    #r=`/etc/init.d/avahi-daemon status`
    #echo $r | grep "active (running)"
    if is_avahi_running;
    then
        echo "Bonjour restarted"
    else
        echo "Failed to restart Bonjour"
    fi
}

setup_wpa_supplicant(){
    echo "Configuring the WPA supplicant"
    #back the old file
    sudo mv -f $wpa_file "${wpa_file}.bak"
    sudo cp -f $wpa_template $wpa_file

    #change the ssid
    sudo sed -i 's/SSID/'"$router_ssid"'/' $wpa_file

    #change the password
    sudo sed -i 's/PASSWORD/'"$router_password"'/' $wpa_file

}

setup_hostapd(){
    echo "Configuring the hostapd"
    #back the old file
    sudo mv -f $hostapd_file "${hostapd_file}.bak"
    sudo cp -f $hostapd_template $hostapd_file

    #change the ssid
    sudo sed -i 's/SSID/'"$ap_ssid"'/' $hostapd_file

    #change the password
    sudo sed -i 's/PASSWORD/'"$ap_password"'/' $hostapd_file

    #change the channel
    sudo sed -i 's/CHANNEL/'"$ap_channel"'/' $hostapd_file
}

setup_hostapd_no_password(){
    echo "Configuring the hostapd with no password"
    #back the old file
    sudo mv -f $hostapd_file "${hostapd_file}.bak"
    sudo cp -f "${hostapd_template}.nopassword" $hostapd_file

    #change the ssid
    sudo sed -i 's/SSID/'"$ap_ssid"'/' $hostapd_file

    #change the channel
    sudo sed -i 's/CHANNEL/'"$ap_channel"'/' $hostapd_file
}



setup_iface_wifi(){
    #back the old file
    sudo mv -f $iface_file "${iface_file}.bak"
    sudo cp -f $iface_file_wifi $iface_file
}

setup_iface_ap(){
    #back the old file
    sudo mv -f $iface_file "${iface_file}.bak"
    sudo cp -f $iface_file_ap $iface_file
}

remove_hostapd_default(){
    #back the old file
    sudo mv -f $default_hostapd_file "${default_hostapd_file}.bak"
}

remove_dhcp_default(){
    #back the old file
    sudo mv -f $default_dhcp_file "${default_dhcp_file}.bak"
}

restore_hostapd_default(){
    #back the old file
    sudo cp -f $safe_hostapd_file $default_hostapd_file
}

restore_dhcp_default(){
    #back the old file
    sudo cp -f $safe_dhcp_file $default_dhcp_file
}

setup_ap_mode()
{
    echo "Configuring to AP mode"

    sleep 1
    sudo /etc/init.d/hostapd stop
    sleep 1
    sudo /etc/init.d/isc-dhcp-server stop


    echo "Restarting wlan0"
    sudo ip link set wlan0 down
    setup_iface_ap
    if [[ $ap_no_password == "true" || $ap_no_password == "True" || $ap_no_password == "TRUE" ]];then
        setup_hostapd_no_password
    else
        setup_hostapd
    fi

    sleep 1
    sudo ip link set wlan0 up

    sleep 2
    sudo /etc/init.d/hostapd start
    sleep 1
    sudo /etc/init.d/isc-dhcp-server start

    echo "Configuring the hostapd daemon"
    sudo /usr/sbin/update-rc.d hostapd defaults
    echo "Configuring DHCP server"
    sudo /usr/sbin/update-rc.d isc-dhcp-server defaults
}

setup_wifi_mode(){
    echo "Configuring to wifi mode"
    setup_iface_wifi
    setup_wpa_supplicant

    echo "Stoping wlan0"
    sudo ip link set wlan0 down

    echo "Stoping the hostapd service"
    sudo /etc/init.d/hostapd stop
    sleep 1

    echo "Stoping the DHCP service"
    sudo /etc/init.d/isc-dhcp-server stop
    sleep 1

    #echo "Starting wlan0"
    #sudo ip link set wlan0 up
    #sudo /etc/init.d/networking rstart
    #sleep 1
    #remove_hostapd_default
    #remove_dhcp_default
    sudo /usr/sbin/update-rc.d hostapd remove
    sleep 1
    sudo /usr/sbin/update-rc.d isc-dhcp-server remove
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
    echo "Checking for file 'canpi'"
    if [[ -f canpi ]];then

        echo "'canpi' file present. Stopping the service"

        stop_canpi
        if [[ $? -eq 1 ]]; then
            exit 1
        fi

        echo "Service stopped. Backing up the file"
        cd "${upgradedir}"
        mv -f ../canpi ../canpi.bkp
        cp -f canpi ../
        chmod +x ../canpi

        echo "Starting the service after upgrade"
        start_canpi

        if [[ $? -eq 1 ]]; then
            echo "Failed to restart the service. Reversing the upgrade"
            cd "${upgradedir}"
            mv -f ../canpi.bkp ../canpi
            chmod +x ../canpi
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
        echo "No 'canpi' file on upgrade files"
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
    copy_config_file dhcpd.conf
    copy_config_file hostapd
    copy_config_file hostapd.conf
    copy_config_file hostapd.conf.nopassword
    copy_config_file interfaces.ap
    copy_config_file interfaces.wifi
    copy_config_file isc-dhcp-server
    copy_config_file multiple.service
    copy_config_file wpa_supplicant.conf
    copy_start_script_file start_canpi.sh
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
        listfiles=`ls -t | grep ".zip" | grep canpiwi-upgrade`
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
        #reconfigure if the push button is pressed
        reconfigure_if_pb_pressed
        setup_red_led
        unset_red_led
        if [[ is_wifi_running -eq 0 ]] ; then
           echo "We found a valid ip. Wifi is running"
           set_red_led
        fi
        setup_bonjour
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
        kill_all_processes "canpi"
    ;;
    startcanpi)
        setup_red_led
        setup_bonjour
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
        #$0 stopcanpi
        stop_canpi
        if [[ $? -eq 1 ]]; then
            exit 1
        fi
        if is_running; then
            echo "Unable to stop, will not attempt to start"
            exit 1
        fi
        #$0 startcanpi
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
    configure)
        echo "Configuring the services"
        blink_red_led
        if [[ $ap_mode == "true" || $ap_mode == "True" || $ap_mode == "TRUE" ]];then
           echo "Starting configuration for AP Mode"
           setup_ap_mode
        else
           echo "Starting configuration for Wifi Mode"
           setup_wifi_mode
        fi
        setup_bonjour
        sleep 1
        echo "Rebooting"
        sudo reboot
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
            echo "Canpi is Running"
        else
            echo "Canpi Stopped"
            exit 1
        fi
    ;;
    shutdown)
        echo "Shutting down"
        sudo halt 
    ;;
    *)
    echo "Usage: $0 {start|stop|restart|status|configure|startcanpi|stopcanpi|restartcanpi|shutdown|upgrade}"
    exit 1
    ;;
esac

exit 0
