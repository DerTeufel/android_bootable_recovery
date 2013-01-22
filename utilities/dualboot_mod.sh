#!/sbin/sh

#
#
umount -f system
MOUNTPOINT=$1
FILE=$2
LOCATION=$3

updater_script_path="META-INF/com/google/android/updater-script"
echo "mountpoint: $MOUNTPOINT"
echo "file: $FILE"
echo "location: $LOCATION"
mkdir -p $MOUNTPOINT/dualboot || exit 1
rm -rf $MOUNTPOINT/dualboot/*

#### unzip META-INF
echo "unzip_binary -o $FILE $updater_script_path -d $MOUNTPOINT/dualboot"
unzip_binary -o $FILE $updater_script_path -d "$MOUNTPOINT"dualboot || exit 1

if [ "$LOCATION" == "primaryrom" ] ; then
ui_print "preparing rom"
ui_print "please be patient"
sed 's|/dev/lvpool/secondary_system|/dev/lvpool/system|g' -i "$MOUNTPOINT"dualboot/$updater_script_path || exit 1
sed 's|/.secondrom/.secondrom/data.img|/dev/lvpool/userdata|g' -i "$MOUNTPOINT"dualboot/$updater_script_path

# get the kernel
dd if=/dev/mtd/mtd0 of="$MOUNTPOINT"dualboot/boot.img || exit 1
cd $MOUNTPOINT/dualboot
zip $FILE $updater_script_path boot.img
ui_print "installing rom now ..."
cd /

elif [ "$LOCATION" == "secondaryrom" ] ; then 
#### change the mountpoint ####
sed 's|/dev/lvpool/system|/dev/lvpool/secondary_system|g' -i "$MOUNTPOINT"dualboot/$updater_script_path || exit 1
sed 's|/dev/lvpool/userdata|/.secondrom/.secondrom/data.img|g' -i "$MOUNTPOINT"dualboot/$updater_script_path

# get the kernel
dd if=/dev/mtd/mtd0 of="$MOUNTPOINT"dualboot/boot.img || exit 1
cd $MOUNTPOINT/dualboot
zip $FILE $updater_script_path boot.img
ui_print "installing rom now ..."
cd /
fi


#umount -f system
#unzip_binary -p -o $FILE $updater_script_path \
#| sed 's|/dev/lvpool/system| /dev/lvpool/secondary_system|g' \
#| zip $FILE $updater_script_path - || exit 1
