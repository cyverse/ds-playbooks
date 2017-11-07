/* This program takes a clear text password and generates a scrambled form
 * suitable for storing in an iRODS ICAT database. The first argument on the
 * command line should be the clear text password. The scrambled form is written
 * to standard output.
 *
 * This logic was taken from the iRODS source code.
 *
 * This executable requires libssl-dev installed.
 *
 * To build do execute the following.
 *
 * g++ scramble.cpp -lcrypto
 */

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/md5.h>

using namespace std;

static size_t const MAX_PASSWORD_LEN = 50;

typedef array< unsigned char, 4 * MD5_DIGEST_LENGTH > HashBufferType;


/* Generate a hash string using MD5 or Sha1
 */
template< typename InputIterator >
static
HashBufferType::iterator
obfMakeOneWayHash( InputIterator const first,
	                 size_t const length,
									 HashBufferType::iterator const hash ) {
    MD5_CTX md5Context;
    MD5_Init( &md5Context );
    MD5_Update( &md5Context, first, length );
    MD5_Final( hash, &md5Context );
    return hash + MD5_DIGEST_LENGTH;
}


/* Set up an array of characters that we will transpose.
 */
static
vector< char >
mk_wheel() {
	vector< char > wheel;

	for ( char i = '0'; i <= '9'; i++ ) {
			wheel.push_back( i );
	}

	for ( char i = 'A'; i <= 'Z'; i++ ) {
			wheel.push_back( i );
	}

	for ( char i = 'a'; i <= 'z'; i++ ) {
			wheel.push_back( i );
	}

	for ( char i = '!'; i <= '/'; i++ ) {
			wheel.push_back( i );
	}

  return wheel;
}


/* Obfuscate a string using the default key
*/
static
string
obfEncodeByKey( string const & password ) {
    string const PASSWORD_DEFAULT_KEY = "a9_3fker";

    array< unsigned char, 100 > keyBuf;
    keyBuf.fill( '\0' );
    copy( PASSWORD_DEFAULT_KEY.begin(), PASSWORD_DEFAULT_KEY.end(), keyBuf.begin() );

    // Get the MD5 digest of the key to get some bytes with many different
    // values.
		HashBufferType hashBuf;
    HashBufferType::iterator hashInitIter = hashBuf.begin();

    hashInitIter = obfMakeOneWayHash( keyBuf.cbegin(), keyBuf.size(), hashInitIter );

    // Hash of the hash
    hashInitIter = obfMakeOneWayHash( hashBuf.begin(), MD5_DIGEST_LENGTH, hashInitIter );

    // Hash of 2 hashes
    hashInitIter = obfMakeOneWayHash( hashBuf.begin(), 2 * MD5_DIGEST_LENGTH, hashInitIter );

    // Hash of 2 hashes
    hashInitIter = obfMakeOneWayHash( hashBuf.begin(), 2 * MD5_DIGEST_LENGTH, hashInitIter );

		vector< char > const wheel = mk_wheel();

    ostringstream scrambled;
    HashBufferType::const_iterator hashIter = hashBuf.cbegin();

    for ( string::const_iterator pwIter = password.begin();
          pwIter != password.end();
          pwIter++ ) {
        bool found = false;

        for ( size_t i = 0; i < wheel.size(); i++ ) {
            if ( *pwIter == wheel[ i ] ) {
                scrambled << wheel[ (i + *hashIter++) % wheel.size() ];
                found = true;
                break;
            }
        }

        if ( !found ) {
            scrambled << *pwIter;
        }

        if ( hashIter > hashBuf.cbegin() + 60 ) {
            hashIter = hashBuf.cbegin();
        }
    }

    return scrambled.str();
}


int
main( int const argc, char * argv [] ) {
    if( argc < 2 ) {
        cerr << "Expected one argument" << endl;
        return EXIT_FAILURE;
    }

    string const password( argv[ 1 ] );

    if( password.length() > MAX_PASSWORD_LEN - 2 )  {
        cerr << "Password exceeds maximum length" << endl;
	return EXIT_FAILURE;
    }

    cout << obfEncodeByKey( password );
    return EXIT_SUCCESS;
}
