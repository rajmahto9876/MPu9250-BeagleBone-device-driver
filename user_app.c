#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "mpu9250_user_interface.h"

/* IOCTL */
#define MPU_IOCTL_MAGIC     'm'
#define MPU_GET_DATA        _IOR(MPU_IOCTL_MAGIC, 1, mpu9250_t)

#define MPU_FILE_NAME       "/dev/mpu9250_device"

static mpu9250_t mpu_data;
int main()
{
    int ret = 0;
    int fd;

    fd = open(MPU_FILE_NAME, O_RDONLY);
    if(fd < 0)
    {
        printf (" File open Error \n");
        return -1;
    }

    memset(&mpu_data, 0x00, sizeof(mpu9250_t));

    ret = ioctl(fd, MPU_GET_DATA, &mpu_data);
    if(ret == -1)
    {
        printf ("ioctl Error \n");
        goto out;
    }

    printf (" Temp-> [%d]", mpu_data.Temperature);
    ret = 0;

out:
    close(fd);
    return ret;
}