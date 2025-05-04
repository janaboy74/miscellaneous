#include <cstdio>
#include <chrono>
#include <cstdint>

// optimalizálás nélkül fordítva működik csak és stdc++11 kell hozzá

double clk() {
    return std::chrono::duration_cast< std::chrono::nanoseconds >( std::chrono::system_clock::now().time_since_epoch() ).count() * 1e-9;
}

uint64_t loop;
double dif;
double mul;

float calcmips() {
    uint64_t rate;
    uint64_t i;
    uint64_t c;
    double mips, rt;
    double t1, t2;
    mips = 0; /* ez egy jó referencia */
    c = 1;
    mul = 5;
    rate = 1;
    rt = 1.0;
    while( mul >.01 ) {
        dif=0;
        while( dif< 0.1 ) {
            rt *= 1.0 + mul;
            rate = ( uint64_t ) rt;
            t1 = clk();
            for( i=0; i < rate; ++i ) {
                c += 0; /* Négy alapmûvelet lemérése */
                c -= 0;
                c *= 1;
                c /= 1;
            }
            t2 = clk();
            dif = t2 - t1;
        }
        rt /= 1.0 + mul;
        mul /= 2.0; /* idõkalibrálás */
    }
    rate *= 20; /* kb 1-2 sec */
    t1 = clk();
    for( i = 0; i < rate; ++i ) {
        c += 0; /* Mips meghatározása */
        c -= 0;
        c *= 1;
        c /= 1;
    }
    t2 = clk();
    dif = t2 - t1; /*  mennyi idõegység ? */
    if( dif!=0.0 )
        mips = ( 5.0 * rate ) / ( dif * 1000000.0 );
    /* mennyi mips ? */
    loop = rate;
    return mips;
}


float calcmflops() {
    uint64_t i, rate;
    float c;
    double mflops, rt;
    double t1, t2;
    mflops = 0;
    c = 1;
    dif = 0;

    mul = 5;
    rate = 1;
    rt = 1.0;
    while( mul >.1 ) {
        dif = 0;
        while( dif < 0.1 ) {
            rt *= 1.0 + mul;
            rate = ( uint64_t ) rt;
            t1 = clk();
            for( i = 0; i < rate; ++i ) {
                c += 0.0; /* Ugynúgy mint CPU-nál */
                c -= 0.0; /* Négy alapmûvelet lemérése */
                c *= 1.0;
                c /= 1.0;
            }
            t2 = clk();
            dif = t2 - t1;
        }
        rt /= 1.0 + mul;
        mul /= 2.0; /* idõkalibrálás */
    }
    rate *= 20; /*  kb 1-2 sec */
    t1 = clk();
    for( i = 0; i < rate; ++i ) {
        c += 0.0; /* Flops meghatározása */
        c -= 0.0;
        c *= 1.0;
        c /= 1.0;
    }
    t2 = clk();
    dif = t2 - t1; /*  mennyi idõegység ? */
    if( dif != 0.0 )
        mflops = ( 4.0 * rate ) / ( dif * 1000000.0 );
    /* mennyi flops ? */
    loop = rate;
    return mflops;
}

int main( int argc, char **argv ) {
    float mips, mipsdif, mflops, mflopsdif;
    long mipsloop, mflopsloop;
    float rel1, rel2;

    mips = calcmips();
    mipsloop = loop;
    mipsdif = dif;

    printf( "%f mips ( %ld loop,%f second )\n", mips, mipsloop, mipsdif );

    mflops = calcmflops();
    mflopsloop = loop;
    mflopsdif = dif;

    printf( "%f mflops ( %ld loop,%f second )\n", mflops, mflopsloop, mflopsdif );

    rel1 = mips / 2.3;
    rel2 = mflops / 1.56;

    printf( "Relative: 040/25Mhz Amiga OS     |SAS/C |   %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    rel1 = mips / 35.2;
    rel2 = mflops / 3.97;

    printf( "Relative: 060/50Mhz AmigaOS MDEV4|GNU-C |   %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    rel1 = mips / 82.5;
    rel2 = mflops / 15.7;

    printf( "Relative: 060/50Mhz AmigaOS      |GNU-C |   %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    rel1 = mips / 71.02;
    rel2 = mflops / 18.21;

    printf( "Relative: AMDK5/100Mhz Linux OS  |GNU-C |   %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    rel1 = mips / 205.8;
    rel2 = mflops / 12.9;

    printf( "Relative: 603e/166Mhz Amiga OS   |S-PPC |   %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    rel1 = mips / 409.0;
    rel2 = mflops / 52.8;

    printf( "Relative: AMDK6-2/500Mhz Linux OS|GNU-C |   %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    rel1 = mips / 2686.5;
    rel2 = mflops / 1373.2;

    printf( "Relative: Ryzen R5 2400G Linux OS|GCC13.3|  %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    rel1 = mips / 2534.35;
    rel2 = mflops / 1278.93;

    printf( "Relative: Ryzen R5 2500U Linux OS|GCC13.3|  %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    rel1 = mips / 2906.85;
    rel2 = mflops / 1480.19;

    printf( "Relative: Ryzen R7 2700  Linux OS|GCC13.3|  %7.2f [ir]   %7.2f [fr]\n", rel1, rel2 );

    return 0;
}

