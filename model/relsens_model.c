         
// --------------- MONITOR INFRASTRUCTURE - USERSPACE DAEMON -----------------
/*
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sys/wait.h> //for wait()
#include <math.h>
#include "chi_square_density.h"

//#define DEBUG // enables printf for debugging
#define DEBUG_VT
#define NUM_CPU 	8 // number of cpu of the target platform
#define SLEEP_TIME 	1 //the reliability sensor updates the value of reliability with this time rate.

// Macros for ioctl
#define SELECT_CPU      1
#define GET_AVG_VOLT    6
#define GET_AVG_TEMP    3
#define RESET_COUNTER   4
#define READ_COUNTER    5

// ---------------- GLOBAL CONSTANTS -------------------------------------------------------------------- //

// NORMAL: use this for exp 10
float offset_a = 2.5;  //1.5
float mult_a = 150;
float tau_a = 0.007;
float tauvolt_a = 3;
float mult_b = 4;
float tau_b = 0.01;
float offset_b = 10;
float multvolt_b = 4; //7

/*
float offset_a=-1;   
float mult_a=100;    
float tau_a=0.01;
float tauvolt_a=2;
float mult_b=10;
float tau_b=0.002;
float offset_b=5; 
float multvolt_b=4;
*/

/*
// LARGE! use this for exp 11 !!
float offset_a=-8;   
float mult_a=100;    
float tau_a=0.007;
float tauvolt_a=1.5;
float mult_b=10;
float tau_b=0.002;
float offset_b=5; 
float multvolt_b=4;
*/


// ---------------- FUNCTIONS DECLARATION-------------------------------------------------------------------- //

float compute_scale_parameter(float T , float V , float offset_a , float mult_a , float tau_a , float tauvolt_a );
float compute_shape_parameter(float T , float V , float mult_b , float tau_b , float offset_b , float multvolt_b);
float compute_reliability(   	float A,
                		float u_max,
				float u_min,
				float v_max,
				float v_min,
				int u_num_step,
				int v_num_step,
				float subdomain_step_u,
				float subdomain_step_v,
				float subdomain_area,
				float pdf_u_mean,
				float pdf_u_sigma,
				float pdf_v_offset,
				float pdf_v_mult,
				float pdf_v_degrees,
				int LI_index,
				float delta_LI,
				float scale_p,
				float shape_p,
				float scale_p_life,
				float shape_p_life,
				float t_life,
				float R);
float g( float u , float v , float t_0 , float scale_p , float shape_p);
float pdf_u(float x , float mean , float sigma);
float pdf_v( float v , float offset , float mult , float degrees);
// ---------------- MAIN FUNCTION --------------------------------------------------------------------------- //

