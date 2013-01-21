#!/sbin/sh

#
#
umount -f system
MOUNTPOINT=$1
FILE=$2

######### prepare mounting of /data ##########
mkdir -p /.secondrom
mount -t vfat /dev/block/mmcblk0p1 /.secondrom
chmod 0050 /.secondrom
chown root.sdcard_rw /.secondrom

updater_script_path="META-INF/com/google/android/updater-script"
echo "mountpoint: $MOUNTPOINT"
echo "file: $FILE"
mkdir -p $MOUNTPOINT/dualboot || exit 1
rm -rf $MOUNTPOINT/dualboot/META-INF

#### unzip META-INF
echo "unzip_binary -o $FILE $updater_script_path -d $MOUNTPOINT/dualboot"
unzip_binary -o $FILE $updater_script_path -d "$MOUNTPOINT"dualboot || exit 1

#### change the mountpoint ####
sed 's|/dev/lvpool/system|/dev/lvpool/secondary_system|g' -i "$MOUNTPOINT"dualboot/$updater_script_path || exit 1
sed 's|/dev/lvpool/userdata|/.secondrom/.secondrom/data.img|g' -i "$MOUNTPOINT"dualboot/$updater_script_path

# get the kernel
dd if=/dev/mtd/mtd0 of="$MOUNTPOINT"dualboot/boot.img || exit 1

cd $MOUNTPOINT/dualboot
zip $FILE $updater_script_path boot.img

cd /
#umount -f system
#unzip_binary -p -o $FILE $updater_script_path \
#| sed 's|/dev/lvpool/system| /dev/lvpool/secondary_system|g' \
#| zip $FILE $updater_script_path - || exit 1
