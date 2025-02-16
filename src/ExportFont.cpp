#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iterator>
#include <list>
#include <string>
#include <vector>
#include <set>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"

#define PADDING_PX                  ( 1 )

struct FontFile
    {
    FontFile( const char *filename_w_path )
        {
        auto file = std::fopen( filename_w_path, "rb" );
        if( !file )
            {
            return;
            }

        std::fseek( file, 0, SEEK_END );
        auto file_sz = std::ftell( file );
        if( !file_sz )
            {
            std::fclose( file );
            return;
            }


        _data = static_cast<unsigned char*>( std::malloc( file_sz ) );
        if( !_data )
            {
            std::fclose( file );
            return;
            }

        std::fseek( file, 0, SEEK_SET );
        if( std::fread( _data, 1, file_sz, file ) != file_sz )
            {
            std::free( _data );
            _data = nullptr;
            return;
            }

        std::fclose( file );
        }

    ~FontFile()
        {
        std::free( _data );
        }

    unsigned char      *_data = nullptr;
    };


struct GlyphBitmap
    {
    GlyphBitmap( const char *filename, const float font_scale, const char glyph, stbtt_fontinfo &font )
        {
        _data = stbtt_GetCodepointBitmap( &font, 0, font_scale, (int)glyph, &_width, &_height, &_x_offset, &_y_offset );
        if( !_data )
            {
            print_error( "ExportFont_Export() could not query the bitmap for a font. file = (%s), glyph = (%c).", filename, glyph );
            return;
            }
        }

    ~GlyphBitmap()
        {
        stbtt_FreeBitmap( _data, nullptr );
        }

    unsigned char      *_data = nullptr;
    int                 _width;
    int                 _height;
    int                 _x_offset;
    int                 _y_offset;
    };

struct PackContext : stbtt_pack_context
    {
    ~PackContext() { stbtt_PackEnd( this ); }
    };


static void        AddAllAlphaGlyphs( bool add_lower, bool add_upper, std::set<char> &glyphs );
static void        AddAllNumericGlyphs( std::set<char> &glyphs );
static void        AddAllSpecial( std::set<char> &glyphs );
static bool        DetermineTextureDims( std::vector<GlyphBitmap> &bitmaps, int &tex_width, int &tex_height );
static std::string ParseGlyphString( const char *glyphs );
static bool        WriteToAssetFile( const AssetFileAssetId id, const std::string &asset_id_str, const uint8_t *pixels, const uint8_t oversample_x, const uint8_t oversample_y, const uint16_t width, const uint16_t height, const uint16_t glyph_cnt, stbtt_packedchar *glyphs, const std::string glyph_str, AssetFileWriter *output );


/*******************************************************************
*
*   ExportFont_Export()
*
*   DESCRIPTION:
*       Load the given texture by filename.
*
*******************************************************************/

