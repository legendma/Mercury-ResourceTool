#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"


/*******************************************************************
*
*   Texture_Load()
*
*   DESCRIPTION:
*       Load the given texture by filename.
*
*******************************************************************/

bool Texture_Load( const AssetFileAssetId id, const char *filename, WriteStats *stats, AssetFileWriter *output )
{
*stats = {};
size_t write_start_size = AssetFile_GetWriteSize( output );
int width = {};
int height = {};
int channel_count = {};

unsigned char *image = stbi_load( filename, &width, &height, &channel_count, 0 );
if( !image )
    {
    print_error( "Texture_Load() could not read image from file (%s).", filename );
    return( false );
    }

int written_length = {};
unsigned char *png = stbi_write_png_to_mem( image, 0, width, height, channel_count, &written_length );
stbi_image_free( image );
if( !png )
    {
    print_error( "Texture_Load() could not convert the original image to PNG form (%s).", filename );
    return( false );
    }

if( !AssetFile_BeginWritingAsset( id, ASSET_FILE_ASSET_KIND_TEXTURE, output ) )
	{
    free( png );
	print_error( "Texture_Load() could not begin writing asset.  Reason: Asset was not in file table (%s).", filename );
	return( false );
	}

if( !AssetFile_DescribeTexture( (uint32_t)width, (uint32_t)height, output )
 || !AssetFile_WriteTexture( png, written_length, output ) )
    {
    free( png );
    print_error( "Texture_Load could not write texture asset header to binary (%s).", filename );
    return( false );
    }

free( png );

size_t write_total_size = AssetFile_GetWriteSize( output ) - write_start_size;
stats->written_sz += write_total_size;
print_info( "[TEXTURE]   %s     %d bytes.", strip_filename( filename ).c_str(), (int)write_total_size );

return( true );

} /* Texture_Load() */