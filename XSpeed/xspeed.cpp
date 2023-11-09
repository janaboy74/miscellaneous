#include <tchar.h>
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <thread>
#include <functional>
#include <unistd.h>
#include <cmath>
#include <codecvt>
#include <locale>
#include <iostream>
#include "sqlite3.h"

using namespace std;
using namespace std::chrono;
using namespace Gdiplus;

#define SAMPLERATE 48000
#define NUMCHANNELS 2
#define BITSPERSAMPLE 16
#define BYTESPERSAMPLE ( BITSPERSAMPLE / 8 )
#define NUMBUFFERS 3
#define BUFFERLENGTH ( SAMPLERATE / 32 * NUMCHANNELS )
HBRUSH hbrWhite, hbrGray, hbrBlack;
RECT rc;
PAINTSTRUCT ps;
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
TCHAR szClassName[ ] = _T("BikerApp");

class corestring : public std::string
{
public:
    corestring() : std::string() {}
    corestring( const std::string &src ) : std::string( src ) {}
    corestring( const char *src ) : std::string( src ) {}
    void formatva( const char *format, va_list &arg_list ) {
        if( format ) {
            va_list cova;
            va_copy( cova, arg_list );
#ifdef _WIN32
            int size = _vscprintf( format, cova );
#endif
#ifdef __linux
            int size = vsnprintf( NULL, 0, format, cova );
#endif
            va_end( arg_list );
            resize( size );
            va_copy( cova, arg_list );
            vsnprintf( &at( 0 ), size + 1, format, cova );
            va_end( arg_list );
        }
    }
    void format( const char *format, ... ) {
        if( format ) {
            va_list arg_list;
            va_start( arg_list, format );
            formatva( format, arg_list );
            va_end( arg_list );
        }
    }
    std::string tolower( std::string str ) {
        std::string result;
        for( auto chr : str ) {
            result.push_back( std::tolower( chr ));
        }
        return result;
    }
    void set( const char *src, int len ) {
        resize( len );
        strncpy( &at( 0 ), src, len );
    }
    long toLong() {
        if( length() )
            return std::stol( c_str() );
        return 0L;
    }
    float toFloat() {
        if( length() )
            return std::stof( c_str() );
        return 0.f;
    }
};

struct Timer
{
    time_point<high_resolution_clock> reference;
    Timer() {
        reset();
    }
    void reset( ) {
        reference = high_resolution_clock::now();
    }
    size_t ms( ) {
        time_point<high_resolution_clock> end = high_resolution_clock::now();
        return duration_cast<milliseconds>(end - reference).count();
    }
};

struct Audio {
    HWAVEIN hWaveIn;
    HWAVEOUT hWaveOut;
    vector<WAVEHDR> inputHeaders;
    vector<WAVEHDR> outputHeaders;
    vector<vector<short>> inputBuffers;
    vector<vector<short>> outputBuffers;
    shared_ptr<thread> processingThread;
    bool runThread;
    std::function<void( short *values, size_t length )> callback;
    size_t filled;

    Audio() {
        hWaveOut = nullptr;
        hWaveIn = nullptr;
        runThread = false;
        filled = 0;
    }
    void init() {
        processingThread = make_shared<thread>([&]() {
            vector<short> buffer( BUFFERLENGTH );
            for( size_t i = 0; i < BUFFERLENGTH; ++i ) {
                buffer.push_back( 0 );
            }
            WAVEFORMATEX pFormat;
            pFormat.wFormatTag = WAVE_FORMAT_PCM;
            pFormat.nChannels = NUMCHANNELS;
            pFormat.nSamplesPerSec = SAMPLERATE;
            pFormat.wBitsPerSample = BITSPERSAMPLE;
            pFormat.nBlockAlign = pFormat.nChannels * pFormat.wBitsPerSample / 8;                  // = n.Channels * wBitsPerSample/8
            pFormat.nAvgBytesPerSec = pFormat.nBlockAlign * pFormat.nSamplesPerSec * pFormat.nChannels;   // = nSamplesPerSec * n.Channels * wBitsPerSample/8
            pFormat.cbSize=sizeof( WAVEFORMATEX );
            waveOutOpen( &hWaveOut, WAVE_MAPPER, &pFormat, 0L, 0L, CALLBACK_NULL );
            waveInOpen( &hWaveIn, WAVE_MAPPER, &pFormat, 0L, 0L, CALLBACK_NULL );
            for( int bufferid = 0; bufferid < NUMBUFFERS; ++bufferid ) {
                outputBuffers.push_back( buffer );
                WAVEHDR WaveOutHdr;
                memset( &WaveOutHdr, 0, sizeof( WaveOutHdr ));
                WaveOutHdr.lpData = ( LPSTR ) &*outputBuffers.rbegin()->begin();
                WaveOutHdr.dwBufferLength = BUFFERLENGTH * sizeof( short );
                WaveOutHdr.dwBytesRecorded=0;
                WaveOutHdr.dwUser = 0L;
                WaveOutHdr.dwFlags = 0L;
                WaveOutHdr.dwLoops = 0L;
                outputHeaders.push_back( WaveOutHdr );

                inputBuffers.push_back( buffer );
                WAVEHDR WaveInHdr;
                memset( &WaveInHdr, 0, sizeof( WaveInHdr ));
                WaveInHdr.lpData = ( LPSTR ) &*inputBuffers.rbegin()->begin();
                WaveInHdr.dwBufferLength = BUFFERLENGTH * sizeof( short );
                WaveInHdr.dwBytesRecorded=0;
                WaveInHdr.dwUser = 0L;
                WaveInHdr.dwFlags = 0L;
                WaveInHdr.dwLoops = 0L;
                inputHeaders.push_back( WaveInHdr );
            }
            for( auto &WaveOutHdr : outputHeaders ) {
                for( size_t i = 0; i < WaveOutHdr.dwBufferLength / 2; ++i ) {
                    (( short* ) WaveOutHdr.lpData)[ i ] = 0;//( ++filled ) & 4 ? 32767 : -32767;
                }
                waveOutPrepareHeader( hWaveOut, &WaveOutHdr, sizeof( WAVEHDR ));
                waveOutWrite( hWaveOut, &WaveOutHdr, sizeof( WAVEHDR ));
            }
            for( auto &WaveInHdr : inputHeaders ) {
                memset( WaveInHdr.lpData, 0, WaveInHdr.dwBufferLength );
                waveInPrepareHeader( hWaveIn, &WaveInHdr, sizeof( WAVEHDR ));
                waveInAddBuffer( hWaveIn, &WaveInHdr, sizeof( WAVEHDR ));
            }
            waveOutSetVolume( hWaveOut, 0xffff );
            waveInStart( hWaveIn );
            runThread = true;
            while( runThread ) {
                for( auto &WaveOutHdr : outputHeaders ) {
                    if( WaveOutHdr.dwFlags & WHDR_DONE ) {
                        WaveOutHdr.dwFlags &= ~WHDR_DONE;
                        for( size_t i = 0; i < WaveOutHdr.dwBufferLength / 2; ++i ) {
                            (( short* ) WaveOutHdr.lpData)[ i ] = 0;//( ++filled ) & 4 ? 32767 : -32767;
                        }
                        waveOutWrite( hWaveOut, &WaveOutHdr, sizeof( WAVEHDR ));
                    }
                }
                for( auto &WaveInHdr : inputHeaders ) {
                    if( WaveInHdr.dwFlags & WHDR_DONE ) {
                        WaveInHdr.dwFlags &= ~WHDR_DONE;
                        callback(( short* ) WaveInHdr.lpData, ( size_t ) WaveInHdr.dwBufferLength / sizeof( short ));
                        memset( WaveInHdr.lpData, 0, WaveInHdr.dwBufferLength );
                        waveInAddBuffer( hWaveIn, &WaveInHdr, sizeof( WAVEHDR ));
                    }
                }
            }
        });
    }
    void setProcessingCallback( std::function<void( short *values, size_t length )> function ) {
        callback = function;
    }
    void stop() {
        runThread = false;
        if( processingThread->joinable() ) {
            processingThread->join();
        }
        processingThread.reset();
        waveOutReset( hWaveOut );
        for( auto &WaveOutHdr : outputHeaders ) {
            if ( WaveOutHdr.dwFlags & WHDR_PREPARED )
                waveOutUnprepareHeader( hWaveOut, &WaveOutHdr, sizeof( WAVEHDR ));
        }
        waveOutClose( hWaveOut );
        waveInReset( hWaveIn );
        for( auto &WaveInHdr : outputHeaders ) {
            if ( WaveInHdr.dwFlags & WHDR_PREPARED )
                waveInUnprepareHeader( hWaveIn, &WaveInHdr, sizeof( WAVEHDR ));
        }
        waveInStop( hWaveIn );
        waveInClose( hWaveIn );
    }
    ~Audio() {}
} audio;

