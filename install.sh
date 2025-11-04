#!/bin/bash

BIN_NAME="lykata"



if [[ ! -f lykata ]]; then
    echo "Please compile the project first. Use 'make' command or './gcc_build.sh'"
    exit
fi


read -p "Strip binary before installing? (y/n): " strip_bin

if [ ${strip_bin^^} == "Y" ]; then
    strip $BIN_NAME
    file $BIN_NAME
fi

echo ""
echo "Where to install $BIN_NAME ?"
echo " 0: /usr/local/bin/"
echo " 1: /usr/bin/"
echo ""

read -p "> " install_opt


install_bin() {
    set -xe
    bin_path="$1$BIN_NAME"
    if [ -f $bin_path ]; then
        echo "ERROR: $bin_path already exists. $BIN_NAME not installed."
        exit
    fi

    sudo cp $BIN_NAME $bin_path

    exit
}

if [ $install_opt == 0 ]; then
    install_bin "/usr/local/bin/"
elif [ $install_opt == 1 ]; then
    install_bin "/usr/bin/"
fi



