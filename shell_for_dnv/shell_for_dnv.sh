#! /bin/bash

function  menu_display( ){
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

function get_system_cpu_type( ){
    echo -n "======================CPU_TYPE_CHECK======================"
    type=`uname -a | awk {'printf($15)'}`
    echo $type
}

function input_num( ){
    echo hello you!
}

input_num
get_system_cpu_type
menu_display


