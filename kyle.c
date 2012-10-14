#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include <signal.h>
#include <sys/wait.h>

#include "bootloader.h"
#include "common.h"
#include "cutils/properties.h"
#include "firmware.h"
#include "install.h"
#include "make_ext4fs.h"
#include "minui/minui.h"
#include "minzip/DirUtil.h"
#include "roots.h"
#include "recovery_ui.h"

#include "../../external/yaffs2/yaffs2/utils/mkyaffs2image.h"
#include "../../external/yaffs2/yaffs2/utils/unyaffs.h"

#include "extendedcommands.h"
#include "nandroid.h"
#include "mounts.h"
#include "flashutils/flashutils.h"
#include "edify/expr.h"
#include <libgen.h>
#include "mtdutils/mtdutils.h"
#include "bmlutils/bmlutils.h"
#include "cutils/android_reboot.h"
#include "kyle.h"
#include "recovery.h"

void show_darkside_menu() {
    static char* headers[] = {  "Darkside Tools (BE CAREFUL!)",
                                "",
                                NULL
    };

    char* list[] = { "darkside.cache.wipe",
        "darkside.super.wipe_ext4 (BE CAREFUL!)",
        NULL
    };

    int chosen_item = get_menu_selection(headers, list, 0, 0);
    switch (chosen_item) {
        case 0:
                ensure_path_mounted("/emmc");
                if( access( "/emmc/clockworkmod/.darkside/cachewipe.zip", F_OK ) != -1) {
                install_zip("/emmc/clockworkmod/.darkside/cachewipe.zip");
                } else {
                ui_print("No darkside files found in /emmc/clockworkmod/.darkside");
                }
                break;
        case 1:
                ensure_path_mounted("/emmc");
                if( access( "/emmc/clockworkmod/.darkside/superwipe.zip", F_OK ) != -1) {
                install_zip("/emmc/clockworkmod/.darkside/superwipe.zip");
                } else {
                ui_print("No darkside files found in /emmc/clockworkmod/.darkside");
                }
                break;
    }
}

void show_efs_menu() {
    static char* headers[] = {  "EFS Tools",
                                "",
                                NULL
    };

    char* list[] = { "backup /efs partition to internal",
                     "restore /efs partition from internal",
                     "backup /efs partition to external",
                     "restore /efs partition from external",
                     NULL
    };

    int chosen_item = get_menu_selection(headers, list, 0, 0);
    switch (chosen_item) {
        case 0:
                ensure_path_mounted("/emmc");
                ensure_path_unmounted("/efs");
                __system("backup-efs.sh /emmc");
                ui_print("/emmc/clockworkmod/.efsbackup/efs.img created\n");
                break;
        case 1:
                ensure_path_mounted("/emmc");
                ensure_path_unmounted("/efs");
                if( access("/emmc/clockworkmod/.efsbackup/efs.img", F_OK ) != -1 ) {
                   __system("restore-efs.sh /emmc");
                   ui_print("/emmc/clockworkmod/.efsbackup/efs.img restored to /efs");
                } else {
                   ui_print("No efs.img backup found.\n");
                }
                break;
        case 2:
                ensure_path_mounted("/sdcard");
                ensure_path_unmounted("/efs");
                __system("backup-efs.sh /sdcard");
                ui_print("/sdcard/clockworkmod/.efsbackup/efs.img created\n");
                break;
        case 3:
                ensure_path_mounted("/sdcard");
                ensure_path_unmounted("/efs");
                if( access("/sdcard/clockworkmod/.efsbackup/efs.img", F_OK ) != -1 ) {
                   __system("restore-efs.sh /sdcard");
                   ui_print("/sdcard/clockworkmod/.efsbackup/efs.img restored to /efs");
                } else {
                   ui_print("No efs.img backup found.\n");
                }
                break;
    }
}

int create_customzip(const char* custompath)
{
    char command[PATH_MAX];
    sprintf(command, "create_update_zip.sh %s", custompath);
    __system(command);
    return 0;
}

#define SCRIPT_COMMAND_SIZE 512

