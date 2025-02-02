#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

typedef struct _WriteStats
    {
    size_t              written_sz;
    uint32_t            models_written;
    uint32_t            materials_written;
    uint32_t            meshes_written;
    uint32_t            nodes_written;
    uint32_t            shaders_written;
    uint32_t            textures_written;
    uint32_t            sound_samples_written;
    uint32_t            music_clips_written;
    } WriteStats;

/*******************************************************************
*
*   ensure()
*
*   DESCRIPTION:
*       Evaulate the expression, and if debug-mode also assert.
*
*******************************************************************/

#if defined( _DEBUG )
#define ensure( _expression ) \
    assert( _expression )
#else
#define ensure( _expression ) \
    (void)( _expression )
#endif

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
*       Print a warning message.
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
*   print_info()
*
*   DESCRIPTION:
*       Print an info message.
*
*******************************************************************/

static inline void print_info( const char *str, ... )
{
printf( "[ResourcePackager] - " );
va_list args;
va_start( args, str );

vprintf( str, args );
printf( "\n" );

va_end( args );

} /* print_info() */


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