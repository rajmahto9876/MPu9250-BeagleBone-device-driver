#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>

#include "mpu9250_helper.h"
/*================================================================
						DRIVER DATA
=================================================================*/
#define MPU9250_NAME		  "mpu9250_dhr_dev"
#define MPU9250_CLASS_NAME    "mpu9250_class"
#define MPU9250_DEVICE_NAME   "mpu9250_device"


#define WAKE_UP_LABEL			"wake_up_gpio"
#define WAKE_UP_IRQ_LABEL		"wake_up_irq"
#define WAKE_UP_GPIO    		60
typedef struct mpu9250_kernel_driver_t

{
    struct spi_device   *mpu9250_client;
    dev_t 				dev_num;
    struct 				cdev cdev;
    struct class 		*class;
    struct device 		*device;
    int 				gpio_num;
    int 				gpio_irq;
    int 				irq_args;
} mpu9250_kernel_driver_t;

/*
	SPI GPIO IN USE:-
	| MPU9250 | BBB SPI1                 |
	| ------- | ------------------------ |
	| VCC     | 3.3V                     |
	| GND     | GND                      |
	| SCK     | P9_31                    |
	| MISO    | P9_29                    |
	| MOSI    | P9_30                    |
	| CS      | P9_28                    |
	| INT     | P9_12 (GPIO1_28 example) |
*/

static mpu9250_kernel_driver_t 	mpu9250_dev;
static mpu9250_t 				mpu9250_device_data;		
/*================================================================
						HELPER FUCNTIONS
=================================================================*/
static void deinit_mpu9250(mpu9250_kernel_driver_t *mpu9250_spi_dvc)
{
	pr_info("In deinit_mpu9250 \n");
	free_irq(mpu9250_spi_dvc->gpio_irq, (void*)&(mpu9250_spi_dvc->irq_args));
	gpio_free(mpu9250_spi_dvc->gpio_num);
}

static void deinit_device(struct mpu9250_kernel_driver_t *mpu9250_driver)
{
	pr_info(" In deinit_device \n");

	deinit_mpu9250(&mpu9250_dev);
	cdev_del(&mpu9250_driver->cdev);
	device_destroy(mpu9250_driver->class, mpu9250_driver->dev_num);
	class_destroy(mpu9250_driver->class);
	unregister_chrdev_region(mpu9250_driver->dev_num, 1);
}

irqreturn_t wake_up_isr(int flag, void *args)
{
	pr_info("In Wake_ up_isr");
	return IRQ_HANDLED;
}

/*================================================================
						LOCAL CALLS
=================================================================*/
static loff_t fops_llseek (struct file *pFile, loff_t offset, int whence)
{
	pr_info("in fops_llseek \n");
	return 0;
}

static ssize_t fops_read (struct file *pFile, char __user * buf, size_t count, loff_t *pos)
{
	pr_info("in fops_read \n");
	return 0;
}

static ssize_t fops_write (struct file *pFile, const char __user *buf, size_t count, loff_t *pos)
{
	pr_info("in fops_write \n");
	return 0;
}

static int fops_open(struct inode *pInode, struct file *pFile)
{
	pr_info("in fops_open \n");
	return 0;
}

static int fops_release(struct inode *pInode, struct file *pFile)
{
	pr_info("in fops_release \n");
	return 0;
}

/* IOCTL */
#define MPU_IOCTL_MAGIC     'm'
#define MPU_GET_DATA        _IOR(MPU_IOCTL_MAGIC, 1, mpu9250_t)

static long fops_unlocked_ioctl(struct file *pFile, unsigned int cmd, unsigned long args)
{
	int ret = 0;
	mpu9250_t  *pMpu9250 = (mpu9250_t *)args;
	pr_info("in fops_unlocked_ioctl \n");

	switch(cmd)
	{
		case MPU_GET_DATA:
		{
			pr_info("Getting Data \n");
			mpu9250_Read_All(&mpu9250_device_data);

			ret = copy_to_user(pMpu9250, &mpu9250_device_data, sizeof(mpu9250_t));
			if(ret!= 0)
			{
				pr_err("copy_to_user Error \n");
				return -1;
			}
		}
		break;

		default:
		{
			pr_info("Cmd nbot defined \n");
		}
		break;
	}
	return 0;
}

