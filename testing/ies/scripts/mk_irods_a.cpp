#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>


using namespace std;

#define FILE_OPEN_ERR             (-900000)
#define FILE_WRITE_ERR            (-902000)
#define NO_PASSWORD_ENTERED       (-909000)
#define PASSWORD_EXCEEDS_MAX_SIZE (-903000)
#define UNABLE_TO_STAT_FILE       (-905000)

#define MAX_PATH_ALLOWED (1024)
#define MAX_NAME_LEN     (MAX_PATH_ALLOWED + 64)

#define MAX_PASSWORD_LEN (50)

#define AUTH_FILENAME ("/var/lib/irods/.irods/.irodsA")

int timeVal = 0;


static
void
obfiEncode( const char * const in, char * out, int const extra ) {
    int j;

    // Set up an array of characters that we will transpose.

    int wheel_len = 26 + 26 + 10 + 15;
    int wheel [26 + 26 + 10 + 15];

    j = 0;

    for ( int i = 0; i < 10; i++ ) {
        wheel[ j++ ] = ( int )'0' + i;
    }

    for ( int i = 0; i < 26; i++ ) {
        wheel[ j++ ] = ( int )'A' + i;
    }

    for ( int i = 0; i < 26; i++ ) {
        wheel[ j++ ] = ( int )'a' + i;
    }

    for ( int i = 0; i < 15; i++ ) {
        wheel[ j++ ] = ( int )'!' + i;
    }

    // get uid to use as part of the key
    int uid = getuid();
    uid = uid & 0xf5f;   // keep it fairly small and not exactly uid

    // get a pseudo random number
    struct timeval nowtime;
    ( void )gettimeofday( &nowtime, ( struct timezone * )0 );
    int rval = nowtime.tv_usec & 0xf;

    // and use it to pick a pattern for ascii offsets
    long seq = 0;

    if( rval == 0 ) {
        seq = 0xd768b678;
    }

    if( rval == 1 ) {
        seq = 0xedfdaf56;
    }

    if( rval == 2 ) {
        seq = 0x2420231b;
    }

    if( rval == 3 ) {
        seq = 0x987098d8;
    }

    if( rval == 4 ) {
        seq = 0xc1bdfeee;
    }

    if( rval == 5 ) {
        seq = 0xf572341f;
    }

    if( rval == 6 ) {
        seq = 0x478def3a;
    }

    if( rval == 7 ) {
        seq = 0xa830d343;
    }

    if( rval == 8 ) {
        seq = 0x774dfa2a;
    }

    if( rval == 9 ) {
        seq = 0x6720731e;
    }

    if( rval == 10 ) {
        seq = 0x346fa320;
    }

    if( rval == 11 ) {
        seq = 0x6ffdf43a;
    }

    if( rval == 12 ) {
        seq = 0x7723a320;
    }

    if( rval == 13 ) {
        seq = 0xdf67d02e;
    }

    if( rval == 14 ) {
        seq = 0x86ad240a;
    }

    if( rval == 15 ) {
        seq = 0xe76d342e;
    }

    // get the timestamp and other id
    int now = timeVal;  // from file, normally

    char headstring [10];
    headstring[ 1 ] = ( ( now >> 4 ) & 0xf ) + 'a';
    headstring[ 2 ] = ( now & 0xf ) + 'a';
    headstring[ 3 ] = ( ( now >> 12 ) & 0xf ) + 'a';
    headstring[ 4 ] = ( ( now >> 8 ) & 0xf ) + 'a';
    headstring[ 5 ] = '\0';
    headstring[ 0 ] = 'S' - ( ( rval & 0x7 ) * 2 );  // another check value

    *out++ = '.';  // store our initial, human-readable identifier

    int addin_i = 0;
    char const * my_in = headstring;  // start with head string

    for( int ii = 0;; ) {
        ii++;

        if( ii == 6 ) {
            *out++ = rval + 'e';  // store the key
            my_in = in;           // now start using the input string
        }

        int found = 0;
        int addin = ( seq >> addin_i ) & 0x1f;
        addin += extra;
        addin += uid;
        addin_i += 3;

        if( addin_i > 28 ) {
            addin_i = 0;
        }

        for( int i = 0; i < wheel_len; i++ ) {
            if( *my_in == ( char )wheel[ i ] ) {
                j = i + addin;
                j = j % wheel_len;
                *out++ = ( char )wheel[ j ];
                found = 1;
                break;
            }
        }

        if( found == 0 ) {
            if( *my_in == '\0' ) {
                *out++ = '\0';
                return;
            } else {
                *out++ = *my_in;
            }
        }

        my_in++;
    }
}


static
int
obfiOpenOutFile( const char * const fileName ) {
    int fd_out = open( fileName, O_CREAT | O_WRONLY | O_EXCL, 0600 );

    if ( fd_out < 0 ) {
        return FILE_OPEN_ERR;
    }

    return fd_out;
}


/* Set timeVal from a fstat of the file after writing to it. Could just use
 * system time, but if it's way off from the file system (NFS on a VM host, for
 * example), it would fail.
 */
static
int
obfiSetTimeFromFile( int const fd ) {
    int wval = write( fd, " ", 1 );

    if ( wval != 1 ) {
        return FILE_WRITE_ERR;
    }

    struct stat statBuf;
    int fval = fstat( fd, &statBuf );

    if ( fval < 0 ) {
        timeVal = 0;
        return UNABLE_TO_STAT_FILE;
    }

    int lval = lseek( fd, 0, SEEK_SET );

    if ( lval < 0 ) {
        return UNABLE_TO_STAT_FILE;
    }

    timeVal = statBuf.st_mtime & 0xffff;   // keep it bounded
    return 0;
}


static
int
obfiWritePw( int const fd, const char * const pw ) {
    int len = strlen( pw );
    int wval = write( fd, pw, len + 1 );

    if ( wval != len + 1 ) {
        return FILE_WRITE_ERR;
    }

    return 0;
}


/* char *pwArg; if non-0-length, this is the new password
 */
static
int
obfSavePw( char const * pwArg ) {
    int i = 0;

    char inbuf [MAX_PASSWORD_LEN + 100];
    strncpy( inbuf, pwArg, MAX_PASSWORD_LEN );

    i = strlen( inbuf );

    if ( i < 1 ) {
        return NO_PASSWORD_ENTERED;
    }

    if ( strlen( inbuf ) > MAX_PASSWORD_LEN - 2 ) {
        return PASSWORD_EXCEEDS_MAX_SIZE;
    }

    if ( inbuf[i - 1] == '\n' ) {
        inbuf[i - 1] = '\0';       // remove trailing \n
    }

    int fd = obfiOpenOutFile( AUTH_FILENAME );

    if ( fd < 0 ) {
        return FILE_OPEN_ERR;
    }

    if ( fd == 0 ) {
        return ( 0 );   /* user canceled */
    }

    i = obfiSetTimeFromFile( fd );

    if ( i < 0 ) {
        close( fd );
        return i;
    }

    char myPw [MAX_PASSWORD_LEN + 10];

    obfiEncode( inbuf, myPw, 0 );
    i = obfiWritePw( fd, myPw );

    close( fd );

    if ( i < 0 ) {
        return i;
    }

    printf( "Successfully wrote %s\n", AUTH_FILENAME );

    return 0;
}


int
main( int const argc, char * argv [] ) {
  if(argc < 2 ) {
    cerr << "Expected one argument" << endl;
    return EXIT_FAILURE;
  }

  string const password( argv[ 1 ] );

  return obfSavePw( password.c_str() ) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
