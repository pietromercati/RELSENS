



#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>
#include "chi_square_density.h"


#define SELECT_CPU	1
#define SET_V_LTC	9
#define GET_DELTA_LI	3
#define GET_V_LI	4
#define SET_V_LI	5
#define GET_t_LI	6
#define SET_t_LI	7
#define SET_V_STC	8
#define GET_JIFFIES	10
#define PASS_PID        11

#define LTC_ALG

#define NUM_CORES 4


// ------------------- LTC VARIABLES AND SETTINGS ------------------------- //


void handler (int signum);



static long unsigned int t_life =  5*365*24*60*60 ;//1000 * 10;   // it's a "fake lifetime" , here it is equal to 10 times the long interval duration

static float Rt = 0.8 ; 

static float error_integral = 0;

struct core_d {
	float scale_par;
	float shape_par;
	float scale_par_life;
	float shape_par_life;
	float scale_par_life_safe;
	float shape_par_life_safe;
	float R;
	float Rp;
	float Rp_safe;
	float Rc;
	float Rc_prec;
	float Rc_life;
	float Rc_life_prec;
	float damage;
	float damage_life;
	float V_LI;    //note: this is in [Volts]
} ; 

struct core_d core_data[NUM_CORES] ;



// functions definitions
float scale_par( float T , float V , float offset_a , float mult_a , float tau_a , float tauvolt_a  );
float shape_par( float T , float V , float mult_b , float tau_b , float offset_b , float multvolt_b);
float calc_R(	float A,
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


float calc_Rp(	float A,
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

float PI(float Kp , 
		float Ki, 
		float Rp , 
		float Rt , 
		float error_integral , 
		int LI_index , 
		float V , 
		float delta_LI);

float g( float u , float v , float t_0 , float scale_p , float shape_p);
float pdf_u(float x , float mean , float sigma);
float pdf_v( float v , float offset , float mult , float degrees);


// ---------------- MAIN FUNCTION ---------------- //


void main(int argc, char ** argv){
	
	FILE *file;
	unsigned int pid;
	FILE *fpid;
	//initialize
	

	//parameters for scale and shape

	float offset_a = 1.5;
	float mult_a = 150;
	float tau_a = 0.007;
	float tauvolt_a = 3;
	float mult_b = 4;
	float tau_b = 0.01;
	float offset_b = 10;
	float multvolt_b = 7;

	//parameters for calculating reliability
	float pdf_v_offset = 1.8502e-005;
	float pdf_v_mult = 1.4500e-005;
	float pdf_v_degrees = 8.77;
	float pdf_u_mean = 0.65;
	float pdf_u_sigma = 0.0087;

	//double integral domain
	float u_max = 0.7;
	float u_min = 0.6;
	float v_max = 0.0040;
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

	// PI controller
	float Kp = 0.5 ; 
	float Ti = 2.5*365*24*60*60  ;   // in the matlab file it was half of the lifetime t_life / 2 
	float Ki; // 1 / 5000 ;         // 1 / Ti  
	float err_int ; 

	// moving average
	float gamma = 0.1 ;
	float V_bar = 0;
	float T_bar = 0; 

	//min and max
	float V_MIN = 0.95;
	float V_MAX = 1.25;

	//other variables
	int fd;
	int i , num_active_cpus;
	long unsigned int 	delta_LI[NUM_CORES] = {0,0,0,0}, 
				V_LI[NUM_CORES] = {0,0,0,0} ,
				t_LI[NUM_CORES] = {0,0,0,0},
				T_LI[NUM_CORES] = {50,50,50,50},
				jiffies[NUM_CORES] = {0,0,0,0};
	float delta_LI_virt = 73 * 24 *60 * 60 ;   //36.5 for 3 years!
	long unsigned int new_V_LTC = 1203, init_t_LI = 0;
	int LI_index[NUM_CORES] = {1,1,1,1};
	int count = 0 , max_num_long_intervals = 25;
	
	//variables initialization

	subdomain_area = subdomain_step_u * subdomain_step_v;
	u_num_step = (int)(( u_max - u_min ) / subdomain_step_u);
	v_num_step = (int)(( v_max - v_min ) / subdomain_step_v);
	Ki = 1 / Ti;


	// ---------------------


	num_active_cpus = atoi(argv[1]);

	printf ("LTC : num_active_cpus = %u\n", num_active_cpus);
	fd = open("dev/ltc" , O_RDWR);

	//open file
	if((file = fopen("data/LTC.txt","w")) == NULL){
		printf("file not found...\n");
		exit(0);
	}	
	
	if( fd == -1) {
		printf("\nopen error...\n");
		exit(0);
	}


	//register application PID
	fpid = fopen("/data/pid_signal_prova.txt","w");
	pid = getpid();
	fprintf (fpid , "%u" , pid);	
	fclose(fpid);

	signal(SIGUSR1,handler);

	ioctl (fd , PASS_PID , &pid);

	//initialize data
	for (i = 0; i<num_active_cpus; i++){
		core_data[i].R = 1;
	}

	
	//get delta_LI
	for (i = 0 ; i<num_active_cpus ; i++){
		ioctl(fd, SELECT_CPU, &i);
		ioctl(fd, GET_DELTA_LI, &delta_LI[i]);
	}
	printf("delta_LI = %lu %lu %lu %lu\n",delta_LI[0],delta_LI[1],delta_LI[2],delta_LI[3] );
	

	while(count < (max_num_long_intervals*num_active_cpus) ){
		sleep(2);			
		for (i = 0 ; i<num_active_cpus ; i++){
			
			//get time inside the long interval
			ioctl(fd, SELECT_CPU, &i);
			ioctl(fd, GET_t_LI, &t_LI[i]);

		

			//check wheter the LI is over
			if (t_LI[i] >= delta_LI[i]){	  //if it enters here, it means that the LI is over	
				


				printf("LTC %u : t_LI = %lu\n" ,i, t_LI[i]);

				ioctl(fd, SELECT_CPU, &i);

				ioctl(fd, GET_JIFFIES, &jiffies[i]);

				ioctl(fd, GET_V_LI, &V_LI[i]);

				


				core_data[i].V_LI = V_LI[i] * 0.001 ; 

				//remove this part !!!!
				if (core_data[i].V_LI>V_MAX){
					core_data[i].V_LI=V_MAX;
				}

				printf("LTC %u : V_LI = %lu\n" ,i, V_LI[i]);
				//to be defined ioctl(fd, GET_T_LI, &T_LI[i]) for temperature


				


				//LONG TERM CONTROLLER -------------------------------

				//calculate V_bar and T_bar
				V_bar = gamma*V_bar + (1 - gamma)*V_LI[i] ; 
				T_bar = gamma*T_bar + (1 - gamma)*T_LI[i] ;

				//calculate the shape and scale parameter
				core_data[i].scale_par = 365*24*60*60*scale_par( T_LI[i] ,  core_data[i].V_LI ,  offset_a ,  mult_a ,  tau_a ,  tauvolt_a );
				core_data[i].shape_par = shape_par( T_LI[i] ,  core_data[i].V_LI ,  mult_b ,  tau_b ,  offset_b ,  multvolt_b); 
				core_data[i].scale_par_life = 365*24*60*60*scale_par(T_bar , V_bar , offset_a ,  mult_a ,  tau_a ,  tauvolt_a );
				core_data[i].shape_par_life = shape_par(T_bar , V_bar , mult_b , tau_b , offset_b , multvolt_b);
				core_data[i].scale_par_life_safe = 365*24*60*60*scale_par(T_bar , V_MIN , offset_a ,  mult_a ,  tau_a ,  tauvolt_a);
				core_data[i].shape_par_life_safe = shape_par(T_bar , V_MIN , mult_b , tau_b , offset_b , multvolt_b);
				

				// compute Rd and Rp
				core_data[i].R = calc_R(A,
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
							LI_index[i],
							delta_LI_virt, //delta_LI[i],
							core_data[i].scale_par,
							core_data[i].shape_par,
							core_data[i].scale_par_life,
							core_data[i].shape_par_life,
							t_life,
							core_data[i].R);


				core_data[i].Rp = calc_Rp(	A,
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
								LI_index[i],
								delta_LI_virt, //delta_LI[i],
								core_data[i].scale_par,
								core_data[i].shape_par,
								core_data[i].scale_par_life,
								core_data[i].shape_par_life,
								t_life,
								core_data[i].R);


				//core_data[i].Rp_safe = calc_Rp(); //same function, change inputs


				// PI control
				new_V_LTC = PI(	Kp , 
						Ki, 
						core_data[i].Rp , 
						Rt , 
						error_integral , 
						LI_index[i], 
						V_LI[i] , 
						delta_LI_virt);//delta_LI[i]);
				printf("LTC : new_V_LTC = %f\n", new_V_LTC);
				// ---------------------------------------------------

				//reset short term time and update quantities
				new_V_LTC = 1200;  //REMOVE THIS !!!!!
				ioctl(fd, SET_t_LI, &init_t_LI);
				ioctl(fd, SET_V_LTC, &new_V_LTC);
				ioctl(fd, SET_V_LI, &new_V_LTC);
				ioctl(fd, SET_V_STC, &new_V_LTC);

				printf("LONG TERM CONTROLLER %u : ACTIVATED !!!!!!!!\n" , i);


				fprintf( file , 
				"%u\t%u\t%lu\t%lu\t%lu\t%f\t%lu\t%f\t%f\t%f\t%f\t%f\n",	
						i,
						LI_index[i],
						jiffies[i],
						delta_LI[i],
						t_LI[i],
						core_data[i].V_LI,
						T_LI[i],
						core_data[i].R,
						core_data[i].Rp,
						core_data[i].Rp_safe,
						core_data[i].scale_par,
						core_data[i].shape_par);


				LI_index[i]++;

				count++;
			} //if (t_LI[i] >= delta_LI[i])
		

		} //for (i = 0 ; i<num_active_cpus ; i++)

	}//while(count < max_count)

	
	printf("LTC IS DEAAAAAAAAAADDDDDD!!!!!!!!!\n");
	fclose(file);
	close(fd);
	exit(0);



}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

// FUNCTIONS
void handler (int signum){
	printf("Signal recieved %d.....\n",signum);
}


float scale_par(float T , float V , float offset_a , float mult_a , float tau_a , float tauvolt_a ){
	float value;
	
	value = offset_a + mult_a * exp(-tau_a*T) * exp(-tauvolt_a*V)    ;
	
	return value;
}


float shape_par(float T , float V , float mult_b , float tau_b , float offset_b , float multvolt_b){
	float value;
	value = mult_b * exp(-tau_b*T) + offset_b - (multvolt_b*V);

	return value;
}



float calc_R(	float A,
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
	float Rc = 0 , Rc_prec = 0;
	
	u = u_min;
	v = v_min;
	t_0 = LI_index * delta_LI ; 


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
			
			
			if ( (i==20)&&(j==100) ){
				printf("\ninside calc_R : g_value = %f exponential_value = %f pdf_u_value = %f pdf_v_value = %f\n",
					g_value , exponential_value ,pdf_u_value, pdf_v_value );
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

	return R;
}

// ------------------------------------------------------

float calc_Rp(	float A,
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
	double u , v , t_0 , g_value_life , exponential_value_life , pdf_u_value , pdf_v_value , 
		joint_pdf_value , integral_subdomain_life , g_value_life_prec , exponential_value_life_prec , 
		integral_subdomain_life_prec , damage_life;
	float Rc_life = 0 , Rc_life_prec = 0 , Rp = 1;

	u = u_min;
	v = v_min;
	t_0 = LI_index * delta_LI ; 

	for (i=0 ; i < u_num_step ; i++){
		for (j=0 ; j<v_num_step ; j++){	

			g_value_life = g( 	u + 0.5*subdomain_step_u ,
						v + 0.5*subdomain_step_v ,
						t_life , scale_p_life , shape_p_life);
	
			exponential_value_life = exp(-A*g_value_life);
			pdf_u_value = pdf_u( u + 0.5*subdomain_step_u , pdf_u_mean , pdf_u_sigma);
			pdf_v_value = pdf_v(  v + 0.5*subdomain_step_v ,  pdf_v_offset ,  pdf_v_mult ,  pdf_v_degrees);    
			joint_pdf_value = pdf_u_value*pdf_v_value ;
			integral_subdomain_life = exponential_value_life * joint_pdf_value * subdomain_area;	
			Rc_life = Rc_life + integral_subdomain_life;


			if ( (i==20)&&(j==100) ){
				printf("\ninside calc_Rp : g_value = %f exponential_value = %f pdf_u_value = %f pdf_v_value = %f\n",
					g_value_life , exponential_value_life ,pdf_u_value, pdf_v_value );
			}
			


			g_value_life_prec = g( 	u + 0.5*subdomain_step_u ,
						v + 0.5*subdomain_step_v ,
						t_0 , scale_p_life , shape_p_life);

			exponential_value_life_prec= exp( -A*g_value_life_prec);
            		integral_subdomain_life_prec= exponential_value_life_prec*joint_pdf_value*subdomain_area;
            		Rc_life_prec=Rc_life_prec + integral_subdomain_life_prec;                    

			v = v + subdomain_step_v;
		} //for (j=0 ; j<v_num_step ; j++)
	v = v_min;
	u = u + subdomain_step_u;
	} //for (i=0 ; i < u_num_step ; i++)

    	damage_life=Rc_life_prec-Rc_life;
	Rp=R-damage_life;

	return Rp;
}

//-----------------------------------------------------


float PI(	float Kp , 
		float Ki, 
		float Rp , 
		float Rt , 
		float error_integral , 
		int LI_index , 
		float V , 
		float delta_LI ){

	float err , V_LTC;
	
	if (LI_index == 1){
		error_integral = error_integral + (err * delta_LI);
	}
	else{
		error_integral = 0;
	}
	V_LTC = V + Kp*(err + Ki*error_integral);

	return V_LTC;
}


//-----------------------------------------------------



float g( float u , float v , float t_0 , float scale_p , float shape_p){
	float r , f , value;
	
	r = log ( t_0 / scale_p ) * shape_p * u ;
	f = log ( (t_0 / scale_p)*(t_0 / scale_p) ) * (shape_p * shape_p) * v * 0.5; 
	
	value = exp ( r + f );

	return value;
}

//-----------------------------------------------------

float pdf_u(float x , float mean , float sigma){
	
	float value , normalization ; 	

	normalization = (0.5 * M_SQRT1_2 * M_2_SQRTPI ) / sigma;

	value = normalization * exp ( - ( x - mean )*( x - mean ) / (2 * sigma * sigma )  );


	return value;
}	 

//-----------------------------------------------------

float pdf_v( float v , float offset , float mult , float degrees){
	float multinv , value;
	multinv = 1/mult ; 

	value = multinv * (Chi_Square_Density (  multinv*(v - offset)  , degrees ) );



	return value;
}	 

//-----------------------------------------------------







