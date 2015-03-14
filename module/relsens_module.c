#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
MODULE_LICENSE("Dual BSD/GPL");


#define MY_INFO(str,arg...) printk(KERN_ALERT str, ## arg);
#define NUM_CPU 8
#define THIS_DELAY 100

#define ODROIDXU3

struct timer_list my_timer;

struct my_timer_data {
	unsigned int index;
	struct timer_list * timer; 
} my_data; 


#ifdef ODROIDXU3 // the 
extern unsigned int volt_relsens[20];
extern unsigned int temp_relsens[20];
extern unsigned int freq_relsens[20];
extern unsigned int volt_relsens_avg[20];
extern unsigned int temp_relsens_avg[20];
DECLARE_PER_CPU(unsigned int , freq_relsens_tmp);
#endif

unsigned int freq_to_volt(unsigned int freq){
	unsigned int volt;

	volt = 1;   // TODO implement this 


	return volt;
}

//----------------------------------------
void read_volt_temp(long unsigned int arg){
	#ifdef DEBUG_TIME
	long unsigned int c_before, c_after, c_diff;
	#endif

	struct my_timer_data *data = (struct my_timer_data *)arg;

	int i ;
	int count = 0 ;


   	unsigned long j = jiffies;

	//MY_INFO("READING STUFF...");

	//debug
	#ifdef DEBUG_TIME	
	asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(0x00000001));   //program_pmcr
	asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));   //enable_all_counter
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (c_before));
	#endif

	// update frequency 
	for ( i = 0 ; i < NUM_CPU ; i++ ) {
		freq_relsens[i] = per_cpu(freq_relsens_tmp , i); 
								//note: freq_relsens_tmp is defined per cpu because we get it inside of cpufreq.c
								// but then it is better to have this data in a single vector freq_relsens
	}		

	// update voltage (converting from frequency)
	for ( i = 0 ; i < NUM_CPU ; i++ ) {
		volt_relsens[i] = freq_to_volt( freq_relsens[i] );
	}

	// update temperature
	for ( i = 0 ; i < NUM_CPU ; i++ ) {
		temp_relsens[i] = 0;   // TODO : find a way to get the real values
	}

	// update average voltage and temeprature
	for ( i = 0 ; i < NUM_CPU ; i ++ ) {
		volt_relsens_avg[i] = count*volt_relsens_avg[i] + volt_relsens[i];
		volt_relsens_avg[i] = volt_relsens_avg[i] / ( count + 1);
	
		temp_relsens_avg[i] = count*temp_relsens_avg[i] + temp_relsens[i];
		temp_relsens_avg[i] = temp_relsens_avg[i] / ( count + 1);

		count = count + 1;
	} 

	//debug
	#ifdef DEBUG_TIME
	asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(0x00000001));   //program_pmcr
	asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));   //enable_all_counter
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (c_after));
	c_diff = c_after - c_before ; 
	printk (KERN_ALERT "PIETRO DEBUG EXEC TIME TEMPERATURE MODULE : cycles_difference = %lu " , c_diff );
	#endif

	data->timer->expires = j + THIS_DELAY;
	data->index = data->index + 1 ;
	add_timer(data->timer)	;

}


static int my_init(void){

	unsigned int j = jiffies ; 

	MY_INFO("INITIALIZING RELSENS MODULE ...");

	my_data.index = 0 ; 
	my_data.timer = &my_timer ;

	my_timer.expires =  j + THIS_DELAY;
	my_timer.function = read_volt_temp ; 
	my_timer.data = (unsigned long)&my_data;

	init_timer(&my_timer);
	
	add_timer(&my_timer);

	MY_INFO("INITIALIZON COMPLETE");
	return 0;
}

static void my_exit(void)
{
	MY_INFO("EXITING RELSENS MODULE ...");

	del_timer(&my_timer);
}

module_init(my_init);
module_exit(my_exit);	

