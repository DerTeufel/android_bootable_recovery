#!/sbin/sh

#
#

MOUNTPOINT=$1
FILE=$2
LOCATION=$3

updater_script_path="META-INF/com/google/android/updater-script"
echo "mountpoint: $MOUNTPOINT"
echo "file: $FILE"
echo "location: $LOCATION"
rm -rf $MOUNTPOINT/dualboot/*
mkdir -p $MOUNTPOINT/dualboot || exit 1
mkdir -p $MOUNTPOINT/dualboot/system/lib/


if [ "$LOCATION" == "primaryrom" ] ; then
#### unzip META-INF
if [ ! -s $FILE ] ; then
echo "could not find the file! Are there spaces in the name or path?"
exit 2
fi

unzip_binary -o $FILE $updater_script_path -d "$MOUNTPOINT"dualboot || exit 1

echo "preparing rom"
echo "please be patient"
sed 's|/dev/lvpool/secondary_system|/dev/lvpool/system|g' -i "$MOUNTPOINT"dualboot/$updater_script_path
sed 's|/.secondrom/.secondrom/data.img|/dev/lvpool/userdata|g' -i "$MOUNTPOINT"dualboot/$updater_script_path

# get the kernel
dd if=/dev/mtd/mtd0 of="$MOUNTPOINT"dualboot/boot.img || exit 1

#### get the modules
if [ -e system/lib/modules/ ] ; then
cp -R system/lib/modules/ "$MOUNTPOINT"dualboot/system/lib/
fi
cd $MOUNTPOINT/dualboot
if [ -e system/lib/modules/ ] ; then
zip $FILE $updater_script_path system/lib/modules/*.ko boot.img
else
zip $FILE $updater_script_path boot.img
fi

echo "installing rom now ..."
cd /

elif [ "$LOCATION" == "secondaryrom" ] ; then

#### unzip META-INF
if [ ! -s $FILE ] ; then
echo "could not find the file! Are there spaces in the name or path?"
exit 2
fi

unzip_binary -o $FILE $updater_script_path -d "$MOUNTPOINT"dualboot || exit 1
#### change the mountpoint ####
sed 's|/dev/lvpool/system|/dev/lvpool/secondary_system|g' -i "$MOUNTPOINT"dualboot/$updater_script_path || exit 1
sed 's|/dev/lvpool/userdata|/.secondrom/.secondrom/data.img|g' -i "$MOUNTPOINT"dualboot/$updater_script_path

# get the kernel
dd if=/dev/mtd/mtd0 of="$MOUNTPOINT"dualboot/boot.img || exit 1

#### get the modules
cp -R system/lib/modules/ "$MOUNTPOINT"dualboot/system/lib/ || exit 1
cd $MOUNTPOINT/dualboot
zip $FILE $updater_script_path system/lib/modules/*.ko boot.img
echo "installing rom now ..."
cd /
fi

umount -f system

#unzip_binary -p -o $FILE $updater_script_path \
#| sed 's|/dev/lvpool/system| /dev/lvpool/secondary_system|g' \
#| zip $FILE $updater_script_path - || exit 1
