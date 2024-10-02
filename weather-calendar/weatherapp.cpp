#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <vector>
#include <mutex>
#include <map>
#include <string>
#include <curl/curl.h>
#include <png.h>
#include <gtkmm.h>
#include <gtkmm/application.h>
#include <gtkmm/drawingarea.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/cursor.h>
#include <cairommconfig.h>
#include <cairomm/cairomm.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <cairomm/fontface.h>
#include "arrow_left.h"
#include "arrow_right.h"
#include "temperature.h"
#include "wind.h"
#include "pressure.h"
#include "visibility.h"
#include "garbage.h"

#define L_ENG 1
#define L_GER 2
#define L_HUN 3

#define USE_LANGUAGE L_ENG

using namespace std;

struct corestring : public string {
  corestring()
    : string() {
  }

  corestring( const string &src )
    : string( src ) {
  }

  corestring( const char *src )
    : string( src ) {
  }

  corestring( const char src )
    : string( &src, 1 ) {
  }

  void formatva( const char *format, va_list &arg_list ) {
    if( format ) {
      va_list cova;
      va_copy( cova, arg_list );
      int size = vsnprintf( NULL, 0, format, cova );
      va_end( cova );
      resize( size );
      va_copy( cova, arg_list );
      vsnprintf( &at( 0 ), size + 1, format, cova );
      va_end( cova );
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
  operator const char *() {
    return c_str();
  }
  bool begins( const char *start ) const {
    return !strncmp( c_str(), start, strlen( start ));
  }
};

class json
{
    map<string, string> properties;
public:
    json() {}
    void parse( const char *jsonText ) {
        vector<string> nodename;
        vector<string> text;
        string str;

        properties.clear();

        if( !jsonText )
            return;

        bool quote = false;
        for( auto ch = jsonText; *ch; ++ch ) {
            if( '"' == *ch ) {
                quote =!quote;
                continue;
            }
            if( quote ) {
                str += *ch;
                continue;
            }
            switch( *ch ) {
            case '[':
            case ']':
                continue;
            case '{':
            case '}':
            case ',':
            case ':':
                if( str.length() )
                    text.push_back( str );
                str.clear();
                text.push_back( corestring( *ch ));
                break;
            case ' ':
            case '\t':
                break;
            default:
                str += *ch;
                break;
            }
        }
        nodename.push_back( "main" );
        properties.clear();
        string name, val;
        while( text.size() ) {
            if( text.front() == "{" ) {
                text.erase( text.begin() );
                if( name.size() )
                    nodename.push_back( name );
                continue;
            }
            if( text.front() == "," ) {
                text.erase( text.begin() );
                continue;
            }
            if( text.front() == "}" ) {
                text.erase( text.begin() );
                nodename.pop_back();
                continue;
            }
            if( text.size() > 1 && text[1] == ":" ) {
                name = text.front(); text.erase( text.begin() ); text.erase( text.begin() );
                continue;
            }
            val = text.front(); text.erase( text.begin() );
            string fullname;
            for( auto curname : nodename ) {
                if( fullname.size() )
                    fullname += ".";
                fullname += curname;
            }
            if( fullname.size() )
                fullname += ".";
            fullname += name;
            properties.insert( pair<string, string>( fullname, val ));
        }
#if 0 // DEBUG
        for( auto prop : properties ) {
            printf( "%s = %s\n", prop.first.c_str(), prop.second.c_str() );
        }
#endif
    }
    string get( const char *nodename ) {
        if( properties.find( nodename ) != properties.end() )
            return properties[ nodename ];
        return " ";
    }
} json;

mutex weather_mutex;

class mutexLocker {
    mutex *mymutex;
public:
    mutexLocker( mutex &mutex ) : mymutex( &mutex ) {
        mymutex->lock();
    }
    ~mutexLocker() {
        mymutex->unlock();
    }
};

size_t curlreaddata( void *contents, size_t size, size_t nmemb, string *str ) {
    size_t newLength = size * nmemb;
    str->append(( char* ) contents, newLength );
    return newLength;
}

const char *weatherinfo =
"{"
"   \"coord\":{"
"      \"lon\":19.0399,"
"      \"lat\":47.498"
"   },"
"   \"weather\":["
"      {"
"         \"id\":800,"
"         \"main\":\"Clear\","
"         \"description\":\"clear sky\","
"         \"icon\":\"01d\""
"      }"
"   ],"
"   \"base\":\"stations\","
"   \"main\":{"
"      \"temp\":30.58,"
"      \"feels_like\":29.79,"
"      \"temp_min\":30.29,"
"      \"temp_max\":31.36,"
"      \"pressure\":1019,"
"      \"humidity\":35"
"   },"
"   \"visibility\":10000,"
"   \"wind\":{"
"      \"speed\":1.34,"
"      \"deg\":311,"
"      \"gust\":3.13"
"   },"
"   \"clouds\":{"
"      \"all\":0"
"   },"
"   \"dt\":1691938070,"
"   \"sys\":{"
"      \"type\":2,"
"      \"id\":2009313,"
"      \"country\":\"HU\","
"      \"sunrise\":1691897798,"
"      \"sunset\":1691949668"
"   },"
"   \"timezone\":7200,"
"   \"id\":3054643,"
"   \"name\":\"Budapest\","
"   \"cod\":200"
"}";

string getWeather() {
    string result;
    CURL *curl = curl_easy_init();

    curl_easy_setopt( curl, CURLOPT_CUSTOMREQUEST, "GET" );
    curl_easy_setopt( curl, CURLOPT_URL, "https://api.openweathermap.org/data/2.5/weather?q=Budapest&appid=[OpenWeatherMapApiKey]&units=metric" );

    curl_easy_setopt( curl, CURLOPT_USERAGENT, "libcurl-agent/1.0" );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curlreaddata );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &result );

