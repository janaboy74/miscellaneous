#include <stdio.h>
#include <windows.h>

#undef FTYPE
typedef float FTYPE;

class dClock
{
	DWORD m_mscount;
public:
	inline dClock()
	{
		m_mscount=GetTickCount();
	}
	inline void Reset()
	{
		m_mscount=GetTickCount();
	}
	float GetAsFloat()
	{
		return (GetTickCount()-m_mscount)/1000.f;
	}
};

class TangentConverter
{
protected:
	FTYPE	m_y0,m_ta,m_tb,m_y1;
public:
	void SetHermite(FTYPE y0, FTYPE t0, FTYPE t1, FTYPE y1)
	{
		m_y0=y0;m_y1=y1;
		m_ta=t0*2;m_tb=t1*2;
	}
	void GetHermite(FTYPE &t0, FTYPE &t1)
	{
		t0=m_ta/2;t1=m_tb/2;
	}
	void SetXFormHermite(FTYPE y0, FTYPE t0, FTYPE t1, FTYPE y1)
	{
		m_y0=y0;m_y1=y1;
		m_ta=y1-t0;m_tb=y0-t1;
	}
	void GetXFormHermite(FTYPE &t0, FTYPE &t1)
	{
		t0=m_y1-m_ta;t1=m_y0-m_tb;
	}
	void SetSpline(FTYPE y0, FTYPE t0, FTYPE t1, FTYPE y1)
	{
		m_y0=y0;m_y1=y1;
		m_ta=(t0-y0)*6.f;m_tb=(t1-y1)*6.f;
	}
	void GetSpline(FTYPE &t0, FTYPE &t1)
	{
		t0=m_ta/6.f+m_y0;t1=m_tb/6.f+m_y1;
	}
};

// Hermite Original
// t(x)=c0*y0+c1*t0+c2*t1+c3*y1
// where
//   y0 - first value
//   t0 - first tangent
//   t1 - secound tangent
//   y1 - secound value
// and the blendind funcions are
// c0 = 2 x^3 - 3 x^2 + 1
// c1 = x^3 - x^2
// c2 = -2 x^3 + 3 x^2
// c3 = x^3 - 2 x^2 + x
inline float hermite( const float x, const float y0, const float t0, const float t1, const float y1)
{
	return ((((y1 - y0) * ( 3.0f - 2.0f * x ) - t0 - ( t0 - t1 ) * ( 1.0f - x ) ) * x + t0 )) * x  + y0;
}

// Hermite (x-form) Original
inline float hermitex1( const float x, const float y0, const float t0, const float t1, const float y1)
{
    const float c0 = y0;
    const float c1 = 0.5f * (y1 - t0);
    const float c2 = t0 - 2.5f * y0 + 2.f * y1 - 0.5f * t1;
    const float c3 = 1.5f * (y0 - y1) + 0.5f * (t1 - t0);

    return ((c3 * x + c2) * x + c1) * x + c0;
}

// Hermite (x-form) James Mccartney
inline float hermitex2( const float x, const float y0, const float t0, const float t1, const float y1)
{
    const float c0 = y0;
    const float c1 = 0.5f * (y1 - t0);
    const float c3 = 1.5f * (y0 - y1) + 0.5f * (t1 - t0);
    const float c2 = t0 - y0 + c1 - c3;

    return ((c3 * x + c2) * x + c1) * x + c0;
}

// Hermite (x-form) James Mccartney
inline float hermitex3( const float x, const float y0, const float t0, const float t1, const float y1)
{
	const float c0 = y0;
	const float c1 = 0.5f * (y1 - t0);
	const float t0my1 = t0 - y0;
	const float c3 = (y0 - y1) + 0.5f * (t1 - t0my1 - y1);
	const float c2 = t0my1 + c1 - c3;
	
	return ((c3 * x + c2) * x + c1) * x + c0;
}

// Hermite (x-form) Laurent De Soras
inline float hermitex4( const float x, const float y0, const float t0, const float t1, const float y1)
{
   const float    c     = (y1 - t0) * 0.5f;
   const float    v     = y0 - y1;
   const float    w     = c + v;
   const float    a     = w + v + (t1 - y0) * 0.5f;
   const float    b_neg = w + a;

   return ((((a * x) - b_neg) * x + c) * x + y0);
}