struct SQLFuncts {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    typedef vector<vector<corestring>> dbResults;
    dbResults users;
    long currentUserID;
    string currentUserName;
    SQLFuncts() : db( 0 ) {
        sqlite3_initialize();
    }
    ~SQLFuncts() {
        sqlite3_close( db );
    }
    dbResults query( const char * query ) {
        dbResults result;
        if( sqlite3_prepare_v2( db, query, -1, &stmt, nullptr ) == SQLITE_OK ) {
            int rc = sqlite3_step( stmt );
            if( rc == SQLITE_ROW ) {
                int ncols = sqlite3_column_count( stmt );
                vector<corestring> row;
                while( rc == SQLITE_ROW ) {
                    row.clear();
                    for( int i = 0; i < ncols; i++ ) {
                        const char *str = ( const char * )sqlite3_column_text( stmt, i );
                        row.push_back( str ? str : "" );
                    }
                    result.push_back( row );
                    rc = sqlite3_step( stmt );
                }
            }
            sqlite3_finalize( stmt );
        }
        return result;
    }
    int exec( const char * query ) {
        return sqlite3_exec( db, query, nullptr, nullptr, nullptr );
    }
    void init( const char *dbName ) {
        sqlite3_open( dbName, &db );
        auto create = "create table if not exists users ( "  \
             "id integer primary key, " \
             "name text not null );";

        exec( create );

        create = "create table if not exists runs ( "  \
             "id integer primary key, " \
             "user_id integer not null, " \
             "length float );";

        exec( create );

        create = "create table if not exists statistics ( "  \
             "id integer primary key, " \
             "user_id integer not null, " \
             "date date, " \
             "start_time text, " \
             "end_time text, " \
             "distance float, " \
             "seconds float );";

        exec( create );

        create = "create table if not exists settings ( "  \
             "parameter text primary key, " \
             "ivalue integer, " \
             "tvalue text );";

        exec( create );

        dbResults currentUser = query( "select id from users;" );
        if( !currentUser.size() ) {
            exec( "insert into users ( name ) values( 'Gyöngyi' );" );
        }
        updateuserslist();

        if( users.size() ) {
            dbResults currentUser = query( "select ivalue, tvalue from settings where parameter = 'current_user';" );
            if( currentUser.size() ) {
                currentUserID = currentUser[ 0 ][ 0 ].toLong();
                currentUserName = currentUser[ 0 ][ 1 ];
            } else {
                currentUserID = users[ 0 ][ 0 ].toLong();
                currentUserName = users[ 0 ][ 1 ];
                corestring sql;
                sql.format( "insert into settings ( parameter, ivalue, tvalue ) values( 'current_user', %ld, '%s' );", currentUserID, currentUserName.c_str() );
                exec( sql.c_str() );
            }
        }
    }
    void updateuserslist() {
        users = query( "select * from users;" );
    }
    long newuser( const char * user_name ) {
        corestring sql;
        sql.format( "insert into users ( name ) values( '%s' );", user_name );
        exec( sql.c_str() );
        currentUserName = user_name;
        sql.format( "select id from users where name = '%s';", user_name );
        dbResults results = query( sql.c_str() );
        if( results.size() ) {
            currentUserID = results[ 0 ][ 0 ].toLong();
            results = query( "select ivalue, tvalue from settings where parameter = 'current_user';" );
            if( results.size() ) {
                sql.format( "update settings set ivalue = %ld, tvalue = '%s' where parameter = 'current_user';", currentUserID, currentUserName.c_str() );
            } else {
                sql.format( "insert into settings ( parameter, ivalue, tvalue ) values( 'current_user', %ld, '%s' );", currentUserID, currentUserName.c_str() );
            }
            exec( sql.c_str() );
        }
        return currentUserID;
    }
    float getuserrun( long user_id ) {
        corestring str;
        str.format( "select length from runs where user_id = %ld", user_id );
        auto runs = query( str.c_str() );
        if( runs.size() )
            return runs[0][0].toFloat();
        return 0;
    }
    void setuserrun( long user_id, float length ) {
        corestring str;
        str.format( "select length from runs where user_id = %ld", user_id );
        auto runs = query( str.c_str() );
        if( runs.size() )
            str.format( "update runs set length = %f where user_id = '%ld'", length, user_id );
        else
            str.format( "insert into runs ( user_id, length ) values ( %ld, %f );", user_id, length );
        exec( str.c_str() );
    }
} sqlfuncts;