bool ExportFont_Export( const AssetFileAssetId id, const char *asset_id_str, const char *filename, const int point_size, const char *glyphs, WriteStats &stats, AssetFileWriter *output )
{
stats = {};
size_t write_start_size = AssetFile_GetWriteSize( output );

/* Read font from disk */
FontFile font_data( filename );
if( !font_data._data )
    {
	print_error( "ExportFont_Export() could not read font from file (%s).", filename );
    return( false );
    }

stbtt_fontinfo font = {};
if( !stbtt_InitFont( &font, font_data._data, stbtt_GetFontOffsetForIndex( font_data._data, 0 ) ) )
    {
    print_error( "ExportFont_Export() could not initialize the font object from our file data (%s).", filename );
    return( false );
    }

/* Get the dimensions of the final texture */
float font_scale = stbtt_ScaleForPixelHeight( &font, (float)point_size );
std::string all_glyphs = ParseGlyphString( glyphs );
std::vector<GlyphBitmap> bitmaps;
bitmaps.reserve( all_glyphs.size() );
for( auto glyph : all_glyphs )
    {
    if( !glyph )
        {
        /* eat the null */
        continue;
        }

    bitmaps.emplace_back( filename, font_scale, glyph, font);
    }

if( bitmaps.size() != all_glyphs.length() )
    {
    return( false );
    }

int tex_width = 0;
int tex_height = 0;
if( !DetermineTextureDims( bitmaps, tex_width, tex_height ) )
    {
    print_error( "ExportFont_Export() could not determine font texture size, likely the maximum texture size needs increased for a large font. font = (%s), point = (%d).", filename, point_size );
    return( false );
    }
else if( !tex_width
      || !tex_height )
    {
    print_info( "ExportFont_Export() resolved font (%s), point (%d) to zero sized texture.  Skipping...", filename, point_size );
    return( true );
    }

/* render the font glyphs into a texture */
unsigned int oversample_x = 1;
unsigned int oversample_y = 1;
if( point_size < 30 )
    {
    /* oversample small fonts */
    oversample_x = 2;
    oversample_y = 2;

    tex_width  *= oversample_x;
    tex_height *= oversample_y;
    }

dyn_array<unsigned char> final_texture( tex_width * tex_height );
PackContext rect_pack = {};
if( !stbtt_PackBegin( &rect_pack, final_texture.data(), tex_width, tex_height, 0, PADDING_PX, nullptr) )
    {
    print_error( "ExportFont_Export() failed to start packing atlas. font = (%s), point = (%d).", filename, point_size );
    return( false );
    }

stbtt_PackSetOversampling( &rect_pack, oversample_x, oversample_y );

dyn_array<stbtt_packedchar> char_data( all_glyphs.size() );
std::vector<stbtt_pack_range> ranges;
ranges.reserve( all_glyphs.size() );
std::list<unsigned char> glyphs_to_process;
std::copy( all_glyphs.begin(), all_glyphs.end(), std::back_inserter( glyphs_to_process ) );
while( glyphs_to_process.size() )
    {
    auto c = glyphs_to_process.front();

    if( ranges.empty()
     || ranges.back().first_unicode_codepoint_in_range + ranges.back().num_chars != c )
        {
        auto i = all_glyphs.size() - glyphs_to_process.size();

        ranges.push_back( {} );
        auto &range = ranges.back();
        range.first_unicode_codepoint_in_range = c;
        range.chardata_for_range = char_data.data() + i;
        range.font_size = (float)point_size;
        }

    auto &range = ranges.back();
    range.num_chars++;

    glyphs_to_process.pop_front();
    }

dyn_array<stbrp_rect> rects( all_glyphs.size() );
if( !stbtt_PackFontRangesGatherRects( &rect_pack, &font, ranges.data(), (int)ranges.size(), rects.data() ) )
    {
    print_error( "ExportFont_Export() failed to pack the atlas. font = (%s), point = (%d).", filename, point_size );
    return( false );
    }

stbtt_PackFontRangesPackRects( &rect_pack, rects.data(), (int)rects.size() );
for( auto &rect : rects )
    {
    assert( rect.was_packed );
    }

if( !stbtt_PackFontRangesRenderIntoRects( &rect_pack, &font, ranges.data(), (int)ranges.size(), rects.data() ) )
    {
    print_error( "ExportFont_Export() failed to render the atlas. font = (%s), point = (%d).", filename, point_size );
    return( false );
    }

bool has_data = false;
for( auto i = 0; i < (int)final_texture.size(); i++ )
    {
    has_data |= !!final_texture[ i ];
    }

assert( has_data );

int space_advance = 0;
stbtt_GetCodepointHMetrics( &font, ' ', &space_advance, nullptr );
char_data.push_back({});
auto &space_char_data = char_data.back();
space_char_data.xadvance = (float)space_advance * font_scale;
all_glyphs.push_back( ' ' );

/* Add it to the asset binary */
assert( char_data.size() == all_glyphs.size() );
if( !WriteToAssetFile( id, strip_filename( asset_id_str ), (uint8_t*)final_texture.data(), (uint8_t)oversample_x, (uint8_t)oversample_y, (uint16_t)tex_width, (uint16_t)tex_height, (uint16_t)char_data.size(), char_data.data(), all_glyphs.c_str(), output ) ) // TODO <MPA> : KERNING!!! :)
    {
    return( false );
    }

size_t write_total_size = AssetFile_GetWriteSize( output ) - write_start_size;
stats.written_sz += write_total_size;
print_info( "[FONT]      %s     glyphs: %d, dimensions: (%d x %d), %d bytes.", strip_filename( filename ).c_str(), (int)char_data.size(), tex_width, tex_height, (int)write_total_size );

return( true );

} /* ExportFont_Export() */


