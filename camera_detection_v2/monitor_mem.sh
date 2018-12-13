#!/bin/bash

dir=/home/pdio/pc_vas
script=monitor.sh
author="Wei Fei"
echo "Enter:"$dir
echo "Script":$script
echo "Author:"$author


PROCESS=camera_detection
LOG=./bin/memlog.txt


#删除上次的监控文件
if [ -f "$LOG" ]
then 
    rm -rf $LOG
else
    touch $LOG
fi

#过滤出需要的进程ID
#PID=`ps -ef | grep "$PROCESS" | grep -v "grep" |  awk '{print $2}'`
PID=16605
echo $PID

while [ "$PID" != "" ]    
do
    str=`cat /proc/$PID/status | grep RSS`
    dateTime=`date +'%Y%m%d %H:%M:%S'`
    tmp=${dateTime}"----"
    data=${tmp}${str}
    echo $data >> $LOG
    sleep 5
    #PID=`ps -ef | grep "$PROCESS" | grep -v "grep" |  awk '{print $2}'`
done

#结束
echo "Script ending........."
