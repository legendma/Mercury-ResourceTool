#include <cassert>
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <stringapiset.h>
#include <d3dcompiler.h>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"


/*******************************************************************
*
*   ExportShader_Export()
*
*   DESCRIPTION:
*       Load and compile the given shader.
*
*******************************************************************/

bool ExportShader_Export( const AssetFileAssetId id, const char *filename, const char *target, const char *entry_point, WriteStats *stats, AssetFileWriter *output )
{
*stats = {};

int wide_size =  ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0 );
WCHAR *wide_string = (WCHAR*)malloc( sizeof(WCHAR) *wide_size );
if( !wide_string )
    {
    assert( false );
    return( false );
    }

ensure( (size_t)::MultiByteToWideChar( CP_UTF8, 0, filename, -1, wide_string, wide_size ) == wide_size );

ID3DBlob *byte_code;
ID3DBlob *error_info;
HRESULT hr = D3DCompileFromFile( wide_string, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point, target, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &byte_code, &error_info );
if( error_info )
    {
    print_error( "ExportShader_Export() failed to compile shader (%s).", filename );
    printf( "%s\n", (char *)error_info->GetBufferPointer());
    error_info->Release();
    error_info = NULL;
    }

free( wide_string );
if( hr != S_OK )
    {
    return( false );
    }

size_t write_start_size = AssetFile_GetWriteSize( output );
if( !AssetFile_BeginWritingAsset( id, ASSET_FILE_ASSET_KIND_SHADER, output ) )
	{
    byte_code->Release();
    byte_code = NULL;
	print_error( "ExportShader_Export() could not begin writing asset.  Reason: Asset was not in file table (%s).", filename );
	return( false );
	}

if( !AssetFile_DescribeShader( (uint32_t)byte_code->GetBufferSize(), output )
 || !AssetFile_WriteShader( (uint8_t*)byte_code->GetBufferPointer(), (uint32_t)byte_code->GetBufferSize(), output ) )
    {
    byte_code->Release();
    byte_code = NULL;
    print_error( "ExportShader_Export could not write shader asset to binary (%s).", filename );
    return( false );
    }

byte_code->Release();
byte_code = NULL;

size_t write_total_size = AssetFile_GetWriteSize( output ) - write_start_size;
stats->written_sz += write_total_size;
print_info( "[SHADER]    %s     %d bytes.", strip_filename( filename ).c_str(), (int)write_total_size );

return( true );

} /* ExportShader_Export() */