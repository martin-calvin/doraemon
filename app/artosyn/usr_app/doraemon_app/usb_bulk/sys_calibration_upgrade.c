/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "usb_bulk_protocol.h"
#include "ar_flash.h"
#include "ar_wdt.h"
#include "sys_utils.h"
#include "sys_calibration_upgrade.h"

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    unsigned int    total_len;
    unsigned char   file_path[128];
    unsigned char   md5sum[16]; 
    unsigned char * pimage;
    int             offset;
}calibration_info;

#pragma pack(pop)


static unsigned char * CALIBRATION_FILE_PATH = "/tmp/calibration.img";
calibration_info image;

pthread_mutex_t mutex;

typedef struct m_UpgradeStatus {
    int16_t    status;
    int16_t    percentage;
    unsigned char  message[ 256 ];
} m_UpgradeStatus;

m_UpgradeStatus upgrade_status;

int sys_calibration_upgrade_init()
{
    memset(&upgrade_status, 0x00, sizeof(m_UpgradeStatus));
    pthread_mutex_init(&mutex, NULL);
    return 0;
}

int check_start_mark(char * data, char * mark, unsigned int len)
{
    int i;
    for(i = 0; i < strlen(mark); ++i)
    {
        if(data[i] != mark[i])
            return -1;
    }
    return 0;
}

void get_calibration_upgrade_status(int16_t *status, int16_t *percentage, unsigned char *message)
{
    pthread_mutex_lock(&mutex);

    *status = upgrade_status.status;
    *percentage = upgrade_status.percentage;
    memcpy(message, upgrade_status.message, sizeof(upgrade_status.message));

    // printf("<< get_calibration_upgrade_status.status: %d, *status = %d, percentage: %d\n", upgrade_status.status, *status, upgrade_status.percentage);

    pthread_mutex_unlock(&mutex);
}

int upgrade_status_update(int state, int percent)
{               
    printf(">> Upgrade status: %d, percent:%d\n", state, percent);

    pthread_mutex_lock(&mutex);

    upgrade_status.status = state;
    upgrade_status.percentage = percent;

    switch(state)
    {
        case CALIB_STAT_PREPAR:
            strcpy(upgrade_status.message, "Prepar ...");
            break;
        case CALIB_ERR_DDR:
            strcpy(upgrade_status.message, "Out of memory space !");
            break;
        case CALIB_ERR_NOT_ENOUGH_SPACE:
            strcpy(upgrade_status.message, "Not enough flash space !");
            break;
        case CALIB_ERR_PATH_NOT_WRITABLE:
            strcpy(upgrade_status.message, "The target file is not writable !");
            break;
        case CALIB_STAT_DONE:
            strcpy(upgrade_status.message, "Update Completed !");
            break;
        case CALIB_ERR_MD5_ERROR:
            strcpy(upgrade_status.message, "File md5 check error !");
            break;
    }

    pthread_mutex_unlock(&mutex);
    return 0;
}

void upgrade_calibration(void *p)
{
    calibration_info *upgrade_img = (calibration_info *)p;
    int ret = ar_flash_program(upgrade_img->pimage, upgrade_img->total_len, upgrade_status_update);
    if(ret < 0)
    {
        printf("error %d, %d, %d\n", __LINE__, ret, errno);
        upgrade_status_update(ret, 0);
    }
    printf("upgrade success!\n");
}

int start_file_upgrade_thread(calibration_info *upgrade_img)
{
    pthread_t thread;
	int ret_thrd1 = pthread_create(&thread, NULL, (void *)&upgrade_calibration, (void *) upgrade_img);
    return ret_thrd1;
}

int sys_calibration_upgrade_recv(void * data, unsigned int len)
{

    int ret = -1;
    // char * buffer = NULL;

    if (len == sizeof(UsbFileTransferBag) && magic_num_check(data) == 0)
    {
        /* Start or Finish or Stop */
        UsbFileTransferBag *usb_file_bag = data;
        switch (usb_file_bag->cmd)
        {
        case USB_FILE_TRANSFER_UPGRADE_START:
            // USB文件传输开始
            printf(">>> Upgrade receive start! len : %d\n", usb_file_bag->file_size);

            if(NULL != image.pimage)
            {
                free(image.pimage);
                image.pimage = NULL;
            }
            image.pimage = (unsigned char *)malloc(usb_file_bag->file_size);
            image.offset = 0;
            image.total_len = usb_file_bag->file_size;
            memcpy(image.md5sum, usb_file_bag->md5sum, 16);
            memset(image.file_path, 0x00, sizeof(image.file_path));
            strcpy(image.file_path, usb_file_bag->target_path);

            if(image.pimage == NULL)
            {
                printf("malloc failed: %d\n", usb_file_bag->file_size);
                upgrade_status_update(ERR_DDR, 0);
                ret = -1;
            }else{
                upgrade_status_update(0, 0);
                ret = 0;
            }
            break;
        case USB_FILE_TRANSFER_UPGRADE_STOP:
            // 传输终止
            printf(">>> Upgrade receive Stop!\n");
            if(NULL != image.pimage)
            {
                free(image.pimage);
                image.pimage = NULL;
            }
            image.offset = 0;
            image.total_len = 0;
            ret = 0;
            break;
        case USB_FILE_TRANSFER_UPGRADE_FINISH:
            // 传输完成
            printf(">>> Upgrade receive Finish!\n");

            if(image.offset >= image.total_len)
            {
                unsigned char decrypt[17];
                memset(decrypt, 0x00, sizeof(decrypt));
                md5_sum(image.pimage, image.total_len, decrypt);

                if (compare_chars(image.md5sum, decrypt, 16) == 0)
                {
                    // TODO: 此处还没有加入升级标定文件的功能
                    ret = write_to_file(image.pimage, CALIBRATION_FILE_PATH, image.total_len);
                    if (ret == 0)
                    {
                        upgrade_status_update(CALIB_STAT_DONE, 100);
                        if (image.pimage != NULL){
                            free(image.pimage);
                            image.offset = 0;
                            image.total_len = 0;
                            image.pimage = NULL;
                        }
                    }else{
                        upgrade_status_update(CALIB_ERR_PATH_NOT_WRITABLE, 100);
                    }
                }else{
                    upgrade_status_update(CALIB_ERR_MD5_ERROR, 0);
                }
            }else{
                printf(">>> The file transfer is complete, but the length is incorrect !");
            }
            ret = 0;
            break;
        default:
            break;
        }
    }else{
        if(0 != image.total_len && image.offset < image.total_len)
        {
            if(image.pimage == NULL)
            {
                printf("buffer allocate for image is null\n");
                upgrade_status_update(ERR_DDR, 0);
                return -1;
            }

            memcpy(image.pimage + image.offset, data, len);
            image.offset += len;
            ret = 0;
        }
        else
        {
            printf("sys_calibration_upgrade_recv no operation!\n");
        }
    }
    return ret;
}