int main(int argc, char ** argv){

	int fd;	
	int cpu;	
	char *log;
	char buf[3];
	char file_name[60];
	FILE *fp;
	int i;
	int LI_index;	

	float volt_relsens_avg[NUM_CPU];
	float temp_relsens_avg[NUM_CPU];
	
	float volt_predicted_avg[NUM_CPU];
	float temp_predicted_avg[NUM_CPU];
	
	float scale_parameter[NUM_CPU];
	float shape_parameter[NUM_CPU];
	float scale_parameter_life[NUM_CPU];
	float shape_parameter_life[NUM_CPU];


	float core_reliability[NUM_CPU];

        //parameters for calculating reliability
        float pdf_v_offset = 1.8502e-005;
        float pdf_v_mult = 1.4500e-005;
        float pdf_v_degrees = 8.77;
        float pdf_u_mean = 0.65;
        float pdf_u_sigma = 0.0087;

        //double integral domain
        float u_max = 0.7;
        float u_min = 0.6;
        float v_max = 0.00040;
        float v_min = 0;
        float subdomain_step_u=0.0025;
        float subdomain_step_v=0.000010; //05
        int u_num_step;
        int v_num_step;
        float subdomain_area;
        float u ;
        float v ;

        // normalized core area wrt the minimum 
        float A = 1;

	// other variables
	float delta_LI_virt = 30 * 24 *60 * 60 ;   //36.5 for 3 years!  //73 for 5 years
	static long unsigned int t_life =  5*365*24*60*60 ;//1000 * 10;   // it's a "fake lifetime" , here it is equal to 10 times the long interval duration
	float gamma = 0.1;

	#ifdef DEBUG
	printf("\n\nStarting Serial Reading\n\n");
	#endif // DEBUG

	// open monitor driver
	fd = open("/dev/relsens_driver",O_RDWR);
	if( fd == -1) {
                printf("Relsens driver open error...\n");
                exit(0);
        }


	LI_index = 0;
		
	system("rm reliability_log.txt");

	// Main Loop ------------------------------------------------------------------------
	while(1){
	
		LI_index++;
	
		// sleep to reduce overhead	
		sleep(SLEEP_TIME);			
		
		//get average voltage
		ioctl(fd, GET_AVG_VOLT, volt_relsens_avg);

		//get average temperature
		ioctl(fd, GET_AVG_TEMP, temp_relsens_avg);

		//get relsens_counter?
		// TODO deicde whether this is useful

		//erase relsens_counter
		ioctl(fd, RESET_COUNTER, 0);


		#ifdef DEBUG_VT
		printf ("average volt = %f", volt_relsens_avg[0]);
		for (i = 1 ; i < NUM_CPU; i++ ){
			printf ("\t%f", volt_relsens_avg[i]);
		}
		printf("\n");
		printf ("average temp = %f", temp_relsens_avg[0]);
		for (i = 0 ; i < NUM_CPU; i++ ){
			printf ("\t%f", temp_relsens_avg[i]);
		}
		printf("\n");
		#endif // DEBUG_VT


		//update relsens_LT_counter (maybe find nother name)? // I don't even rememeber what thsi is......




		// compute scale and shape parameter for each core
		
		for ( i = 0 ; i < NUM_CPU ; i ++ ){

			//calculate V_bar and T_bar
			volt_predicted_avg[i] = gamma*volt_predicted_avg[i] + (1 - gamma)*volt_relsens_avg[i] ; 
			temp_predicted_avg[i] = gamma*temp_predicted_avg[i] + (1 - gamma)*temp_relsens_avg[i] ;


			scale_parameter[i] = 365*24*60*60* compute_scale_parameter(	temp_relsens_avg[i]	, 
											volt_relsens_avg[i]	, 
											offset_a		, 
											mult_a 			, 
											tau_a 			, 
											tauvolt_a 		);

			shape_parameter[i] = compute_shape_parameter( 	temp_relsens_avg[i] 	,  
									volt_relsens_avg[i] 	,  
									mult_b 			,  
									tau_b 			,  
									offset_b 		,  
									multvolt_b		);

			scale_parameter_life[i] = 365*24*60*60* compute_scale_parameter(	temp_relsens_avg[i] 	, 
												volt_relsens_avg[i] 	, 
												offset_a 		,  
												mult_a 			,  
												tau_a 			,  
												tauvolt_a );

			shape_parameter_life[i] = compute_shape_parameter(	temp_relsens_avg[i] 	, 
										volt_relsens_avg[i] 	, 
										mult_b 			, 
										tau_b 			, 
										offset_b 		, 
										multvolt_b);
	
		}// looping over all cores to compute scale and shape parameters
		

		for ( i = 0 ; i < NUM_CPU ; i ++ ){
			core_reliability[i] = compute_reliability(	A,
									u_max,
									u_min,
									v_max,
									v_min,
									u_num_step,
									v_num_step,
									subdomain_step_u,
									subdomain_step_v,
									subdomain_area,
									pdf_u_mean,
									pdf_u_sigma,
									pdf_v_offset,
									pdf_v_mult,
									pdf_v_degrees,
									LI_index,
									delta_LI_virt, //delta_LI[i],
									scale_parameter[i],
									shape_parameter[i],
									scale_parameter_life[i],
									shape_parameter_life[i],
									t_life,
									core_reliability[i]);
		}

		fp = fopen("reliability_log.txt","a");
		fprintf(fp, "%u", LI_index);
		for ( i = 0 ; i < NUM_CPU ; i ++ ){
			fprintf(fp,"\t%f",core_reliability[i]);
		}
		fprintf(fp,"\n");
		fclose(fp);


	} // end of Main loop
	return 0;
} 



//--------------- FUNCTIONS IMPLEMENTATION -------------------------------------------------------------------------

float compute_scale_parameter(float T , float V , float offset_a , float mult_a , float tau_a , float tauvolt_a ){
        float value;
        value = offset_a + mult_a * exp(-tau_a*T) * exp(-tauvolt_a*V)    ;
        return value;
}

