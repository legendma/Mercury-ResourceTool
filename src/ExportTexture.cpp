#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "AssetFile.hpp"
#include "ExportTexture.hpp"
#include "ResourceUtilities.hpp"


/*******************************************************************
*
*   ExportTexture_Export()
*
*   DESCRIPTION:
*       Load the given texture by filename.
*
*******************************************************************/

bool ExportTexture_Export( const AssetFileAssetId id, const char *filename, AssetIdToExtentMap &extent_map, WriteStats *stats, AssetFileWriter *output )
{
*stats = {};
size_t write_start_size = AssetFile_GetWriteSize( output );
int width = {};
int height = {};
int channel_count = {};

unsigned char *image = stbi_load( filename, &width, &height, &channel_count, 0 );
if( !image )
    {
    print_error( "ExportTexture_Export() could not read image from file (%s).", filename );
    return( false );
    }

int written_length = {};

assert( extent_map.find( id ) == extent_map.end() );
extent_map[ id ] = { (uint16_t)width, (uint16_t)height };
//unsigned char *png = stbi_write_png_to_mem( image, 0, width, height, channel_count, &written_length );
//stbi_image_free( image );
//if( !png )
//    {
//    print_error( "ExportTexture_Export() could not convert the original image to PNG form (%s).", filename );
//    return( false );
//    }
//
//if( !AssetFile_BeginWritingAsset( id, ASSET_FILE_ASSET_KIND_TEXTURE, output ) )
//	{
//    free( png );
//	print_error( "ExportTexture_Export() could not begin writing asset.  Reason: Asset was not in file table (%s).", filename );
//	return( false );
//	}
//
//if( !AssetFile_DescribeTexture( written_length, output )
// || !AssetFile_WriteTexture( png, written_length, output ) )
//    {
//    free( png );
//    print_error( "ExportTexture_Export could not write texture asset header to binary (%s).", filename );
//    return( false );
//    }
//
//free( png );

if( !AssetFile_BeginWritingAsset( id, ASSET_FILE_ASSET_KIND_TEXTURE, output ) )
    {
    print_error( "ExportTexture_Export() could not begin writing asset.  Reason: Asset was not in file table (%s).", filename );
    return( false );
    }

int pixels_length = width * height * channel_count;
if( !AssetFile_DescribeTexture2( channel_count, width, height, pixels_length, output )
 || !AssetFile_WriteTexture( image, pixels_length, output ) )
    {
    print_error( "ExportTexture_Export could not write texture asset header to binary (%s).", filename );
    return( false );
    }


size_t write_total_size = AssetFile_GetWriteSize( output ) - write_start_size;
stats->written_sz += write_total_size;
print_info( "[TEXTURE]   %s     %d bytes.", strip_filename( filename ).c_str(), (int)write_total_size );

return( true );

} /* ExportTexture_Export() */


/*******************************************************************
*
*   ExportTexture_WriteTextureExtents()
*
*   DESCRIPTION:
*       Write the map of texture asset ID's to their width/height.
*
*******************************************************************/

bool ExportTexture_WriteTextureExtents( AssetIdToExtentMap &extent_map, AssetFileWriter *output )
{
if( !AssetFile_BeginWritingAsset( ASSET_FILE_TEXTURE_EXTENT_ASSET_ID, ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS, output ) )
    {
    print_error( "ExportTexture_WriteTextureExtents() could not begin writing texture extent map." );
    return( false );
    }

if( !AssetFile_DescribeTextureExtents( (uint16_t)extent_map.size(), output ) )
    {
    print_error( "ExportTexture_WriteTextureExtents() could not describe the texture extent map." );
    return( false );
    }

for( auto &extent : extent_map )
    {
    AssetFile_WriteTextureExtent( extent.first, extent.second.width, extent.second.height, output );
    }

if( !AssetFile_EndWritingTextureExtents( output ) )
    {
    print_error( "ExportTexture_WriteTextureExtents() could not finish writing the texture extent map." );
    return( false );
    }

return( true );

}   /* ExportTexture_WriteTextureExtents() */