/*******************************************************************
*
*   AddAllAlphaGlyphs()
*
*******************************************************************/

static void AddAllAlphaGlyphs( bool add_lower, bool add_upper, std::set<char> &glyphs )
{
unsigned char start = 65;
unsigned char count = 26;

if( add_lower )
    {
    for( unsigned char i = 0; i < count; i++ )
        {
        glyphs.insert( char_make_lowercase( start + i ) );
        }
    }
    
if( add_upper )
    {
    for( unsigned char i = 0; i < count; i++ )
        {
        glyphs.insert( start + i );
        }
    }

}   /* AddAllAlphaGlyphs() */


/*******************************************************************
*
*   AddAllNumericGlyphs()
*
*******************************************************************/

static void AddAllNumericGlyphs( std::set<char> &glyphs )
{
unsigned char start = 48;
unsigned char count = 10;
for( unsigned char i = 0; i < count; i++ )
    {
    glyphs.insert( start + i );
    }

}   /* AddAllNumericGlyphs() */


/*******************************************************************
*
*   AddAllSpecial()
*
*******************************************************************/

static void AddAllSpecial( std::set<char> &glyphs )
{
glyphs.insert( '!' );
glyphs.insert( '#' );
glyphs.insert( '%' );
glyphs.insert( '-' );
glyphs.insert( '+' );
glyphs.insert( '.' );
glyphs.insert( ',' );
glyphs.insert( '?' );
glyphs.insert( ':' );
glyphs.insert( '\'' );
glyphs.insert( '\"' );
glyphs.insert( '\\' );
glyphs.insert( '/' );

}   /* AddAllSpecial() */


/*******************************************************************
*
*   DetermineTextureDims()
*
*******************************************************************/

static bool DetermineTextureDims( std::vector<GlyphBitmap> &bitmaps, int &tex_width, int &tex_height )
{
tex_width = 0;
tex_height = 0;
if( !bitmaps.size() )
    {
    return( true );
    }

int try_width = 2048;
int try_height = 2048;

std::vector<stbrp_rect> rects;
for( auto &bitmap : bitmaps )
    {
    rects.push_back( {} );
    auto &rect = rects.back();
    rect.w = bitmap._width + PADDING_PX;
    rect.h = bitmap._height + PADDING_PX;
    }

/* stbtt doesn't seem to have a good method of determining the proper size for textures - so we'll just discover it via trial and error */
stbrp_context context = {};
bool failed;
do
    {
    dyn_array<stbrp_node> nodes( try_width - PADDING_PX );
    stbrp_init_target( &context, try_width - PADDING_PX, try_height - PADDING_PX, nodes.data(), (int)nodes.size() );

    failed = !stbrp_pack_rects( &context, rects.data(), (int)rects.size() );
    if( !failed )
        {
        tex_width = try_width;
        tex_height = try_height;

        if( try_height > try_width )
            {
            try_height >>= 1;
            }
        else
            {
            try_width >>= 1;
            }
        }

    for( auto &rect : rects )
        {
        assert( failed || rect.was_packed );
        }

    } while( !failed
          && try_width
          && try_height );

return( tex_height
     && tex_width );

}   /* DetermineTextureDims() */


/*******************************************************************
*
*   ParseGlyphString()
*
*   DESCRIPTION:
*       Parse the given raw glyph string from the JSON file, in
*       order to handle special codes.
*
*******************************************************************/