int run_custom_ors(const char* ors_script) {
	FILE *fp = fopen(ors_script, "r");
	int ret_val = 0, cindex, line_len, i, remove_nl;
	char script_line[SCRIPT_COMMAND_SIZE], command[SCRIPT_COMMAND_SIZE],
		 value[SCRIPT_COMMAND_SIZE], mount[SCRIPT_COMMAND_SIZE],
		 value1[SCRIPT_COMMAND_SIZE], value2[SCRIPT_COMMAND_SIZE];
	char *val_start, *tok;
	int ors_system = 0;
	int ors_data = 0;
	int ors_cache = 0;
	int ors_recovery = 0;
	int ors_boot = 0;
	int ors_andsec = 0;
	int ors_sdext = 0;

	if (fp != NULL) {
		while (fgets(script_line, SCRIPT_COMMAND_SIZE, fp) != NULL && ret_val == 0) {
			cindex = 0;
			line_len = strlen(script_line);
			//if (line_len > 2)
				//continue; // there's a blank line at the end of the file, we're done!
			ui_print("script line: '%s'\n", script_line);
			for (i=0; i<line_len; i++) {
				if ((int)script_line[i] == 32) {
					cindex = i;
					i = line_len;
				}
			}
			memset(command, 0, sizeof(command));
			memset(value, 0, sizeof(value));
			if ((int)script_line[line_len - 1] == 10)
					remove_nl = 2;
				else
					remove_nl = 1;
			if (cindex != 0) {
				strncpy(command, script_line, cindex);
				ui_print("command is: '%s' and ", command);
				val_start = script_line;
				val_start += cindex + 1;
				strncpy(value, val_start, line_len - cindex - remove_nl);
				ui_print("value is: '%s'\n", value);
			} else {
				strncpy(command, script_line, line_len - remove_nl + 1);
				ui_print("command is: '%s' and there is no value\n", command);
			}
			if (strcmp(command, "install") == 0) {
				// Install zip
				ui_print("Installing zip file '%s'\n", value);
				ret_val = install_zip(value);
				if (ret_val != INSTALL_SUCCESS) {
					LOGE("Error installing zip file '%s'\n", value);
					ret_val = 1;
				}
			} else if (strcmp(command, "wipe") == 0) {
				// Wipe
				if (strcmp(value, "cache") == 0 || strcmp(value, "/cache") == 0) {
					ui_print("-- Wiping Cache Partition...\n");
					erase_volume("/cache");
					ui_print("-- Cache Partition Wipe Complete!\n");
				} else if (strcmp(value, "dalvik") == 0 || strcmp(value, "dalvick") == 0 || strcmp(value, "dalvikcache") == 0 || strcmp(value, "dalvickcache") == 0) {
					ui_print("-- Wiping Dalvik Cache...\n");
					if (0 != ensure_path_mounted("/data")) {
						ret_val = 1;
						break;
					}
					ensure_path_mounted("/sd-ext");
					ensure_path_mounted("/cache");
					if (confirm_selection( "Confirm wipe?", "Yes - Wipe Dalvik Cache")) {
						__system("rm -r /data/dalvik-cache");
						__system("rm -r /cache/dalvik-cache");
						__system("rm -r /sd-ext/dalvik-cache");
						ui_print("Dalvik Cache wiped.\n");
					}
					ensure_path_unmounted("/data");

					ui_print("-- Dalvik Cache Wipe Complete!\n");
				} else if (strcmp(value, "data") == 0 || strcmp(value, "/data") == 0 || strcmp(value, "factory") == 0 || strcmp(value, "factoryreset") == 0) {
					ui_print("-- Wiping Data Partition...\n");
					wipe_data(0);
					ui_print("-- Data Partition Wipe Complete!\n");
				} else {
					LOGE("Error with wipe command value: '%s'\n", value);
					ret_val = 1;
				}
			} else if (strcmp(command, "backup") == 0) {
				// Backup
				char backup_path[PATH_MAX];

				tok = strtok(value, " ");
				strcpy(value1, tok);
				tok = strtok(NULL, " ");
				if (tok != NULL) {
					memset(value2, 0, sizeof(value2));
					strcpy(value2, tok);
					line_len = strlen(tok);
					if ((int)value2[line_len - 1] == 10 || (int)value2[line_len - 1] == 13) {
						if ((int)value2[line_len - 1] == 10 || (int)value2[line_len - 1] == 13)
							remove_nl = 2;
						else
							remove_nl = 1;
					} else
						remove_nl = 0;
					strncpy(value2, tok, line_len - remove_nl);
					ui_print("Backup folder set to '%s'\n", value2);
					sprintf(backup_path, "/emmc/clockworkmod/backup/%s", value2);
				} else {
					time_t t = time(NULL);
					struct tm *tmp = localtime(&t);
					if (tmp == NULL)
					{
						struct timeval tp;
						gettimeofday(&tp, NULL);
						sprintf(backup_path, "/emmc/clockworkmod/backup/%d", tp.tv_sec);
					}
					else
					{
						strftime(backup_path, sizeof(backup_path), "/emmc/clockworkmod/backup/%F.%H.%M.%S", tmp);
					}
				}

				ui_print("Backup options are ignored in CWMR: '%s'\n", value1);
				nandroid_backup(backup_path);
				ui_print("Backup complete!\n");
			} else if (strcmp(command, "restore") == 0) {
				// Restore
				tok = strtok(value, " ");
				strcpy(value1, tok);
				ui_print("Restoring '%s'\n", value1);
				tok = strtok(NULL, " ");
				if (tok != NULL) {
					ors_system = 0;
					ors_data = 0;
					ors_cache = 0;
					ors_boot = 0;
					ors_sdext = 0;

					memset(value2, 0, sizeof(value2));
					strcpy(value2, tok);
					ui_print("Setting restore options:\n");
					line_len = strlen(value2);
					for (i=0; i<line_len; i++) {
						if (value2[i] == 'S' || value2[i] == 's') {
							ors_system = 1;
							ui_print("System\n");
						} else if (value2[i] == 'D' || value2[i] == 'd') {
							ors_data = 1;
							ui_print("Data\n");
						} else if (value2[i] == 'C' || value2[i] == 'c') {
							ors_cache = 1;
							ui_print("Cache\n");
						} else if (value2[i] == 'R' || value2[i] == 'r') {
							ui_print("Option for recovery ignored in CWMR\n");
						} else if (value2[i] == '1') {
							ui_print("%s\n", "Option for special1 ignored in CWMR");
						} else if (value2[i] == '2') {
							ui_print("%s\n", "Option for special1 ignored in CWMR");
						} else if (value2[i] == '3') {
							ui_print("%s\n", "Option for special1 ignored in CWMR");
						} else if (value2[i] == 'B' || value2[i] == 'b') {
							ors_boot = 1;
							ui_print("Boot\n");
						} else if (value2[i] == 'A' || value2[i] == 'a') {
							ui_print("Option for android secure ignored in CWMR\n");
						} else if (value2[i] == 'E' || value2[i] == 'e') {
							ors_sdext = 1;
							ui_print("SD-Ext\n");
						} else if (value2[i] == 'M' || value2[i] == 'm') {
							ui_print("MD5 check skip option ignored in CWMR\n");
						}
					}
				} else
					LOGI("No restore options set\n");
				nandroid_restore(value1, ors_boot, ors_system, ors_data, ors_cache, ors_sdext, 0);
				ui_print("Restore complete!\n");
			} else if (strcmp(command, "mount") == 0) {
				// Mount
				if (value[0] != '/') {
					strcpy(mount, "/");
					strcat(mount, value);
				} else
					strcpy(mount, value);
				ensure_path_mounted(mount);
				ui_print("Mounted '%s'\n", mount);
			} else if (strcmp(command, "unmount") == 0 || strcmp(command, "umount") == 0) {
				// Unmount
				if (value[0] != '/') {
					strcpy(mount, "/");
					strcat(mount, value);
				} else
					strcpy(mount, value);
				ensure_path_unmounted(mount);
				ui_print("Unmounted '%s'\n", mount);
			} else if (strcmp(command, "set") == 0) {
				// Set value
				tok = strtok(value, " ");
				strcpy(value1, tok);
				tok = strtok(NULL, " ");
				strcpy(value2, tok);
				ui_print("Setting function disabled in CWMR: '%s' to '%s'\n", value1, value2);
			} else if (strcmp(command, "mkdir") == 0) {
				// Make directory (recursive)
				ui_print("Recursive mkdir disabled in CWMR: '%s'\n", value);
			} else if (strcmp(command, "reboot") == 0) {
				// Reboot
			} else if (strcmp(command, "cmd") == 0) {
				if (cindex != 0) {
					__system(value);
				} else {
					LOGE("No value given for cmd\n");
				}
			} else {
				LOGE("Unrecognized script command: '%s'\n", command);
				ret_val = 1;
			}
		}
		fclose(fp);
		ui_print("Done processing script file\n");
	} else {
		LOGE("Error opening script file '%s'\n", ors_script);
		return 1;
	}
	return ret_val;
}

