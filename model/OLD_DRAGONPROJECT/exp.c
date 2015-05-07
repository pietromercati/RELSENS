

#include "./math_pietro.h"




    Main Page
    Modules
    Data Types
    Files
     Search for  	

osprey/libm/mips/exp.c
Go to the documentation of this file.

00001 /*
00002  * Copyright 2003, 2004, 2005, 2006 PathScale, Inc.  All Rights Reserved.
00003  */
00004 
00005 
00006 /*
00007 
00008   Copyright (C) 2000, 2001 Silicon Graphics, Inc.  All Rights Reserved.
00009 
00010   This program is free software; you can redistribute it and/or modify it
00011   under the terms of version 2.1 of the GNU Lesser General Public License 
00012   as published by the Free Software Foundation.
00013 
00014   This program is distributed in the hope that it would be useful, but
00015   WITHOUT ANY WARRANTY; without even the implied warranty of
00016   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
00017 
00018   Further, this software is distributed without any warranty that it is
00019   free of the rightful claim of any third person regarding infringement 
00020   or the like.  Any license provided herein, whether implied or 
00021   otherwise, applies only to this software file.  Patent licenses, if
00022   any, provided herein do not apply to combinations of this program with 
00023   other software, or any other product whatsoever.  
00024 
00025   You should have received a copy of the GNU Lesser General Public 
00026   License along with this program; if not, write the Free Software 
00027   Foundation, Inc., 59 Temple Place - Suite 330, Boston MA 02111-1307, 
00028   USA.
00029 
00030   Contact information:  Silicon Graphics, Inc., 1600 Amphitheatre Pky,
00031   Mountain View, CA 94043, or:
00032 
00033   http://www.sgi.com
00034 
00035   For further information regarding this notice, see:
00036 
00037   http://oss.sgi.com/projects/GenInfo/NoticeExplan
00038 
00039 */
00040 
00041 /* ====================================================================
00042  * ====================================================================
00043  *
00044  * Module: exp.c
00045  * $Revision: 1.5 $
00046  * $Date: 04/12/21 14:58:21-08:00 $
00047  * $Author: bos@eng-25.internal.keyresearch.com $
00048  * $Source: /home/bos/bk/kpro64-pending/libm/mips/SCCS/s.exp.c $
00049  *
00050  * Revision history:
00051  *  09-Jun-93 - Original Version
00052  *
00053  * Description: source code for exp function
00054  *
00055  * ====================================================================
00056  * ====================================================================
00057  */
00058 
00059 static char *rcs_id = "$Source: /home/bos/bk/kpro64-pending/libm/mips/SCCS/s.exp.c $ $Revision: 1.5 $";
00060 
00061 #ifdef _CALL_MATHERR
00062 #include <stdio.h>
00063 #include <math.h>
00064 #include <errno.h>
00065 #endif
00066 
00067 #include "libm.h"
00068 
00069 /*  Algorithm adapted from
00070   "Table-driven Implementation of the Exponential Function in
00071   IEEE Floating Point Arithmetic", Peter Tang, ACM Transactions on
00072   Mathematical Software, Vol. 15, No. 2, June 1989
00073  */
00074 
00075 #if defined(mips) && !defined(__GNUC__)
00076 extern  double  exp(double);
00077 
00078 #pragma weak exp = __exp
00079 #endif
00080 
00081 #if defined(BUILD_OS_DARWIN) /* Mach-O doesn't support aliases */
00082 extern double __exp(double);
00083 #pragma weak exp
00084 double exp( double x ) {
00085   return __exp( x );
00086 }
00087 #elif defined(__GNUC__)
00088 extern  double  __exp(double);
00089 
00090 double    exp() __attribute__ ((weak, alias ("__exp")));
00091 
00092 #endif
00093 
00094 extern  const du  _exptabhi[];
00095 extern  const du  _exptablo[];
00096 
00097 static const  du  Qnan =
00098 {D(QNANHI, QNANLO)};
00099 
00100 static const  du  Inf =
00101 {D(0x7ff00000, 0x00000000)};
00102 
00103 static const du Ulimit =
00104 {D(0x40862e42, 0xfefa39ef)};
00105 
00106 static const du Llimit =
00107 {D(0xc0874910, 0xd52d3051)};
00108 
00109 static const du rln2by32 =
00110 {D(0x40471547, 0x652b82fe)};
00111 
00112 static const du ln2by32hi =
00113 {D(0x3f962e42, 0xfef00000)};
00114 
00115 static const du ln2by32lo =
00116 {D(0x3d8473de, 0x6af278ed)};
00117 
00118 static const du one =
00119 {D(0x3ff00000, 0x00000000)};
00120 
00121 /* coefficients for polynomial approximation of exp on +/- log(2)/64     */
00122 
00123 static const du P[] =
00124 {
00125 {D(0x3ff00000, 0x00000000)},
00126 {D(0x3ff00000, 0x00000000)},
00127 {D(0x3fe00000, 0x00000000)},
00128 {D(0x3fc55555, 0x55548f7c)},
00129 {D(0x3fa55555, 0x55545d4e)},
00130 {D(0x3f811115, 0xb7aa905e)},
00131 {D(0x3f56c172, 0x8d739765)},
00132 };
00133 
00134 
00135 /* ====================================================================
00136  *
00137  * FunctionName   exp
00138  *
00139  * Description    computes exponential of arg
00140  *
00141  * ====================================================================
00142  */
00143 
00144 double
00145 __exp( double x )
00146 {
00147 #ifdef _32BIT_MACHINE
00148 
00149 int ix, xpt;
00150 int l;
00151 
00152 #else
00153 
00154 long long ix, xpt;
00155 long long l;
00156 
00157 #endif
00158 
00159 double  y1, y2, y;
00160 double  p, q;
00161 int n, m, j;
00162 int m1, m2;
00163 double  nd;
00164 double  s, s_lead, s_trail;
00165 double  twopm;
00166 double  twopm1, twopm2;
00167 double  result;
00168 #ifdef _CALL_MATHERR
00169 struct exception  exstruct;
00170 #endif
00171 
00172   /* extract exponent of x for some quick screening */
00173 
00174 #ifdef _32BIT_MACHINE
00175 
00176   DBLHI2INT(x, ix); /* copy MSW of x to ix  */
00177 #else
00178   DBL2LL(x, ix);    /* copy x to ix */
00179 #endif
00180 
00181   xpt = (ix >> DMANTWIDTH);
00182   xpt &= 0x7ff;
00183 
00184   if ( xpt < 0x408 )
00185   {
00186     /*      |x| < 512.0 */
00187 
00188     if ( xpt >= 0x3c8 )
00189     {
00190       /* |x| >= 2^(-54) */
00191 
00192       /* reduce x to +/- log(2)/64     */
00193 
00194       nd = x*rln2by32.d;
00195       n = ROUND(nd);
00196       nd = n;
00197 
00198       y1 = x - nd*ln2by32hi.d;
00199       y2 = nd*ln2by32lo.d;
00200       y = y1 - y2;
00201 
00202       j = n & 0x1f;
00203       m = n >> 5;
00204 
00205       s_lead = _exptabhi[j].d;
00206       s_trail = _exptablo[j].d;
00207       s = s_lead + s_trail;
00208 
00209       l = m + DEXPBIAS;
00210       l <<= DMANTWIDTH;
00211 
00212 #ifdef _32BIT_MACHINE
00213 
00214       twopm = 0.0;
00215       INT2DBLHI(l, twopm);  /* copy MSW of l to twopm */
00216 #else
00217       LL2DBL(l, twopm); /* copy l to twopm  */
00218 #endif
00219       q = ((((P[6].d*y + P[5].d)*y + P[4].d)*y + P[3].d)*y +
00220         P[2].d)*(y*y);
00221 
00222       p = (q - y2) + y1;
00223 
00224       result = s_lead + (s_trail + s*p);
00225       result *= twopm;
00226       return ( result );
00227     }
00228 
00229     return ( one.d + x );
00230   }
00231 
00232   if ( x != x )
00233   {
00234     /* x is a NaN; return a quiet NaN */
00235 
00236 #ifdef _CALL_MATHERR
00237 
00238     exstruct.type = DOMAIN;
00239     exstruct.name = "exp";
00240     exstruct.arg1 = x;
00241     exstruct.retval = Qnan.d;
00242 
00243     if ( matherr( &exstruct ) == 0 )
00244     {
00245       fprintf(stderr, "domain error in exp\n");
00246       SETERRNO(EDOM);
00247     }
00248 
00249     return ( exstruct.retval );
00250 #else
00251     NAN_SETERRNO(EDOM);
00252 
00253     return ( Qnan.d );
00254 #endif
00255   }
00256 
00257   if ( x > Ulimit.d )
00258   {
00259 #ifdef _CALL_MATHERR
00260 
00261     exstruct.type = OVERFLOW;
00262     exstruct.name = "exp";
00263     exstruct.arg1 = x;
00264     exstruct.retval = Inf.d;
00265 
00266     if ( matherr( &exstruct ) == 0 )
00267     {
00268       fprintf(stderr, "overflow error in exp\n");
00269       SETERRNO(ERANGE);
00270     }
00271 
00272     return ( exstruct.retval );
00273 #else
00274     SETERRNO(ERANGE);
00275 
00276     return ( Inf.d );
00277 #endif
00278   }
00279 
00280   if ( x < Llimit.d )
00281   {
00282 #ifdef _CALL_MATHERR
00283 
00284     exstruct.type = UNDERFLOW;
00285     exstruct.name = "exp";
00286     exstruct.arg1 = x;
00287     exstruct.retval = 0.0;
00288 
00289     if ( matherr( &exstruct ) == 0 )
00290     {
00291       fprintf(stderr, "underflow error in exp\n");
00292       SETERRNO(ERANGE);
00293     }
00294 
00295     return ( exstruct.retval );
00296 #else
00297     SETERRNO(ERANGE);
00298 
00299     return ( 0.0 );
00300 #endif
00301   }
00302 
00303   nd = x*rln2by32.d;
00304   n = ROUND(nd);
00305   nd = n;
00306 
00307   /* reduce x to +/- log(2)/64     */
00308 
00309   y1 = x - nd*ln2by32hi.d;
00310   y2 = nd*ln2by32lo.d;
00311   y = y1 - y2;
00312 
00313   j = n & 0x1f;
00314   m = n >> 5;
00315 
00316   s_lead = _exptabhi[j].d;
00317   s_trail = _exptablo[j].d;
00318   s = s_lead + s_trail;
00319 
00320   q = ((((P[6].d*y + P[5].d)*y + P[4].d)*y + P[3].d)*y +
00321     P[2].d)*(y*y);
00322 
00323   p = (q - y2) + y1;
00324 
00325   result = s_lead + (s_trail + s*p);
00326 
00327   /* must be more careful here when forming 2^m*result;
00328      the exponent may underflow or overflow
00329   */
00330 
00331   m1 = (m >> 1);
00332   m2 = m - m1;
00333 
00334 #ifdef _32BIT_MACHINE
00335 
00336   twopm1 = 0.0;
00337   l = m1 + DEXPBIAS;
00338   l <<= DMANTWIDTH;
00339 
00340   INT2DBLHI(l, twopm1);
00341 
00342   twopm2 = 0.0;
00343   l = m2 + DEXPBIAS;
00344   l <<= DMANTWIDTH;
00345 
00346   INT2DBLHI(l, twopm2);
00347 #else
00348   l = m1 + DEXPBIAS;
00349   l <<= DMANTWIDTH;
00350 
00351   LL2DBL(l, twopm1);
00352 
00353   l = m2 + DEXPBIAS;
00354   l <<= DMANTWIDTH;
00355 
00356   LL2DBL(l, twopm2);
00357 #endif
00358   result *= twopm1;
00359   result *= twopm2;
00360 
00361   return ( result );
00362 }
00363 
00364 #ifdef NO_LONG_DOUBLE
00365 
00366 #if defined(BUILD_OS_DARWIN) /* Mach-O doesn't support aliases */
00367 extern  long double  __expl(long double);
00368 long double expl( long double x ) { 
00369   return ( (long double)__exp((double)x) );
00370 }
00371 #elif defined(__GNUC__)
00372 extern  long double  __expl(long double);
00373 
00374 long double    expl() __attribute__ ((weak, alias ("__expl")));
00375 
00376 #endif
00377 
00378 long double
00379 __expl( long double x )
00380 { 
00381   return ( (long double)__exp((double)x) );
00382 }
00383 
00384 #endif
00385 

Generated on Wed Apr 8 14:16:21 2009 for Open64 by  doxygen 1.5.6

