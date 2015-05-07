



#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>


#define SELECT_CPU	1
#define SET_V_LTC	9
#define GET_DELTA_LI	3
#define GET_V_LI	4
#define SET_V_LI	5
#define GET_t_LI	6
#define SET_t_LI	7
#define SET_V_STC	8


// ------------------- LTC VARIABLES AND SETTINGS ------------------------- //

static long unsigned int t_life = 1000 * 10;   // it's a "fake lifetime" , here it is equal to 10 times the long interval duration

static Rt = 0.8 ; 

struct core_d {
	float scale_par,
	float shape_par,
	float scale_par_life,
	float shape_par_life,
	float scale_par_life_safe,
	float shape_par_life_safe,
	float R,
	float Rp,
	float Rc,
	float Rc_prec,
	float Rc_life,
	float Rc_life_prec,
	float damage,
	float damage_life,
} core_data[4]; 

//parameters for scale and shape
static float offset_a = 3;
static float mult_a = 95;
static float tau_a = 0.01;
static float tauvolt_a = 3;
static float mult_b = 3;
static float tau_b = 0.01;
static float offset_b = 10;
static float multvolt_b = 7;

//parameters for calculating reliability
static float pdf_v_offset = 1.8502e-005;
static float pdf_v_mult = 1.4500e-005;
static float pdf_v_degrees = 8.77;
static float pdf_u_mean = 0.65;
static float pdf_u_sigma = 0.0087;

//double integral domain
static float u_max = 0.7;
static float u_min = 0.6;
static float v_max = 0.0040;
static float v_min = 0;
static float subdomain_step_u=0.0025;
static float subdomain_step_v=0.000010; //05
static float u = u_min ; 
static float v = v_min ; 

// normalized core area wrt the minimum 
static float A = 1;

// PI controller
static float Kp = 0.5 ; 
static float Ti = t_life / 2 ;    // in the matlab file it was half of the lifetime 
static float Ki = 1 / Ti ; 
static float err_int ; 









// ---------------- MAIN FUNCTION ---------------- //


void main(int argc, char ** argv){

	int fd;
	int i , num_active_cpus;
	long unsigned int delta_LI[4] = {0,0,0,0}, V_LI[4] = {0,0,0,0} , t_LI[4] = {0,0,0,0};
	long unsigned int new_V_LTC = 1203, init_t_LI = 0;
	int count = 0 , max_num_long_intervals = 1;
	


	num_active_cpus = atoi(argv[1]);

	printf ("LTC : num_active_cpus = %u\n", num_active_cpus);
	fd = open("dev/ltc" , O_RDWR);

	if( fd == -1) {
		printf("\nopen error...\n");
		exit(0);
	}


	
	//get delta_LI
	for (i = 0 ; i<num_active_cpus ; i++){
		ioctl(fd, SELECT_CPU, &i);
		ioctl(fd, GET_DELTA_LI, &delta_LI[i]);
	}
	printf("delta_LI = %lu %lu %lu %lu\n",delta_LI[0],delta_LI[1],delta_LI[2],delta_LI[3] );
	

	while(count < (max_num_long_intervals*num_active_cpus) ){
		sleep(1);			
		for (i = 0 ; i<num_active_cpus ; i++){
			
			//get time inside the long interval
			ioctl(fd, SELECT_CPU, &i);
			ioctl(fd, GET_t_LI, &t_LI[i]);



			//check wheter the LI is over
			if (t_LI[i] >= delta_LI[i]){	  //if it enters here, it means that the LI is over	
				


				printf("LTC %u : t_LI = %lu\n" ,i, t_LI[i]);

				ioctl(fd, SELECT_CPU, &i);
				ioctl(fd, GET_V_LI, &V_LI[i]);
		

				//LONG TERM CONTROLLER
				/*
				*
				*  you should insert here the various operations
				*
				*/


				//reset short term time and update quantities
				//new_V_LTC = 1192;
				ioctl(fd, SET_t_LI, &init_t_LI);
				ioctl(fd, SET_V_LTC, &new_V_LTC);
				ioctl(fd, SET_V_LI, &new_V_LTC);
				ioctl(fd, SET_V_STC, &new_V_LTC);

				printf("LONG TERM CONTROLLER %u : ACTIVATED !!!!!!!!\n" , i);
				count++;
			} //if (t_LI[i] >= delta_LI[i])
		

		} //for (i = 0 ; i<num_active_cpus ; i++)

	}//while(count < max_count)

	
	printf("LTC IS DEAAAAAAAAAADDDDDD!!!!!!!!!\n");
	close(fd);
	exit(0);



}







