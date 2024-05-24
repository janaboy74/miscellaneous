#include <cmath>
#include <string>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

typedef char CHR;
typedef CHR *STR;
typedef const char CCHR;
typedef CCHR *CSTR;
typedef CCHR * const CSTRC;

template <typename T>
std::string attribute(std::string const & attribute_name, T const & value, std::string const & unit = "")
{
    std::stringstream ss;
    ss << attribute_name << "=\"" << value << unit << "\" ";
    return ss.str();
}
std::string elemStart(std::string const & element_name)
{
    return "\t<" + element_name + " ";
}
std::string elemEnd(std::string const & element_name)
{
    return "</" + element_name + ">\n";
}
std::string emptyElemEnd()
{
    return "/>\n";
}
template <typename T>
std::string translate(T const & x, T const & y)
{
    std::stringstream ss;
    ss << "translate" << "(" << x << "," << y << ")";
    return ss.str();
}

template <typename T>
std::string point(T const & x, T const & y)
{
    std::stringstream ss;
    ss << x << "," << y << " ";
    return ss.str();
}


class svgen{
    std::ofstream ofs;
public:
    double w,h;
    svgen( CSTR name , double width, double height, bool enableFillBackground, bool enableViewBox ):ofs( name ) {
        w = width;
        h = height;
        *this << "<svg "
              << attribute("width", width, "")
              << attribute("height", height, "")
              << attribute("xmlns", "http://www.w3.org/2000/svg")
              << attribute("version", "1.1")
            ;
        if( enableViewBox )
            *this << "viewBox=\"" << -width / 2 << " " << -height / 2 << " " << width << " " << height << "\" ";
        *this << ">\n";
        if( enableFillBackground )
            *this << "<rect x=\"" << -width / 2 << "\" y=\"" << -height / 2 << "\" width=\"" << width << "\" height=\"" << height << "\" fill=\"black\"/>";
    }
    svgen& operator<<(const std::string &custom)
    {
        ofs << custom;
        return *this;
    }
    svgen& operator<<(const double val)
    {
        ofs << val;
        return *this;
    }
    ~svgen() {
        *this << "</svg>";
        ofs.close();
    }
    inline double snap( double v ) {
        return (int(v)+0.5);
    }
    double nx( double x ) {
        return snap(x+52);
    }
    double ny( double y ) {
        return snap(y);
    }
    double cx( double x ) {
        return snap(x+52);
    }
    double cy( double y ) {
        return snap(h-y-32);
    }
    double fx( double x ) {
        return snap(w-x);
    }
    double fy( double y ) {
        return snap(h-y);
    }
};

#include "float.h"

class limits {
public:
    double l, h;
    double s, e, m;
    int hri, lri;
    int d;
    limits(){init();}
    void init() {
        l = + DBL_MAX;
        h = - DBL_MAX;
    }
    void add( double val ) {
        if ( l > val ) {
            l = val;
        }
        if ( h < val ) {
            h = val;
        }
    }
    double check( double o ) {
        double ex = 1;
        for ( ; o >= 2 ; ) {
            o /= 10;
            ex *= 10;
        }
        for ( ; o < 2 ; ) {
            o *= 10;
            ex /= 10;
        }
        if ( o <= 5 ) {
            return ex*0.5;
        } else if ( o <= 10 ) {
            return ex*1;
        }
        return ex*2;
    }
    void setup() {
        double o = 0.2;
        for ( s = 1; int( l * s * o ) != int( h * s * o ); ++e ) {
            s /= 10;
        }
        for ( ; int( l * s * o ) == int( h * s * o ); ++e ) {
            s *= 10;
        }
        lri = l * s;
        hri = ( h * s ) + 1;
        d = hri - lri;
        if ( d <= 5 ) {
            m = 0.5;
        } else if ( d <= 10 ) {
            m = 1;
        } else {
            m = 2;
        }
        printf("%d(%f) %d(%f) %d +%f %f",lri,l,hri,h,d,m,s);
    }
};


#ifndef _WIN32
#define __cdecl
int _vscprintf(CSTR fmt, va_list args) {
    va_list cova;
    va_copy( cova, args );
    int len = vsnprintf(NULL, 0, fmt, cova);
    va_end( cova );
    return len;
}
#endif

#if !defined _MSC_VER && !defined _STDIO_S_DEFINED
int __cdecl _vsnprintf_s(char * _DstBuf, size_t _DstSizeInWords, size_t _MaxCount, const char * _Format, va_list _ArgList) {
    if (_DstBuf) {
        return vsnprintf(_DstBuf, _DstSizeInWords, _Format, _ArgList);
    }

    return 0;
}
#endif


