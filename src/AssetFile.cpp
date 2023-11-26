#include <cassert>
#include <cstdio>

#include "AssetFile.hpp"

#define make_fourcc( _a, _b, _c, _d ) \
    ( ( _a << 0 ) | ( _b << 8 ) | ( _c << 16 ) | (_d << 24 ) )

static const uint32_t ASSET_FILE_MAGIC = make_fourcc( 'M', 'e', 'r', 'c' );

typedef struct _AssetFileHeader
    {
    uint32_t            magic;      /* magic sentinel number        */
    uint32_t            table_cnt;  /* number entries in table      */
    } AssetFileHeader;

typedef struct _AssetFileTableRow
    {
    AssetFileAssetId    id;         /* asset ID hash                */
    AssetFileAssetKind  kind;       /* type of asset                */
    uint32_t            starts_at;  /* file offset to start of asset*/
    } AssetFileTableRow;

typedef struct _ModelHeader
    {
    uint32_t            node_count; /* number of nodes              */
    uint32_t            mesh_count; /* number of meshes             */
    uint32_t            material_cnt;
                                    /* number unique materials      */
    uint32_t            root_node_element;
                                    /* element index of root node   */
    } ModelHeader;

typedef struct _ModelMaterialHeader
    {
    AssetFileModelMaterialBits
                        map_bits;   /* present texture maps         */
    } ModelMaterialHeader;

typedef struct _ModelMeshHeader
    {
    uint32_t            vertex_cnt; /* number of vertices           */
    uint32_t            material;   /* material element index       */
    } ModelMeshHeader;

typedef struct _ModelNodeHeader
    {
    uint32_t            node_count; /* number of child nodes        */
    uint32_t            mesh_count; /* number of child meshes       */
    } ModelNodeHeader;

typedef struct _ModelTableRow
    {
    uint32_t            starts_at;  /* file offset to element start */
    } ModelTableRow;

typedef struct _ShaderHeader
    {
    uint32_t            byte_size;  /* byte code blob size          */
    } ShaderHeader;

typedef struct _TextureHeader
    {
    uint32_t            width;      /* image width                  */
    uint32_t            height;     /* image height                 */
    } TextureHeader;

static bool JumpToAssetInTable( const AssetFileAssetId id, AssetFileWriter *output );


/*******************************************************************
*
*   AssetFile_BeginWritingAsset()
*
*   DESCRIPTION:
*       Start writing an asset of the given ID, by updating the
*       asset table to reflect the current file caret.
*
*******************************************************************/

bool AssetFile_BeginWritingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileWriter *output )
{
output->kind        = ASSET_FILE_ASSET_KIND_INVALID;
output->asset_start = 0;

if( !JumpToAssetInTable( id, output ) )
    {
    return( false );
    }

output->asset_start = output->caret;
output->kind        = kind;

AssetFileTableRow row = {};
row.id        = id;
row.kind      = kind;
row.starts_at = output->caret;

assert( fwrite( &row, 1, sizeof(row), output->fhnd ) == sizeof(row) );
if( fseek( output->fhnd, (long)output->caret, SEEK_SET ) )
    {
    return( false );
    }

return( true );

} /* AssetFile_BeginWritingAsset() */


/*******************************************************************
*
*   AssetFile_BeginWritingModelElement()
*
*   DESCRIPTION:
*       Begin writing a model element (mesh, material, etc) given
*       its index.
*
*******************************************************************/

bool AssetFile_BeginWritingModelElement( const AssetFileModelElementKind kind, const uint32_t element_index, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

uint32_t row_location = output->asset_start
                      + (uint32_t)sizeof(ModelHeader)
                      + element_index * (uint32_t)sizeof(ModelTableRow);

if( fseek( output->fhnd, row_location, SEEK_SET ) )
    {
    return( false );
    }

AssetFileTableRow row = {};
row.starts_at = output->caret;
assert( fwrite( &row, 1, sizeof(row), output->fhnd ) == sizeof(row) );

if( fseek( output->fhnd, output->caret, SEEK_SET ) )
    {
    return( false );
    }

return( true );

} /* AssetFile_BeginWritingAsset() */


/*******************************************************************
*
*   AssetFile_CreateForWrite()
*
*   DESCRIPTION:
*       Create a new file with the given filename and path, and
*       initialize asset ID the table
*
*******************************************************************/

