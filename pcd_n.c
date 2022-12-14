#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>

#define NUMBER_OF_DEVICES 4

#define DEV_MEM_SIZE_PCDEV1 1024
#define DEV_MEM_SIZE_PCDEV2 1024
#define DEV_MEM_SIZE_PCDEV3 1024
#define DEV_MEM_SIZE_PCDEV4 1024

#define RDONLY 0x1
#define WRONLY 0x10
#define RDWR   0x11

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/* Pseudo driver memory buffer */
char memory_buffer_pcdev1[DEV_MEM_SIZE_PCDEV1];
char memory_buffer_pcdev2[DEV_MEM_SIZE_PCDEV2];
char memory_buffer_pcdev3[DEV_MEM_SIZE_PCDEV3];
char memory_buffer_pcdev4[DEV_MEM_SIZE_PCDEV4];

/* Device private data structure */

struct pcdev_private_data
{
	char *buffer;
	int size;
	const char *serial_number;
	/* Device permission */
	int perm;
	/* Cdev Variable */
	struct cdev cdev;
};

	int i;
/* Driver private data structure */

struct pcdrv_private_data
{
	int total_devices;

	struct pcdev_private_data pcdev_data[NUMBER_OF_DEVICES];
	/* This hold the device number */
	dev_t device_number;

	/* Declaration class structure pointer */
	struct class *pcd_class;

	/* Declaration of device structure pointer */
	struct device *pcd_device;
};

struct pcdrv_private_data pcdrv_data =
{
	.total_devices = NUMBER_OF_DEVICES,
	.pcdev_data = {
		[0] = {
			.buffer = memory_buffer_pcdev1,
			.size = DEV_MEM_SIZE_PCDEV1,
			.serial_number = "PCDEV1",
			.perm = RDONLY /* RDONLY */
		},

		[1] = {
			.buffer = memory_buffer_pcdev2,
			.size = DEV_MEM_SIZE_PCDEV2,
			.serial_number = "PCDEV2",
			.perm = WRONLY /* WRONLY */
		},

		[2] = {
			.buffer = memory_buffer_pcdev3,
			.size = DEV_MEM_SIZE_PCDEV3,
			.serial_number = "PCDEV3",
			.perm = RDWR /* RDWR */
		},

		[3] = {
			.buffer = memory_buffer_pcdev4,
			.size = DEV_MEM_SIZE_PCDEV4,
			.serial_number = "PCDEV4",
			.perm = RDWR /* RDWR */
		}

	}

};

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{

	/* Define current cursor position */
	loff_t curr_f_pos;

	/* Extracting private data file structure from filp */
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	/* Defining device memory size */
	unsigned int dev_mem_size = pcdev_data -> size;

	pr_info("lseek requested \n");
	pr_info("Current value of file position = %lld", filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:

			/* Check if offset is greater than buffer array size */

			if(offset > dev_mem_size  || offset < 0)
			{
				return -EINVAL;
			}

			filp-> f_pos = offset;
			break;

		case SEEK_CUR:

			/*Check if sum of current file position and offset is not greater than buffer size and less than 0 */
			curr_f_pos = filp-> f_pos + offset;
			if(curr_f_pos > dev_mem_size || curr_f_pos <0)
			{
				return -EINVAL;
			}

			filp-> f_pos = curr_f_pos;

			break;

		case SEEK_END:

			/*Check if sum of current file position and offset is not greater than buffer size and less than 0 */
			curr_f_pos = dev_mem_size + offset;
			if(curr_f_pos > dev_mem_size || curr_f_pos <0)
			{
				return -EINVAL;
			}
			filp-> f_pos = curr_f_pos;
			break;
		default:
			/* Invalid whence argument has been received */
			return -EINVAL;

		pr_info("New value of file pointer file position = %lld\n", filp->f_pos);
	}

	return filp->f_pos;

}
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{

	/* Extracting private data file structure from filp */
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	/* Defining device memory size */
	unsigned int dev_mem_size = pcdev_data -> size;

	/* 0. Print read request amount of data bytes and actuall position of data before read */

	pr_info("\nread requested for %zu bytes \n",count);
	pr_info("\nCurrent position of data before reading process = %lld\n", *f_pos);

	/* 1. Check if value of count data is not greater than buffer size, and if is trim count value */

	if((*f_pos + count) > dev_mem_size)
	{
		count = dev_mem_size - *f_pos;
	}

	/* 2. Copy kernel buffer data to user space */

	if(copy_to_user(buff, pcdev_data->buffer+(*f_pos), count))
	{
		return -EFAULT;
	}

	/* 4. Update (increment) the current file position */

	*f_pos += count;

	/* 5. Print amount of data successfully read and updated file position */

	pr_info("\nNumber of bytes successfully read = %zu\n", count);
	pr_info("\nUpdated position of data = %lld\n", *f_pos);

	/* 6. Return amount of data bytes if data was successfully read */

	return count;


	return 0;

}
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count,  loff_t *f_pos)
{

	/* Extracting private data file structure from filp */
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	/* Defining device memory size */
	unsigned int dev_mem_size = pcdev_data -> size;

	/* 0. Print read request amount of data bytes and actuall position of data before read */

	pr_info("Write requested for %zu bytes\n",count);
	pr_info("\nCurrent position of data before writing process = %lld\n", *f_pos);

	/* 1. Check if value of count data is not greater than buffer size, and if is trim count value */

	if((*f_pos + count) > dev_mem_size)
	{
		count = dev_mem_size - *f_pos;
	}

	/* If on input is zero value of data return error */

	if(!count)
	{
		pr_err("No memory left on device\n");
		return -ENOMEM;
	}

	/* 2. Copy user space data to kernel space */

	if(copy_from_user(pcdev_data->buffer+(*f_pos), buff, count))
	{
		return -EFAULT;
	}

	/* 4. Update (increment) the current file position */

	*f_pos += count;

	/* 5. Print amount of data successfully written and updated file position */

	pr_info("\nNumber of bytes successfully written = %zu\n", count);
	pr_info("\nUpdated position of data = %lld\n", *f_pos);

	/* 6. Return count of data bytes if data was successfully written */

	return count;

}

