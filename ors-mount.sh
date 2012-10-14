#!/sbin/sh

MOUNT=`cat /etc/fstab | grep 28 | awk '{print $2}'`

if [ $MOUNT="/emmc" ]
then
sed -i 's/\/sdcard/\/emmc/g' /cache/recovery/openrecoveryscript
fi