static struct file_operations fops = 
{
	.owner 			= THIS_MODULE,
	.open  			= fops_open,
	.write  		= fops_write,
	.read  			= fops_read,
	.llseek 		= fops_llseek,
	.release  		= fops_release,
	.unlocked_ioctl = fops_unlocked_ioctl,
};

static int init_device(struct mpu9250_kernel_driver_t *mpu9250_driver)
{
	int ret = 0;
	pr_info("In init_device \n");

	ret = alloc_chrdev_region(&mpu9250_driver->dev_num, 0, 1, MPU9250_NAME);
    if (ret)
    {
    	pr_err("Alloc Chr Dev Region Error [%d]\n", ret);
        return ret;
    }

    mpu9250_driver->class = class_create(THIS_MODULE, MPU9250_CLASS_NAME);
    if(IS_ERR(mpu9250_driver->class))
    {
    	pr_err("mpu9250_class create error \n");
    	ret =  PTR_ERR(mpu9250_driver->class);
    	goto unregister_chr_dev;
    }

    mpu9250_driver->device = device_create(mpu9250_driver->class, NULL, 
    					mpu9250_driver->dev_num, NULL, MPU9250_DEVICE_NAME);

    if(IS_ERR(mpu9250_driver->device))
    {
    	pr_err("mpu9250_device create error \n");
    	ret =  PTR_ERR(mpu9250_driver->device);
    	goto class_destroy;
    }

    cdev_init(&mpu9250_driver->cdev, &fops);
    ret = cdev_add(&mpu9250_driver->cdev, mpu9250_driver->dev_num, 1);
    if (ret)
    {
    	pr_err("Cdev Add Error [%d]\n", ret);
        goto device_destroy;
    }
    mpu9250_driver->cdev.owner = THIS_MODULE;

    pr_info ("init_device OK \n");
	return 0;

device_destroy:
	device_destroy(mpu9250_driver->class, mpu9250_driver->dev_num);

class_destroy:
	class_destroy(mpu9250_driver->class);

unregister_chr_dev:
	unregister_chrdev_region(mpu9250_driver->dev_num, 1);

	return ret;
}

/*================================================================
						STATIC CALLS
=================================================================*/
int mpu9250_read_block_data(int reg, u8 *data, u8 size)
{
	int ret = 0;
    u8 *tx_buf;
    u8 *rx_buf;
    struct spi_transfer transfer = {0};
    struct spi_message msg;

    pr_info("spi_sync mpu9250_read_block_data\n");

    tx_buf = kzalloc(size + 1, GFP_KERNEL);
    if (!tx_buf)
    {
    	pr_err("No Mem Error tx_buf\n");
        return -ENOMEM;
    }

    rx_buf = kzalloc(size + 1, GFP_KERNEL);
    if (!rx_buf)
    {
    	pr_err("No Mem Error rx_buf\n");
        kfree(tx_buf);
        return -ENOMEM;
    }

    /*
     * MPU READ bit
     */
    tx_buf[0] = reg | 0x80;

    transfer.tx_buf = tx_buf;
    transfer.rx_buf = rx_buf;
    transfer.len    = size + 1;

    spi_message_init(&msg);

    spi_message_add_tail(&transfer, &msg);

    ret = spi_sync(mpu9250_dev.mpu9250_client, &msg);

    if (ret)
    {
        pr_err("spi_sync failed\n");
        goto out;
    }

    /*
     * Skip first dummy byte as it is dummy for sending the Reg Value
     */
    memcpy(data, &rx_buf[1], size);

out:
    kfree(tx_buf);
    kfree(rx_buf);
    return ret;
}

int mpu9250_write_block_data(int reg, const u8 *data, u8 size)
{
    int ret;
    u8 *tx_buf;
    struct spi_transfer transfer = {0};
    struct spi_message msg;

    pr_info("spi_sync mpu9250_write_block_data\n");

    tx_buf = kzalloc(size + 1, GFP_KERNEL);
    if (!tx_buf)
    {
    	pr_err("No Mem Error tx_buf\n");
        return -ENOMEM;
    }

    /*
     * Write operation
     * MSB = 0
     */
    tx_buf[0] = reg & 0x7F;

    memcpy(&tx_buf[1], data, size);

    transfer.tx_buf = tx_buf;
    transfer.len    = size + 1;

    spi_message_init(&msg);

    spi_message_add_tail(&transfer, &msg);

    ret = spi_sync(mpu9250_dev.mpu9250_client, &msg);
    if (ret)
    {
        pr_err("spi_sync failed\n");
    }

    kfree(tx_buf);
    return ret;
}

