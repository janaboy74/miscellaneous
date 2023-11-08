#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <memory>
#include <algorithm>

using namespace std;

//-------------------------------------------------------------
inline void interleave( float *ptr, float *ptr2, float *pend )
//-------------------------------------------------------------
{
    for( float *pa = ptr2 - 1, *pb = ptr2; pb < pend; ) {
        float *pr = pb;
        for( ; pa >= ptr && pb < pend && *pb <= *pa; pa--, pb++ );
        float *pl = pa;
        for( pa++, pb--, pl = pa; pr <= pb; )
            swap( *pl++, *pr++ );
        if( pa > ptr && *pa < *( pa - 1 ))
            pb = pa--;
        else
            for( pa = pb++; pb < pend && *pa <= *pb; pa++, pb++ );
    }
}

//-------------------------------------------------------------
void binary( float *ptr, size_t len )
//-------------------------------------------------------------
{
    vector<thread> threads;
    for( size_t step = 1; step < len; step <<= 1 ) {
        for( size_t pos = 0; pos + ( step << 1 ) <= len; pos += step << 1 ) {
            interleave( ptr + pos, ptr + pos + step, ptr + pos + ( step << 1 ));
        }
    }
    for( size_t step = 1, rest = len, rest2 = len; step < len; step <<= 1 ) {
        if( step & len ) {
            rest -= step;
            if( rest >= 0 && rest2 < len ) {
                interleave( ptr + rest, ptr + rest2, ptr + len );
            }
            rest2 = rest;
        }
    }
}

//-------------------------------------------------------------
void binary_threads( float *ptr, size_t len )
//-------------------------------------------------------------
{
    vector<thread> threads;
    size_t tcount = thread::hardware_concurrency();
    size_t delta = len / tcount;
    size_t count = tcount;
    size_t from = 0, to = delta;
    for( size_t i = 0; i <= count; ++i ) {
        if( to > len )
            to = len;
        threads.push_back( thread([ ptr, from, to ]() {
            for( size_t step = 1; step < to; step <<= 1 ) {
                for( size_t pos = from; pos + ( step << 1 ) <= to; pos += step << 1 ) {
                    interleave( ptr + pos, ptr + pos + step, ptr + pos + ( step << 1 ));
                }
            }
            size_t range = to - from;
            for( size_t step = 1, rest = range, rest2 = range; step < range; step <<= 1 ) {
                if( step & range ) {
                    rest -= step;
                    if( rest >= 0 && rest2 < range ) {
                        interleave( ptr + from + rest, ptr + from + rest2, ptr + to );
                    }
                    rest2 = rest;
                }
            }
        }));
        from += delta;
        to += delta;
    }
    for( auto &t : threads ) {
        t.join();
    };
#if 0
    for( size_t step = delta; step < len; step <<= 1 ) {
        threads.clear();
        threads.push_back( thread([ ptr, step, len ]() {
            for( size_t pos = 0; pos + ( step << 1 ) <= len; pos += step << 1 ) {
                interleave( ptr + pos, ptr + pos + step, ptr + pos + ( step << 1 ));
            }
        }));
        for( auto &t : threads ) {
            t.join();
        };
    }
#else
    for( size_t step = delta; step < len; step <<= 1 ) {
        for( size_t pos = 0; pos + step < len ; pos += step << 1 ) {
            size_t to = pos + ( step << 1 );
            if( to > len )
                to = len;
            interleave( ptr + pos, ptr + pos + step, ptr + to );
        }
    }
#endif
}

//-------------------------------------------------------
void quicksort( float *a, int lo, int hi )
//-------------------------------------------------------
{
    int l( lo ), r( hi );
    float m( a[( l + r ) / 2 ]);

    for(;;) {
      while( l < hi && a[ l ] < m ) l++;
      while( r > lo && m < a[ r ] ) r--;

      if( l <= r )
        swap( a[ l ], a[ r ] ), l++, r--;
      else
        break;
    }

    if( lo < r ) quicksort( a, lo, r );
    if( l < hi ) quicksort( a, l, hi );
}

//-------------------------------------------------------
void quicksort_threads( float *ptr, int lo, int hi )
//-------------------------------------------------------
{
    vector<thread> threads;
    size_t len = hi - lo + 1;
    size_t tcount = thread::hardware_concurrency();
    size_t delta = len / tcount;
    size_t count = tcount;
    count = len / delta;
    size_t from = lo, to = lo + delta;
    for( size_t i = 0; i < count; ++i ) {
        if( to > len )
            to = len;
        threads.push_back( thread([ ptr, from, to ]() {
            quicksort( ptr, from, to - 1);
        }));
        from += delta;
        to += delta;
    }

    for( auto &t : threads ) {
        t.join();
    };
    quicksort( ptr, lo, hi );
}

