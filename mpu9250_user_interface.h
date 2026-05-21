#ifndef __MP9U250_USER_INTERFACE_H_
#define __MPU9250_USER_INTERFACE_H_


#define RAD_TO_DEG                  57.295779513082320876798154814105
#define ACCEL_Z_CORRECTOR           14418

#ifndef mpu9250_ADDR
    #define mpu9250_ADDR 0xD0
#endif

#define WHO_AM_I_REG                0x75
#define PWR_MGMT_1_REG              0x6B
#define SMPLRT_DIV_REG              0x19
#define ACCEL_CONFIG_REG            0x1C
#define ACCEL_XOUT_H_REG            0x3B
#define TEMP_OUT_H_REG              0x41
#define GYRO_CONFIG_REG             0x1B
#define GYRO_XOUT_H_REG             0x43


typedef struct mpu9250_t
{
    int Accel_X_RAW;
    int Accel_Y_RAW;
    int Accel_Z_RAW;
    int Ax;
    int Ay;
    int Az;

    int Gyro_X_RAW;
    int Gyro_Y_RAW;
    int Gyro_Z_RAW;
    int Gx;
    int Gy;
    int Gz;

    int Temperature;
} mpu9250_t;

#endif /* __mpu9250_USER_INTERFACE_H_ */