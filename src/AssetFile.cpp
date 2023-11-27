#include <cassert>
#include <cstdio>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"

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
    uint32_t            total_vertex_count;
                                    /* number of vertices in model  */
    uint32_t            total_index_count;
                                    /* number of indices in model   */
    } ModelHeader;

typedef struct _ModelMaterialHeader
    {
    AssetFileModelMaterialBits
                        map_bits;   /* present texture maps         */
    } ModelMaterialHeader;

typedef struct _ModelMeshHeader
    {
    uint32_t            vertex_cnt; /* number of vertices           */
    uint32_t            index_cnt;  /* number of indices            */
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

static bool JumpToAssetInTable( const AssetFileAssetId id, const uint32_t table_count, FILE *file );


/*******************************************************************
*
*   AssetFile_BeginReadingAsset()
*
*   DESCRIPTION:
*       Start writing an asset of the given ID, by updating the
*       asset table to reflect the current file caret.
*
*******************************************************************/

bool AssetFile_BeginReadingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileReader *input )
{
input->kind        = ASSET_FILE_ASSET_KIND_INVALID;
input->asset_start = 0;

if( !JumpToAssetInTable( id, input->table_cnt, input->fhnd ) )
    {
    return( false );
    }

AssetFileTableRow row = {};
if( fread_s( &row, sizeof(row), 1, sizeof(row), input->fhnd ) != sizeof(row) )
    {
    return( false );
    }

assert( row.id == id );
if( row.kind != kind )
    {
    return( false );
    }

if( fseek( input->fhnd, row.starts_at, SEEK_SET ) )
    {
    return( false );
    }

input->caret = 0;
input->asset_start = row.starts_at;
input->kind = kind;

return( true );

} /* AssetFile_BeginReadingAsset() */


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

if( !JumpToAssetInTable( id, output->table_cnt, output->fhnd ) )
    {
    return( false );
    }

output->asset_start            = output->caret;
output->kind                   = kind;
output->model_indices_written  = 0;
output->model_vertices_written = 0;

AssetFileTableRow row = {};
row.id        = id;
row.kind      = kind;
row.starts_at = output->caret;

ensure( fwrite( &row, 1, sizeof(row), output->fhnd ) == sizeof(row) );
if( fseek( output->fhnd, (long)output->caret, SEEK_SET ) )
    {
    return( false );
    }

return( true );

} /* AssetFile_BeginWritingAsset() */


/*******************************************************************
*
*   AssetFile_CloseForRead()
*
*   DESCRIPTION:
*       Complete reading of the asset file.
*
*******************************************************************/

bool AssetFile_CloseForRead( AssetFileReader *input )
{
bool ret = !fclose( input->fhnd );
*input = {};

return( ret );

} /* AssetFile_CloseForRead() */


/*******************************************************************
*
*   AssetFile_CloseForWrite()
*
*   DESCRIPTION:
*       Complete writing for the asset file.
*
*******************************************************************/

bool AssetFile_CloseForWrite( AssetFileWriter *output )
{
bool ret = !fclose( output->fhnd );
*output = {};

return( ret );

} /* AssetFile_CloseForWrite() */


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
ensure( fwrite( &row, 1, sizeof(row), output->fhnd ) == sizeof(row) );

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

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