struct runInfo {
    runInfo() : distance( 0 ), avgSpeed( 0 ) {}
    corestring date;
    float distance;
    float avgSpeed;
};

#define LISTEN_PORT 80
#define BACKLOG 5
#define PROTOCOL "HTTP/1.1"

const char *dayNames[]={ "Vasárnap", "Hétfő", "Kedd", "Szerda", "Csütörtök", "Péntek", "Szombat" };
const char *keyboardKeys[]={ "0123456789öü←", "qwertzuiopőúű", "asdfghjkléáíó","yxcvbnm,.-↲" };

struct MainProcessor {
    static float dist;
    static float prevdist;
    static float laststoredvalue;
    static float speed;
    static float startDist;
    static vector<short> input;
    static Timer trigger;
    static time_t startTime;
    static float runningms;
    Timer refresh;
    Timer statistics;
    Timer updatedb;
    bool running;
    runInfo runInfoDay;
    vector<runInfo> runInfoWeek;
    runInfo runInfoMonth;
    shared_ptr<Gdiplus::Image> bicyclist;
    shared_ptr<Gdiplus::Image> speedometer;
    HFONT smallFont;
    HFONT normalFont;
    HFONT mediumFont;
    HFONT bigFont;
    shared_ptr<thread> webServerThread;
    long editedUserID;
    wstring userName;
    bool runThread;
    enum MENU {
        MPM_MAIN,
        MPM_STAT,
        MPM_USERS,
        MPM_NEWUSER
    };
    enum USERMODE {
        UM_UNSET,
        UM_EDIT,
        UM_DELETE
    };
    MENU menu;
    USERMODE usermode;
    MainProcessor() : running( false ), menu( MPM_MAIN ) {
        audio.setProcessingCallback( &processingValues );
    }
    ~MainProcessor() {
        DeleteObject( smallFont );
        DeleteObject( normalFont );
        DeleteObject( mediumFont );
        DeleteObject( bigFont );
        runThread = false;
        if( webServerThread->joinable() ) {
            webServerThread->join();
        }
        webServerThread.reset();
    }
    string wStringToUtf( const wchar_t * str ) {
        std::wstring_convert<std::conditional<sizeof(wchar_t) == 4,std::codecvt_utf8_utf16<wchar_t>,std::codecvt_utf8<wchar_t>>::type> converter;
        return converter.to_bytes( str );
    }
    wstring utfToWString( const char * str ) {
        std::wstring_convert<std::conditional<sizeof(wchar_t) == 4,std::codecvt_utf8<wchar_t>,std::codecvt_utf8_utf16<wchar_t>>::type> converter;
        return converter.from_bytes( str );
    }
    void setcurrentuser( long user_id ) {
        corestring query;
        query.format( "select name from users where id = %ld", user_id );
        SQLFuncts::dbResults userName = sqlfuncts.query( query.c_str() );
        if( userName.size() ) {
            sqlfuncts.currentUserID = user_id;
            sqlfuncts.currentUserName = userName[ 0 ][ 0 ];
            query.format( "update settings set ivalue = %ld, tvalue = '%s' where parameter = 'current_user';", user_id, sqlfuncts.currentUserName.c_str() );
            sqlfuncts.exec( query.c_str( ));
        }
        updateRunInfo();
    }
    wchar_t getClickedKey( HWND hwnd, long x, long y ) {
        RECT rect;
        GetClientRect( hwnd, &rect );
        const int width = rect.right;
        const int height = rect.bottom;
        float scale = width / 1920.f;
        float hscale = height / 1080.f;
        int hs = 140 * scale;
        int vs = 140 * hscale;
        int ypos = height - vs * ( sizeof( keyboardKeys ) / sizeof( *keyboardKeys ));
        for( string line : keyboardKeys ) {
            wstring wstr = utfToWString( line.c_str() );
            int xpos = ( width - hs * wstr.length() ) / 2;
            for( auto wch : wstr ) {
                if( xpos <= x && x < xpos + hs && ypos <= y && y < ypos + vs ) {
                    return wch;
                }
                xpos += hs;
            }
            ypos += vs;
        }
        return 0;
    }
    void setMenu( MENU newMenu ) {
        menu = newMenu;
        if( MPM_MAIN == menu )
            usermode = UM_UNSET;
    }
    void clicked( HWND hwnd, long x, long y ) {
        RECT rect;
        GetClientRect( hwnd, &rect );
        const int width = rect.right;
        const int height = rect.bottom;
        float scale = width / 1920.f;
        float hscale = height / 1080.f;
        if( y < 350 * hscale ) {
            switch( menu ) {
            case MPM_MAIN:
                setMenu( MPM_STAT );
                break;
            case MPM_STAT:
                setMenu( MPM_USERS );
                break;
            case MPM_NEWUSER:
            case MPM_USERS:
            default:
                setMenu( MPM_MAIN );
                break;
            }
        }
        if( MPM_USERS == menu ) {
            int spc = 20 * scale;
            int hs = ( width - 100 * scale - spc ) / 3;
            int vs = 140 * hscale;
            int ypos = 350 * hscale;
            int xpos = 50 * scale;

            if( UM_UNSET == usermode ) {
                if( xpos <= x && x < xpos + hs && ypos <= y && y < ypos + vs ) {
                    userName.clear();
                    setMenu( MPM_NEWUSER );
                }
                xpos += hs + spc;
                if( xpos <= x && x < xpos + hs && ypos <= y && y < ypos + vs ) {
                    usermode = UM_EDIT;
                }
                xpos += hs + spc;
                if( xpos <= x && x < xpos + hs && ypos <= y && y < ypos + vs ) {
                    usermode = UM_DELETE;
                }
            }

            hs = ( width - 100 * scale - spc ) / 4;
            xpos = 50 * scale;
            ypos += vs + spc;
            for( auto user : sqlfuncts.users ) {
                if( xpos <= x && x < xpos + hs && ypos <= y && y < ypos + vs ) {
                    if( UM_EDIT == usermode ) {
                        editedUserID = user[ 0 ].toLong();
                        userName = utfToWString( user[ 1 ].c_str() );
                        setMenu( MPM_NEWUSER );
                        return;
                    } else if( UM_DELETE == usermode ) {
                        if( user[ 0 ].toLong() == sqlfuncts.currentUserID ) {
                            setMenu( MPM_MAIN );
                            return;
                        } else {
                            corestring sql;
                            sql.format( "delete from users where id = %ld;", user[ 0 ].toLong() );
                            sqlfuncts.exec( sql.c_str() );
                            sqlfuncts.updateuserslist();
                        }
                    } else {
                        sqlfuncts.currentUserID = user[ 0 ].toLong();
                        sqlfuncts.currentUserName = user[ 1 ].c_str();
                        setcurrentuser( sqlfuncts.currentUserID );
                        setMenu( MPM_MAIN );
                        return;
                    }
                }
                xpos += hs + spc;
                if( xpos > width ) {
                    xpos = 50 * scale;
                    ypos += vs + spc;
                }
            }
        }
        if( MPM_NEWUSER == menu ) {
            auto prkey( getClickedKey( hwnd, x, y ));
            if( L'←' == prkey ) {
                if( userName.length() )
                    userName.resize( userName.size() - 1 );
            } else if( L'↲' == prkey ) {
                string user = wStringToUtf( userName.c_str() );
                corestring query;
                if( UM_EDIT == usermode ) {
                    query.format( "update users set name = '%s' where id = '%ld';", user.c_str(), editedUserID );
                    sqlfuncts.exec( query.c_str() );
                    if( editedUserID == sqlfuncts.currentUserID ) {
                        sqlfuncts.currentUserName = user.c_str();
                        setcurrentuser( editedUserID );
                    }
                } else {
                    query.format( "select id from users where name = '%s';", user.c_str() );
                    auto results = sqlfuncts.query( query.c_str() );
                    if( results.size() ) {
                        setcurrentuser( results[ 0 ][ 0 ].toLong() );
                    } else {
                        sqlfuncts.newuser( user.c_str() );
                    }
                }
                sqlfuncts.updateuserslist();
                usermode = UM_UNSET;
                setMenu( MPM_MAIN );
            } else if( 0 != prkey ) {
                if( !userName.length() )
                    userName += towupper( prkey );
                else
                    userName += prkey;
            }
        }
    }
    void init( HWND hwnd ) {
        RECT rect;
        GetClientRect( hwnd, &rect );
        const int width = rect.right;
        bicyclist = make_shared<Gdiplus::Image>( L"bicyclist-small.png" );
        speedometer = make_shared<Gdiplus::Image>( L"speedometer.png" );
        smallFont = CreateFont( width/24, 0, 0, 0, FW_BOLD,
                                 FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, VARIABLE_PITCH,
                                 TEXT( "Noto Sans Lisu" ));
        normalFont = CreateFont( width/16, 0, 0, 0, FW_BOLD,
                                 FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, VARIABLE_PITCH,
                                 TEXT( "Noto Sans Lisu" ));
        mediumFont = CreateFont( width/10, 0, 0, 0, FW_MEDIUM,
                                 FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, VARIABLE_PITCH,
                                 TEXT( "Arial" ));
        bigFont = CreateFont( width/6, 0, 0, 0, FW_MEDIUM,
                                 FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, VARIABLE_PITCH,
                                 TEXT( "Arial" ));
        updateRunInfo();
        startWebServer();
    }
    void drawRect( HDC hdc, int x, int y, int w, int h ) {
       Graphics graphics(hdc);
       SolidBrush bbrush( Color( 55, 55, 55, 255 ));
       Pen      pen( Color( 85, 85, 85, 255 ));
       Pen      pen2( Color( 35, 35, 35, 255 ));
       graphics.FillRectangle( &bbrush, x + 1, y + 1, w - 2, h - 2 );
       graphics.DrawLine( &pen, x, y, x + w, y);
       graphics.DrawLine( &pen, x, y, x, y + h);
       graphics.DrawLine( &pen2, x + w, y + h, x + w, y);
       graphics.DrawLine( &pen2, x + w, y + h, x, y + h);
    }
    void paint( HWND hwnd ) {
        auto fdc = BeginPaint( hwnd, &ps );
        RECT rect;
        GetClientRect( hwnd, &rect );
        const int width = rect.right;
        const int height = rect.bottom;
        float scale = width / 1920.f;
        float hscale = height / 1080.f;

        HDC hdc = CreateCompatibleDC( fdc );

        HBITMAP backbuffer = CreateCompatibleBitmap( fdc, width, height );

        int savedDC = SaveDC( hdc );
        SelectObject( hdc, backbuffer );
        HBRUSH hBrush = CreateSolidBrush( RGB( 15, 0, 70 ));
        FillRect( hdc, &rect, hBrush );
        DeleteObject( hBrush );

        Gdiplus::Graphics gr(hdc);
        corestring user = 1 ? sqlfuncts.currentUserName : "User";

        if( menu == MPM_MAIN ) {
            gr.DrawImage( bicyclist.get(), int( width - 500 * scale ), int( 30 * hscale ), int( bicyclist->GetWidth() * scale ), int( bicyclist->GetHeight() * scale ));
            gr.DrawImage( speedometer.get(),  int( width / 2 - ( 460 + speedometer->GetWidth() ) * scale ), int( 80 * hscale ), int( speedometer->GetWidth() * scale ), int( speedometer->GetHeight() * scale ));

            SetBkMode( hdc, TRANSPARENT );
            corestring str;
            SIZE size;

            SelectObject( hdc, mediumFont );
            SetTextColor( hdc, RGB( 40, 10, 255 ));
            str.format( "XSpeed", speed );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, ( width - size.cx ) / 2, int( 40 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, smallFont );
            SetTextColor( hdc, RGB( 40, 10, 255 ));
            str.format( "%s", user.c_str() );
            auto ws = utfToWString( str.c_str() );
            GetTextExtentPoint32W( hdc, ws.c_str(), ws.length(), &size );
            TextOutW( hdc, ( width - size.cx ) / 2, int( 260 * hscale ), ws.c_str(), ws.length() );

            SelectObject( hdc, bigFont );
            SetTextColor( hdc, RGB( 255, 255, 0 ));
            str.format( "%1.2f km/h", speed );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, ( width - size.cx ) / 2, int( 325 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, bigFont );
            SetTextColor( hdc, RGB( 0, 255, 0 ));
            str.format( "%1.4f km", dist / 1000.f );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, ( width - size.cx ) / 2, int( 630 * hscale ), str.c_str(), str.length() );
        }
        if( menu == MPM_STAT ) {
            gr.SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBicubic );
            gr.DrawImage( bicyclist.get(), int( width - 800 * scale ), int( 30 * hscale ), int ( bicyclist->GetWidth() * scale / 2 ), int( bicyclist->GetHeight() * scale / 2 ));
            gr.DrawImage( speedometer.get(), int( width / 2 - ( 170 + speedometer->GetWidth() / 2 ) * scale ), int( 55 * hscale ), int( speedometer->GetWidth() * scale / 2 ), int( speedometer->GetHeight() * scale / 2 ));