int check_permission(int device_permission, int access_permission)
{
	if(device_permission == RDWR)
	{
		return 0;
	}
	/* Ensures read-only access */
	if((device_permission == RDONLY) && (access_permission & FMODE_READ) && !(access_permission & FMODE_WRITE))
	{
		return 0;
	}
	/* Ensures write-only access */
	if((device_permission == WRONLY) && (access_permission & FMODE_WRITE) && !(access_permission & FMODE_READ))
	{
		return 0;
	}

	return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
	int minor_n;
	int ret;

	struct pcdev_private_data *pcdev_data;

	/* Find out on which device numer open was attempted by user space */
	minor_n = MINOR(inode->i_rdev);
	pr_info("Device number %d was attempted\n",minor_n);

	/* Extract device private data structure from *cdev */

	pcdev_data = container_of(inode -> i_cdev, struct pcdev_private_data, cdev);

	/* Supply other methods with device private data */
	filp->private_data = pcdev_data;

	ret = check_permission(pcdev_data->perm, filp->f_mode);

	if(!ret)
	{
		pr_info("Open was succesfull\n");
	}
	else
	{
		pr_info("Open was unsuccessfull\n");
		return -EPERM;
	}

	return 0;

}
int pcd_release(struct inode *inode, struct file *flip)
{
	pr_info("Close was successful\n");
	return 0;
}

/* File operations of the driver */

struct file_operations pcd_fops =
{
	.open = pcd_open,
	.write = pcd_write,
	.read = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};


/* Module initialization fucntion */

static int __init pcd_init(void)
{


/* Module init function with implemented error handling functionality */

	int ret;

	/* 1.	Dynamically allocate a device number */
	ret = alloc_chrdev_region(&pcdrv_data.device_number,0,NUMBER_OF_DEVICES,"pcd_n");

	/* 1.e Error handling */
	if(ret < 0)
	{
		goto out;
	}

	/* 2. Create device class under /sys/class/ */
	pcdrv_data.pcd_class = class_create(THIS_MODULE, "pcd_class");

	if(IS_ERR(pcdrv_data.pcd_class))
	{
		/* Print some error info */
		pr_err("Class creation failed\n");
		/*Convert pointer to error code int */
		ret = PTR_ERR(pcdrv_data.pcd_class);
		goto unreg_chardev_region;
	}



	for(i=0; i< NUMBER_OF_DEVICES; i++)
	{
		pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(pcdrv_data.device_number+i), MINOR(pcdrv_data.device_number+i));

		/* 3. Initialization the cdev structure with fops */
		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);

		/* 4. Register a device (cdev structure) with VFS */
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number+i,1);

		/* 4.e Error handling */
		if(ret < 0)
		{
			pr_err("Cdev add failed\n");
			goto cdev_del;
		}
		/* 5. Populate the sysfs (/sys/class/) with device information */
		pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, pcdrv_data.device_number+i, NULL, "pc-dev-%d", i+1);

		/* 5.e Error handling */
		if(IS_ERR(pcdrv_data.pcd_device))
		{
			pr_err("Device was not created\n");
			ret = PTR_ERR(pcdrv_data.pcd_device);
			goto class_del;
		}

	}

	/* Confirmation of successfully ended initialization */
	pr_info("Module initialization was succesfull\n");
	return 0;


cdev_del:
class_del:
for (;i>=0;i--)
{
	device_destroy(pcdrv_data.pcd_class,pcdrv_data.device_number+i);
	cdev_del(&pcdrv_data.pcdev_data[i].cdev);
}

	class_destroy(pcdrv_data.pcd_class);

unreg_chardev_region:
	unregister_chrdev_region(pcdrv_data.device_number, NUMBER_OF_DEVICES);

out:
	pr_err("Module insertion failed\n");
	return ret;

}

/* Module clean-up function */

static void __exit pcd_cleanup(void)
{

	int i;
	/* Clean-up functions should be called in revers order compare to __init functions order */



	for(i=0;i<NUMBER_OF_DEVICES;i++)
	{
		/* 1. Destroy device information in sysfs */
		device_destroy(pcdrv_data.pcd_class,pcdrv_data.device_number+i);
		/* 3. Unregister a device (cdev structure) from VFS */
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}


	/* 2. Destroy device class in sysfs */
	class_destroy(pcdrv_data.pcd_class);

	/* 4. Deallocate a device number */

	unregister_chrdev_region(pcdrv_data.device_number, NUMBER_OF_DEVICES);

	/* 5. Some clean-up message */

	pr_info("Module was succesfully unloaded\n");



}



/* Adding function to init call */
module_init(pcd_init);

/* Adding function to exit call */
module_exit(pcd_cleanup);

/* Module description */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MKI");
MODULE_DESCRIPTION("A pseudo character device driver which can handle n device");

/* Hello github */
