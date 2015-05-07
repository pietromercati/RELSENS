

/* -------   MONITOR MODULE FOR ANDROID/LINUX   ----------------------------
 * 
 * Author: Andrea Bartolini
 *
 * Modified by : Pietro Mercati
 * email : pimercat@eng.ucsd.edu
 * 
 * If using this code for research purposes, include 
 * references to the following publications
 * 
 * 1) P.Mercati, A. Bartolini, F. Paterna, T. Rosing and L. Benini; A Linux-governor based 
 *    Dynamic Reliability Manager for android mobile devices. DATE 2014.
 * 2) P.Mercati, A. Bartolini, F. Paterna, L. Benini and T. Rosing; An On-line Reliability 
 *    Emulation Framework. EUC 2014
 * 
	This file is part of DRM.
        DRM is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        DRM is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with DRM.  If not, see <http://www.gnu.org/licenses/>.
*/





#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <linux/cpumask.h>

#include <linux/cdev.h>
#include <linux/device.h>  //for class_create

#define RELSENS_MAJOR 33
#define RELSENS_NAME "relsens_driver"

#define EXYNOS_TMU_COUNT 5 // this has to be the same as in exynos_thermal.c and core.c

#define SELECT_CPU 	1
#define GET_AVG_VOLT 	6
#define GET_AVG_TEMP	3
#define RESET_COUNTER	4
#define READ_COUNTER	5

#define NUM_CPU		8
extern int relsens_count;
extern unsigned int volt_relsens_avg[20];
extern unsigned int temp_relsens_avg[20];




static int selected_cpu = 0;

/* ---- Private Constants and Types -------------------------------------- */
static char relsensBanner[] __initdata = KERN_INFO "User Mode REL_SENS MODULE Driver: 1.00\n";

/* ---- Private Variables ------------------------------------------------ */
static  dev_t           relsensDrvDevNum = 0;   //P: actual device number
static  struct class   *relsensDrvClass = NULL; 
static  struct  cdev    relsensDrvCDev;  //P: kernel's internal structure that represent char devices

//------------------------------------------------------------------------------------------------------


long relsens_mod_init(void){
printk(KERN_ALERT "PIETRO ALERT MODULE relsens driver init CPU%d ", smp_processor_id());
	return 0;
}

long relsens_mod_exit(void){
	printk(KERN_ALERT "PIETRO ALERT MODULE relsens driver exit CPU%d ", smp_processor_id());
	return 0;
}

/*long relsens_int_ready(void* _ext_buf){
	int * ext_buf = _ext_buf;
	if (copy_to_user(ext_buf,&__get_cpu_var(monitor_stats_index), sizeof(int)))
		return -EFAULT;
	return 0;
}*/

static int relsens_open (struct inode *inode, struct file *file) {
	unsigned int i;
	printk(KERN_ALERT "relsens_driver_open\n");
	// Init the MONITOR kernel process on a per cpu bases!
	for_each_online_cpu(i) {
		work_on_cpu(i, (void*)relsens_mod_init, NULL);
	}
	return 0;
}

/* close function - called when the "file" /dev/monitor is closed in userspace  
*	execute the monitor_mod_exit for each cpu on-line in the systems
*/
static int relsens_release (struct inode *inode, struct file *file) {
	int i = 0;
	printk(KERN_ALERT "relsens_release\n");
	for_each_online_cpu(i){
		work_on_cpu(i, (void*)relsens_mod_exit, NULL);
	}
	return 0;
}