            SetBkMode( hdc, TRANSPARENT );
            corestring str;
            SIZE size;

            SelectObject( hdc, smallFont );
            SetTextColor( hdc, RGB( 40, 10, 255 ));
            str.format( "XSpeed", speed );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, ( width - size.cx ) / 2, int( 40 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, smallFont );
            SetTextColor( hdc, RGB( 40, 10, 255 ));
            str.format( "%s", sqlfuncts.currentUserName.c_str() );
            auto ws = utfToWString( str.c_str() );
            GetTextExtentPoint32W( hdc, ws.c_str(), ws.length(), &size );
            TextOutW( hdc, ( width - size.cx ) / 2, int( 120 * hscale ), ws.c_str(), ws.length() );

            SelectObject( hdc, normalFont );
            SetTextColor( hdc, RGB( 255, 255, 0 ));
            str.format( "Napi" );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, int( 50 * scale ), int( 325 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, normalFont );
            SetTextColor( hdc, RGB( 255, 255, 0 ));
            str.format( "%1.4f km", runInfoDay.distance / 1000 );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, int( 350 * scale ), int( 325 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, normalFont );
            SetTextColor( hdc, RGB( 255, 255, 0 ));
            str.format( "%1.2f km/h", runInfoDay.avgSpeed );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, int( 950 * scale ), int( 325 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, normalFont );
            SetTextColor( hdc, RGB( 255, 255, 0 ));
            str.format( "Havi", speed );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, int( 50 * scale ), int( 525 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, normalFont );
            SetTextColor( hdc, RGB( 255, 255, 0 ));
            str.format( "%1.4f km", runInfoMonth.distance / 1000 );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, int( 350 * scale ), int( 525 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, normalFont );
            SetTextColor( hdc, RGB( 255, 255, 0 ));
            str.format( "%1.2f km/h", runInfoMonth.avgSpeed );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, int( 950 * scale ), int( 525 * hscale ), str.c_str(), str.length() );
        }
        if( menu == MPM_USERS ) {
            gr.SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBicubic );
            gr.DrawImage( bicyclist.get(), int( width - 800 * scale ), int( 30 * hscale ), int ( bicyclist->GetWidth() * scale / 2 ), int( bicyclist->GetHeight() * scale / 2 ));
            gr.DrawImage( speedometer.get(), int( width / 2 - ( 170 + speedometer->GetWidth() / 2 ) * scale ), int( 55 * hscale ), int( speedometer->GetWidth() * scale / 2 ), int( speedometer->GetHeight() * scale / 2 ));

            SetBkMode( hdc, TRANSPARENT );
            corestring str;
            SIZE size;

            SelectObject( hdc, smallFont );
            SetTextColor( hdc, RGB( 40, 10, 255 ));
            str.format( "XSpeed", speed );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, ( width - size.cx ) / 2, int( 40 * hscale ), str.c_str(), str.length() );

            int spc = 20 * scale;
            int hs = ( width - 100 * scale - spc ) / 3;
            int vs = 140 * hscale;
            int ypos = 350 * hscale;
            int xpos = 50 * scale;

            if( UM_UNSET == usermode ) {
                drawRect( hdc, xpos, ypos, hs - 1, vs - 1 );
                wstring ws = L"Új";
                GetTextExtentPoint32W( hdc, ws.c_str(), ws.length(), &size );
                TextOutW( hdc, xpos + ( hs - size.cx ) / 2, ypos + 30 * hscale, ws.c_str(), ws.length() );
                xpos += hs + spc;

                drawRect( hdc, xpos, ypos, hs - 1, vs - 1 );
                ws = L"Módosít";
                GetTextExtentPoint32W( hdc, ws.c_str(), ws.length(), &size );
                TextOutW( hdc, xpos + ( hs - size.cx ) / 2, ypos + 30 * hscale, ws.c_str(), ws.length() );
                xpos += hs + spc;

                drawRect( hdc, xpos, ypos, hs - 1, vs - 1 );
                ws = L"Töröl";
                GetTextExtentPoint32W( hdc, ws.c_str(), ws.length(), &size );
                TextOutW( hdc, xpos + ( hs - size.cx ) / 2, ypos + 30 * hscale, ws.c_str(), ws.length() );
                xpos += hs + spc;
            } else if( UM_EDIT == usermode ) {
                xpos += hs + spc;
                drawRect( hdc, xpos, ypos, hs - 1, vs - 1 );
                wstring ws = L"Módosít";
                GetTextExtentPoint32W( hdc, ws.c_str(), ws.length(), &size );
                TextOutW( hdc, xpos + ( hs - size.cx ) / 2, ypos + 30 * hscale, ws.c_str(), ws.length() );
            } else if( UM_DELETE == usermode ) {
                xpos += hs + spc;
                drawRect( hdc, xpos, ypos, hs - 1, vs - 1 );
                wstring ws = L"Töröl";
                GetTextExtentPoint32W( hdc, ws.c_str(), ws.length(), &size );
                TextOutW( hdc, xpos + ( hs - size.cx ) / 2, ypos + 30 * hscale, ws.c_str(), ws.length() );
            }

            hs = ( width - 100 * scale - spc ) / 4;
            xpos = 50 * scale;
            ypos += vs + spc;

            for( auto user : sqlfuncts.users ) {
                drawRect( hdc, xpos, ypos, hs - 1, vs - 1 );
                auto ws = utfToWString( user[ 1 ].c_str() );
                GetTextExtentPoint32W( hdc, ws.c_str(), ws.length(), &size );
                TextOutW( hdc, xpos + ( hs - size.cx ) / 2, ypos + 30 * hscale, ws.c_str(), ws.length() );
                xpos += hs + spc;
                if( xpos > width ) {
                    xpos = 50 * scale;
                    ypos += vs + spc;
                }
            }
        }
        if( menu == MPM_NEWUSER ) {
            gr.SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBicubic );
            gr.DrawImage( bicyclist.get(), int( width - 800 * scale ), int( 30 * hscale ), int ( bicyclist->GetWidth() * scale / 2 ), int( bicyclist->GetHeight() * scale / 2 ));
            gr.DrawImage( speedometer.get(), int( width / 2 - ( 170 + speedometer->GetWidth() / 2 ) * scale ), int( 55 * hscale ), int( speedometer->GetWidth() * scale / 2 ), int( speedometer->GetHeight() * scale / 2 ));

            SetBkMode( hdc, TRANSPARENT );
            corestring str;
            SIZE size;

            SelectObject( hdc, smallFont );
            SetTextColor( hdc, RGB( 40, 10, 255 ));
            str.format( "XSpeed", speed );
            GetTextExtentPoint32( hdc, str.c_str(), str.length(), &size );
            TextOut( hdc, ( width - size.cx ) / 2, int( 40 * hscale ), str.c_str(), str.length() );

            SelectObject( hdc, smallFont );
            SetTextColor( hdc, RGB( 40, 10, 255 ));
            GetTextExtentPoint32W( hdc, userName.c_str(), userName.length(), &size );
            TextOutW( hdc, ( width - size.cx ) / 2, int( 120 * hscale ), userName.c_str(), userName.length() );

            int hs = 140 * scale;
            int vs = 140 * hscale;
            int ypos = height - vs * ( sizeof( keyboardKeys ) / sizeof( *keyboardKeys ));
            SelectObject( hdc, normalFont );
            SetTextColor( hdc, RGB( 40, 10, 255 ));
            for( string line : keyboardKeys ) {
                wstring wstr = utfToWString( line.c_str() );
                int xpos = ( width - hs * wstr.length() ) / 2;
                for( auto wch : wstr ) {
                    drawRect( hdc, xpos, ypos, hs - 1, vs - 1 );
                    wchar_t ech;
                    if( !userName.length() )
                        ech = towupper( wch );
                    else
                        ech = wch;
                    GetTextExtentPoint32W( hdc, &ech, 1, &size );
                    TextOutW( hdc, xpos + ( hs - size.cx ) / 2, ypos, &ech, 1 );
                    xpos += hs;
                }
                ypos += vs;
            }
        }