//-------------------------------------------------------
void quicksort_binary( float *ptr, int lo, int hi )
//-------------------------------------------------------
{
    vector<thread> threads;
    size_t len = hi - lo + 1;
    size_t tcount = thread::hardware_concurrency();
    size_t delta = len / tcount;
    size_t count = tcount;
    count = len / delta;
    size_t from = lo, to = lo + delta;
    for( size_t i = 0; i < count; ++i ) {
        if( to > len )
            to = len;
        threads.push_back( thread([ ptr, from, to ]() {
            quicksort( ptr, from, to - 1);
        }));
        from += delta;
        to += delta;
    }

    for( auto &t : threads ) {
        t.join();
    };
    for( size_t step = delta; step < len; step <<= 1 ) {
        for( size_t pos = 0; pos + step < len ; pos += step << 1 ) {
            size_t to = pos + ( step << 1 );
            if( to > len )
                to = len;
            interleave( ptr + pos, ptr + pos + step, ptr + to );
        }
    }
}

//-------------------------------------------------------
void stdsort_threads( float *start, float *end )
//-------------------------------------------------------
{
    vector<thread> threads;
    size_t len = end - start;
    size_t tcount = thread::hardware_concurrency();
    size_t delta = len / tcount;
    size_t count = tcount;
    count = len / delta;
    float *from = start, *to = start + delta;
    for( size_t i = 0; i < count; ++i ) {
        if( to > end )
            to = end;
        threads.push_back( thread([ from, to ]() {
            std::sort( from, to);
        }));
        from += delta;
        to += delta;
    }

    for( auto &t : threads ) {
        t.join();
    };

    std::sort( start, end);
}

//-------------------------------------------------------
void stdsort_binary( float *start, float *end )
//-------------------------------------------------------
{
    vector<thread> threads;
    size_t len = end - start;
    size_t tcount = thread::hardware_concurrency();
    size_t delta = len / tcount;
    size_t count = tcount;
    count = len / delta;
    float *from = start, *to = start + delta;
    for( size_t i = 0; i < count; ++i ) {
        if( to > end )
            to = end;
        threads.push_back( thread([ from, to ]() {
            std::sort( from, to);
        }));
        from += delta;
        to += delta;
    }

    for( auto &t : threads ) {
        t.join();
    };

    for( size_t step = delta; step < len; step <<= 1 ) {
        for( size_t pos = 0; pos + step < len ; pos += step << 1 ) {
            size_t to = pos + ( step << 1 );
            if( to > len )
                to = len;
            interleave( start + pos, start + pos + step, start + to );
        }
    }
}

//-------------------------------------------------------
class Timer
//-------------------------------------------------------
{
    chrono::time_point<chrono::steady_clock> start;
public:
    Timer()
    {
        set();
    }
    void set()
    {
        start = chrono::steady_clock::now();
    }
    float getFloat()
    {
        return chrono::duration<float>( chrono::steady_clock::now() - start ).count();
    }
};

//-------------------------------------------------------
class Rnd
//-------------------------------------------------------
{
    size_t random;
public:
    Rnd( size_t seed ) { Set( seed ); };
    void Set( size_t seed ) { random = seed; };
    size_t Get( size_t max )
    {
        random = random * 0x372ce9b9 + 0xb9e92c37;
#ifdef _WIN32
        if( max )
            return _rotl( random, 13 ) % max;
        return _rotl( random, 13 );
#else
        if( max )
            return __rotl( random, 13 ) % max;
        return __rotl( random, 13 );
#endif
    };
} rnd( 0 );

//-------------------------------------------------------
void fill_rnd( float *val, size_t initialvalue, size_t len )
//-------------------------------------------------------
{
    rnd.Set( initialvalue );
    for( size_t i = 0; i < len; ++i )
        val[ i ] = ( float )( rnd.Get( 10 * len ) * 1e-5f );
}

//-------------------------------------------------------
void fill_sorted( float *val, size_t len )
//-------------------------------------------------------
{
    for( size_t i = 0; i < len; ++i )
        val[ i ] = ( float ) i;
}

//-------------------------------------------------------
void fill_revese( float *val, size_t len )
//-------------------------------------------------------
{
    for( size_t i = 0; i < len; ++i )
        val[ i ] = ( float ) len - i;
}

//-------------------------------------------------------
int check( float *val, size_t len )
//-------------------------------------------------------
{
    size_t bug = 0;
    for( size_t i = 1; i < len; ++i )
        if( val[ i - 1 ] > val[ i ])
            ++bug;

    return bug;
}

