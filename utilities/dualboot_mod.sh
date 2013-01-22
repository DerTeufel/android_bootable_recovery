#!/sbin/sh

#
#
umount -f system
MOUNTPOINT=$1
FILE=$2

# ui_print by Chainfire
OUTFD=$(ps | grep -v "grep" | grep -o -E "update_binary(.*)" | cut -d " " -f 3);
echo "OUTFD: $OUTFD"
ui_print() {
  if [ $OUTFD != "" ]; then
    echo "ui_print ${1} " 1>&$OUTFD;
    echo "ui_print " 1>&$OUTFD;
  else
    echo "${1}";
  fi;
}

updater_script_path="META-INF/com/google/android/updater-script"
echo "mountpoint: $MOUNTPOINT"
echo "file: $FILE"
mkdir -p $MOUNTPOINT/dualboot || exit 1
rm -rf $MOUNTPOINT/dualboot/META-INF

#### unzip META-INF
ui_print "preparing rom"
ui_print "please be patient"
echo "unzip_binary -o $FILE $updater_script_path -d $MOUNTPOINT/dualboot"
unzip_binary -o $FILE $updater_script_path -d "$MOUNTPOINT"dualboot || exit 1

#### change the mountpoint ####
sed 's|/dev/lvpool/system|/dev/lvpool/secondary_system|g' -i "$MOUNTPOINT"dualboot/$updater_script_path || exit 1
sed 's|/dev/lvpool/userdata|/.secondrom/.secondrom/data.img|g' -i "$MOUNTPOINT"dualboot/$updater_script_path

# get the kernel
dd if=/dev/mtd/mtd0 of="$MOUNTPOINT"dualboot/boot.img || exit 1

cd $MOUNTPOINT/dualboot
zip $FILE $updater_script_path boot.img
ui_print "installing rom now ..."
cd /
#umount -f system
#unzip_binary -p -o $FILE $updater_script_path \
#| sed 's|/dev/lvpool/system| /dev/lvpool/secondary_system|g' \
#| zip $FILE $updater_script_path - || exit 1
