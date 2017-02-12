#!/bin/bash

ip=$1

scp *.cpp pi@"$ip":~/canpi/
scp *.h pi@"$ip":~/canpi/
scp webserver/canpiconfig.py pi@"$ip":~/canpi/webserver/
scp webserver/templates/index.html pi@"$ip":~/canpi/webserver/templates/
scp webserver/templates/reboot.html pi@"$ip":~/canpi/webserver/templates/