void show_custom_ors_menu(const char* ors_path)
{
    if (ensure_path_mounted(ors_path) != 0) {
        LOGE("Can't mount %s\n", ors_path);
        return;
    }

    static char* headers[] = {  "Choose a script to run",
                                "",
                                NULL
    };

    char tmp[PATH_MAX];
    sprintf(tmp, "%s/clockworkmod/ors/", ors_path);
    char* ors_file = choose_file_menu(tmp, ".ors", headers);
    if (ors_file == NULL)
        return;

    if (confirm_selection("Confirm run script?", "Yes - Run")) {
	run_custom_ors(ors_file);
    }
}

void show_extras_menu()
{
    static char* headers[] = {  "Extras Menu",
                                "",
                                NULL
    };

    static char* list[] = { "disable install-recovery.sh",
                            "enable/disable one confirm",
                            "hide/show backup & restore progress",
			    "set android_secure internal/external",
			    "aroma file manager",
			    "darkside tools",
			    "/efs tools",
			    "create custom zip",
			    "run custom openrecoveryscript",
			    "recovery info",
                            NULL
    };

    for (;;)
    {
        int chosen_item = get_filtered_menu_selection(headers, list, 0, 0, sizeof(list) / sizeof(char*));
        if (chosen_item == GO_BACK)
            break;
        switch (chosen_item)
        {
            case 0:
                if (ensure_path_mounted("/system") != 0)
                return 0;
                int ret = 0;
                struct stat st;
                if (0 == lstat("/system/etc/install-recovery.sh", &st)) {
                if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                ui_show_text(1);
                ret = 1;
                if (confirm_selection("ROM may flash stock recovery on boot. Fix?", "Yes - Disable recovery flash")) {
                __system("chmod -x /system/etc/install-recovery.sh");
            }
        }
    }
                ensure_path_unmounted("/system");
                return ret;
                break;
	    case 1:
		ensure_path_mounted("/emmc");
                if( access("/emmc/clockworkmod/.one_confirm", F_OK ) != -1 ) {
                   __system("rm -rf /emmc/clockworkmod/.one_confirm");
                   ui_print("one confirm disabled\n");
                } else {
                   __system("touch /emmc/clockworkmod/.one_confirm");
                   ui_print("one confirm enabled\n");
                }
		break;
	    case 2:
                ensure_path_mounted("/emmc");
                if( access("/emmc/clockworkmod/.hidenandroidprogress", F_OK ) != -1 ) {
                   __system("rm -rf /emmc/clockworkmod/.hidenandroidprogress");
                   ui_print("nandroid progress will be shown\n");
                } else {
                   __system("touch /emmc/clockworkmod/.hidenandroidprogress");
                   ui_print("nandroid progress will be hidden\n");
                }
                break;
	    case 3:
                ensure_path_mounted("/emmc");
                if( access("/emmc/clockworkmod/.is_as_external", F_OK ) != -1 ) {
                   __system("rm -rf /emmc/clockworkmod/.is_as_external");
                   ui_print("android_secure will be set to internal\n");
                } else {
                   __system("touch /emmc/clockworkmod/.is_as_external");
                   ui_print("android_secure will be set to external\n");
                }
                break;
	    case 4:
		ensure_path_mounted("/emmc");
		if( access( "/emmc/clockworkmod/.aromafm/aromafm.zip", F_OK ) != -1) {
                install_zip("/emmc/clockworkmod/.aromafm/aromafm.zip");
		} else {
                ui_print("No aroma files found in /emmc/clockworkmod/.aromafm");
		}
		break;
	    case 5:
		show_darkside_menu();
		break;
	    case 6:
		show_efs_menu();
		break;
	    case 7:
		ensure_path_mounted("/system");
		ensure_path_mounted("/emmc");
                if (confirm_selection("Create a zip from system and boot?", "Yes - Create custom zip")) {
		ui_print("Creating custom zip...\n");
		ui_print("This may take a while. Be Patient.\n");
                    char custom_path[PATH_MAX];
                    time_t t = time(NULL);
                    struct tm *tmp = localtime(&t);
                    if (tmp == NULL)
                    {
                        struct timeval tp;
                        gettimeofday(&tp, NULL);
                        sprintf(custom_path, "/emmc/clockworkmod/zips/%d", tp.tv_sec);
                    }
                    else
                    {
                        strftime(custom_path, sizeof(custom_path), "/emmc/clockworkmod/zips/%F.%H.%M.%S", tmp);
                    }
                    create_customzip(custom_path);
		ui_print("custom zip created in /emmc/clockworkmod/zips/\n");
	}
		ensure_path_unmounted("/system");
		break;
	    case 8:
		show_custom_ors_menu("/emmc");
		break;
	    case 9:
		ui_print("ClockworkMod Recovery 6.0.1.4 Touch v13.5\n");
		ui_print("Created By: sk8erwitskil (Kyle Laplante)\n");
		ui_print("Build Date: 10/11/2012 4:27 pm\n");
	}
    }
}
