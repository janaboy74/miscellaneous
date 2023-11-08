#include <math.h>
#include <time.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#ifndef __linux__
#include <windows.h>
#endif // __linux__
#ifdef __GNUC__
#include <unistd.h>
#else
const int CLOCK_REALTIME = 0;
int clock_gettime(int, struct timespec *spec) {
    int64_t wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime      -=116444736000000000LL;
    spec->tv_sec  =wintime / 10000000LL;
    spec->tv_nsec =wintime % 10000000LL *100;
    return 0;
};
#endif

double time(timespec &start_time) {
		clock_gettime(CLOCK_REALTIME,&start_time);
		return start_time.tv_sec+start_time.tv_nsec*1e-9;
}

timespec times;
/* A következõ sörök akkor kellenek ha a C perdítõ nem eszi meg a kódot */
/* #define CLOCKS_PER_SEC 50 */
#define HZ (CLOCKS_PER_SEC)

long loop;
double dif;
double mul;

float calcmips(void)
{
 long rate;
 long i;
 register long c;
 double mips,rt;
 double t1,t2;
 mips=0; /* ez egy jó referencia */
 c=0;
 t1=0;
 t2=0;
 mul=5;
 rate=1;
 rt=1.0;
 while (mul>.01)
 {
 dif=0;
  while (dif<0.1)
  {
   rt*=(1.0+mul);
   rate=(long)rt;
   t1=time(times);
   for(i=0;i<rate;i++)
   {
     c+=0; /* Négy alapmûvelet lemérése */
     c-=0;
     c*=1;
     c/=1;
   }
   t2=time(times);
   dif=t2-t1;
  }
  rt/=(1.0+mul);
  mul/=2.0; /* idõkalibrálás */
 }
 rate*=100; /* kb 1-2 sec */
 t1=time(times);
 for(i=0;i<rate;i++)
 {
  c+=0; /* Mips meghatározása */
  c-=0;
  c*=1;
  c/=1;
 }
 t2=time(times);
 dif=t2-t1; /*  mennyi idõegység ? */
 if (dif!=0.0) mips=(5.0*rate)/(dif*1000000.0);
             /* mennyi mips ? */
 loop=rate;
 return(mips);
}


float calcmflops(void)
{
 long i,rate;
 register float c;
 double mflops,rt;
 double t1,t2;
 mflops=0;
 c=0;
 t1=0;
 t2=0;
 dif=0;

 mul=5;
 rate=1;
 rt=1.0;
 while (mul>.1)
 {
 dif=0;
  while (dif<0.1)
  {
   rt*=(1.0+mul);
   rate=(long)rt;
   t1=time(times);
   for(i=0;i<rate;i++)
   {
     c+=0.0; /* Ugynúgy mint CPU-nál */
     c-=0.0; /* Négy alapmûvelet lemérése */
     c*=1.0;
     c/=1.0;
   }
   t2=time(times);
   dif=t2-t1;
  }
  rt/=(1.0+mul);
  mul/=2.0; /* idõkalibrálás */
 }
 rate*=100; /*  kb 1-2 sec */
 t1=time(times);
 for(i=0;i<rate;i++)
 {
  c+=0.0; /* Flops meghatározása */
  c-=0.0;
  c*=1.0;
  c/=1.0;
 }
 t2=time(times);
 dif=t2-t1; /*  mennyi idõegység ? */
 if (dif!=0.0) mflops=(4.0*rate)/(dif*1000000.0);
          /* mennyi flops ? */
 loop=rate;
 return(mflops);
}

int main(int argc, char **argv)
{
 float mips,mipsdif,mflops,mflopsdif;
 long mipsloop,mflopsloop;
 float rel1,rel2;

 mips=calcmips();
 mipsloop=loop;
 mipsdif=dif;

 printf("%f mips (%ld loop,%f time)\n",mips,mipsloop,mipsdif);

 mflops=calcmflops();
 mflopsloop=loop;
 mflopsdif=dif;

 printf("%f mflops (%ld loop,%f time)\n",mflops,mflopsloop,mflopsdif);

 rel1=(mips/2.3);
 rel2=(mflops/1.56);

 printf("Relative: 040/25Mhz Amiga OS     |SAS/C |   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/35.2);
 rel2=(mflops/3.97);

 printf("Relative: 060/50Mhz AmigaOS MDEV4|GNU-C |   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/82.5);
 rel2=(mflops/15.7);

 printf("Relative: 060/50Mhz AmigaOS      |GNU-C |   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/71.02);
 rel2=(mflops/18.21);

 printf("Relative: AMDK5/100Mhz Linux OS  |GNU-C |   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/205.8);
 rel2=(mflops/12.9);

 printf("Relative: 603e/166Mhz Amiga OS   |S-PPC |   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/409.0);
 rel2=(mflops/52.8);

 printf("Relative: AMDK6-2/500Mhz Linux OS|GNU-C |   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/1072.3);
 rel2=(mflops/931.9);

 printf("Relative: ATHLON64 3000+ Windows |GCC422|   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/2748.8);
 rel2=(mflops/2154.5);

 printf("Relative: Ryzen R5 2400G Win/Lin |GCC8.1|   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/2561.6);
 rel2=(mflops/2030.7);

 printf("Relative: Ryzen R5 2500U Win/Lin |GCC8.2|   %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 rel1=(mips/2879.9);
 rel2=(mflops/2304.6);

 printf("Relative: Ryzen R7 2700      Lin |GCC11.4|  %7.2f [ir]   %7.2f [fr]\n",rel1,rel2);

 return 0;
}

