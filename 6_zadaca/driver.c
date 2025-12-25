/***************************************************************************//**
*  \file       driver.c
*  \details    Driver za Conway (10x10) -> Game of Life
*  \author     andjelas
*
*  \Tested with Linux 6.12.47+rpt-rpi-v7
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/device/class.h>
#include <linux/ioctl.h>
#include <linux/types.h>


#define GOL_N     10
#define GOL_CELLS (GOL_N * GOL_N) 

typedef struct __attribute__((packed)) {
    __u32  n;                      /* mora biti 10 (dimenzija NxN) */
    __u8   cells[GOL_CELLS];       /* 0/1  (row-wise) */
} FRAME;

typedef struct __attribute__((packed)) {
    __u64 generations;             /* broj prelaza generacija*/
    __u64 total_live;              /* broj živih u posljednjem okviru */
    __u64 born;                    /* koliko ih je ozivjelo, rodjeno*/
    __u64 died;                    /* koliko ih je umrlo*/
} STATISTICS;


#define WR_VALUE _IOW('a','a', FRAME*)
#define RD_VALUE _IOR('a','b', STATISTICS*)


 
static dev_t devno;
static struct cdev etx_cdev;
static struct class *etx_class;
static DEFINE_MUTEX(gol_mx);

static __u8 prev[GOL_CELLS];  //prev- prethodna tabela (10x10) koju driver pamti
static bool have_prev;		//za poredjenje da li postoji prethodna tabela
static STATISTICS stats;

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
static void 	gol_reset_locked(void);


/*
** File operation sturcture
*/
static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = etx_open,
    .release        = etx_release,
    .read           = etx_read,
    .write          = etx_write,
    .unlocked_ioctl = etx_ioctl,
};

static void gol_reset_locked(void)
{
    memset(prev, 0, sizeof(prev));
    have_prev = false;
    memset(&stats, 0, sizeof(stats));
}


static int etx_open(struct inode *inode, struct file *file)    
{ 
	pr_info("etx_device: opened\n");
	return 0; 
}

static int etx_release(struct inode *inode, struct file *file) 
{
	pr_info("etx_device: closed\n");
	return 0; 
}


static ssize_t etx_read(struct file *f, char __user *b, size_t l, loff_t *o)
{ 
	pr_info("Read Function\n");
	return 0; 
}


static ssize_t etx_write(struct file *f, const char __user *b, size_t l, loff_t *o)
{   
	pr_info("Write function\n");
	return l; 
}


static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {

    case WR_VALUE: 
	{ 
        FRAME frm;

        if (copy_from_user(&frm, (void __user *)arg, sizeof(frm)))
        {
            pr_err("etx_device:Data Write : Error!\n");
			return -EFAULT;
        }

		/*provjera dimenzija*/
        if (frm.n != GOL_N)
		{
			pr_err("etx_device:Data Write : incorrect dimension!\n");
            return -EINVAL;
		}
       
        mutex_lock(&gol_mx);
 
        __u64 live_now = 0;		/*br. zivih u novom okviru*/
		
        for (int i = 0; i < GOL_CELLS; ++i)
		{
			 if (frm.cells[i]) 
					 live_now++;
		}

        if (!have_prev) 
		{
            
			/*Prvu primljenu - tabelu samo zapamti, bez poređenja */
			/*generations/born/died = 0 za prvi okvir */
            memcpy(prev, frm.cells, GOL_CELLS);
            have_prev = true;
            stats.total_live = live_now;
			
        } 
		else 
		{
           /* izračunaj rođene/umrle u trenutnoj iteraciji */
		   /*ovdje se poredi*/
            __u64 born_now = 0, died_now = 0;
            for (int i = 0; i < GOL_CELLS; ++i) 
			{
				__u8 wsa, now; 
				
                was = prev[i] ? 1 : 0;
                now = frm.cells[i] ? 1 : 0;

                born_now += (!was &&  now); 
                died_now += ( was && !now);
			}

  
            stats.born += born_now;
            stats.died += died_now;
            stats.generations += 1;
            stats.total_live  = live_now;

            memcpy(prev, frm.cells, GOL_CELLS);
        }

        mutex_unlock(&gol_mx);
        return 0;
    }

    case RD_VALUE:
	{
		if (!have_prev)
			return -ENODATA;  
			
		STATISTICS tmp;    
        mutex_lock(&gol_mx);
        tmp = stats;
        mutex_unlock(&gol_mx);

        
        if (copy_to_user((void __user *)arg, &tmp, sizeof(tmp)))
            return -EFAULT;
        return 0;
    }

    default:
        pr_info("etx_device: Unknown ioctl command\n");
        return -ENOTTY;
    }
}

static int __init etx_driver_init(void)
{
    int r;

    gol_reset_locked();

    r = alloc_chrdev_region(&devno, 0, 1, "etx_Dev");
    if (r) 
		return r;

    cdev_init(&etx_cdev, &fops);
    r = cdev_add(&etx_cdev, devno, 1);
    if (r) 
		goto err_chr;

    etx_class = CLASS_CREATE("etx_class");
    if (IS_ERR(etx_class)) 
	{ 
		r = PTR_ERR(etx_class); 
		goto err_cdev; 
	}

    if (IS_ERR(device_create(etx_class, NULL, devno, NULL, "etx_device")))
	{
        r = -ENOMEM; 
		goto err_class;
    }

    pr_info("etx_device: ready (major=%d)\n", MAJOR(devno));
    return 0;

err_class:
    class_destroy(etx_class);
err_cdev:
    cdev_del(&etx_cdev);
err_chr:
    unregister_chrdev_region(devno, 1);
    return r;
}

static void __exit etx_driver_exit(void)
{
    device_destroy(etx_class, devno);
    class_destroy(etx_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(devno, 1);
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("andjela.savicic@student.etf.unibl.org");
MODULE_DESCRIPTION("Conway 10x10 char driver + stats via IOCTL");