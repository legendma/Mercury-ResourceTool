#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

/*******************************************************************
*
*   print_error()
*
*   DESCRIPTION:
*       Print an error message.
*
*******************************************************************/

static inline void print_error( const char *str, ... )
{
va_list args;
va_start( args, str );

printf( "ERROR - [ResourcePackager] - " );
vprintf( str, args );;
printf( "\n" );

va_end( args );

} /* print_error() */


/*******************************************************************
*
*   print_warning()
*
*   DESCRIPTION:
*       Print an warning message.
*
*******************************************************************/

static inline void print_warning( const char *str, ... )
{
va_list args;
va_start( args, str );

printf( "Warning - [ResourcePackager] - " );
vprintf( str, args );
printf( "\n" );

va_end( args );

} /* print_warning() */


/*******************************************************************
*
*   strip_filename()
*
*   DESCRIPTION:
*       Strip a string down to just the filename (no path).
*
*******************************************************************/

static inline std::string strip_filename( const char *in )
{
std::string ret( in );

size_t last_slash = ret.find_last_of( "/\\" );
if( last_slash != std::string::npos )
    {
    ret = ret.substr( last_slash + 1, ret.size() );
    }

return( ret );

} /* strip_filename() */