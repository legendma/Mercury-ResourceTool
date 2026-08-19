#pragma once

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

typedef struct _WriteStats
    {
    size_t              written_sz;
    uint32_t            fonts_written;
    uint32_t            models_written;
    uint32_t            materials_written;
    uint32_t            meshes_written;
    uint32_t            nodes_written;
    uint32_t            shaders_written;
    uint32_t            textures_written;
    uint32_t            sound_samples_written;
    uint32_t            music_clips_written;
    } WriteStats;


#define ASSET_STR_KIND_COLUMN_WIDTH "16"
#define ASSET_STR_FILENAME_COLUMN_WIDTH "25"
#define ASSET_STR_FORMAT_STRING "%-" ASSET_STR_KIND_COLUMN_WIDTH"s %-" ASSET_STR_FILENAME_COLUMN_WIDTH"s %s"

static inline std::string sprint_info2( const char *str, va_list args );

using free_signature = void(*)( void * );
template <typename T> struct free_ptr : std::unique_ptr<T, free_signature>
{
    free_ptr( void *ptr ) : std::unique_ptr<T, free_signature>( static_cast<T *>( ptr ), std::free ) {}
};

template <typename T> struct dyn_array : std::vector<T>
{
    dyn_array( size_t size ) { this->resize( size ); std::memset( this->data(), 0, sizeof( T ) * this->size() ); }
};

/*******************************************************************
*
*   char_is_letter_lowercase
*
*   DESCRIPTION:
*       Is the given character a lower-case letter?
*
*******************************************************************/

#define char_is_letter_lowercase( _char )                           \
    ( (_char) >= 0x61                                               \
   && (_char) <= 0x7a )


/*******************************************************************
*
*   char_is_letter_uppercase
*
*   DESCRIPTION:
*       Is the given character an upper-case letter?
*
*******************************************************************/

#define char_is_letter_uppercase( _char )                           \
    ( (_char) >= 0x41                                               \
   && (_char) <= 0x5a )


/*******************************************************************
*
*   char_make_lowercase
*
*   DESCRIPTION:
*       Convert the given uppercase character to lowercase.
*
*******************************************************************/

#define char_make_lowercase( _upper ) \
    ( (_upper) + 0x20 )


/*******************************************************************
*
*   char_make_uppercase
*
*   DESCRIPTION:
*       Convert the given lowercase character to uppercase.
*
*******************************************************************/

#define char_make_uppercase( _lower ) \
    ( (_lower) - 0x20 )


/*******************************************************************
*
*   _countof
*
*   DESCRIPTION:
*       Find count of array.
*
*******************************************************************/

#if !defined( _countof )
#define _countof( _arr ) \
    ( sizeof( _arr ) / sizeof( *( _arr ) ) )
#endif


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
*   print_info()
*
*   DESCRIPTION:
*       Print an info message to string.
*
*******************************************************************/

static inline void print_info( const char *str, ... )
{
va_list args;
va_start( args, str );
std::string info = sprint_info2( str, args );
va_end( args );

printf( info.c_str() );

} /* print_info() */


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
*   resolve_environments()
*
*   DESCRIPTION:
*       Resolve environment variables in a string.
*
*******************************************************************/

static inline std::string resolve_environments( const char *in )
{
std::string ret( in );

size_t a = ret.find_first_of( "%" );
while( a != std::string::npos )
    {
    size_t b = ret.find_first_of( "%", a + 1 );
    if( b == std::string::npos )
        {
        break;
        }

    std::string var = ret.substr( a + 1, b - ( a + 1 ) );
    std::string resolved( getenv( var.c_str() ) );
    std::stringstream ss;
    if( a )
        {
        ss << ret.substr( 0, a - 1 );
        }
    ss << resolved << ret.substr( b + 1, ret.size() );
    ret = ss.str();

    /* next */
    a = ret.find_first_of( "%" );
    }

return( ret );

} /* resolve_environments() */


/*******************************************************************
*
*   sprint_info()
*
*   DESCRIPTION:
*       Print an info message to string.
*
*******************************************************************/

static inline std::string sprint_info( const char *str, ... )
{
va_list args;
va_start( args, str );
std::string ret = sprint_info2( str, args );
va_end( args );

return( ret );

} /* sprint_info() */


/*******************************************************************
*
*   sprint_info2()
*
*   DESCRIPTION:
*       Print an info message to string.
*
*******************************************************************/

static inline std::string sprint_info2( const char *str, va_list args )
{
std::string ret;
ret.append( "[ResourcePackager] - " );
va_list args2;
va_copy( args2, args );

int length = std::vsnprintf( NULL, 0, str, args ) + 1;
std::string format_str;
format_str.resize( length );

std::vsnprintf( format_str.data(), (size_t)length, str, args2 );
format_str.resize( length - 1 );

ret.append( format_str );
ret.append( "\n" );

return( ret );

} /* sprint_info2() */


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


/*******************************************************************
*
*   str_contains_str()
*
*   DESCRIPTION:
*       Does the given string contain the search string?
*
*******************************************************************/

static inline bool str_contains_str( const char *str, const bool case_insensitive, const char *search )
{
if( !search
 || search[ 0 ] == 0 )
    {
    assert( false );
    return( false );
    }

int search_i = 0;
for( int i = 0; str[ i ] != 0; i++ )
    {
    char this_char = str[ i ];
    char that_char = search[ search_i ];
    bool matches = ( this_char == that_char );

    /* handle case insensitivety */
    if( !matches
     && case_insensitive )
        {
        if( char_is_letter_lowercase( that_char ) )
            {
            that_char = char_make_uppercase( that_char );
            matches = ( this_char == that_char );
            }
        else if( char_is_letter_uppercase( that_char ) )
            {
            that_char = char_make_lowercase( that_char );
            matches = ( this_char == that_char );
            }
        }

    if( !matches )
        {
        search_i = 0;
        continue;
        }

    search_i++;
    if( search[ search_i ] == 0 )
        {
        return( true );
        }
    }

return( false );

} /* str_contains_str() */

void create_dir( const char *name );
bool does_file_exist( const char *filename );
std::string get_current_dir_str();