static long relsens_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg) {
	long retval = 0;

	switch ( cmd ) {
		/* Pietro : the value of seleceted_cpu should be passed from userspace and determined the location
			of all variables which are defined per cpu */
		case SELECT_CPU:
			if ( copy_from_user( &selected_cpu, (int *)arg, sizeof(selected_cpu) )) {
							
				return -EFAULT;
			}
			break;
		case GET_AVG_VOLT:

		printk (KERN_ALERT "test avg voltages %u, %u, %u, %u, %u, %u, %u, %u\n",        volt_relsens_avg[0],
                	                                                                        volt_relsens_avg[1],
                        	                                                                volt_relsens_avg[2],
                                	                                                        volt_relsens_avg[3],
                                        	                                                volt_relsens_avg[4],
                                                	                                        volt_relsens_avg[5],
                                                        	                                volt_relsens_avg[6],
                                                                	                        volt_relsens_avg[7]);

			if (copy_to_user((unsigned int *)arg, volt_relsens_avg, NUM_CPU*sizeof(unsigned int)));
                                return -EFAULT;
                        break;

		case GET_AVG_TEMP:
			if (copy_to_user((unsigned int *)arg, temp_relsens_avg, NUM_CPU*sizeof(unsigned int)));
                                return -EFAULT;
			break;		
		case RESET_COUNTER:
			relsens_count = 0;
                        break;
	
		default:
			printk(KERN_ALERT "DEBUG: relsens_ioctrl - You should not be here!!!!\n");
			retval = -EINVAL;
	}
	return retval;
}

// define which file operations are supported
struct file_operations relsens_fops = {
	.owner		=	THIS_MODULE,
	.llseek		=	NULL,
	.read		=	NULL,
	.write		=	NULL,
	.readdir	=	NULL,
	.poll		=	NULL,
	.unlocked_ioctl	=	relsens_ioctl,
	.mmap		=	NULL,
	.open		=	relsens_open,
	.flush		=	NULL,
	.release	=	relsens_release,
	.fsync		=	NULL,
	.fasync		=	NULL,
	.lock		=	NULL,
	//.readv		=	NULL,
	//.writev		=	NULL,
};


//module initialization
static int __init relsens_init_module (void) {
    int     rc = 0;

    printk( relsensBanner );

    if ( MAJOR( relsensDrvDevNum ) == 0 )
    {
        /* Allocate a major number dynamically */
        if (( rc = alloc_chrdev_region( &relsensDrvDevNum, 0, 1, RELSENS_NAME )) < 0 )
        {
            printk( KERN_WARNING "%s: alloc_chrdev_region failed; err: %d\n", __func__, rc );
            return rc;
        }
    }
    else
    {
        /* Use the statically assigned major number */
        if (( rc = register_chrdev_region( relsensDrvDevNum, 1, RELSENS_NAME )) < 0 )  //P: returns 0 if ok, negative if error
        {
           printk( KERN_WARNING "%s: register_chrdev failed; err: %d\n", __func__, rc );
           return rc;
        }
    }

    cdev_init( &relsensDrvCDev, &relsens_fops );
    relsensDrvCDev.owner = THIS_MODULE;

    if (( rc = cdev_add( &relsensDrvCDev, relsensDrvDevNum, 1 )) != 0 )
    {
        printk( KERN_WARNING "%s: cdev_add failed: %d\n", __func__, rc );
        goto out_unregister;
    }

    /* Now that we've added the device, create a class, so that udev will make the /dev entry */

    relsensDrvClass = class_create( THIS_MODULE, RELSENS_NAME );
    if ( IS_ERR( relsensDrvClass ))
    {
        printk( KERN_WARNING "%s: Unable to create class\n", __func__ );
        rc = -1;
        goto out_cdev_del;
    }

    device_create( relsensDrvClass, NULL, relsensDrvDevNum, NULL, RELSENS_NAME );

    goto done;

out_cdev_del:
    cdev_del( &relsensDrvCDev );

out_unregister:
    unregister_chrdev_region( relsensDrvDevNum, 1 );

done:
    return rc;
}

//module exit function
static void __exit relsens_cleanup_module (void) {
        printk(KERN_ALERT "cleaning up module\n");
	
	device_destroy( relsensDrvClass, relsensDrvDevNum );
	class_destroy( relsensDrvClass );
	cdev_del( &relsensDrvCDev );

	unregister_chrdev_region( relsensDrvDevNum, 1 );
}

module_init(relsens_init_module);
module_exit(relsens_cleanup_module);
MODULE_AUTHOR(" Pietro Mercati and Andrea Bartolini" );
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samples voltage adn frequency and updates average values");