//-------------------------------------------------------
void print( float *val, size_t len )
//-------------------------------------------------------
{
    for( size_t i = 0; i < len; ++i )
         cout << fixed << setprecision( 5 ) << val[ i ] << ( i % 5 ? ", ":"\n" );
}

//-------------------------------------------------------
int main(int argc, char* argv[])
//-------------------------------------------------------
{
    Timer tm;
    size_t len = 1000000;
    shared_ptr<float[]> val = shared_ptr<float[]>( new float[ len ]);

    cout << "SORTING TEST of " << len << " FLOAT NUMBERS (RESULT TIMES)\n";
    cout << "-----------------------------------------------------------------\n\n";

    cout << "---------------\n";
    cout << "  RANDOM DATA: \n";
    cout << "---------------\n";
    auto clk = clock();

    fill_rnd( val.get(), clk, len );
    tm.set();
    binary( val.get(), len );
    cout << "binary sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_rnd( val.get(), clk, len );
    tm.set();
    binary_threads( val.get(), len );
    cout << "binary sort threads: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_rnd( val.get(), clk, len );
    tm.set();
    quicksort( val.get(), 0, len - 1 );
    cout << "quick sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_rnd( val.get(), clk, len );
    tm.set();
    quicksort_threads( val.get(), 0, len - 1 );
    cout << "quick sort threads: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_rnd( val.get(), clk, len );
    tm.set();
    quicksort_binary( val.get(), 0, len - 1 );
    cout << "quick sort threads + binary: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_rnd( val.get(), clk, len );
    tm.set();
    sort( val.get(), val.get() + len );
    cout << "std sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_rnd( val.get(), clk, len );
    tm.set();
    stdsort_threads( val.get(), val.get() + len );
    cout << "std sort threads: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_rnd( val.get(), clk, len );
    tm.set();
    stdsort_binary( val.get(), val.get() + len );
    cout << "std sort threads + binary: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    cout << "---------------\n";
    cout << "  SORTED DATA: \n";
    cout << "---------------\n";

    fill_sorted( val.get(), len );
    tm.set();
    binary( val.get(), len );
    cout << "binary sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_sorted( val.get(), len );
    tm.set();
    binary_threads( val.get(), len );
    cout << "binary sort threads: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_sorted( val.get(), len );
    tm.set();
    quicksort( val.get(), 0, len - 1 );
    cout << "quick sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_sorted( val.get(), len );
    tm.set();
    quicksort_threads( val.get(), 0, len - 1 );
    cout << "quick sort threads: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_sorted( val.get(), len );
    tm.set();
    quicksort_binary( val.get(), 0, len - 1 );
    cout << "quick sort threads + binary: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_sorted( val.get(), len );
    tm.set();
    sort( val.get(), val.get() + len );
    cout << "std sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_sorted( val.get(), len );
    tm.set();
    stdsort_threads( val.get(), val.get() + len );
    cout << "std sort threads: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_sorted( val.get(), len );
    tm.set();
    stdsort_binary( val.get(), val.get() + len );
    cout << "std sort threads + binary: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    cout << "---------------\n";
    cout << " REVERSE DATA: \n";
    cout << "---------------\n";

    fill_revese( val.get(), len );
    tm.set();
    binary( val.get(), len );
    cout << "binary sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_revese( val.get(), len );
    tm.set();
    binary_threads( val.get(), len );
    cout << "binary sort threads: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_revese( val.get(), len );
    tm.set();
    quicksort( val.get(), 0, len - 1 );
    cout << "quick sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_revese( val.get(), len );
    tm.set();
    quicksort_threads( val.get(), 0, len - 1 );
    cout << "quick sort threads: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_revese( val.get(), len );
    tm.set();
    quicksort_binary( val.get(), 0, len - 1 );
    cout << "quick sort threads + binary: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_revese( val.get(), len );
    tm.set();
    sort( val.get(), val.get() + len );
    cout << "std sort: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_revese( val.get(), len );
    tm.set();
    stdsort_threads( val.get(), val.get() + len );
    cout << "std sort thread: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    fill_revese( val.get(), len );
    tm.set();
    stdsort_binary( val.get(), val.get() + len );
    cout << "std sort thread + binary: " << fixed << setprecision( 5 ) << tm.getFloat() << " ";
    cout << ( check( val.get(), len ) ? "BUG" : "OK " ) << "\n";

    cout << "-----------------------------------------------------------------\n";
    cout << "Test Finsihed.\n";

#if 0
    cout << "Press any key to exit.";
    getchar();
#endif
    return 0;
}
