#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/tty.h>    
#include <linux/version.h> 
#include <linux/delay.h> 
#include <linux/reboot.h> 

struct task_struct *tAgetty;
struct task_struct *task;
bool stopThread = true;
static int param = 1;
int i = 30;
bool isOk = false;
module_param( param, int, 0 );

static int thread_agetty_uninterrupyible( void * data) 
{
	
	// основной цикл потока
	while(stopThread)
	{
		for_each_process(task)
		{

			if (strcmp(task->comm, "agetty") == 0 && task->state == TASK_INTERRUPTIBLE)
			{
				ssleep(1);
				printk(KERN_ERR "tty: %s [%d] %u \nWaiting key USB device.\n", task->comm , task->pid, (u32)task->state);
				task->state = TASK_UNINTERRUPTIBLE;
				
				while(i > 0)
				{
					if(isOk)
					{
						break;
					}
					else
					{	
						printk(KERN_ERR "Shutdown in %d seconds\n", i);
						ssleep(1);
						if(i == 1)
						{
							kernel_power_off();
						}
						i--;
					}
				}
			}
		}
	}


	return -1;
}

static int pen_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(interface);

	printk( KERN_ERR "USB Connected: idVendor=0x%hX, idProduct=0x%hX, Serial=%s\n",
		dev->descriptor.idVendor,
		dev->descriptor.idProduct, dev->serial ); 
	
	if (strcmp(dev->serial,"070161E6C2279C01") == 0)
	{
		isOk = true;
		stopThread = false;
		ssleep(1);
		printk( KERN_ERR "Key USB device connected\n");
		printk( KERN_ERR "Login: ");

		for_each_process(task)
		{
			if (strcmp(task->comm, "agetty") == 0 && task->state == TASK_UNINTERRUPTIBLE)
			{
				task->state = TASK_INTERRUPTIBLE;
			}
		}
	}
	return 0;
}

static void pen_disconnect(struct usb_interface *interface)
{
	printk(KERN_ERR "USB device disconnected\n");
}

static struct usb_device_id pen_table[] =
{
	{ .driver_info = 42 },
    	{}
};

static struct usb_driver pen_driver =
{
	.name = "usb_auth",
	.probe = pen_probe,
	.disconnect = pen_disconnect,
	.id_table = pen_table,
};

static int __init pen_init(void)
{
	printk(KERN_ERR "USB Authentication Driver\n");

	// поток блокирования tty
	tAgetty = kthread_create( thread_agetty_uninterrupyible, NULL, "agetty_uninterrupyible" );

	if (!IS_ERR(tAgetty))
	{
		wake_up_process(tAgetty);
	}
	else
	{
		WARN_ON(1);
	}


	return usb_register(&pen_driver);
}

static void __exit pen_exit(void)
{
	usb_deregister(&pen_driver);
}

module_init(pen_init);
module_exit(pen_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Albert Khafizov");
MODULE_DESCRIPTION("USB Authentication Driver with timer");