class str {
protected:
    STR mStr;
    void init() {
        mStr = 0;
    }
    void drop( void *ptr ) {
        if ( ptr ) {
            free( ptr );
        }
    }
public:
    void alloc( size_t len ) {
        mStr = (STR) realloc( mStr, len );
    }
    void drop() {
        drop ( mStr );
        init();
    }
    ~str() {
        drop();
    }
    str( const str &src ) {
        init();
        set(src.mStr);
    }
    str( CSTR src ) {
        init();
        set(src);
    }
    str() {
        init();
    }
    CSTR set( CSTR src ) {
        drop();
        if ( !src ) {
            return mStr;
        }
        alloc( strlen( src ) + 1 ) ;
        strcpy( mStr, src );
        return mStr;
    }
    CSTR operator =(const str &src) {
        set( src.mStr );
        return mStr;
    }
    operator CSTR() const{
        return mStr;
    }
    STR get() const {
        return mStr;
    }
};

str format( CSTR format, ... ) {
    str out;
    if( format ) {
        va_list arg_list;
        va_start( arg_list, format );
        int len = _vscprintf( format, arg_list );
        out.alloc( len + 1 );
        if( out.get() ) {
            _vsnprintf_s( out.get(), len+1, len, format, arg_list );
        }
        va_end( arg_list );
    }
    return out;
}

inline float hermite( const float x, const float y0, const float t0, const float t1, const float y1 ) {
    return ((((y1 - y0) * ( 3.0f - 2.0f * x ) - t0 - ( t0 - t1 ) * ( 1.0f - x ) ) * x + t0 )) * x  + y0;
}

template <class T> struct PointClass {
    T                                   x;
    T                                   y;
    /* constructor */                   PointClass( T x = 0, T y = 0 );
    /* constructor */                   PointClass( const PointClass &other );
    PointClass &operator                = ( const PointClass &delta );
    PointClass operator                 - ( const PointClass &delta );
    PointClass operator                 + ( const PointClass &delta );
    PointClass operator                 * ( const float scale );
    PointClass operator                 -= ( const PointClass &delta );
    PointClass operator                 += ( const PointClass &delta );
    PointClass operator                 *= ( const float scale );
    PointClass                          split( const PointClass &next, float pos );
    PointClass                          rotate( float angle );
    void                                snap( int32_t scale );
    double                              dist() const;
};

///////////////////////////////////////
template <class T> PointClass<T>::PointClass( T x, T y ) {
    ///////////////////////////////////////
    this->x = x;
    this->y = y;
}

///////////////////////////////////////
template <class T> PointClass<T>::PointClass( const PointClass &other ) {
    ///////////////////////////////////////
    x = other.x;
    y = other.y;
}

///////////////////////////////////////
template <class T> PointClass<T> &PointClass<T>::operator = ( const PointClass &delta ) {
    ///////////////////////////////////////
    x = delta.x;
    y = delta.y;
    return *this;
}

///////////////////////////////////////
template <class T> PointClass<T> PointClass<T>::operator - ( const PointClass &delta ) {
    ///////////////////////////////////////
    PointClass mp;
    mp.x = x - delta.x;
    mp.y = y - delta.y;
    return mp;
}

///////////////////////////////////////
template <class T> PointClass<T> PointClass<T>::operator + ( const PointClass &delta ) {
    ///////////////////////////////////////
    PointClass mp;
    mp.x = x + delta.x;
    mp.y = y + delta.y;
    return mp;
}

template <class T> PointClass<T> PointClass<T>::operator *( const float scale )
{
    PointClass mp;
    mp.x = x * scale;
    mp.y = y * scale;
    return mp;
}

///////////////////////////////////////
template <class T> PointClass<T> PointClass<T>::operator -= ( const PointClass &delta )
///////////////////////////////////////
{
    x -= delta.x;
    y -= delta.y;
    return *this;
}

///////////////////////////////////////
template <class T> PointClass<T> PointClass<T>::operator += ( const PointClass &delta )
///////////////////////////////////////
{
    x += delta.x;
    y += delta.y;
    return *this;
}

template <class T> PointClass<T> PointClass<T>::operator *=(const float scale)
{
    x *= scale;
    y *= scale;
    return *this;
}

template<class T> PointClass<T> PointClass<T>::rotate(float angle)
{
    float ph = angle * M_PI / 180;
    PointClass out( x * cos( ph ) - y * sin( ph ), y * cos( ph ) + x * sin( ph ));
    return out;
}

template <class T> PointClass<T> PointClass<T>::split( const PointClass &next, float pos )
{
    PointClass delta( next.x - x, next.y - y );
    delta *= pos;
    delta += *this;
    return delta;
}

template <class T> void PointClass<T>::snap( int32_t scale )
{
    x += scale / 2;
    y += scale / 2;
    x /= scale;
    y /= scale;
    x *= scale;
    y *= scale;
}

