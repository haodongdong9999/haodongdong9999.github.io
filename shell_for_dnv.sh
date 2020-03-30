#! /bin/bash




###############################################################################################
#
#    Version ：1.0
#
#     AUTHOR : David Hao
#     CREATE DATE : 2020-01-20
#     PLATFORM : Linux/AIX/HP-UX
#     USAGE : sh
#     FUNCTION : checkLink
#
export LC_ALL=zh_CN         #Set the LANG and all environment variable beginning with LC_ to C
export LANG=zh_C
###############################################################################################






filetime=`date +'%Y%m%d%H%M%S'`

NET_PORT=vx0

menu_display () {
    echo "------------------------------"
    echo "    Test Option List          "
    echo " 1: Get the cpu load          "
    echo " 2: Get the memrory           "
    echo " 3: Test the Ip addr          "
    echo " 4: Get the SW version        "
    echo "------------------------------"

    read -p "Please give me a option: " option
    echo "Your input the option is $option"
}

get_system_cpu_type () {
    echo -e "======================DNV SYSTEM KERNEL TYPE_CHECK======================"
    type=`uname -a`
    echo "The system : $type"
}

check_dnv_network_config () {

    echo "===========================DNV NETWORK INFORMATION============================"
    mac_addr=`ifconfig $NET_PORT | grep address | cut -c 10-`
    ip_addr=`ifconfig $NET_PORT | grep inet | cut -c 6-| cut -c -16`
    ip_addr=`ifconfig $NET_PORT | grep netmask | /data/cut -c 6- | /data/cut -c -12`
    #echo "MAC: ${mac_addr##* } "
    #echo "ip: ${ip_addr%%netmask*}"
    echo "MAC: $mac_addr"
    echo "IP:  $ip_addr"
}

get_sw_version () {

    echo "=============================Get SW version================================="
    SW_VER=`cat /app/etc/buildinfo`
    echo "SW_VER= $SW_VER"
}

get_flash_information () {

    echo "================================get the flash information======================"
    Flash = `ls /dev|grep fs`
    echo "Flash : $Flash"
}

get_process_information () {

    echo "===================================Get Process information============================="
    Process=`pidin a`
    echo " $Process"
}

ping_test () {

    ecdho "===================================ping test======================================"
    local ip=198.18.38.11
    if [ ! -z $ip ];then
        local rate=`ping -c 1 -w 3 $ip|grep 'packet loss'|grep -v grep|awk -F',' '{print $3}'|awk -F'%' '{print $1}'|awk '{print $NF}'`
        if [ "${rate}" = "errors"  ]; then
            rate=`ping -c 1 -w 3 $ip|grep 'packet loss'|grep -v grep|awk -F',' '{print $4}'|awk -F'%' '{print $1}'|awk '{print $NF}'`
        else if [ "${rate}" = "100" ];then
            echo "Network to Aurix is Not Ok"
        else
            echo "Ethernet is OK"
        fi
    fi
 }

input_num () {
        echo hello you!
    }


get_system_cpu_type
check_dnv_network_config
#menu_display
get_sw_version
get_flash_information
get_process_information
ping_test