bool AssetFile_CreateForWrite( const char *filename, const AssetFileAssetId *ids, const uint32_t ids_count, AssetFileWriter *output )
{
*output = {};

errno_t err = fopen_s( &output->fhnd, filename, "w+" );
if( err )
    {
    return( false );
    }

AssetFileHeader header = {};
header.magic     = ASSET_FILE_MAGIC;
header.table_cnt = ids_count;

assert( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

AssetFileTableRow row;
for( uint32_t i = 0; i < ids_count; i++ )
    {
    row = {};
    row.id = ids[ i ];

    assert( fwrite( &row, 1, sizeof(row), output->fhnd ) == sizeof(row) );
    }

output->table_cnt = header.table_cnt;
output->caret     = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_CreateForWrite() */


/*******************************************************************
*
*   AssetFile_DescribeModel()
*
*   DESCRIPTION:
*       Provide the number of table entries for the model under
*       write.
*
*******************************************************************/

bool AssetFile_DescribeModel( const uint32_t node_count, const uint32_t mesh_count, const uint32_t material_count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start
 || fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }

ModelHeader header = {};
header.node_count   = node_count;
header.mesh_count   = mesh_count;
header.material_cnt = material_count;

assert( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

uint32_t row_count = node_count + mesh_count + material_count;
ModelTableRow row = {};
for( uint32_t i = 0; i < row_count; i++ )
    {
    assert( fwrite( &row, 1, sizeof(row), output->fhnd ) == sizeof(row) );
    }

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_DescribeModel() */


/*******************************************************************
*
*   AssetFile_DescribeModelMaterial()
*
*   DESCRIPTION:
*       Provide the details of the material about to be written.
*
*******************************************************************/

bool AssetFile_DescribeModelMaterial( const AssetFileModelMaterialBits maps, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

ModelMaterialHeader header = {};
header.map_bits = maps;

assert( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_DescribeModelMaterial() */


/*******************************************************************
*
*   AssetFile_DescribeModelMesh()
*
*   DESCRIPTION:
*       Provide the details of the mesh about to be written.
*
*******************************************************************/

bool AssetFile_DescribeModelMesh( const uint32_t material_element_index, const uint32_t vertex_cnt, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

ModelMeshHeader header = {};
header.vertex_cnt = vertex_cnt;
header.material   = material_element_index;

assert( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_DescribeModelMesh() */


/*******************************************************************
*
*   AssetFile_DescribeModelNode()
*
*   DESCRIPTION:
*       Provide the details of the model node about to be written.
*
*******************************************************************/

bool AssetFile_DescribeModelNode( const uint32_t node_count, const uint32_t mesh_count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

ModelNodeHeader header = {};
header.node_count = node_count;
header.mesh_count = mesh_count;

assert( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_DescribeModelNode() */


/*******************************************************************
*
*   AssetFile_DescribeShader()
*
*   DESCRIPTION:
*       Provide the byte code size of the shader under write.
*
*******************************************************************/

bool AssetFile_DescribeShader( const uint32_t byte_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_SHADER
 || !output->asset_start
 || fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }

ShaderHeader header = {};
header.byte_size = byte_size;

assert( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_DescribeShader() */


/*******************************************************************
*
*   AssetFile_DescribeTexture()
*
*   DESCRIPTION:
*       Provide the dimensions of the texture under write.
*
*******************************************************************/

bool AssetFile_DescribeTexture( const uint32_t width, const uint32_t height, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !output->asset_start
 || fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }

TextureHeader header = {};
header.width  = width;
header.height = height;

assert( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_DescribeTexture() */


/*******************************************************************
*
*   AssetFile_EndWritingModel()
*
*   DESCRIPTION:
*       Finish writing a model by setting its root node.
*
*******************************************************************/

bool AssetFile_EndWritingModel( const uint32_t root_node_element, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

if( fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }

ModelHeader header = {};
assert( fread( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

header.root_node_element = root_node_element;
if( fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }
    
assert( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
if( fseek( output->fhnd, output->caret, SEEK_SET ) )
    {
    return( false );
    }

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( true );

} /* AssetFile_EndWritingModel() */


/*******************************************************************
*
*   AssetFile_GetWriteSize()
*
*   DESCRIPTION:
*       Get the current file written size.
*
*******************************************************************/

size_t AssetFile_GetWriteSize( const AssetFileWriter *output )
{
return( (size_t)output->caret );

} /* AssetFile_GetWriteSize() */


/*******************************************************************
*
*   AssetFile_OpenForRead()
*
*   DESCRIPTION:
*       Open the asset file for read-only.
*
*******************************************************************/

bool AssetFile_OpenForRead( const char *filename, AssetFileReader *input )
{
*input = {};
errno_t err = fopen_s( &input->fhnd, filename, "r" );
if( err )
    {
    return( false );
    }

AssetFileHeader file_header = {};
if( fread( &file_header, 1, sizeof(file_header), input->fhnd ) != sizeof(file_header) )
    {
    assert( fclose( input->fhnd ) == 0 );
    input->fhnd = 0;
    return( false );
    }

if( file_header.magic != ASSET_FILE_MAGIC )
    {
    assert( fclose( input->fhnd ) == 0 );
    input->fhnd = 0;
    return( false );
    }

input->table_cnt = file_header.table_cnt;

return( true );

} /* AssetFile_OpenForRead() */


/*******************************************************************
*
*   AssetFile_WriteModelMaterialTextureMaps()
*
*   DESCRIPTION:
*       Write the given mesh material texture map asset IDs.
*
*******************************************************************/

bool AssetFile_WriteModelMaterialTextureMaps( const AssetFileAssetId *asset_ids, const uint8_t count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

assert( fwrite( asset_ids, sizeof(*asset_ids), count, output->fhnd) == count );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_WriteModelMaterialTextureMaps() */


/*******************************************************************
*
*   AssetFile_WriteModelMeshVertex()
*
*   DESCRIPTION:
*       Write the given mesh vertex.
*
*******************************************************************/

bool AssetFile_WriteModelMeshVertex( const AssetFileVertex *vertex, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

assert( fwrite( vertex, 1, sizeof(*vertex), output->fhnd ) == sizeof(*vertex) );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_WriteModelMeshVertex() */


/*******************************************************************
*
*   AssetFile_WriteModelNodeChildElements()
*
*   DESCRIPTION:
*       Write the child node/mesh element indices for a node.
*
*******************************************************************/

bool AssetFile_WriteModelNodeChildElements( const AssetFileAssetId *asset_ids, const uint32_t count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

assert( fwrite( asset_ids, sizeof(*asset_ids), count, output->fhnd) == count );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_WriteModelNodeChildElements() */


/*******************************************************************
*
*   AssetFile_WriteShader()
*
*   DESCRIPTION:
*       Write the shader program blob to the asset binary.  This
*       also ends the asset writing session.
*
*******************************************************************/

bool AssetFile_WriteShader( const uint8_t *blob, const uint32_t blob_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_SHADER
 || !output->asset_start )
    {
    return( false );
    }

assert( fwrite( blob, sizeof(*blob), blob_size, output->fhnd ) == blob_size );

output->caret = (uint32_t)ftell( output->fhnd );

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( true );

} /* AssetFile_WriteShader() */


/*******************************************************************
*
*   AssetFile_WriteTexture()
*
*   DESCRIPTION:
*       Write the texture data to the asset binary.  This also ends
*       the asset writing session.
*
*******************************************************************/

bool AssetFile_WriteTexture( const uint8_t *image, const uint32_t image_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !output->asset_start )
    {
    return( false );
    }

assert( fwrite( image, sizeof(*image), image_size, output->fhnd) == image_size );

output->caret = (uint32_t)ftell( output->fhnd );

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( true );

} /* AssetFile_WriteTexture() */


/*******************************************************************
*
*   JumpToAssetInTable()
*
*   DESCRIPTION:
*       Do a binary search to find the asset id in the table,
*       leaving the current file position at the start of the table
*       row.
*
*******************************************************************/

static bool JumpToAssetInTable( const AssetFileAssetId id, AssetFileWriter *output )
{
long table_start = (long)sizeof(AssetFileHeader);
long row_stride  = (long)sizeof(AssetFileTableRow);

bool found_it = false;    
long remain = (long)output->table_cnt;
long middle;
long top = 0;
AssetFileTableRow row;
while( remain > 0 )
    {
    middle = top + remain / 2;
    remain /= 2;
    
    if( fseek( output->fhnd, table_start + middle * row_stride, SEEK_SET ) )
        {
        return( false );
        }

    row = {};
    if( fread( &row, 1, sizeof(row), output->fhnd ) != sizeof(row) )
        {
        return( false );
        }

    if( row.id == id )
        {
        found_it = true;
        if( fseek( output->fhnd, -row_stride, SEEK_CUR ) )
            {
            return( false );
            }

        break;
        }
    else if( row.id < id )
        {
        top = middle + 1;
        }
    }

return( found_it );

} /* JumpToAssetInTable() */

