#include <windows.h>
#include <stdio.h>

//-------------------------------------------------------

#define LEN 500000

//-------------------------------------------------------------
inline void interleave( float *ptr, float *ptr2, float *pend )
//-------------------------------------------------------------
{
	for( float *pa = ptr2 - 1, *pb = ptr2; pb < pend; ) {
	    float *pr = pb;
		for( ; pa >= ptr && pb < pend && *pb <= *pa; pa--, pb++);
	    float *pl = pa;
		for( pa++, pb--, pl = pa; pr <= pb; ) {
			const float t = *pl;
			*pl++ = *pr;
			*pr++ = t;
		}
		if( pa > ptr && *pa < *( pa - 1 ))
			pb = pa--;
		else
			for( pa = pb++; pb < pend && *pa <= *pb; pa++, pb++ );
	}
}

//-------------------------------------------------------------
void binary( float *ptr, unsigned long len )
//-------------------------------------------------------------
{
	unsigned long step, rest, rest2;
	for( step = 1; step < len; step <<= 1 ) {
		for( unsigned long pos = 0; pos + ( step << 1 ) <= len; pos += step << 1 )
			interleave( ptr + pos, ptr + pos + step, ptr + pos + ( step << 1 ));
	}
	for( step = 1, rest = len, rest2 = len; step < len; step <<= 1 ) {
		if( step & len ) {
			rest -= step;
			if( rest2 < len )
				interleave( ptr + rest, ptr + rest2, ptr + len );
			rest2 = rest;
		}
	}
}

//-------------------------------------------------------
void qs(float *a,int lo, int hi)
//-------------------------------------------------------
{
	int	l(lo), r(hi);
	float tmp;
	float m(a[(l+r)/2]);

    for (;;)
	{
	  while (l < hi && a[l]<m ) l++;
	  while (r > lo && m<a[r] ) r--;

	  if (l <= r)
		tmp = a[l],a[l] = a[r],a[r] = tmp,l++, r--;
	  else
        break;
	}

	if (lo < r) qs(a,lo, r);
	if (l < hi) qs(a,l, hi);
}

//-------------------------------------------------------
class Timer
//-------------------------------------------------------
{
    LONGLONG frequency;
    long double ldfrequency;
    LONGLONG count;
    LONGLONG ldiff;
    DWORD lastdiff;
public:
    Timer()
    {
        QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
        ldfrequency=frequency;
        frequency/=1000;
        Set();
    }
    DWORD Set()
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&ldiff);
        ldiff-=count;
        count+=ldiff;
        lastdiff=ldiff/frequency;
        return lastdiff;
    }
    void Add(DWORD diff)
    {
        count+=diff;
    }
    DWORD Get()
    {
        LONGLONG diff;
        QueryPerformanceCounter((LARGE_INTEGER*)&diff);
        diff-=count;
        return diff/frequency;
    }
    operator float()
    {
        LONGLONG diff;
        QueryPerformanceCounter((LARGE_INTEGER*)&diff);
        diff-=count;
        return diff/ldfrequency;
	}
};

//-------------------------------------------------------
class Rnd
//-------------------------------------------------------
{
	unsigned int randNumber;
public:
	Rnd(int seed=GetTickCount()){Set(seed);};
	void Set(int seed){randNumber=seed;};
	int Get(unsigned int max)
	{
		randNumber *= 134775813; randNumber++;
		if (max)
			return ((randNumber<<19^randNumber>>13)  % max);
		return (randNumber<<19^randNumber>>13);
	};
} rnd;

//-------------------------------------------------------
void fill_rnd(float *val)
//-------------------------------------------------------
{
    int i;
    rnd.Set(12345678);
	for (i=0;i<LEN;i++)
		val[i]=(float)(rnd.Get(10*LEN));//
}

//-------------------------------------------------------
void fill_inc(float *val)
//-------------------------------------------------------
{
    int i;
	for (i=0;i<LEN;i++)
		val[i]=(float)i;//
}

//-------------------------------------------------------
int check(float *val)
//-------------------------------------------------------
{
    int i,bug;
	for (i=1,bug=0;i<LEN;i++)
		if (val[i-1]>val[i])
			bug++;//printf("#%d(%d): %f %f\n",,i,val[i-1],val[i]);

    return bug;
}

//-------------------------------------------------------
int main(int argc, char* argv[])
//-------------------------------------------------------
{
	Timer tm;
	float *val=new float[LEN];
/*	int i;

    float vals[]={1,5,6,2,3,4};
    for(i=0;i<6;i++)
        printf("%1.1f,",vals[i]);
    printf("\n");
    interleave(vals,vals+3,vals+6);
    for(i=0;i<6;i++)
        printf("%1.1f,",vals[i]);
    printf("\n");
*/
    printf("SORTING TEST of %d FLOAT NUMBERS (RESULT TIMES)\n",LEN);


	printf("-------------------------------------------------------\n");
    printf(" RANDOM DATA: ");
    fill_rnd(val);tm.Set();
	binary(val,LEN);
	printf("binary sort: %1.4f / ",(float)tm);
	printf(check(val)?"BUG":"OK");
    fill_rnd(val);tm.Set();
	qs(val,0,LEN-1);
	printf("quick sort: %1.4f\n",(float)tm);

    printf(" SORTED DATA: ");
    fill_inc(val);tm.Set();
	binary(val,LEN);
	printf("binary sort: %1.4f / ",(float)tm);
    fill_inc(val);tm.Set();
	qs(val,0,LEN-1);
	printf("quick sort: %1.4f\n",(float)tm);
	printf("-------------------------------------------------------\nTest Finsihed.\nPress any key to exit.");

	delete [] val;
	getchar();
	return 0;
}
