#!/sbin/sh

#
#
# mount secondary filesystem

BB="busybox"
MOUNT="busybox mount"
UMOUNT="busybox umount -f"
CHOWN="busybox chown"
CHMOD="busybox chmod"

mount_sdcard=0

$UMOUNT /system
$UMOUNT /data
$UMOUNT /datadata
$UMOUNT /.secondrom


########## if called with umount parameter, just umount everything and exit ######################
if [ "$1" == "umount" ] ; then
   $BB cp /etc/default.fstab /etc/fstab
   if $BB grep -q "/system" /proc/mounts ||
	$BB grep -q "/data" /proc/mounts ; then
		exit 1
   fi
elif [ "$1" == "primary" ] ; then
	$BB cp /etc/primary.fstab /etc/fstab
	if $BB grep -q "secondary_system" /proc/mounts ||
	$BB grep -q "loop0" /proc/mounts ; then
	   exit 1
	fi
	$MOUNT -t ext4 -o rw /dev/lvpool/userdata /data
	$MOUNT -t ext4 -o rw /dev/lvpool/system /system

	if ! $BB grep -q "lvpool/system" /proc/mounts ||
	! $BB grep -q "userdata" /proc/mounts ; then
		exit 1
	fi
elif [ "$1" == "secondary" ] ; then
	$BB cp /etc/secondary.fstab /etc/fstab
	if $BB grep -q "lvpool/system" /proc/mounts ||
	$BB grep -q "lvpool/userdata" /proc/mounts ; then
		exit 1
	fi
	
	if $BB grep -q sdcard /proc/mounts ; then
	mount_sdcard=1
	$UMOUNT /sdcard
	fi

	$MOUNT -t vfat /dev/block/mmcblk0p1 /.secondrom
	$CHMOD 0050 /.secondrom
	$CHOWN root.sdcard_rw /.secondrom
	data=/.secondrom/.secondrom/data.img
	size=`$BB du -ks $data |$BB cut -f1`
	echo "size: $size"
	if $BB [ "$size" -le 700000 ] || $BB [ ! -s $data ] ; then
		$BB rm -rf $data
	fi
#################### check if data.img already created #############
	if $BB [ ! -f $data ] ; then
        # create a file 700MB
        $BB dd if=/dev/zero of=$data bs=1024 count=716800
         # create ext4 filesystem
		 $BB mke2fs -F -T ext4 $data
	fi
	$BB mkdir -p /sdcard

	$MOUNT -t ext4 -o rw /.secondrom/.secondrom/data.img /data
	$MOUNT -t ext4 -o rw /dev/lvpool/secondary_system /system

	if $BB [ "$mount_sdcard" -eq 1 ] ; then
		$MOUNT /sdcard
	fi

	if ! $BB grep -q "secondary_system" /proc/mounts ||
	! $BB grep -q "loop0" /proc/mounts ; then
		exit 1
	fi
else
	echo "missing paramter"
	exit 1
fi

exit 0
