// Random number generator copied without changes from ncsimulation8july.cpp
// (function randomgeneration, lines 1695-1734, and the #define constants it
// uses from the top of that file). It is ran2 from Numerical Recipes in C:
// two combined linear congruential generators with a 32-entry shuffle table.
//
// The seed handling (what value is passed in on each call) is done by the
// caller, exactly as in ncsimulation8july.cpp. See drawRandom() in
// src/simulation.cpp.
#include "ran2.h"

#define IM1 2147483563
#define IM2 2147483399
#define AM (1.0/IM1)
#define IMM1 (IM1-1)
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791
#define NTAB 32
#define NDIV (1+IMM1/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

 float randomgeneration(long idum)
{
int j;
long k;
static long idum2=123456789;
static long iy=0;
static long iv[NTAB];
float temp;
if (idum <= 0)
{                                //Initialize.
if (-(idum) < 1)
idum=1;                          //Be sure to preventidum=0.
else idum = -(idum);
idum2=(idum);
for (j=NTAB+7;j>=0;j--)
{ //Load the shue table (after 8 warm-ups).
k=(idum)/IQ1;
idum=IA1*(idum-k*IQ1)-k*IR1;
if (idum < 0)
idum += IM1;
if (j < NTAB)
iv[j] = idum;
}
iy=iv[0];
}
k=(idum)/IQ1; //Start here when not initializing.
idum=IA1*(idum-k*IQ1)-k*IR1; //Computeidum=(IA1*idum) % IM1without overflows by Schrage's method.
if (idum < 0)
idum += IM1;
k=idum2/IQ2;
idum2=IA2*(idum2-k*IQ2)-k*IR2; //Computeidum2=(IA2*idum) % IM2likewise.
if (idum2 < 0) idum2 += IM2;
j=iy/NDIV; //Will be in the range0..NTAB-1.
iy=iv[j]-idum2; //Hereidumis shued, idumandidum2are combined to generate output.
iv[j] = idum;
if (iy < 1) iy += IMM1;
if ((temp=AM*iy) > RNMX) return RNMX; //Because users don't expect endpoint values.
else return temp;
}