///////////////////////////////////////
template <class T> double PointClass<T>::dist() const {
    ///////////////////////////////////////
    return sqrt( x * x + y * y );
}

typedef PointClass<int32_t> Point;
typedef PointClass<float> PointF;

void printusage( const char *exename ) {
    cerr << exename << " ( scorpion file coder )\n";
    cerr << "\n";
    cerr << "params:\n";
    cerr << "-o - output filename\n";
    cerr << "-f - enable fill background\n";
    cerr << "-v - set viewbox\n";
    cerr << "-i - inverted wheel\n";
    cerr << "-r - radius\n";
    cerr << "-d - depth\n";
    cerr << "-c - cogs\n";
    cerr << "-z - zoom\n";
    cerr << "-gt - graphical ratio of top\n";
    cerr << "-gb - graphical ratio of bottom\n";
    cerr << "-gc - graphical ratio of curve\n";
    cerr << "-st - step for top\n";
    cerr << "-sb - step for bottom\n";
    cerr << "-sc - step for curve\n";
    cerr << "-b - bend ratio ( float )\n";
    cerr << "-h - this help\n";
}

#define DEPTH 80
#define ZOOM 200
#define NRADIUS 300
#define NTSTEP 3
#define NBSTEP 2
#define NCSTEP 6
#define NCOGS 12
#define NBEND 0.4
#define NTOP 13
#define NBOTTOM 5
#define NCURVE 5
#define IRADIUS 500
#define ITSTEP 1
#define IBSTEP 3
#define ICSTEP 6
#define ICOGS 20
#define IBEND 0.2
#define ITOP 2
#define IBOTTOM 6
#define ICURVE 7

