#include "mpu9250_helper.h"

u8 mpu9250_Init()
{
    u8 check = 0x00;

    // check device ID WHO_AM_I
    check = mpu9250_read_block_data(WHO_AM_I_REG, &check, 1);
    if(check!= 0x68)
    {
        return -1;
    }

    // power management register 0X6B we should write all 0's to wake the sensor up
    check = 0x00;
    mpu9250_write_block_data(PWR_MGMT_1_REG, &check, 1);

    // Set DATA RATE of 1KHz by writing SMPLRT_DIV register
    check = 0x07;
    mpu9250_write_block_data(SMPLRT_DIV_REG, &check, 1);

    // Set accelerometer configuration in ACCEL_CONFIG Register
    check = 0x00;
    mpu9250_write_block_data(ACCEL_CONFIG_REG, &check, 1);

    // Set Gyroscopic configuration in GYRO_CONFIG Register
    check = 0x00;
    mpu9250_write_block_data(GYRO_CONFIG_REG, &check, 1);
    
    return 0;
}

void mpu9250_Read_Accel(mpu9250_t *DataStruct)
{
    u8 Rec_Data[6];

    // Read 6 BYTES of data starting from ACCEL_XOUT_H register
    mpu9250_read_block_data(ACCEL_XOUT_H_REG, Rec_Data, 6);

    DataStruct->Accel_X_RAW = (int)(Rec_Data[0] << 8 | Rec_Data[1]);
    DataStruct->Accel_Y_RAW = (int)(Rec_Data[2] << 8 | Rec_Data[3]);
    DataStruct->Accel_Z_RAW = (int)(Rec_Data[4] << 8 | Rec_Data[5]);

    /*** convert the RAW values into acceleration in 'g'
         we have to divide according to the Full scale value set in FS_SEL
         I have configured FS_SEL = 0. So I am dividing by 16384.0
         for more details check ACCEL_CONFIG Register              
    ****/

    DataStruct->Ax = (DataStruct->Accel_X_RAW * 1000) / 16384;
    DataStruct->Ay = (DataStruct->Accel_Y_RAW * 1000) / 16384;
    DataStruct->Az = (DataStruct->Accel_Z_RAW * 1000) / ACCEL_Z_CORRECTOR;
}

void mpu9250_Read_Gyro(mpu9250_t *DataStruct)
{
    u8 Rec_Data[6];

    // Read 6 BYTES of data starting from GYRO_XOUT_H register
    mpu9250_read_block_data(GYRO_XOUT_H_REG, Rec_Data, 6);

    DataStruct->Gyro_X_RAW = (int)(Rec_Data[0] << 8 | Rec_Data[1]);
    DataStruct->Gyro_Y_RAW = (int)(Rec_Data[2] << 8 | Rec_Data[3]);
    DataStruct->Gyro_Z_RAW = (int)(Rec_Data[4] << 8 | Rec_Data[5]);

    /*** convert the RAW values into dps (�/s)
         we have to divide according to the Full scale value set in FS_SEL
         I have configured FS_SEL = 0. So I am dividing by 131.0
         for more details check GYRO_CONFIG Register              ****/

    DataStruct->Gx = (DataStruct->Gyro_X_RAW * 1000) / 131;
    DataStruct->Gy = (DataStruct->Gyro_Y_RAW * 1000) / 131;
    DataStruct->Gz = (DataStruct->Gyro_Z_RAW * 1000) / 131;
}

void mpu9250_Read_Temp(mpu9250_t *DataStruct)
{
    u8 Rec_Data[2];
    int temp;

    // Read 2 BYTES of data starting from TEMP_OUT_H_REG register
    mpu9250_read_block_data(TEMP_OUT_H_REG, Rec_Data, 2);

    temp = (int)(Rec_Data[0] << 8 | Rec_Data[1]);
    DataStruct->Temperature = ((temp * 1000) / (340 + 36));
}

void mpu9250_Read_All(mpu9250_t *DataStruct)
{
    u8 Rec_Data[14];
    int temp;

    // Read 14 BYTES of data starting from ACCEL_XOUT_H register
    mpu9250_read_block_data(ACCEL_XOUT_H_REG, Rec_Data, 14);

    DataStruct->Accel_X_RAW = (int)(Rec_Data[0] << 8 | Rec_Data[1]);
    DataStruct->Accel_Y_RAW = (int)(Rec_Data[2] << 8 | Rec_Data[3]);
    DataStruct->Accel_Z_RAW = (int)(Rec_Data[4] << 8 | Rec_Data[5]);
    temp = (int)(Rec_Data[6] << 8 | Rec_Data[7]);
    DataStruct->Gyro_X_RAW = (int)(Rec_Data[8] << 8 | Rec_Data[9]);
    DataStruct->Gyro_Y_RAW = (int)(Rec_Data[10] << 8 | Rec_Data[11]);
    DataStruct->Gyro_Z_RAW = (int)(Rec_Data[12] << 8 | Rec_Data[13]);

    DataStruct->Ax = (DataStruct->Accel_X_RAW * 1000) / 16384;
    DataStruct->Ay = (DataStruct->Accel_Y_RAW * 1000) / 16384;
    DataStruct->Az = (DataStruct->Accel_Z_RAW * 1000) / ACCEL_Z_CORRECTOR;
    DataStruct->Temperature = ((temp * 1000) / (340 + 36));
    DataStruct->Gx = DataStruct->Gyro_X_RAW / 131;
    DataStruct->Gy = DataStruct->Gyro_Y_RAW / 131;
    DataStruct->Gz = DataStruct->Gyro_Z_RAW / 131;
}