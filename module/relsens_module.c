#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
MODULE_LICENSE("Dual BSD/GPL");


#define MY_INFO(str,arg...) printk(KERN_ALERT str, ## arg);
#define NUM_CPU 4
#define THIS_DELAY 100

#define ODROIDXU3

struct timer_list my_timer;

struct my_timer_data {
	unsigned int index;
	struct timer_list * timer; 
} my_data; 


#ifdef ODROIDXU3 // the 
extern long unsigned int volt_relsens_module;
extern long unsigned int temp_relsens_module;
#endif


//----------------------------------------
void read_temperature(long unsigned int arg){
	//long unsigned int c_before, c_after, c_diff;

	struct my_timer_data *data = (struct my_timer_data *)arg;

   	unsigned long j = jiffies;

	//MY_INFO("READING TEMPERATURE...");

	//debug
	/*	
	asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(0x00000001));   //program_pmcr
	asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));   //enable_all_counter
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (c_before));
	*/

	// TODO : insert here the sampling and update of average temperature and voltages
	// update average voltage
	// update average temperature


	//debug
	/*
	asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(0x00000001));   //program_pmcr
	asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));   //enable_all_counter
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (c_after));
	c_diff = c_after - c_before ; 
	printk (KERN_ALERT "PIETRO DEBUG EXEC TIME TEMPERATURE MODULE : cycles_difference = %lu " , c_diff );
	*/	

	//printk(KERN_ALERT "PIETRO ALERT temperature module T_LI = %lu", T_LI);
	data->timer->expires = j + THIS_DELAY;
	data->index = data->index + 1 ;
	add_timer(data->timer)	;

}


static int my_init(void){

	unsigned int j = jiffies ; 

	MY_INFO("INITIALIZING READ TEMPERATURE MODULE ...");

	my_data.index = 0 ; 
	my_data.timer = &my_timer ;

	my_timer.expires =  j + THIS_DELAY;
	my_timer.function = read_temperature ; 
	my_timer.data = (unsigned long)&my_data;

	init_timer(&my_timer);
	
	add_timer(&my_timer);

	MY_INFO("INITIALIZON COMPLETE");
	return 0;
}

static void my_exit(void)
{
	MY_INFO("EXITING READ TEMPERATURE MODULE ...");

	del_timer(&my_timer);
}

module_init(my_init);
module_exit(my_exit);	