AssetFileTableRow row;
for( uint32_t i = 0; i < ids_count; i++ )
    {
    row = {};
    row.id = ids[ i ];

    ensure( fwrite( &row, 1, sizeof(row), output->fhnd ) == sizeof(row) );
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

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

uint32_t row_count = node_count + mesh_count + material_count;
ModelTableRow row = {};
for( uint32_t i = 0; i < row_count; i++ )
    {
    ensure( fwrite( &row, 1, sizeof(row), output->fhnd ) == sizeof(row) );
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

bool AssetFile_DescribeModelMesh( const uint32_t material_element_index, const uint32_t vertex_cnt, const uint32_t index_cnt, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

ModelMeshHeader header = {};
header.vertex_cnt = vertex_cnt;
header.index_cnt  = index_cnt;
header.material   = material_element_index;

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

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

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

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

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
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

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_DescribeTexture() */


/*******************************************************************
*
*   AssetFile_EndReadingModel()
*
*   DESCRIPTION:
*       Finish reading a model.
*
*******************************************************************/

bool AssetFile_EndReadingModel( AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start )
    {
    return( false );
    }

input->caret = 0;
input->asset_start = 0;
input->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( true );

} /* AssetFile_EndReadingModel() */


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
ensure( fread( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );

header.root_node_element  = root_node_element;
header.total_index_count  = output->model_indices_written;
header.total_vertex_count = output->model_vertices_written;
if( fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }
    
ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
if( fseek( output->fhnd, output->caret, SEEK_SET ) )
    {
    return( false );
    }

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;
output->model_indices_written = 0;
output->model_vertices_written = 0;

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
    ensure( fclose( input->fhnd ) == 0 );
    input->fhnd = 0;
    return( false );
    }

if( file_header.magic != ASSET_FILE_MAGIC )
    {
    ensure( fclose( input->fhnd ) == 0 );
    input->fhnd = 0;
    return( false );
    }

input->table_cnt = file_header.table_cnt;

return( true );

} /* AssetFile_OpenForRead() */


/*******************************************************************
*
*   AssetFile_ReadModelStorageRequirements()
*
*   DESCRIPTION:
*       Read the count of each of the model's elements.
*
*******************************************************************/

bool AssetFile_ReadModelStorageRequirements( uint32_t *vertex_count, uint32_t *index_count, uint32_t *mesh_count, uint32_t *node_count, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

ModelHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

if( vertex_count )
    {
    *vertex_count = header.total_vertex_count;
    }

if( index_count )
    {
    *index_count = header.total_index_count;
    }

if( mesh_count )
    {
    *mesh_count = header.mesh_count;
    }

if( node_count )
    {
    *node_count = header.node_count;
    }

return( true );

} /* AssetFile_ReadModelStorageRequirements() */


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

ensure( fwrite( asset_ids, sizeof(*asset_ids), count, output->fhnd) == count );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_WriteModelMaterialTextureMaps() */


/*******************************************************************
*
*   AssetFile_WriteModelMeshIndex()
*
*   DESCRIPTION:
*       Write the given mesh index.
*
*******************************************************************/

bool AssetFile_WriteModelMeshIndex( const AssetFileModelIndex index, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

ensure( fwrite( &index, 1, sizeof(index), output->fhnd ) == sizeof(index) );

output->caret = (uint32_t)ftell( output->fhnd );
output->model_indices_written++;

return( true );

} /* AssetFile_WriteModelMeshIndex() */


/*******************************************************************
*
*   AssetFile_WriteModelMeshVertex()
*
*   DESCRIPTION:
*       Write the given mesh vertex.
*
*******************************************************************/

bool AssetFile_WriteModelMeshVertex( const AssetFileModelVertex *vertex, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

ensure( fwrite( vertex, 1, sizeof(*vertex), output->fhnd ) == sizeof(*vertex) );

output->caret = (uint32_t)ftell( output->fhnd );
output->model_vertices_written++;

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

ensure( fwrite( asset_ids, sizeof(*asset_ids), count, output->fhnd) == count );

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

ensure( fwrite( blob, sizeof(*blob), blob_size, output->fhnd ) == blob_size );

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

ensure( fwrite( image, sizeof(*image), image_size, output->fhnd) == image_size );

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

static bool JumpToAssetInTable( const AssetFileAssetId id, const uint32_t table_count, FILE *file )
{
long table_start = (long)sizeof(AssetFileHeader);
long row_stride  = (long)sizeof(AssetFileTableRow);

bool found_it = false;    
long remain = (long)table_count;
long middle;
long top = 0;
AssetFileTableRow row;
while( remain > 0 )
    {
    middle = top + remain / 2;
    remain /= 2;
    
    if( fseek( file, table_start + middle * row_stride, SEEK_SET ) )
        {
        return( false );
        }

    row = {};
    if( fread( &row, 1, sizeof(row), file ) != sizeof(row) )
        {
        return( false );
        }

    if( row.id == id )
        {
        found_it = true;
        if( fseek( file, -row_stride, SEEK_CUR ) )
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

