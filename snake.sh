#!/bin/bash

Green_font_prefix="\033[32m"
Yellow_font_prefix="\033[33m"
Red_font_prefix="\033[31m"
Font_color_suffix="\033[0m"
Info="[${Green_font_prefix}信息${Font_color_suffix}]"
Warning="[${Yellow_font_prefix}警告${Font_color_suffix}]"
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

IsDriverLoaded() {
    local ret=0
    major=$(awk "\$2==\"${Device}\" {print \$1}" /proc/devices)
    if [ "$major" = "" ]; then
        if [ "$*" != "" ]; then
            echo -e "${Error} $*"
        fi
        ret=1
    fi
    return $ret
}

Build() {
    cd ${DriverDir}
    make $* && echo -e "${Info} 完成"
}

Install() {
    CheckRoot && exit 1

    /sbin/insmod ${Pwd}/mod/${Module}.ko $* || exit 1

    rm -f /dev/${Device}

    major=$(awk "\$2==\"${Device}\" {print \$1}" /proc/devices)

    mknod /dev/${Device} c ${major} 0

    group="staff"
    grep -q '^staff:' /etc/group || group="wheel"

    chgrp ${group} /dev/${Device}
    chmod ${Mod} /dev/${Device}

    if [ -f "$WebDir/$Link" ]; then
        rm ${WebDir}/${Link}
    fi
    ln -s /dev/${Device} ${WebDir}/${Link} && echo -e "${Info} 安装成功"
}

Uninstall() {
    CheckRoot && exit 1

    /sbin/rmmod ${Module} $* || exit 1

    rm -f src/web/${Link}
    rm -f /dev/${Device} && echo -e "${Info} 卸载成功"
}

Reinstall() {
    if IsDriverLoaded; then
        Uninstall $*
    else
        echo -e "${Warning} 设备未安装"
    fi
    Install $*
}

Run() {
    IsDriverLoaded "设备未安装" || exit 1
    if [ ! "$*" = "" ]; then
        caddy run $*
    else
        caddy run --config ${WebDir}/${Caddyfile}
    fi
}

Move() {
    IsDriverLoaded "设备未安装" || exit 1
    case $* in
        "UP")
            echo "W" > ${WebDir}/${Link}
            ;;
        "DOWN")
            echo "S" > ${WebDir}/${Link}
            ;;
        "LEFT")
            echo "A" > ${WebDir}/${Link}
            ;;
        "RIGHT")
            echo "D" > ${WebDir}/${Link}
            ;;
        "PAUSE")
            echo "P" > ${WebDir}/${Link}
            ;;
        "RESTART")
            echo "R" > ${WebDir}/${Link}
            ;;
        *)
            echo -e "${Error} 请使用UP、DOWN、LEFT、RIGHT控制移动，使用PAUSE暂停，RESTART重新开始"
            ;;
    esac
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
    "reinstall")
        Reinstall ${@:2}
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