static int initilse_mpu9250(mpu9250_kernel_driver_t *mpu9250_spi_dvc)
{
	int ret = 0;
	pr_info ("initilse_mpu9250 \n");
	/*
		Initilisation Interrupt Gpio
	*/

	ret = mpu9250_Init();
	if(ret!= 0)
	{
		pr_err("mpu9250_Init Failed \n");
		goto out;
	}

	mpu9250_spi_dvc->irq_args = 0xDEADBEEF;
	mpu9250_spi_dvc->gpio_num = WAKE_UP_GPIO;
	ret = gpio_request(mpu9250_spi_dvc->gpio_num, WAKE_UP_LABEL);
    if (ret < 0)
    {
		pr_err("%s: failed to request WAKE_UP_GPIO: %d\n", __func__, ret);
		goto out;
	}

	gpio_direction_input(mpu9250_spi_dvc->gpio_num);

    /* IRQ setup */
    mpu9250_spi_dvc->gpio_irq = gpio_to_irq(mpu9250_spi_dvc->gpio_num);
    ret = request_irq(mpu9250_spi_dvc->gpio_irq, wake_up_isr,
     					IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, WAKE_UP_IRQ_LABEL, 
     					(void*)&(mpu9250_spi_dvc->irq_args)
     				);

    if (ret)
    {
		pr_err("%s: failed to request WAKE_UP_GPIO IRQ: %d\n", __func__, ret);
		goto gpio_irq_unassign;
	}

	pr_info("Intilisation Successful\n");
	ret = 0;
	return ret;

gpio_irq_unassign:
	free_irq(mpu9250_spi_dvc->gpio_irq, (void*)&(mpu9250_spi_dvc->irq_args));

out:
	return ret;
}

static int mpu9250_probe(struct spi_device *mpu9250_spi)
{
	int ret = 0;
	pr_info("In Probe \n");

	mpu9250_dev.mpu9250_client = mpu9250_spi;
	ret = init_device(&mpu9250_dev);
	if(ret!= 0)
	{
		pr_err("init_device Error\n");
		return ret;
	}

	ret = initilse_mpu9250(&mpu9250_dev);
	if(ret!= 0)
	{
		pr_err("initilse_mpu9250 Error\n");
		return ret;
	}

	pr_info("mpu9250 Probe Successful \n");
	return 0;
}

static int mpu9250_remove(struct spi_device *mpu9250_spi)
{
	pr_info("In Remove\n");
	deinit_device(&mpu9250_dev);
	return 0;
}

/*================================================================
						INIT CALLS
=================================================================*/
/*--------------------------------------------------------------
	Device Tree Matching ( By .compatible) -> NEW
----------------------------------------------------------------*/
const struct of_device_id match_table_id[] = 
{
	{.name = "mpu9250", .compatible = "raj,mpu9250", .data = (void*)0x123476589},
	{}/* Null Termination */
};

MODULE_DEVICE_TABLE(of, match_table_id);

/*------------------------------------------------
	Name Matching ( .name) -> LEGACY
---------------------------------------------------*/
const struct spi_device_id mpu9250_ids[] = 
{
	{.name = "mpu9250", .driver_data = 0x1476589A},
	{ }, /* Null Termination */
};

MODULE_DEVICE_TABLE(spi, mpu9250_ids);

/*------------------------------------------------
			INITILISATON PROBE
---------------------------------------------------*/
static struct spi_driver mpu9250_spi_driver = 
{
	.driver = {
		.name	= "mpu9250",
		.of_match_table = match_table_id,
	},
	.probe		= mpu9250_probe,
	.remove		= mpu9250_remove,
	.id_table	= mpu9250_ids,
};

module_spi_driver(mpu9250_spi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raj Kumar Mahto");
MODULE_DESCRIPTION("mpu9250 Driver SPI");
MODULE_FIRMWARE("V 1.0");