static std::string ParseGlyphString( const char *glyphs )
{
std::string input( glyphs );
std::set<char> glyph_set;
std::string keyword;
size_t find_pos = {};

/* all special */
keyword = std::string( "__all_special" );
find_pos = input.find( keyword );
if( find_pos != std::string::npos )
    {
    AddAllSpecial( glyph_set );
    input.erase( find_pos, keyword.length() );
    }

/* all numeric */
keyword = std::string( "__all_numeric" );
find_pos = input.find( keyword );
if( find_pos != std::string::npos )
    {
    AddAllNumericGlyphs( glyph_set );
    input.erase( find_pos, keyword.length() );
    }

/* all alpha */
keyword = std::string( "__all_alpha" );
find_pos = input.find( keyword );
if( find_pos != std::string::npos )
    {
    AddAllAlphaGlyphs( true, true, glyph_set );
    input.erase( find_pos, keyword.length() );
    }

/* all uppercase alpha */
keyword = std::string( "__all_upper" );
find_pos = input.find( keyword );
if( find_pos != std::string::npos )
    {
    AddAllAlphaGlyphs( false, true, glyph_set );
    input.erase( find_pos, keyword.length() );
    }

/* all lowercase alpha */
keyword = std::string( "__all_lower" );
find_pos = input.find( keyword );
if( find_pos != std::string::npos )
    {
    AddAllAlphaGlyphs( true, false, glyph_set );
    input.erase( find_pos, keyword.length() );
    }

/* all */
keyword = std::string( "__all" );
find_pos = input.find( keyword );
if( find_pos != std::string::npos )
    {
    AddAllAlphaGlyphs( true, true, glyph_set );
    AddAllNumericGlyphs( glyph_set );
    AddAllSpecial( glyph_set );

    input.erase( find_pos, keyword.length() );
    }

/* remaining glyphs */
for( auto c : input )
    {
    glyph_set.insert( c );
    }

glyph_set.erase( ' ' );

input.clear();
for( auto c : glyph_set )
    {
    input.push_back( c );
    }

std::sort( input.begin(), input.end() );

return( input );

} /* ParseGlyphString() */


/*******************************************************************
*
*   WriteToAssetFile()
*
*******************************************************************/

static bool WriteToAssetFile( const AssetFileAssetId id, const std::string &asset_id_str, const uint8_t *pixels, const uint8_t oversample_x, const uint8_t oversample_y, const uint16_t width, const uint16_t height, const uint16_t glyph_cnt, stbtt_packedchar *glyphs, const std::string glyph_str, AssetFileWriter *output )
{
if( !AssetFile_BeginWritingAsset( id, ASSET_FILE_ASSET_KIND_FONT, output ) )
    {
    print_error( "ExportFont_Export() could not begin writing asset.  Reason: Asset was not in file table (%s).", asset_id_str.c_str() );
    return( false );
    }

if( !AssetFile_DescribeFont( oversample_x, oversample_y, width, height, width * height * sizeof( *pixels ), pixels, glyph_cnt, (unsigned char*)glyph_str.c_str(), output ) )
    {
	print_error( "ExportFont_Export() could not write font header (%s).", asset_id_str.c_str() );
	return( false );
    }

for( auto i = 0; i < glyph_cnt; i++ )
    {
    auto *box = &glyphs[ i ];
    if( !AssetFile_WriteFontGlyph( glyph_str[ i ], box->x0, box->y0, box->x1, box->y1, box->xoff, box->yoff, box->xadvance, output ) )
        {
        print_error( "ExportFont_Export() failed to write a glyph's character data (%s), for character (%c).", asset_id_str.c_str(), glyph_str[ i ] );
        return( false );
        }
    }

if( !AssetFile_EndWritingFont( output ) )
    {
    print_error( "ExportFont_Export() failed to end writing a font (%s).", asset_id_str.c_str() );
    return( false );
    }

return( true );

}   /* WriteToAssetFile() */
