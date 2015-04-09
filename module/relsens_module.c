#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
MODULE_LICENSE("Dual BSD/GPL");

#define DEBUG_ON

#define MY_INFO(str,arg...) printk(KERN_ALERT str, ## arg);
#define NUM_CPU 8

#ifdef DEBUG_ON
#define THIS_DELAY 100 // default is 100
#else
#define THIS_DELAY 100
#endif


#define ODROIDXU3

struct timer_list my_timer;

struct my_timer_data {
	unsigned int index;
	struct timer_list * timer; 
} my_data; 


#ifdef ODROIDXU3 // the 
#define EXYNOS_TMU_COUNT        5 // this should be the same as in exynos_thermal.c
#define POWER_SENSOR_COUNT	4
extern unsigned int volt_relsens[20];
extern unsigned int temp_relsens[20];
extern unsigned int freq_relsens[20];
extern unsigned int volt_relsens_avg[20];
extern unsigned int temp_relsens_avg[20];
extern unsigned int temp_relsens_exynos[EXYNOS_TMU_COUNT];
DECLARE_PER_CPU(unsigned int , freq_relsens_tmp);
extern unsigned int voltage_sensors_odroid[POWER_SENSOR_COUNT];
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
	unsigned int temp_worst_case;

   	unsigned long j = jiffies;

	//MY_INFO("READING STUFF...");

	//debug
	#ifdef DEBUG_TIME	
	asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(0x00000001));   //program_pmcr
	asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));   //enable_all_counter
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (c_before));
	#endif
	// update frequency 
	freq_relsens[0] = per_cpu(freq_relsens_tmp , 0); 
	freq_relsens[1] = per_cpu(freq_relsens_tmp , 0); 
	freq_relsens[2] = per_cpu(freq_relsens_tmp , 0); 
	freq_relsens[3] = per_cpu(freq_relsens_tmp , 0); 
	freq_relsens[4] = per_cpu(freq_relsens_tmp , 4); 
	freq_relsens[5] = per_cpu(freq_relsens_tmp , 4); 
	freq_relsens[6] = per_cpu(freq_relsens_tmp , 4); 
	freq_relsens[7] = per_cpu(freq_relsens_tmp , 4); 
								//note: freq_relsens_tmp is defined per cpu because we get it inside of cpufreq.c
								// but then it is better to have this data in a single vector freq_relsens

	// update voltage (converting from frequency)
	volt_relsens[0] = voltage_sensors_odroid[1];
	volt_relsens[1] = voltage_sensors_odroid[1];
	volt_relsens[2] = voltage_sensors_odroid[1];
	volt_relsens[3] = voltage_sensors_odroid[1];
	volt_relsens[4] = voltage_sensors_odroid[0];
	volt_relsens[5] = voltage_sensors_odroid[0];
	volt_relsens[6] = voltage_sensors_odroid[0];
	volt_relsens[7] = voltage_sensors_odroid[0];

	// update temperature

	//----------temperture option #1: worst case---------------------
	// get the maximum temperature of all sensors and assume that is the temperature of all cores (worst case assumption)
	/*
	temp_worst_case = temp_relsens_exynos[0];
	for ( i = 1 ; i < EXYNOS_TMU_COUNT ; i++ ){
		if (temp_worst_case < temp_relsens_exynos[i]){
			temp_worst_case = temp_relsens_exynos[i];
		}
	}

	for ( i = 0 ; i < NUM_CPU ; i++ ) {
		temp_relsens[i] = temp_worst_case;   // TODO : find a way to get the real values
	}
	*/

	//---------temeprature option #2: profiled---------------	
	temp_relsens[0] = temp_relsens_exynos[2];
	temp_relsens[1] = temp_relsens_exynos[2];
	temp_relsens[2] = temp_relsens_exynos[2];
	temp_relsens[3] = temp_relsens_exynos[2];
	temp_relsens[4] = temp_relsens_exynos[0];
	temp_relsens[5] = temp_relsens_exynos[3];
	temp_relsens[6] = temp_relsens_exynos[2];
	temp_relsens[7] = temp_relsens_exynos[1];

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

	#ifdef DEBUG_ON
	printk (KERN_ALERT "test temperature %u, %u, %u, %u, %u, %u, %u, %u\n", temp_relsens[0], 
										temp_relsens[1], 
										temp_relsens[2], 
										temp_relsens[3], 
										temp_relsens[4], 
										temp_relsens[5], 
										temp_relsens[6], 
										temp_relsens[7]);

	printk (KERN_ALERT "test avg temperature %u, %u, %u, %u, %u, %u, %u, %u\n", temp_relsens_avg[0], 
										temp_relsens_avg[1], 
										temp_relsens_avg[2], 
										temp_relsens_avg[3], 
										temp_relsens_avg[4], 
										temp_relsens_avg[5], 
										temp_relsens_avg[6], 
										temp_relsens_avg[7]);

	printk (KERN_ALERT "test frequencies %u, %u, %u, %u, %u, %u, %u, %u\n", 	freq_relsens[0], 
											freq_relsens[1], 
											freq_relsens[2], 
											freq_relsens[3], 
											freq_relsens[4], 
											freq_relsens[5], 
											freq_relsens[6], 
											freq_relsens[7]);

	printk (KERN_ALERT "test voltages %u, %u, %u, %u, %u, %u, %u, %u\n",         	volt_relsens[0],
											volt_relsens[1],
											volt_relsens[2],
											volt_relsens[3],
											volt_relsens[4],
											volt_relsens[5],
											volt_relsens[6],
											volt_relsens[7]);

        printk (KERN_ALERT "test avg voltages %u, %u, %u, %u, %u, %u, %u, %u\n",            volt_relsens_avg[0],
                                                                                        volt_relsens_avg[1],
                                                                                        volt_relsens_avg[2],
                                                                                        volt_relsens_avg[3],
                                                                                        volt_relsens_avg[4],
                                                                                        volt_relsens_avg[5],
                                                                                        volt_relsens_avg[6],
                                                                                        volt_relsens_avg[7]);

	#endif //DEBUG_ON

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

