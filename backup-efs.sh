#!/sbin/sh

mkdir -p $1/clockworkmod/.efsbackup
dd if=/dev/block/mmcblk0p21 of=$1/clockworkmod/.efsbackup/efs.img
