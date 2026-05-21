#ifndef __mpu9250_H_
#define __mpu9250_H_

	#include <linux/types.h>
	#include "mpu9250_user_interface.h"

	u8 mpu9250_Init(void);
	void mpu9250_Read_Accel(mpu9250_t *DataStruct);
	void mpu9250_Read_Gyro( mpu9250_t *DataStruct);
	void mpu9250_Read_Temp(mpu9250_t *DataStruct);
	void mpu9250_Read_All(mpu9250_t *DataStruct);
	int mpu9250_read_block_data(int reg, u8 *data, u8 size);
	int mpu9250_write_block_data(int reg, const u8 *data, u8 size);

#endif /* mpu9250_H */