#if 0
        int pos = 0;
        for( auto val : input ) {
            SetPixel( hdc, pos++, val / 200 + int( 500 * hscale ), RGB( 255, 255, 0 ));
        }
#endif
        BitBlt( fdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY );
        RestoreDC( hdc, savedDC );

        DeleteObject( backbuffer );
        DeleteDC( hdc );

        EndPaint( hwnd, &ps );
    };
    static void triggerRound() {
        float time = trigger.ms();
        if( time > 150 ) {
            if( time < 5000 ) {
                dist += 6.22; // meter: 6.22 / turn
                speed = 3.6 * 6.22 * 1000 / time;
                runningms += time;
            }
            trigger.reset();
        }
        if( time > 3000 ) {
            speed = 0;
        }
    }
    void updateRunInfo() {
        struct tm *timeinfo;
        corestring sDate;
        corestring query;
        auto now = std::chrono::system_clock::now();
        time_t tnow = chrono::system_clock::to_time_t( now );

        timeinfo = localtime ( &tnow );
        sDate.format( "%04d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday );
        query.format( "select sum( distance ), sum( seconds ) from statistics where user_id = %ld and date = '%s';", sqlfuncts.currentUserID, sDate.c_str() );
        SQLFuncts::dbResults results = sqlfuncts.query( query.c_str() );
        if( results.size() ) {
            runInfoDay.date = sDate;
            runInfoDay.distance = results[0][0].toFloat();
            float time = results[0][1].toFloat();
            if( time )
                runInfoDay.avgSpeed = 3.6 * runInfoDay.distance / time;
            else
                runInfoDay.avgSpeed = 0;
        }

        sDate.format( "%04d-%02d%%", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1 );
        query.format( "select sum( distance ), sum( seconds ) from statistics where user_id = %ld and date like '%s';", sqlfuncts.currentUserID, sDate.c_str() );
        results = sqlfuncts.query( query.c_str() );
        if( results.size() ) {
            runInfoMonth.date = sDate;
            runInfoMonth.distance = results[0][0].toFloat();
            float time = results[0][1].toFloat();
            if( time )
                runInfoMonth.avgSpeed = 3.6 * runInfoMonth.distance / time;
            else
                runInfoMonth.avgSpeed = 0;
        }
        timeinfo->tm_mon -= 1;
        time_t from = mktime( timeinfo );
        timeinfo = localtime ( &from );
        sDate.format( "%04d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday );
        query.format( "select date, sum(distance), sum(seconds) from statistics where user_id = %ld  and date >= '%s' group by date order by date desc", sqlfuncts.currentUserID, sDate.c_str() );
        results = sqlfuncts.query( query.c_str() );
        for( auto &res : results ) {
            memset( timeinfo, 0, sizeof( *timeinfo ));
            corestring cut;
            cut.set( &res[0][0], 4 );
            timeinfo->tm_year = cut.toLong() - 1900;
            cut.set( &res[0][5], 2 );
            timeinfo->tm_mon = cut.toLong() - 1;
            cut.set( &res[0][8], 2 );
            timeinfo->tm_mday = cut.toLong();
            time_t date = mktime( timeinfo );
            timeinfo = localtime ( &date );
            runInfo day;
            size_t tdiff = difftime( tnow, date ) / 60 / 60 / 24;
            corestring tmp, info;
            if( tdiff >= 7 ) {
                tmp.format( "%ld hete + ", tdiff / 7 );
                info += tmp;
            }
            tdiff %= 7;
            if( tdiff > 0 ) {
                tmp.format( "%ld napja", tdiff );
                info += tmp;
            } else {
                info += "ma";
            }
            day.date.format( "%s ( %s : %s )", res[0].c_str(), dayNames[ timeinfo->tm_wday ], info.c_str() );
            day.distance = res[1].toFloat();
            float time = res[2].toFloat();
            if( time )
                day.avgSpeed = 3.6 * day.distance / time;
            else
                day.avgSpeed = 0;
            runInfoWeek.push_back( day );
        }
    }
    void resetRound() {
        auto now = std::chrono::system_clock::now();
        dist = 0;
        prevdist = 0;
        speed = 0;
        laststoredvalue = 0 ;
        startDist = 0;
        startTime = chrono::system_clock::to_time_t( now );
        runningms = 0;
        sqlfuncts.setuserrun( sqlfuncts.currentUserID, dist );
        running = false;
    }
    static void processingValues( short *values, size_t length ) {
        enum { HIGH = 1, LOW = 0 };
        static int cut = BUFFERLENGTH;
        static int state = LOW;
        static int prevstate = LOW;
        static short prevvalue = 0;
        input.clear();
        for( size_t pos = 0; pos < length; pos += 2 ) {
            static float x;
            x = values[ pos ];
            input.push_back( x );
            int delta = x - prevvalue;
            if( delta > -50 && x > 10000 ) {
                state = HIGH;
            } else {
                state = LOW;
            }
            float time = trigger.ms();
            if( state != prevstate && !cut ) {
                if( time > 150 && state == HIGH ) {
                    if( time < 5000 ) {
                        dist += 6.22; // meter: 6.22 / turn
                        speed = 3.6 * 6.22 * 1000 / time;
                        runningms += time;
                    }
                    trigger.reset();
                }
            }
            if( time > 3000 ) {
                speed = 0;
            }
            prevstate = state;
            if( cut )
                cut--;
            prevvalue = x;
        }
    }
    void startWebServer() {
        webServerThread = make_shared<thread>([&]() {
            runThread = true;
            SOCKET listenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
            if( listenSocket == INVALID_SOCKET ) {
                return;
            }

            sockaddr_in serverService;
            serverService.sin_family = AF_INET;
            serverService.sin_addr.s_addr = INADDR_ANY;
            serverService.sin_port = htons( LISTEN_PORT );
            if( bind( listenSocket, ( SOCKADDR* )&serverService, sizeof( serverService )) == SOCKET_ERROR ) {
                return;
            }

            if( listen( listenSocket, BACKLOG ) == SOCKET_ERROR ) {
                return;
            }
            while( runThread ) {
                struct sockaddr_in from;
                int fromLen = sizeof( from );
                SOCKET msgSocket = accept( listenSocket, (struct sockaddr*)&from, &fromLen );
                if( msgSocket == INVALID_SOCKET ) {
                    return;
                }

                DWORD ms = 200;
                setsockopt( msgSocket, SOL_SOCKET, SO_RCVTIMEO, ( const char * ) &ms, sizeof( ms ));
                setsockopt( msgSocket, SOL_SOCKET, SO_SNDTIMEO, ( const char * ) &ms, sizeof( ms ));
                corestring input;
                corestring line;

                long pos = 0;
                for( ;; ) {
                    long crec = pos + 200;
                    input.resize( crec );
                    long nrec = recv( msgSocket, &input[ pos ], crec - pos, 0 );
                    if( WSAGetLastError() || !nrec ) {
                        break;
                    }
                    pos += nrec;
                }
                input[ pos ] = '\0';
                if( pos ) {
                    for( auto ch : input ) {
                        switch ( ch ) {
                        case '\n':
                        case '\r':
                            if ( line.length() ) {
                                corestring sndbuf, buf, line, content;
                                line.format( "<h1>Napi: %1.4f km - %1.2f km/h</h1>", runInfoDay.distance / 1000, runInfoDay.avgSpeed );
                                content += line;
                                line.format( "<h1>Havi: %1.4f km - %1.2f km/h</h1>", runInfoMonth.distance / 1000, runInfoMonth.avgSpeed );
                                content += line;
                                content += "<h2>Részletes:</h2><h3>";
                                for( auto &day : runInfoWeek ) {
                                    line.format( "%s: %1.4f km - %1.2f km/h<br>", day.date.c_str(), day.distance / 1000, day.avgSpeed );
                                    content += line;
                                }
                                content += "</h3>";
                                buf.format( "<html><head><meta charset=\"UTF-8\"/><meta http-equiv=\"refresh\" content=\"1\"/></head><body><h1>XSpeed</h1>%s</body></html>", content.c_str() );
                                sndbuf.format( "HTTP/1.1 200 OK\r\nServer: XSpeed Webserver\r\nContent-Length: %d\r\nContent-Type: text/html;\r\n\n%s", buf.length(), buf.c_str() );
                                send( msgSocket, sndbuf.c_str(), sndbuf.length(), 0 );
                            }
                            line.clear();
                            break;
                        default:
                            line += ch;
                            break;
                        }
                    }
                }
                closesocket( msgSocket );
            }
            closesocket( listenSocket );
        });
    }
    void update( HWND hwnd ) {
        if( refresh.ms() > 1 ) {
            InvalidateRect( hwnd, NULL, FALSE );
            refresh.reset();
        }
        if( statistics.ms() > 600000 && running ) { // 10 min
            resetRound();
            running = false;
        }
        if( updatedb.ms() > 1000 ) {
            if( laststoredvalue != dist ) {
                sqlfuncts.setuserrun( sqlfuncts.currentUserID, dist );

                struct tm *timeinfo;
                auto now = std::chrono::system_clock::now();
                if( !running ) {
                    startTime = chrono::system_clock::to_time_t( now ) - 1;
                    startDist = laststoredvalue;
                    running = true;
                }
                corestring sDate;
                corestring sStartTime;
                corestring sEndTime;
                timeinfo = localtime ( &startTime );
                sDate.format( "%04d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday );
                sStartTime.format( "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
                time_t endTime = chrono::system_clock::to_time_t( now );
                timeinfo = localtime ( &endTime );
                sEndTime.format( "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
                float distance = dist - startDist;
                float seconds = runningms * 1e-3f;
                // week number : ( timeinfo.tm_yday - timeinfo.tm_wday + 7 ) / 7;

                corestring query;
                query.format( "select id from statistics where date = '%s' and start_time = '%s';", sDate.c_str(), sStartTime.c_str() );
                SQLFuncts::dbResults results = sqlfuncts.query( query.c_str() );
                if( results.size() ) {
                    query.format( "update statistics set user_id = %ld, start_time = '%s', end_time = '%s', distance = %f, seconds = %f where date = '%s' and start_time = '%s';", sqlfuncts.currentUserID, sStartTime.c_str(), sEndTime.c_str(), distance, seconds, sDate.c_str(), sStartTime.c_str() );
                } else {
                    query.format( "insert into statistics ( user_id, date, start_time, end_time, distance, seconds ) values ( %ld, '%s', '%s', '%s', %f, %f );", sqlfuncts.currentUserID, sDate.c_str(), sStartTime.c_str(), sEndTime.c_str(), distance, seconds );
                }
                sqlfuncts.exec( query.c_str() );
                updateRunInfo();
                laststoredvalue = dist;
                statistics.reset();
            }
            updatedb.reset();
        }
    }
} mainProcessor;

vector<short> MainProcessor::input;
float MainProcessor::dist( 0 );
float MainProcessor::prevdist( 0 );
float MainProcessor::laststoredvalue( 0 );
float MainProcessor::speed( 0 );
float MainProcessor::startDist( 0 );
time_t MainProcessor::startTime( 0 );
float MainProcessor::runningms( 0 );
Timer MainProcessor::trigger;

int WINAPI WinMain( HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow )
{
    HWND hwnd;
    MSG msg;
    WNDCLASSEX wincl;

    WSAData wsadata;
    if( WSAStartup( MAKEWORD( 2, 2 ), &wsadata) != NO_ERROR ) {
        return 0;
    }

    setlocale( LC_ALL, "Hun" );
    char exePath[ _MAX_PATH + 1 ];
    GetModuleFileName( NULL, exePath, _MAX_PATH );
    char *pos = strrchr( exePath, '\\' );
    *pos = 0;
    SetCurrentDirectory( exePath );

    sqlfuncts.init( "xspeed.db" );
    MainProcessor::prevdist = MainProcessor::laststoredvalue = MainProcessor::dist = sqlfuncts.getuserrun( sqlfuncts.currentUserID );

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL );

    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = 0;
    wincl.cbSize = sizeof (WNDCLASSEX);

    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0;
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    if( !RegisterClassEx( &wincl ))
        return 0;

    hwnd = CreateWindowEx ( 0, szClassName, _T( "Biker App" ),
               WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
               10, 10, HWND_DESKTOP,
               NULL, hThisInstance, NULL );

    SetWindowLongPtr( hwnd, GWL_EXSTYLE, WS_EX_TOPMOST );
    SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
    ShowWindow( hwnd, SW_SHOWMAXIMIZED );
    audio.init();
    mainProcessor.init( hwnd );

    bool bloop = true;
    while( bloop ) {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )) {
            if( msg.message == WM_QUIT ) {
                bloop = false;
            } else {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        } else {
            mainProcessor.update( hwnd );
        }
    }
    Gdiplus::GdiplusShutdown( gdiplusToken );
    WSACleanup();

    return msg.wParam;
}

LRESULT CALLBACK WindowProcedure( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
    case WM_QUIT:
        audio.stop();
        break;
    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;
    case WM_CREATE:        hbrWhite = ( HBRUSH )GetStockObject( WHITE_BRUSH );
        hbrGray  = ( HBRUSH )GetStockObject( GRAY_BRUSH );
        hbrBlack = ( HBRUSH )GetStockObject( BLACK_BRUSH );
        break;
    case WM_ERASEBKGND:
        return 1;
        break;
    case WM_LBUTTONDOWN:
        mainProcessor.clicked( hwnd, LOWORD( lParam ), HIWORD( lParam ));
        break;
    case WM_RBUTTONDOWN:
        mainProcessor.resetRound();
        break;
    case WM_KEYDOWN:
        switch( wParam )
        {
        case 'A':
            MainProcessor::triggerRound();
            break;
        case VK_ESCAPE:
            PostQuitMessage( 0 );
            break;
        }
        break;
    case WM_PAINT:
        mainProcessor.paint( hwnd );
        break;
    default:
        return DefWindowProc( hwnd, message, wParam, lParam );
    }

    return 0;
}
