
#include <linux/module.h>  
#include <linux/kernel.h>   
#include <linux/fs.h>	   
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>

#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/gpio.h>
#include <asm/gpio.h>
#include <linux/interrupt.h>
#include <linux/time.h>

#define DEVICE_NAME  "sensor"

struct sensor_dev
{
	struct cdev cdev;
	int distance;
} *sensor_devp;

static dev_t sensor_dev_number;
struct class *sensor_dev_class;
static struct device *sensor_dev_device;
static struct task_struct * LEDThread = NULL;
static struct task_struct * sendThread = NULL;
static volatile int echo = 0;
static volatile int readTime = 0;
static struct timespec start;
static struct timespec stop;


int sensor_driver_open(struct inode *inode, struct file *file)
{
	struct sensor_dev *devp;
	devp = container_of(inode->i_cdev, struct sensor_dev, cdev);
	file->private_data = devp;

	return 0;
}



int sensor_driver_release(struct inode *inode, struct file *file)
{
	//struct sensor_dev *devp = file->private_data;
	return 0;
}

ssize_t sensor_driver_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	//struct sensor_dev *devp = file->private_data;
	return 0;
}

ssize_t sensor_driver_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int ret;
	struct sensor_dev *devp = file->private_data;
	ret = copy_to_user(buf, &devp->distance, sizeof(int));
	if(ret<0)
	{
		return -1;
	}
	return sizeof(int);
}

static struct file_operations sensor_fops = {
    .owner		= THIS_MODULE,           	/* Owner */
    .open		= sensor_driver_open,       /* Open method */
    .release	= sensor_driver_release,    /* Release method */
    .write		= sensor_driver_write,      /* Write method */
    .read		= sensor_driver_read,       /* Read method */
};




static int checkEcho(void *data)

{
	while(!kthread_should_stop())
	{
		if(readTime == 1)
		{
			sensor_devp->distance = ((stop.tv_sec*1000000000 + stop.tv_nsec) - (start.tv_sec*1000000000 + start.tv_nsec))/58000;
			readTime=0;
		}
		msleep(100);
	}
	return 0;
}

static int sendTrigger(void *data)

{
	while(!kthread_should_stop())
	{
		gpio_set_value_cansleep(15, 1);
		udelay(20);
		gpio_set_value_cansleep(15, 0);
		msleep(1000);
	}
	return 0;
}
static irq_handler_t echo_handler(int irq, void *dev_id, struct pt_regs *regs)

{
	if(echo==0)
	{
		getnstimeofday(&start);
		irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);
		echo=1;
	}
	else
	{
		getnstimeofday(&stop);
		irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
		echo=0;
		readTime=1;
	}
	return (irq_handler_t)IRQ_HANDLED;
}


int __init sensor_driver_init(void)
{
	int ret;
	unsigned int echo_irq=0;
	
	if (alloc_chrdev_region(&sensor_dev_number, 0, 1, DEVICE_NAME) < 0) 
	{
		printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	sensor_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	
	sensor_devp = kmalloc(sizeof(struct sensor_dev), GFP_KERNEL);	
	if (!sensor_devp) 
	{
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	cdev_init(&sensor_devp->cdev, &sensor_fops);
	sensor_devp->cdev.owner = THIS_MODULE;

	ret = cdev_add(&sensor_devp->cdev, (sensor_dev_number), 1);

	if (ret) 
	{
		printk("Bad cdev\n");
		return ret;
	}

	sensor_dev_device = device_create(sensor_dev_class, NULL, MKDEV(MAJOR(sensor_dev_number), 0), NULL, DEVICE_NAME);		

	ret = gpio_request(31, "gpio_1_Mux");
	ret = gpio_request(30, "gpio_2_Mux");
	ret = gpio_request(14, "gpio_1");
	ret = gpio_request(15, "gpio_2");

	if (ret<0)
	{
		printk("Cannot get GPIO pin...\n");
		return -1;
	}

	ret = gpio_direction_output(31, 0);
	ret = gpio_direction_output(30, 0);
	ret = gpio_direction_input(14);
	ret = gpio_direction_output(15, 0);

	if (ret<0)
	{
		printk("Cannot get GPIO pin...\n");
		return -1;
	}

	gpio_set_value_cansleep(31, 0);
	gpio_set_value_cansleep(30, 0);
	gpio_set_value_cansleep(15, 0);


	echo_irq = gpio_to_irq(14);

	ret = request_irq(echo_irq, (irq_handler_t)echo_handler, IRQF_TRIGGER_RISING, "Button_Dev", (void *)(echo_irq));

	if(ret < 0)

	{
		printk("Error requesting IRQ: %d\n", ret);
	}

	LEDThread = kthread_run(checkEcho, NULL,"echo");
	sendThread = kthread_run(sendTrigger, NULL,"trigger");

	return 0;
}
/* Driver Exit */
void __exit sensor_driver_exit(void)
{
	unsigned int echo_irq = 0;

	unregister_chrdev_region((sensor_dev_number), 1);
	device_destroy (sensor_dev_class, MKDEV(MAJOR(sensor_dev_number), 0));
	cdev_del(&sensor_devp->cdev);
	kfree(sensor_devp);
	class_destroy(sensor_dev_class);

	
	echo_irq = gpio_to_irq(14);

	free_irq(echo_irq, (void *)(echo_irq));
	gpio_free(15);
	gpio_free(31);
	gpio_free(30);
	gpio_free(14);

	if(LEDThread)
	{
		kthread_stop(LEDThread);
	}

	if(sendThread)
	{
		kthread_stop(sendThread);
	}
}

module_init(sensor_driver_init);
module_exit(sensor_driver_exit);
MODULE_LICENSE("GPL v2");
