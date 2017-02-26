#/bin/bash

PATH=/sbin:/bin:/usr/sbin:/usr/bin
. /lib/lsb/init-functions

dir="/home/pi"
canpidir="/home/pi/canpiserver"

append_to_file(){
   val=$1
   file=$2
   grep -q "$val" "$file" || echo "$val" >> $file
}

config_boot_config(){
   bootconf="/boot/config.txt"
   cp $bootconf "/boot/config.txt.$RANDOM"
   sed -i "s/dtparam=spi=off/dtparam=spi=on/" $bootconf
   #in case the entry is not there
   append_to_file "dtparam=spi=on" $bootconf
   ls /boot/overlays/mcp2515-can0-overlay*
   if [ $? == 0 ];then
       append_to_file "dtoverlay=mcp2515-can0-overlay,oscillator=16000000,interrupt=25" $bootconf
   else
       append_to_file "dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=25" $bootconf
   fi
}

add_can_interface(){
  f="/etc/network/interfaces"
  append_to_file "auto can0" $f
  append_to_file "iface can0 can static" $f
  append_to_file "bitrate 125000 restart-ms 1000" $f
}

echo "########### APT UPDATE ###############"
apt-get -y update
echo "########### GIT ###############"
apt-get -y install git
echo "########### CAN UTILS ###############"
apt-get -y install can-utils

echo "########### BOOT CONFIG ###############"
config_boot_config
echo "########### CAN INTERFACE ###############"
add_can_interface

echo "########### GET THE CANPI CODE ###############"
#get the code
cd $dir
git clone https://github.com/amaurial/canpiserver.git

#echo "########### COMPILE CANPI ###############"
#compile the code
cd canpiserver
make clean
make all

echo "########### WEBSERVER ###############"
#install the webpy
tar xvf webpy.tar.gz
mv webpy-webpy-770baf8 webpy
cd webpy
python setup.py install

echo "########### CHANGE DIR OWNER ###############"
cd $dir
chown -R pi.pi canpiserver

echo "########### MOVE CONFIG FILES ###############"
#backup and move some basic files

echo "########### CONFIG SCRIPT FILES ###############"
#copy the configure script
cp "$canpidir/canpiserver.sh" /etc/init.d/
chmod +x /etc/init.d/canpiserver.sh
update-rc.d canpiserver.sh defaults

echo "########### START THE SERVICE ###############"
#run configure
/etc/init.d/canpiserver.sh start 

