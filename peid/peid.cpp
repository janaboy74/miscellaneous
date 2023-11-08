// peid

#include <tchar.h>
#include <stdio.h>
#include <windows.h>

#define DATALEN 6

int _tmain( int argc, _TCHAR* argv[] )
{
  DWORD offset = 0;
  unsigned char data[ DATALEN ];

  if ( argc > 1 ) {

    FILE *fp = _tfopen( argv[ 1 ], TEXT( "rb" ));
    if ( fp != NULL ) {

      fread( data, DATALEN, 1, fp );

      // checking "MZ" signature
      if ( data[ 0 ] == 'M' && data[ 1 ] == 'Z' ) {

        fseek( fp, 0x3c, SEEK_SET );
        fread( data, DATALEN, 1, fp );

        offset = DWORD( data[ 0 ] ) + ( DWORD( data[ 1 ] ) << 8 ) + ( DWORD( data[ 2 ] ) << 16 ) + ( DWORD( data[ 3 ] ) << 24 );

        fseek( fp, offset, SEEK_SET );
        fread( data, DATALEN, 1, fp );

        // checking for "PE" signature
        if ( data[ 0 ] == 'P' && data[ 1 ] == 'E' && data[ 2 ] == 0 && data[ 3 ] == 0 ) {
          if ( data[ 4 ] == 0x4c && data[ 5 ] == 0x01 ) {
            _tprintf( TEXT( "32 bit executable\n" ));
          } else if ( data[ 4 ] == 0x64 && data[ 5 ] == 0x86 ) {
            _tprintf( TEXT( "64 bit executable\n" ));
          } else {
            _tprintf( TEXT( "unknown executable type\n" ));
          }
        } else {
          _tprintf( TEXT( "not a valid windows executable\n" ));
        }
      } else {
        _tprintf( TEXT( "not a valid windows executable\n" ));
      }
    } else {
      _tprintf( TEXT( "unable to load %s\n" ), argv[1] );
    }
  } else {
    _tprintf( TEXT( "Usage: %s [executable file name]\n" ), argv[0] );
  }
	return 0;
}
