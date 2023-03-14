#!/bin/bash

Green_font_prefix="\033[32m"
Yellow_font_prefix="\033[33m"
Red_font_prefix="\033[31m"
Font_color_suffix="\033[0m"
Info="[${Green_font_prefix}信息${Font_color_suffix}]"
Warring="[${Yellow_font_prefix}警告${Font_color_suffix}]"
Error="[${Red_font_prefix}错误${Font_color_suffix}]"

Mod="666"
Pwd="$(pwd)"
Module="snake"
Device="char_snake"
Link="snake_device"
DriverDir=${Pwd}/src/driver
WebDir=${Pwd}/src/web
Caddyfile="Caddyfile"

CheckRoot() {
    [[ ${EUID} != 0 ]] && echo -e "${Error} 当前账号非ROOT账号(或无ROOT权限)" && exit 1
}

Build() {
    cd ${DriverDir}
    make $* && echo -e "${Info} 完成"
}

Install() {
    /sbin/insmod ${Pwd}/mod/${Module}.ko $* || exit 1

    rm -f /dev/${Device}

    major=$(awk "\$2==\"${Device}\" {print \$1}" /proc/devices)

    mknod /dev/${Device} c ${major} 0

    group="staff"
    grep -q '^staff:' /etc/group || group="wheel"

    chgrp ${group} /dev/${Device}
    chmod ${Mod} /dev/${Device}

    if [ ! -f "$WebDir/$Link" ]; then
        rm ${WebDir}/${Link}
    fi
    ln -s /dev/${Device} ${WebDir}/${Link} && echo -e "${Info} 安装成功"
}

Uninstall() {
    /sbin/rmmod ${Module} $* || exit 1

    rm -f src/web/${Link}
    rm -f /dev/${Device} && echo -e "${Info} 卸载成功"
}

Run() {
    major=$(awk "\$2==\"${Device}\" {print \$1}" /proc/devices)
    if [ ! "$major" = "" ]; then
        if [ ! "$*" = "" ]; then
            caddy run $*
        else
            caddy run --config ${WebDir}/${Caddyfile}
        fi
    else
        echo -e "${Error} 设备未安装"
    fi
}

Move() {
    major=$(awk "\$2==\"${Device}\" {print \$1}" /proc/devices)
    if [ ! "$major" = "" ]; then
        case $* in
            "UP")
                echo "U" > ${WebDir}/${Link}
                ;;
            "DOWN")
                echo "D" > ${WebDir}/${Link}
                ;;
            "LEFT")
                echo "L" > ${WebDir}/${Link}
                ;;
            "RIGHT")
                echo "R" > ${WebDir}/${Link}
                ;;
            "PAUSE")
                echo "P" > ${WebDir}/${Link}
                ;;
            *)
                echo -e "${Error} 方向错误，请使用UP、DOWN、LEFT、RIGHT控制移动，使用PAUSE暂停"
                ;;
        esac
    else
        echo -e "${Error} 设备未安装"
    fi
}

case "$1" in
    "build")
        Build ${@:2}
        ;;
    "install")
        Install ${@:2}
        ;;
    "uninstall")
        Uninstall ${@:2}
        ;;
    "run")
        Run ${@:2}
        ;;
    "move")
        Move ${@:2}
        ;;
    *)
        echo -e "${Error} 命令错误"
        ;;
esac