float compute_shape_parameter(float T , float V , float mult_b , float tau_b , float offset_b , float multvolt_b){
        float value;
        value = mult_b * exp(-tau_b*T) + offset_b - (multvolt_b*V);
        return value;
}

float compute_reliability(	float A,
				float u_max,
				float u_min,
				float v_max,
				float v_min,
				int u_num_step,
				int v_num_step,
				float subdomain_step_u,
				float subdomain_step_v,
				float subdomain_area,
				float pdf_u_mean,
				float pdf_u_sigma,
				float pdf_v_offset,
				float pdf_v_mult,
				float pdf_v_degrees,
				int LI_index,
				float delta_LI,
				float scale_p,
				float shape_p,
				float scale_p_life,
				float shape_p_life,
				float t_life,
				float R){

	int i, j;
	float u , v , t_0 , g_value , exponential_value , pdf_u_value , pdf_v_value , 
		joint_pdf_value , integral_subdomain , g_value_prec , exponential_value_prec , 
		integral_subdomain_prec , damage;
	float Rc  , Rc_prec ;
	
	u = u_min;
	v = v_min;
	t_0 = LI_index * delta_LI ; 

	Rc = 0;
	Rc_prec = 0;

	for (i=0 ; i < u_num_step ; i++){
		for (j=0 ; j<v_num_step ; j++){


			g_value = g( 	u + 0.5*subdomain_step_u ,
					v + 0.5*subdomain_step_v ,
					t_0 , scale_p , shape_p);
			exponential_value = exp( -A*g_value);
			pdf_u_value = pdf_u( u + 0.5*subdomain_step_u , pdf_u_mean , pdf_u_sigma);
			pdf_v_value = pdf_v(  v + 0.5*subdomain_step_v ,  pdf_v_offset ,  pdf_v_mult ,  pdf_v_degrees);    
			joint_pdf_value = pdf_u_value*pdf_v_value ;
			integral_subdomain= exponential_value*joint_pdf_value*subdomain_area;                    
			Rc = Rc + integral_subdomain;
			
			
			if ( (i==20)&&(j==20) ){
				printf("\ninside calc_R : t_0 = %f g_value = %f exponential_value = %f pdf_u_value = %f pdf_v_value = %f Rc = %u\n",
					t_0, g_value , exponential_value ,pdf_u_value, pdf_v_value , Rc );
			}
			

			if (LI_index > 1){
				g_value_prec = g( 	u + 0.5*subdomain_step_u ,
							v + 0.5*subdomain_step_v ,
							t_0 - delta_LI , scale_p , shape_p);
				exponential_value_prec = exp( -A*g_value_prec);
				integral_subdomain_prec = exponential_value_prec*joint_pdf_value*subdomain_area;
				Rc_prec = Rc_prec + integral_subdomain_prec;
			}
			else{
				Rc_prec = 1 ;
			}




			v = v + subdomain_step_v;
		} //for (j=0 ; j<v_num_step ; j++)
	v = v_min;
	u = u + subdomain_step_u;
	} //for (i=0 ; i < u_num_step ; i++)

	damage = Rc_prec - Rc ; 	
	
	R = R - damage ; 

	printf("\ninside calc_R : R = %f Rc = %f Rc_prec = %f damage = %f \n", R, Rc, Rc_prec, damage );
	return R;
}

float g( float u , float v , float t_0 , float scale_p , float shape_p){
	float r , f , value;
	r = log ( t_0 / scale_p ) * shape_p * u ;
	f = log ( (t_0 / scale_p)*(t_0 / scale_p) ) * (shape_p * shape_p) * v * 0.5; 
	value = exp ( r + f );
	return value;
}

float pdf_u(float x , float mean , float sigma){
	float value , normalization ; 	
	normalization = (0.5 * M_SQRT1_2 * M_2_SQRTPI ) / sigma;
	value = normalization * exp ( - ( x - mean )*( x - mean ) / (2 * sigma * sigma )  );
	return value;
}
	 
float pdf_v( float v , float offset , float mult , float degrees){
	float multinv , value;
	multinv = 1/mult ; 
	value = multinv * (Chi_Square_Density (  multinv*(v - offset)  , degrees ) );
	return value;
}	 








