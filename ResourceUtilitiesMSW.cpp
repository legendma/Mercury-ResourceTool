#include <algorithm>
#include <windows.h>
#include <io.h>
#include <string>

#include "ResourceUtilities.hpp"

/*******************************************************************
*
*   create_dir()
*
*   DESCRIPTION:
*       Create the requested directory at the current directory.
*
*******************************************************************/

void create_dir( const char *name )
{

std::string path = name;
std::replace( path.begin(), path.end(), '/', '\\' );
if( path[ path.length() - 1 ] != '\\' )
    {
    path.append( "\\" );
    }

size_t pos = 0;
while( ( pos = path.find_first_of( "\\", pos ) ) != std::string::npos )
    {
    std::string subpath = path.substr( 0, pos );
    pos++;

    if( subpath.empty()
     || subpath[ subpath.length() - 1 ] == ':' )
        {
        continue;
        }

    if( !CreateDirectoryA( subpath.c_str(), 0 ) )
        {
        DWORD err = GetLastError();
        if( err == ERROR_ALREADY_EXISTS )
            {
            continue;
            }

        LPSTR message_buffer = NULL;
        size_t size = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                      NULL, 
                                      err, 
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                                      (LPSTR)&message_buffer,
                                      0, 
                                      NULL );

        if( size )
            {
            print_error( "create_dir() failed with error: %s", message_buffer );
            LocalFree( message_buffer );
            }
        }
    }

} /* create_dir() */


/*******************************************************************
*
*   does_file_exist()
*
*   DESCRIPTION:
*       Does the given file exist?
*
*******************************************************************/

bool does_file_exist( const char *filename )
{
#define ACCESS_CHECK_EXISTANCE      ( 0x00 )
int result = _access( filename, ACCESS_CHECK_EXISTANCE );

return( result != -1 );

#undef ACCESS_CHECK_EXISTANCE
} /* does_file_exist() */


/*******************************************************************
*
*   get_current_dir_str()
*
*   DESCRIPTION:
*       Query the current working directory as a string.
*
*******************************************************************/

std::string get_current_dir_str()
{
std::string ret;
ret.resize( 1024 );
GetCurrentDirectoryA( (DWORD)ret.capacity(), ret.data() );

return( ret );

} /* get_current_dir_str() */