    curl_easy_perform( curl );
    curl_easy_cleanup( curl );

    return result;
}

struct image {
    vector<unsigned char> imgbuff;
    Glib::RefPtr<Gdk::Pixbuf> img;
    Glib::RefPtr<Gdk::Pixbuf> readPngFromMemory( const unsigned char *file_buffer, int length ) {
        png_image image;

        memset( &image, 0, sizeof( image ));
        image.version = PNG_IMAGE_VERSION;
        if( png_image_begin_read_from_memory( &image, file_buffer, length ) == 0) {
            return img;
        }

        image.format = PNG_FORMAT_RGBA;
        imgbuff.resize( PNG_IMAGE_SIZE( image ));

        if( png_image_finish_read( &image, NULL, ( void * ) &*imgbuff.begin(), 0, NULL ) == 0) {
            return img;
        }

        img = Gdk::Pixbuf::create_from_data((const guint8 *) &*imgbuff.begin(), Gdk::Colorspace::COLORSPACE_RGB, true, 8, image.width, image.height, PNG_IMAGE_ROW_STRIDE( image ));

        return img;
    }
};

#if USE_LANGUAGE == L_ENG
const char *monthNames[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
#elif USE_LANGUAGE == L_GER
const char *monthNames[] = { "Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember" };
#elif USE_LANGUAGE == L_HUN
const char *monthNames[] = { "Január", "Február", "Március", "Április", "Május", "Június", "Július", "Augusztus", "Szeptember", "Október", "November", "December" };
#endif
#if USE_LANGUAGE == L_ENG
const char *weekdayNames[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
#elif USE_LANGUAGE == L_GER
const char *weekdayNames[] = { "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag", "	Sonntag" };
#elif USE_LANGUAGE == L_HUN
const char *weekdayNames[] = { "Hétfő", "Kedd", "Szerda", "Csütörtök", "Péntek", "Szombat", "Vasárnap" };
#endif
static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
static int mdays[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int dayNumber( int year, int month, int day ) {
    if( month > 12 || month < 1 )
        return 0;
    year -= month < 3;
    return ( year + year / 4 - year / 100 + year / 400 + t[ month - 1 ] + day + 6 ) % 7;
}

int monthDays( int year, int month ) {
    if( month > 12 || month < 1 )
        return 0;
    if( 2 == month )
        return  (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) ? 29 : 28;
    return mdays[ month - 1 ];
}

struct color {
    float r;
    float g;
    float b;
    color( float red, float green, float blue ) : r( red ), g( green ), b( blue ) {}
};

struct date {
    int year;
    int month;
} curdate;

void drawRect( const Cairo::RefPtr<Cairo::Context>& cr, int x, int y, int w, int h, color &bgcolor, bool flip = false ) {
    cr->set_line_width( 1 );
    color ct = color( 1, 1, 1 );
    color cb = color( 0.4, 0.4, 0.4 );
    color cc = bgcolor;
    if( flip )
        cr->set_source_rgb( cb.r, cb.g, cb.b );
    else
        cr->set_source_rgb( ct.r, ct.g, ct.b );
    cr->move_to( x        , y + h - 1 );
    cr->line_to( x        , y         );
    cr->line_to( x + w - 1, y         );
    cr->stroke();
    if( flip )
        cr->set_source_rgb( ct.r, ct.g, ct.b );
    else
        cr->set_source_rgb( cb.r, cb.g, cb.b );
    cr->move_to( x + w - 1, y         );
    cr->line_to( x + w - 1, y + h - 1 );
    cr->line_to( x        , y + h - 1 );
    cr->stroke();
    cr->set_source_rgb( cc.r, cc.g, cc.b );
    cr->rectangle( x + 1, y + 1, w - 3, h - 3 );
    cr->stroke_preserve();
    cr->fill();
}

struct touchpos {
    float x;
    float y;
};

struct point {
    int x;
    int y;
    point() : x( 0 ), y( 0 ) {}
    point( int _x, int _y ) : x( _x ), y( _y ) {}
    const point &operator -= ( const point &pnt ) {
        x -= pnt.x;
        y -= pnt.y;
        return *this;
    }
    float length() {
        return sqrt( x * x + y * y );
    }
};

struct rect : public point {
    int w;
    int h;
    rect() : point( 0, 0 ), w( 0 ), h( 0 ) {}
    rect( const rect &_rect ) : point( _rect.x, _rect.y ), w( _rect.w ), h( _rect.h ) {}
    rect( int _x, int _y, int _w, int _h ) : point( _x, _y ), w( _w ), h( _h ) {}
    bool inside( int _x, int _y ) const {
        return ( x <= _x && _x <= x + w && y <= _y && _y <= y + h );
    }
};

struct area : public rect {
    string areaName;
    area() : rect () {}
    area( string name, int _x, int _y, int _w, int _h ) : rect( _x, _y, _w, _h ), areaName( name ) {}
    area( string name, const rect &_rect ) : rect( _rect ), areaName( name ) {}
    bool operator < ( const area &other ) const {
        return areaName < other.areaName;
    }
};

set< area > areas;
map< corestring, string > areaEvents;

class MyArea : public Gtk::DrawingArea
{
public:
    MyArea() {
        arrow_left.readPngFromMemory( arrow_left_png, sizeof( arrow_left_png ));
        arrow_right.readPngFromMemory( arrow_right_png, sizeof( arrow_right_png ));
        temperature.readPngFromMemory( temperature_png, sizeof( temperature_png ));
        wind.readPngFromMemory( wind_png, sizeof( wind_png ));
        pressure.readPngFromMemory( pressure_png, sizeof( pressure_png ));
        visibility.readPngFromMemory( visibility_png, sizeof( visibility_png ));
        garbage.readPngFromMemory( garbage_png, sizeof( garbage_png ));
    };
    virtual ~MyArea(){ };
    bool buttonpressed;
    point clickpos;
    point mousepos;
protected:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override{
        mutexLocker lock( weather_mutex );

        Gtk::Allocation allocation = get_allocation();
        const int widget_width = allocation.get_width();
        const int widget_height = allocation.get_height();

        Cairo::TextExtents te;

        int font_size = widget_height / 22;
        int text_height = font_size * 1.15;
        int text_xpos = 0;
        int text_ypos = text_height * 1.5;

        time_t rawtime;
        struct tm timeinfo;
        static const long daysecs = 24 * 60 * 60;
        long selective = 1683064800;
        corestring clock;
        static long selected_day = 0;

        for( auto &event : areaEvents ) {
            if( "calendar_minus" == event.first ) {
                --curdate.month;
                if( curdate.month < 1 ) {
                    curdate.month = 12;
                    --curdate.year;
                }
            } else if( "calendar_plus" == event.first ) {
                ++curdate.month;
                if( curdate.month > 12 ) {
                    curdate.month = 1;
                    ++curdate.year;
                }
            } else if( "calendar_name" == event.first ) {
                time_t rawtime;
                struct tm timeinfo;
                time( &rawtime );
                localtime_r( &rawtime, &timeinfo );
                curdate.year = timeinfo.tm_year + 1900;
                curdate.month = timeinfo.tm_mon + 1;
            } else if( event.first.begins( "day." )) {
                string str = event.first;
                str.erase( 0, 4 );
                selected_day = atol( str.c_str() );
            }
        }
        areaEvents.clear();

        text_ypos = text_height * 1.4;
        time( &rawtime );

        long delta = ( rawtime - selective ) / daysecs;
        delta = 27 - ( delta % 28 );
        rawtime += delta * daysecs;
        localtime_r( &rawtime, &timeinfo );

        cr->set_font_size( font_size );

        Gdk::Cairo::set_source_pixbuf( cr, garbage.img, widget_width - 20 - garbage.img->get_width(), text_ypos - garbage.img->get_height() * 0.75 - text_height * 0.15  );
        cr->rectangle( 0, 0, garbage.img->get_width(), garbage.img->get_height() );
        cr->paint();
        cr->stroke();

        cr->select_font_face( "Bitstream Vera Sans",Cairo::FontSlant::FONT_SLANT_NORMAL, Cairo::FontWeight::FONT_WEIGHT_BOLD );
        cr->set_source_rgb( 0.2, 1, 0.5 );
        clock.format( "%04i.%02i.%02i", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to( widget_width - 50 - garbage.img->get_width() - te.width, text_ypos ); text_ypos += text_height;
        cr->show_text( clock.c_str() );

        text_ypos = text_height * 1.4;

        cr->set_source_rgb( 0.15, 0.4, 1 );
        clock.format( "%s", json.get( "main.weather.description" ).c_str() );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to( 60, text_ypos ); text_ypos += text_height;
        cr->show_text( clock.c_str() );

        Gdk::Cairo::set_source_pixbuf( cr, temperature.img, 20, text_ypos - temperature.img->get_height() * 0.75 - text_height * 0.15 );
        cr->rectangle( 0, 0, temperature.img->get_width(), temperature.img->get_height() );
        cr->paint();
        cr->stroke();

        cr->set_source_rgb( 0.15, 0.4, 1 );
        clock.format( "%s℃", json.get( "main.main.temp" ).c_str() );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to( temperature.img->get_width() + 30, text_ypos );
        cr->show_text( clock.c_str() );
        text_xpos = temperature.img->get_width() + 30 + te.width;

        cr->set_font_size( font_size * 0.5 );

        clock.format( "%s℃", json.get( "main.main.temp_max" ).c_str() );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to( text_xpos + 30, text_ypos - text_height * 0.5 + 4 ); text_ypos += text_height * 0.5;
        cr->show_text( clock.c_str() );

        clock.format( "%s℃", json.get( "main.main.temp_min" ).c_str() );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to( text_xpos + 30, text_ypos - text_height * 0.5 + 4 ); text_ypos += text_height * 0.5;
        cr->show_text( clock.c_str() );

        Gdk::Cairo::set_source_pixbuf( cr, wind.img, 20, text_ypos - wind.img->get_height() * 0.75 - text_height * 0.15  );
        cr->rectangle( 0, 0, wind.img->get_width(), wind.img->get_height() );
        cr->paint();
        cr->stroke();

        cr->set_font_size( font_size );
        cr->set_source_rgb( 0.15, 0.4, 1 );
        clock.format( "%s m/s", json.get( "main.wind.speed" ).c_str() );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to( wind.img->get_width() + 30, text_ypos ); text_ypos += text_height;
        cr->show_text( clock.c_str() );

        Gdk::Cairo::set_source_pixbuf( cr, pressure.img, 20, text_ypos - pressure.img->get_height() * 0.75 - text_height * 0.15  );
        cr->rectangle( 0, 0, pressure.img->get_width(), pressure.img->get_height() );
        cr->paint();
        cr->stroke();

        cr->set_source_rgb( 0.15, 0.4, 1 );
        clock.format( "%s hPa", json.get( "main.main.pressure" ).c_str() );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to( pressure.img->get_width() + 30, text_ypos ); text_ypos += text_height;
        cr->show_text( clock.c_str() );

        Gdk::Cairo::set_source_pixbuf( cr, visibility.img, 20, text_ypos - visibility.img->get_height() * 0.75 - text_height * 0.15  );
        cr->rectangle( 0, 0, visibility.img->get_width(), visibility.img->get_height() );
        cr->paint();
        cr->stroke();

        cr->set_source_rgb( 0.15, 0.4, 1 );
        clock.format( "%s m", json.get( "main.visibility" ).c_str() );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to( visibility.img->get_width() + 30, text_ypos ); text_ypos += text_height;
        cr->show_text( clock.c_str() );

    /*
        printf( "name: %s\n", json.get( "main.name" ).c_str());
        printf( "country: %s\n", json.get( "main.sys.country" ).c_str());
        printf( "wather condition: %s\n", json.get( "main.weather.description" ).c_str());
        printf( "temperature: %s\n", json.get( "main.main.temp" ).c_str());
        printf( "humidity: %s\n", json.get( "main.main.humidity" ).c_str());
        printf( "pressure: %s\n", json.get( "main.main.pressure" ).c_str());
        printf( "sunrise: %s\n", json.get( "main.sys.sunrise" ).c_str());
        printf( "sunset: %s\n", json.get( "main.sys.sunset" ).c_str());
        printf( "speed: %s\n", json.get( "main.wind.speed" ).c_str());
        printf( "tMin: %s\n", json.get( "main.main.temp_min" ).c_str());
        printf( "tMax: %s\n", json.get( "main.main.temp_max" ).c_str());
        printf( "visibility: %s\n", json.get( "main.visibility" ).c_str());
        printf( "windAngle: %s\n", json.get( "main.wind.deg" ).c_str());
        drawRect( cr, mousepos.x, mousepos.y, 10, 10, cl );
    */

        text_ypos = text_height * 1.2;

        cr->set_font_size( font_size );
        cr->set_source_rgb( 1, 1, 1 );
        clock.format( "%s(%s)", json.get( "main.name" ).c_str(), json.get( "main.sys.country" ).c_str() );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to(( widget_width - ( te.x_bearing + te.width )) / 2, text_ypos ); text_ypos += text_height * 1.2;
        cr->show_text( clock.c_str() );

        time( &rawtime );
        localtime_r( &rawtime, &timeinfo );

        cr->select_font_face( "Liberation Sans",Cairo::FontSlant::FONT_SLANT_NORMAL, Cairo::FontWeight::FONT_WEIGHT_BOLD );
        cr->set_source_rgb( 1, 1, 0.2 );
        clock.format( "%04i.%02i.%02i(%s)", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, weekdayNames[ dayNumber( timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday )]);
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to(( widget_width - ( te.x_bearing + te.width )) / 2, text_ypos ); text_ypos += text_height * 2.25;
        cr->show_text( clock.c_str() );

        cr->set_font_size( font_size * 2.5 );
        clock.format( "%02i:%02i:%02i", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to(( widget_width - ( te.x_bearing + te.width )) / 2, text_ypos ); text_ypos += text_height * 1.35;
        cr->show_text( clock.c_str() );

        cr->set_font_size( font_size );

        int rwidth = widget_width / 12;
        int rheight = widget_width / 24;
        int xpos = ( widget_width - rwidth * 7 ) / 2;
        int ypos = text_ypos - rheight * 0.3;


        color cl( 0.2, 0.2, 0.2 );

        drawRect( cr, xpos, ypos, rwidth * 7, rheight, cl );

        rect monthMinusRect( xpos + 1 + ( rheight - 2 - arrow_left.img->get_width() ) / 2, ypos + 1 + ( rheight - 2 - arrow_left.img->get_height() ) / 2, arrow_left.img->get_width() + 1, arrow_left.img->get_height() + 1 );
        areas.insert( area( "calendar_minus", monthMinusRect ));
        drawRect( cr, monthMinusRect.x, monthMinusRect.y, monthMinusRect.w, monthMinusRect.h, cl, buttonpressed && monthMinusRect.inside( clickpos.x, clickpos.y ));
        Gdk::Cairo::set_source_pixbuf( cr, arrow_left.img, monthMinusRect.x, monthMinusRect.y + ( monthMinusRect.h - arrow_left.img->get_height() ) / 2 );
        cr->rectangle( 0, 0, arrow_left.img->get_width(), arrow_left.img->get_height() );
        cr->paint();
        cr->stroke();

        rect monthPlusRect( xpos - 1 + rwidth * 7 - arrow_right.img->get_width() - ( rheight - 2 - arrow_right.img->get_width() ) / 2 , ypos + 1 + ( rheight - 2 - arrow_right.img->get_height() ) / 2, arrow_right.img->get_width() + 1, arrow_right.img->get_height() + 1 );
        areas.insert( area( "calendar_plus", monthPlusRect ));
        drawRect( cr, monthPlusRect.x, monthPlusRect.y, monthPlusRect.w, monthPlusRect.h, cl, buttonpressed && monthPlusRect.inside( clickpos.x, clickpos.y ));
        Gdk::Cairo::set_source_pixbuf( cr, arrow_right.img, monthPlusRect.x, monthPlusRect.y + ( monthPlusRect.h - arrow_right.img->get_height() ) / 2 );
        cr->rectangle( 0, 0, arrow_right.img->get_width(), arrow_right.img->get_height() );
        cr->paint();
        cr->stroke();

        rect monthNameRect( xpos + rheight + 1, ypos + 1 + ( rheight - 2 - arrow_left.img->get_height() ) / 2, rwidth * 7 - rheight * 2 - 1, arrow_right.img->get_height() + 1 );
        areas.insert( area( "calendar_name", monthNameRect ));
        drawRect( cr, monthNameRect.x, monthNameRect.y, monthNameRect.w, monthNameRect.h, cl, buttonpressed && monthNameRect.inside( clickpos.x, clickpos.y ));
        cr->set_font_size( font_size * 0.7 );
        clock.format( "%d - %s", curdate.year, monthNames[ curdate.month - 1 ]);
        cr->set_source_rgb( 0.8, 0.8, 0.5 );
        cr->get_text_extents( clock.c_str(), te );
        cr->move_to(( widget_width - ( te.x_bearing + te.width )) / 2, ypos + ( rheight + text_height * 0.35f ) / 2 );
        cr->show_text( clock.c_str() );
        ypos += rheight;

        rheight = widget_width / 32;
        cr->set_font_size( font_size * 0.4 );
        for( int day = 0; day < 7; ++day ) {
            color cl( 0.6, 0.6, 0.6 );
            drawRect( cr, xpos, ypos, rwidth, rheight, cl );
#if USE_LANGUAGE == L_HUN
            clock.format( weekdayNames[ day ]);
#else
            clock.format( weekdayNames[( day + 6 ) % 7 ]);
#endif
            cr->set_source_rgb( 0.1, 0.1, 0.5 );
            cr->get_text_extents( clock.c_str(), te );
            cr->move_to( xpos + ( rwidth - ( te.x_bearing + te.width )) / 2 - 1, ypos + ( rheight + text_height * 0.2f ) / 2 );
            cr->show_text( clock.c_str() );
            xpos += rwidth;
        }

        cr->set_font_size( font_size );
        xpos = ( widget_width - rwidth * 7 ) / 2;
        ypos += rheight;
        rheight = widget_width / 18;

        time( &rawtime );
        localtime_r( &rawtime, &timeinfo );

        int lastDay = monthDays( curdate.year, curdate.month );
#if USE_LANGUAGE == L_HUN
        int firstDayOfWeek = dayNumber( curdate.year, curdate.month, 1 );
#else
        int firstDayOfWeek = ( dayNumber( curdate.year, curdate.month, 1 ) + 1 ) % 7;
#endif
        drawRect( cr, xpos, ypos, rwidth * 7, rheight * (( firstDayOfWeek + lastDay + 6 ) / 7 ), cl );
        xpos = ( widget_width - rwidth * 7 ) / 2 + rwidth * firstDayOfWeek;
        for( int day = 1; day <= lastDay; ++day ) {
            color cl( 0.4, 0.4, 0.4 );
            if( firstDayOfWeek > 4 )
                cl = color( 0.6, 0.6, 0.2 );
            if( day == timeinfo.tm_mday && curdate.year == timeinfo.tm_year + 1900 && curdate.month == timeinfo.tm_mon + 1 )
                cl = color( 0.0, 0.7, 0.0 );
            rect dayRect( xpos, ypos, rwidth, rheight );
            clock.format( "day.%d", day );
            areas.insert( area( clock, dayRect ));
            drawRect( cr, xpos, ypos, rwidth, rheight, cl, buttonpressed && dayRect.inside( clickpos.x, clickpos.y ));
            cr->set_source_rgb( 0.1, 0.1, 0.5 );
            clock.format( "%d%s", day, day == selected_day ? "(x)" : "" );
            cr->get_text_extents( clock.c_str(), te );
            cr->move_to( xpos + ( rwidth - ( te.x_bearing + te.width )) / 2 - 1, ypos + 0.85 * ( rheight + text_height ) / 2 );
            cr->show_text( clock.c_str() );
            xpos += rwidth;
            if( ++firstDayOfWeek > 6 ) {
                firstDayOfWeek = 0;
                ypos += rheight;
                xpos = ( widget_width - rwidth * 7 ) / 2;
            }
        }

        cr->stroke();

        return true;
    };
public:
    bool onMousePress( GdkEventButton* button_event ) {
        buttonpressed = true;
        mousepos.x = clickpos.x = button_event->x;
        mousepos.y = clickpos.y = button_event->y;
        return true;
    }
    bool onMouseRelease( GdkEventButton* button_event ) {
        buttonpressed = false;
        point curpos( button_event->x, button_event->y );
        curpos -= clickpos;
        if( curpos.length() < 10 ) {
            for( auto &area : areas ) {
                if( area.inside( clickpos.x, clickpos.y )) {
                    areaEvents.insert( pair<string, string>( area.areaName, "clicked" ));
                }
            }
        }
        return true;
    }
    bool onMouseMove( GdkEventMotion* event_motion ) {
        mousepos.x = event_motion->x;
        mousepos.y = event_motion->y;
        return true;
    }
private:
    image arrow_left;
    image arrow_right;
    image temperature;
    image wind;
    image pressure;
    image visibility;
    image garbage;
};

class MyWindow : public Gtk::Window
{
    std::map<GdkEventSequence*, touchpos> touches;
public:

    MyWindow() {
        bool hasTouch = false;
        Glib::RefPtr<Gdk::DeviceManager> dev_manager = get_display()->get_device_manager();
        std::vector<Glib::RefPtr<Gdk::Device>> devices = dev_manager->list_devices( Gdk::DEVICE_TYPE_SLAVE );
        for( size_t i = 0; i < devices.size(); ++i ) {
            if( devices[i]->property_num_touches() > 0 ) {
                hasTouch = true;
            }
        }

        set_default_size(800, 800);

        auto css_provider = Gtk::CssProvider::create();
        css_provider->load_from_data( "* { background-image: none; background-color: #000000;}" );
        get_style_context()->add_provider( css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER );
        add( drawArea );
        refresh_weather_info();
        auto refreshconnection = Glib::signal_timeout().connect( sigc::mem_fun( *this, &MyWindow::on_refresh ), 1000 / 50 );
        //refreshconnection.disconnect();
        auto weatherrefreshconnection = Glib::signal_timeout().connect( sigc::mem_fun( *this, &MyWindow::refresh_weather_info ), 600000 );

        time_t rawtime;
        struct tm timeinfo;

        time( &rawtime );
        localtime_r( &rawtime, &timeinfo );

        curdate.year = timeinfo.tm_year + 1900;
        curdate.month = timeinfo.tm_mon + 1;

        add_events( Gdk::TOUCH_MASK | Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON_PRESS_MASK );
        signal_key_press_event().connect([&](GdkEventKey* event)->bool {
            if( event->keyval == GDK_KEY_Escape ) {
                close();
                return true;
            }
            return false;
        });

        if( hasTouch ) {
            signal_touch_event().connect([&](GdkEventTouch* event)->bool {
                touchpos touch;
                touch.x = event->x;
                touch.y = event->y;
                switch(event->type)
                {
                case GDK_TOUCH_BEGIN:
                    touches[ event->sequence ] = touch;
                    break;
                case GDK_TOUCH_UPDATE:
                    break;
                case GDK_TOUCH_END:
                    if( touches.find( event->sequence ) != touches.end() ) {
                        GdkEventButton button_event;
                        button_event.x = touches[ event->sequence ].x;
                        button_event.y = touches[ event->sequence ].y;
                        drawArea.onMousePress( &button_event );
                        button_event.x = event->x;
                        button_event.y = event->y;
                        drawArea.onMouseRelease( &button_event );
                        touches.erase(event->sequence);
                    }
                    break;
                case GDK_TOUCH_CANCEL:
                    if( touches.find( event->sequence ) != touches.end() ) {
                        touches.erase(event->sequence);
                    }
                    break;
                default:
                    break;
                }
                return GDK_EVENT_PROPAGATE;
            });
        }

        signal_button_press_event().connect([&](GdkEventButton* button_event)->bool {
            return drawArea.onMousePress( button_event );
        });
        signal_button_release_event().connect([&](GdkEventButton* button_event)->bool {
            return drawArea.onMouseRelease( button_event );
        });
        signal_motion_notify_event().connect([&](GdkEventMotion* event_motion)->bool {
            return drawArea.onMouseMove( event_motion );
        });
        //signal_motion_notify_event().connect([&](GdkEventTouch* event)->bool sigc::mem_fun(*this,&MyWindow::rotate));
        /* signal_event().connect([&](GdkEvent* event)->bool
        {
            //std::cout<<"EVENT: "<<event->type<<std::endl;
            return GDK_EVENT_PROPAGATE;
        });*/

        show_all();
        fullscreen();
        if( hasTouch ) {
            auto cursor = Gdk::Cursor::create( Gdk::Display::get_default(), "none" );
            get_window()->set_cursor( cursor );
        }
        //maximize();
    }
    bool refresh_weather_info()
    {
        mutexLocker lock( weather_mutex );
#if 1
        json.parse( getWeather().c_str() );
#else
        json.parse( weatherinfo );
#endif
        return TRUE;
    }

    bool on_refresh( ) {
        drawArea.queue_draw();
        return true;
    }
    MyArea drawArea;
private:
};

int main(int argc, char *argv[])
{
    auto app = Gtk::Application::create(argc, argv, "my.app");

    MyWindow window;

    return app->run(window);
}
