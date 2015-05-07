////////////////////////////////////////////////////////////////////////////////
// File: chi_square_density.c                                                 //
// Routine(s):                                                                //
//    Chi_Square_Density                                                      //
////////////////////////////////////////////////////////////////////////////////

#include <math.h>                    // required for log() and exp()
#include <float.h>                   // required for DBL_MAX and
#include "chi_square_density.h"
//                         Externally Defined Routines                        //

extern double Ln_Gamma_Function(double a);

////////////////////////////////////////////////////////////////////////////////
// double Chi_Square_Density( double x, int n )                               //
//                                                                            //
//  Description:                                                              //
//     The density of the Chi-Square distribution is                          //
//                               0                              if x < 0,     //
//       [1 / (2^(n/2) * Gamma(n/2))] * x^(n/2-1) * exp(-x/2)   if x >= 0,    //
//     where n >= 1 and Gamma() is the gamma function.                        //
//                                                                            //
//  Arguments:                                                                //
//     double x   Argument of the density.                                    //
//     int    n   The number of degrees of freedom.                           //
//                                                                            //
//  Return Values:                                                            //
//     If x < 0, then 0 is returned, if 0 <= x < inf then if n = 1 and x = 0, //
//     then DBL_MAX is returned or if n = 2 and x = 0 then 1/2 is returned,   //
//     otherwise 1/(2^(n/2) * Gamma(n/2)) * x^(n/2-1) * exp(-x/2) is          //
//     returned.                                                              //
//                                                                            //
//  Example:                                                                  //
//     double p, x;                                                           //
//     int    n;                                                              //
//                                                                            //
//     p = Chi_Square_Density(x, n);                                          //
////////////////////////////////////////////////////////////////////////////////

double Chi_Square_Density( double x, float n )
{
   double n2 = 0.5 * (double) n;
   double ln_density;

   if ( x < 0.0 ) return 0.0;
   if ( x == 0.0 ) {
      if ( n == 1 ) return DBL_MAX;
      if ( n == 2 ) return 0.5;
      return 0.0;
   }
   ln_density = (n2 - 1.0) * log(0.5 * x) - 0.5 * x - Ln_Gamma_Function(n2);
   return 0.5 * exp(ln_density);
}