int main( int argc, const char **argv )
{
    int depth = DEPTH;
    int zoom = ZOOM;
    int radius = NRADIUS;
    int tstep = NTSTEP;
    int bstep = NBSTEP;
    int cstep = NCSTEP;
    int cogs = NCOGS;
    float bend = NBEND;
    int top = NTOP;
    int bottom = NBOTTOM;
    int curve = NCURVE;

    enum PHASE {
        P_TOP, P_DOWN, P_BOTTOM, P_UP
    } curPhase;

    bool enableFillBackground = false;
    bool enableViewBox = false;
    const char *exename = argv[ 0 ];
    string filename;
    for( int i = 1; i < argc; ++i ) {
        auto arglen = ::strlen( argv[ i ]);
        if( arglen < 1 )
            continue;
        int val;
        switch( argv[ i ][ 0 ] ) {
        case '-':
            if( argc > 1 + i ) {
                switch( argv[ i ][ 1 ] ) {
                case 'o':
                    if( argc > i ) {
                        filename = argv[ ++i ];
                    }
                    break;
                case 'f':
                    enableFillBackground = true;
                    break;
                case 'v':
                    enableViewBox = true;
                    break;
                case 'i':
                    radius = IRADIUS;
                    tstep = ITSTEP;
                    bstep = IBSTEP;
                    cstep = ICSTEP;
                    cogs = ICOGS;
                    bend = IBEND;
                    top = ITOP;
                    bottom = IBOTTOM;
                    curve = ICURVE;
                    break;
                case 'r':
                    if( argc > i ) {
                        radius = atol( argv[ ++i ]);
                    }
                    break;
                case 'd':
                    if( argc > i ) {
                        depth = atol( argv[ ++i ]);
                    }
                    break;
                case 'c':
                    if( argc > i ) {
                        val = atol( argv[ ++i ]);
                        if( val >= 1 )
                            cogs = val;
                    }
                    break;
                case 'z':
                    if( argc > i ) {
                        zoom = atol( argv[ ++i ]);
                    }
                    break;
                case 'g': {
                    int len( strlen( argv[ i ] ));
                    if( 3 == len ) {
                        if( !strcmp( argv[ i ], "-gt" ) ) {
                            val = atol( argv[ ++i ]);
                            if( val >= 1 )
                                top = val;
                        } else if( !strcmp( argv[ i ], "-gb" ) ) {
                            val = atol( argv[ ++i ]);
                            if( val >= 1 )
                                bottom = val;
                        } else if( !strcmp( argv[ i ], "-gc" ) ) {
                            val = atol( argv[ ++i ]);
                            if( val >= 1 )
                                curve = val;
                        }
                    }
                    break;
                }
                case 's': {
                    int len( strlen( argv[ i ] ));
                    if( 3 == len ) {
                        if( !strcmp( argv[ i ], "-st" ) ) {
                            val = atol( argv[ ++i ]);
                            if( val >= 1 )
                                tstep = val;
                        } else if( !strcmp( argv[ i ], "-sb" ) ) {
                            val = atol( argv[ ++i ]);
                            if( val >= 1 )
                                bstep = val;
                        } else if( !strcmp( argv[ i ], "-sc" ) ) {
                            val = atol( argv[ ++i ]);
                            if( val >= 1 )
                                cstep = val;
                        }
                    }
                    break;
                }
                case 'b':
                    if( argc > i ) {
                        bend = atof( argv[ ++i ]);
                    }
                    break;
                case 'h':
                    printusage( exename );
                    break;
                }
            }
            break;
        }
    }

    if( !filename.length() )
        filename = "gear.svg";
    svgen svg( filename.c_str(), 2 * radius * zoom, 2 * radius * zoom, enableFillBackground, enableViewBox );
    int ALL = top + 2 * curve + bottom;
    //float RES = STEP * COGS;
    float step = 1.f / cogs;
    float topPh = step * top;
    float curvePh = step * curve;
    float bottomPh = step * bottom;
    float cogPh = step * ALL;
    float curveAngle = 360 * curvePh / ALL;
    float g1 = topPh;
    float g2 = g1 + curvePh;
    float g3 = g2 + bottomPh;
    float g4 = g3 + curvePh;
    int d1 = tstep - 1;
    int d2 = d1 + cstep - 1;
    int d3 = d2 + bstep - 1;
    int d4 = d3 + cstep - 1;

    svg << "<g id=\"gear\">";
    svg << elemStart("polygon points=\"");

    PointF a( 0, DEPTH );
    PointF b( 0, 0 );
    PointF ao = a;
    ao += PointF( 0, -radius * bend );
    ao = ao.rotate( -2 * curveAngle );
    ao -= PointF( 0, 2 * radius * bend );
    ao = ao.rotate( curveAngle );
    ao += PointF( 0, -radius * bend );
    PointF ai = a;
    ai += PointF( 0, -radius * bend );
    ai = ai.rotate( 2 * curveAngle );
    ai -= PointF( 0, 2 * radius * bend );
    ai = ai.rotate( -curveAngle );
    ai += PointF( 0, -radius * bend );

    bool first = true;
    for( int32_t c = 0 ; c < cogs; ++c ) {
        float ph = 1.f * c / cogs;
        float dc = d1;
        float ga = g1;
        float gp = 0;
        for( int32_t i = 0 ; i < dc; ++i ) {
            float subph = 1.f * i / dc;
            PointF s( 0, depth );
            s.y -= radius;
            float angle = 360 * ( ph + 1.f * gp / g4 / cogs + subph * ga / g4 / cogs );
            s = s.rotate( angle );
            svg << point( zoom * s.x, zoom * s.y );
        }
        dc = d2 - d1;
        ga = g2 - g1;
        gp = g1;
        for( int32_t i = 0 ; i < dc; ++i ) {
            float subph = 1.f * i / dc;
            float fogph = 1 - subph;
            PointF a ( ai );
            PointF b ( 0, 0 );
            a -= PointF( 0, -radius * bend );
            a = a.rotate( curveAngle * fogph );
            a += PointF( 0, 2 * radius * bend );
            a = a.rotate( -2 * curveAngle * fogph );
            a -= PointF( 0, -radius * bend );
            PointF s = a.split( b, 1 - fogph );
            s.y -= radius;
            float angle = 360 * ( ph + 1.f * gp / g4 / cogs + subph * ga / g4 / cogs );
            s = s.rotate( angle );
            svg << point( zoom * s.x, zoom * s.y );
        }
        dc = d3 - d2;
        ga = g3 - g2;
        gp = g2;
        for( int32_t i = 0 ; i < dc; ++i ) {
            float subph = 1.f * i / dc;
            PointF s( 0, 0 );
            s.y -= radius;
            float angle = 360 * ( ph + 1.f * gp / g4 / cogs + subph * ga / g4 / cogs );
            s = s.rotate( angle );
            svg << point( zoom * s.x, zoom * s.y );
        }
        dc = d4 - d3;
        ga = g4 - g3;
        gp = g3;
        for( int32_t i = 0 ; i < dc; ++i ) {
            float subph = 1.f * i / dc;
            float fogph = subph;
            PointF a ( ao );
            PointF b ( 0, 0 );
            a -= PointF( 0, -radius * bend );
            a = a.rotate( -curveAngle * fogph );
            a += PointF( 0, 2 * radius * bend );
            a = a.rotate( 2 * curveAngle * fogph );
            a -= PointF( 0, -radius * bend );
            PointF s = a.split( b, 1 - fogph );
            s.y -= radius;
            float angle = 360 * ( ph + 1.f * gp / g4 / cogs + subph * ga / g4 / cogs );
            s = s.rotate( angle );
            svg << point( zoom * s.x, zoom * s.y );
        }
    }
    svg << "\" fill=\"white\" />";
    svg << "</g>";
    return 0;
}
