#!/bin/sh

Green_font_prefix="\033[32m"
Yellow_font_prefix="\033[33m"
Red_font_prefix="\033[31m"
Font_color_suffix="\033[0m"
Info="[${Green_font_prefix}信息${Font_color_suffix}]"
Warring="[${Yellow_font_prefix}警告${Font_color_suffix}]"
Error="[${Red_font_prefix}错误${Font_color_suffix}]"

Pwd="$(pwd)"
Module="snake"
Device="char_snake"
Mod="666"

CheckRoot() {
    [[ ${EUID} != 0 ]] && echo -e "${Error} 当前账号非ROOT账号(或无ROOT权限)" && exit 1
}

Install() {
    /sbin/insmod ${Pwd}/mod/${Module}.ko $* || exit 1

    rm -f /dev/${Device}

    major=$(awk "\$2==\"${Device}\" {print \$1}" /proc/devices)

    mknod /dev/${Device} c ${major} 0

    group="staff"
    grep -q '^staff:' /etc/group || group="wheel"

    chgrp ${group} /dev/${Device}
    chmod ${Mod} /dev/${Device} && echo -e "${Info} 安装成功"
}

Uninstall() {
    /sbin/rmmod ${Module} $* || exit 1

    rm -f /dev/${Device} && echo -e "${Info} 卸载成功"
}

case "$1" in
    "install")
        Install ${@:2}
        ;;
    "uninstall")
        Uninstall ${@:2}
        ;;
    *)
        echo -e "${Error} 命令错误"
        ;;
esac