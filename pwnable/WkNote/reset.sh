#!/bin/sh

if [ $# -gt 0 ]; then
 TEAM=$1
else
 TEAM="UNKNOWN"
fi

DATE=$(date "+%y%m%d%H%M%S")
DIR="$HOME/exploits/$TEAM/$DATE"

mkdir -p $DIR
sudo mv /srv/ftp/wknote/* $DIR/

UUID=$(cat $HOME/2023_ctf_finals/pwnable/WkNote/files/.vagrant/machines/default/virtualbox/id)
VBoxManage controlvm $UUID poweroff

while [ "$(VBoxManage showvminfo $UUID | grep running)" ]; do
	echo Waiting...
	sleep 1
done

VBoxManage snapshot $UUID restorecurrent
VBoxManage sharedfolder add $UUID --name wknote --hostpath /srv/ftp/wknote/ --automount
