#pragma once

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <memory>
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


/*******************************************************************
*
*   str_contains_str()
*
*   DESCRIPTION:
*       Does the given string contain the search string?
*
*******************************************************************/

static __inline bool str_contains_str( const char *str, const bool case_insensitive, const char *search )
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