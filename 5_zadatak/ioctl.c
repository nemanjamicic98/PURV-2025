/***************************************************************************//**
*  \file       ioctl.c
*
*  \details    Simple Linux device driver (IOCTL) for Game of Life 
*
*  \author     EmbeTronicX/andjelas
*
*  \Tested with Linux raspberrypi 
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 //kmalloc()
#include <linux/uaccess.h>              //copy_to_user()/copy_from_user()
#include <linux/ioctl.h>
#include <linux/err.h>


/* === DODATO za 6.x kompatibilnost === */
#include <linux/version.h>
#include <linux/device/class.h>
/* class_create API:
 *  - 5.x: class_create(THIS_MODULE, name)
 *  - 6.x: class_create(name)
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
#define CLASS_CREATE(_name) class_create(_name)
#else
#define CLASS_CREATE(_name) class_create(THIS_MODULE, _name)
#endif
/* ==================================== */


#define WR_VALUE _IOW('a','a',int32_t*) 	/* write 9 cells */
#define RD_VALUE _IOR('a','b',int32_t*)		/* read new central cell state */

//int32_t value=0;

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

/* 3x3 block stored row-wise: indices 0..8, center = 4 */
static int32_t cells[9]; 			
static int data_written=0;			


/*
** Function Prototypes
*/
static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long     etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/*
** File operation sturcture
*/
static struct file_operations fops =
{
        .owner          = THIS_MODULE,			
        .read           = etx_read,				
        .write          = etx_write,
        .open           = etx_open,				
        .unlocked_ioctl = etx_ioctl,			
        .release        = etx_release,			
};

/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
        pr_info("etx_device: opened\n");
        return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
        pr_info("etx_device: closed\n");
        return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        pr_info("Read Function\n");
        return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        pr_info("Write function\n");
        return len;
}

/*
** This function will be called when we write IOCTL on the Device file
*/
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	
	int i;
	int zivi_susjedi=0;
	const int centar=4;	
	int32_t nova_vrijednost;
	
	
         switch(cmd) {
                case WR_VALUE:
                        if( copy_from_user(cells ,(int32_t __user*) arg, sizeof(cells)) )
                        {
                                pr_err("etx_device:Data Write : Error!\n");
				return -EFAULT;
                        }
			data_written=1;
			
			//upisivanje celija
			pr_info("etx_device: Cells written: ");
			for(i=0; i<9; ++i)
				pr_info("%d", cells[i]);
			pr_cont("\n");
                        break;
						
						
                case RD_VALUE:
                        if(!data_written) //ako podatak nije upisan 
                        {
				//Zahtjev za citanje,ali nije upisano nista!= kao greska
                                pr_err("etx_device: Read attempted before write!\n");
				return -ENODATA;/* nema podataka */
                        }
			//prebojavanje zivih susjeda, centralna je calls[4]
			zivi_susjedi = 0;
			for (i=0; i<9; ++i)
			{
				if(i!=centar && cells[i]!= 0) 
					zivi_susjedi++;
			}			
			if(cells[centar]) 
			{
				//ako je ziva, njen novi status je 
				nova_vrijednost=(zivi_susjedi==2 || zivi_susjedi==3) ? 1 :0;
			}
			else
			{
				//ako je mrtva, njen novi status je 
				nova_vrijednost=(zivi_susjedi ==3)? 1 : 0;
			}
						
			if(copy_to_user((int32_t __user*) arg, &nova_vrijednost, sizeof(nova_vrijednost)))
			{
				pr_err("etx_device: Data Read: Error!");
				return -EFAULT;
			}
			//nova vrijednoost centralne celije 
			pr_info("etx_device: Central cell new state: %s .\n", (nova_vrijednost != 0) ? "alive" : "dead");
                        break;
						
						
                default:
                        pr_info("etx_device: Unknown ioctl command\n");//nepoznata komanda
                        break;
        }
        return 0;
}
 
/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
        /*Allocating Major number-dodjeljivanje glavnog br.*/
        if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
		//ne moze da dodjeli taj glavni br. major
                pr_err("etx_device: Cannot allocate major number\n");
                return -1;
        }
        pr_info("etx_device: Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
        /*Creating cdev structure*/
        cdev_init(&etx_cdev,&fops);
 
        /*Adding character device to the system*/
        if((cdev_add(&etx_cdev,dev,1)) < 0){
            pr_err("etx_device: Cannot add the device to the system\n");
            goto r_class;
        }
 
        /*Creating struct class*/
	dev_class=CLASS_CREATE("etx_class");
        if(IS_ERR(dev_class)){
            pr_err("etx_device: Cannot create the struct class\n");
            goto r_class;
        }
 
        /*Creating device*/
        if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
            pr_err("etx_device: Cannot create the Device 1\n");
            goto r_device;
        }
        pr_info("etx_device: Device Driver Insert...Done!!!\n");
        return 0;
 
r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev,1);
        return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        pr_info("etx_device: Device Driver Remove...Done!!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("andjela.savicic@student.etf.unibl.org");
MODULE_DESCRIPTION("Simple Linux device driver (IOCTL)- Conway Game of Life.");
MODULE_VERSION("1.5");
