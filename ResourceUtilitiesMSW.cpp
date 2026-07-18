#include <windows.h>
#include <io.h>
#include <string>

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
CreateDirectoryA( name, 0 );

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