// Spline
inline float spline( const float x, const float y0, const float t0, const float t1, const float y1)
{
	const float nx(1.0f-x);
	return y0*nx*nx*nx+x*(t0*3.0f*nx*nx+x*(t1*3.0f*nx+x*y1));
}

int main()
{
	int k,l;
	float fx[4]={0,1/3.f,2/3.f,1};
	float fx3[3]={0,0.5,1};
	float fx2[3]={0,1};
	float t0,t1,y0,y1;
	float pos,scl=3;
	dClock clk;
	
	// full interpolate with hermite
	// calculate 2 tangent from 4 reference point

	// hermite
	t0=(fx[3]*2.f-fx[2]*9+fx[1]*18-fx[0]*11.f)/2.f;
	t1=(fx[0]*2.f-fx[1]*9+fx[2]*18-fx[3]*11.f)/2.f;
	printf("- hermite - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<4*scl-scl+1;k++)
	{
		pos=k/3.f/scl;
		printf("%1.3f : %1.3f\n",pos,hermite(pos,fx[0],t0,t1,fx[3]));
	}

	// x-form hermite
	t0=-fx[3]*1.f+fx[2]*9-fx[1]*18+fx[0]*11.0f;
	t1=-fx[0]*1.f+fx[1]*9-fx[2]*18+fx[3]*11.0f;
	printf("- x-form hermite - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<4*scl-scl+1;k++)
	{
		pos=k/3.f/scl;
		printf("%1.3f : %1.3f\n",pos,hermitex4(pos,fx[0],t0,t1,fx[3]));
	}

	// spline
	t0=(fx[3]*2.f-fx[2]*9+fx[1]*18-fx[0]*5.f)/6.f;
	t1=(fx[0]*2.f-fx[1]*9+fx[2]*18-fx[3]*5.f)/6.f;
	printf("- spline - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<4*scl-scl+1;k++)
	{
		pos=k/3.f/scl;
		printf("%1.3f : %1.3f\n",pos,spline(pos,fx[0],t0,t1,fx[3]));
	}

	// full interpolate with hermite
	// calculate 2 tangent from 3 reference point

	// hermite
	t0=-fx3[2]+fx3[1]*4-fx3[0]*3.f;
	t1=-fx3[0]+fx3[1]*4-fx3[2]*3.f;
	printf("- hermite - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<3*scl-scl+1;k++)
	{
		pos=k/2.f/scl;
		printf("%1.3f : %1.3f\n",pos,hermite(pos,fx3[0],t0,t1,fx3[2]));
	}

	// x-form hermite
	t0=fx3[2]*3.f-fx3[1]*8+fx3[0]*6.f;
	t1=fx3[0]*3.f-fx3[1]*8+fx3[2]*6.f;
	printf("- x-form hermite - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<3*scl-scl+1;k++)
	{
		pos=k/2.f/scl;
		printf("%1.3f : %1.3f\n",pos,hermitex4(pos,fx3[0],t0,t1,fx3[2]));
	}

	// spline
	t0=(-fx3[2]+fx3[1]*4)/3.f;
	t1=(-fx3[0]+fx3[1]*4)/3.f;
	printf("- spline - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<3*scl-scl+1;k++)
	{
		pos=k/2.f/scl;
		printf("%1.3f : %1.3f\n",pos,spline(pos,fx3[0],t0,t1,fx3[2]));
	}

	// full interpolate with hermite
	// calculate 2 tangent from 2 reference point

	// hermite
	t0=fx2[1]-fx2[0];
	t1=fx2[0]-fx2[1];
	printf("- hermite - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<2*scl-scl+1;k++)
	{
		pos=k/scl;
		printf("%1.3f : %1.3f\n",pos,hermite(pos,fx2[0],t0,t1,fx2[1]));
	}

	// x-form hermite
	t0=-fx2[1]+2*fx2[0];
	t1=-fx2[0]+2*fx2[1];
	printf("- x-form hermite - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<2*scl-scl+1;k++)
	{
		pos=k/scl;
		printf("%1.3f : %1.3f\n",pos,hermitex4(pos,fx2[0],t0,t1,fx2[1]));
	}

	// spline
	t0=(fx2[1]+2.0f*fx2[0])/3.f;
	t1=(fx2[0]+2.0f*fx2[1])/3.f;
	printf("- spline - t0 : %1.3f || t1 : %1.3f \n",t0,t1);
	for(k=0;k<2*scl-scl+1;k++)
	{
		pos=k/scl;
		printf("%1.3f : %1.3f\n",pos,spline(pos,fx2[0],t0,t1,fx2[1]));
	}
/*
	printf("speedtests\n");
	int limit=50000;
	float sum=0,subsum;
	pos=0.5;subsum=0;
	y0=222;t0=333;t1=-533;y1=111;
	sum+=subsum;clk.Reset();subsum=0;for(l=0;l<1000;l++)for(k=0;k<limit;k++)
		subsum+=hermite(1.f*k/limit,y0,t0,t1,y1);
	printf("hermite: %f\n",clk.GetAsFloat());
	sum+=subsum;clk.Reset();subsum=0;for(l=0;l<1000;l++)for(k=0;k<limit;k++)
		subsum+=hermitex1(1.f*k/limit,y0,t0,t1,y1);
	printf("hermitex1: %f\n",clk.GetAsFloat());
	sum+=subsum;clk.Reset();subsum=0;for(l=0;l<1000;l++)for(k=0;k<limit;k++)
		subsum+=hermitex2(1.f*k/limit,y0,t0,t1,y1);
	printf("hermitex2: %f\n",clk.GetAsFloat());
	sum+=subsum;clk.Reset();subsum=0;for(l=0;l<1000;l++)for(k=0;k<limit;k++)
		subsum+=hermitex3(1.f*k/limit,y0,t0,t1,y1);
	printf("hermitex3: %f\n",clk.GetAsFloat());
	sum+=subsum;clk.Reset();subsum=0;for(l=0;l<1000;l++)for(k=0;k<limit;k++)
		subsum+=hermitex4(1.f*k/limit,y0,t0,t1,y1);
	printf("hermitex4: %f\n",clk.GetAsFloat());
	sum+=subsum;clk.Reset();subsum=0;for(l=0;l<1000;l++)for(k=0;k<limit;k++)
		subsum+=spline(1.f*k/limit,y0,t0,t1,y1);
	printf("spline: %f\n",clk.GetAsFloat());
	sum+=subsum;
	
*/

	printf("-TangentConverter-\n");

	TangentConverter ctc;
	y0=1;
	y1=4;
	//hermite
	printf("-input:hermite-\n");
	t0=2;
	t1=3;
	ctc.SetHermite(y0,t0,t1,y1);
	for(k=0;k<2*scl-scl+1;k++)
	{
		pos=k/scl;
		ctc.GetHermite(t0,t1);
		printf("%1.3f : ",pos);
		printf("%1.3f ",hermite(pos,y0,t0,t1,y1));
		ctc.GetXFormHermite(t0,t1);
		printf("%1.3f ",hermitex4(pos,y0,t0,t1,y1));
		ctc.GetSpline(t0,t1);
		printf("%1.3f\n",spline(pos,y0,t0,t1,y1));
	}
	//x-form hermite
	printf("-input:x-form hermite-\n");
	t0=2;
	t1=3;
	ctc.SetXFormHermite(y0,t0,t1,y1);
	for(k=0;k<2*scl-scl+1;k++)
	{
		pos=k/scl;
		ctc.GetHermite(t0,t1);
		printf("%1.3f : ",pos);
		printf("%1.3f ",hermite(pos,y0,t0,t1,y1));
		ctc.GetXFormHermite(t0,t1);
		printf("%1.3f ",hermitex4(pos,y0,t0,t1,y1));
		ctc.GetSpline(t0,t1);
		printf("%1.3f\n",spline(pos,y0,t0,t1,y1));
	}
	//spline
	printf("-input:spline-\n");
	t0=2;
	t1=3;
	ctc.SetSpline(y0,t0,t1,y1);
	for(k=0;k<2*scl-scl+1;k++)
	{
		pos=k/scl;
		ctc.GetHermite(t0,t1);
		printf("%1.3f : ",pos);
		printf("%1.3f ",hermite(pos,y0,t0,t1,y1));
		ctc.GetXFormHermite(t0,t1);
		printf("%1.3f ",hermitex4(pos,y0,t0,t1,y1));
		ctc.GetSpline(t0,t1);
		printf("%1.3f\n",spline(pos,y0,t0,t1,y1));
	}

	printf("-Press a Key-\n");
	getchar();
//	printf("%1.1f",sum);
   return 0;
}
