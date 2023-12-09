#pragma once
#include <cstdint>
#include <cstdio>

typedef uint32_t AssetFileAssetId;
typedef enum _AssetFileAssetKind
    {
    ASSET_FILE_ASSET_KIND_INVALID,
    ASSET_FILE_ASSET_KIND_MODEL,
    ASSET_FILE_ASSET_KIND_SHADER,
    ASSET_FILE_ASSET_KIND_TEXTURE
    } AssetFileAssetKind;

typedef enum _AssetFileModelElementKind
    {
    ASSET_FILE_MODEL_ELEMENT_KIND_INVALID,
    ASSET_FILE_MODEL_ELEMENT_KIND_NODE,
    ASSET_FILE_MODEL_ELEMENT_KIND_MESH,
    ASSET_FILE_MODEL_ELEMENT_KIND_MATERIAL
    } AssetFileModelElementKind;

typedef uint8_t AssetFileModelMaterialBits;
enum
    {
    ASSET_FILE_MODEL_MATERIAL_BIT_DIFFUSE_MAP  = ( 1 << 0 ),
    ASSET_FILE_MODEL_MATERIAL_BIT_SPECULAR_MAP = ( 1 << 1 ),
    ASSET_FILE_MODEL_MATERIAL_BIT_TRANSPARENCY = ( 1 << 2 ),
    /* count */
    ASSET_FILE_MODEL_MATERIAL_BIT_SPECULAR_MAP_COUNT = 2
    };

typedef struct _AssetFileModelVertex
    {
    float               x;          /* vertex position              */
    float               y;          /* vertex position              */
    float               z;          /* vertex position              */
    float               u0;         /* first texture coordinate     */
    float               v0;         /* first texture coordinate     */
    } AssetFileModelVertex;

typedef uint16_t AssetFileModelIndex;

typedef struct _AssetFileWriter
    {
    FILE               *fhnd;       /* file handle                  */
    uint32_t            caret;      /* working write location       */
    uint32_t            table_cnt;  /* number entries in table      */
    AssetFileAssetKind  kind;       /* asset kind under write       */
    uint32_t            asset_start;/* start of asset under write   */
    uint32_t            model_vertices_written;
    uint32_t            model_indices_written;
    } AssetFileWriter;

typedef struct _AssetFileReader
    {
    FILE               *fhnd;       /* file handle                  */
    AssetFileAssetKind  kind;       /* asset kind under read        */
    uint32_t            asset_start;/* start of asset under read    */
    uint32_t            table_cnt;  /* number entries in table      */
    } AssetFileReader;


bool   AssetFile_BeginReadingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileReader *input );
bool   AssetFile_BeginWritingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileWriter *output );
bool   AssetFile_BeginWritingModelElement( const AssetFileModelElementKind kind, const uint32_t element_index, AssetFileWriter *output );
bool   AssetFile_CloseForRead( AssetFileReader *input );
bool   AssetFile_CloseForWrite( AssetFileWriter *output );
bool   AssetFile_CreateForWrite( const char *filename, const AssetFileAssetId *ids, const uint32_t ids_count, AssetFileWriter *output );
bool   AssetFile_DescribeModel( const uint32_t node_count, const uint32_t mesh_count, const uint32_t material_count, AssetFileWriter *output );
bool   AssetFile_DescribeModelMaterial( const AssetFileModelMaterialBits maps, AssetFileWriter *output );
bool   AssetFile_DescribeModelMesh( const uint32_t material_element_index, const uint32_t vertex_cnt, const uint32_t index_cnt, AssetFileWriter *output );
bool   AssetFile_DescribeModelNode( const uint32_t node_count, const uint32_t mesh_count, AssetFileWriter *output );
bool   AssetFile_DescribeShader( const uint32_t byte_size, AssetFileWriter *output );
bool   AssetFile_DescribeTexture( const uint32_t width, const uint32_t height, AssetFileWriter *output );
bool   AssetFile_EndReadingAsset( AssetFileReader *input );
bool   AssetFile_EndWritingModel( const uint32_t root_node_element, AssetFileWriter *output );
size_t AssetFile_GetWriteSize( const AssetFileWriter *output );
bool   AssetFile_OpenForRead( const char *filename, AssetFileReader *input );
bool   AssetFile_ReadModelMeshIndices( const uint32_t mesh_index, const uint32_t index_capacity, uint32_t *index_count, AssetFileModelIndex *indices, AssetFileReader *input );
bool   AssetFile_ReadModelMeshVertices( const uint32_t mesh_index, const uint32_t vertex_capacity, uint32_t *vertex_count, AssetFileModelVertex *vertices, AssetFileReader *input );
bool   AssetFile_ReadModelStorageRequirements( uint32_t *vertex_count, uint32_t *index_count, uint32_t *mesh_count, uint32_t *node_count, AssetFileReader *input );
bool   AssetFile_ReadShaderBinary( const uint32_t buffer_sz, uint32_t *read_sz, uint8_t *buffer, AssetFileReader *input );
bool   AssetFile_ReadShaderStorageRequirements( uint32_t *byte_count, AssetFileReader *input );
bool   AssetFile_WriteModelMaterialTextureMaps( const AssetFileAssetId *asset_ids, const uint8_t count, AssetFileWriter *output );
bool   AssetFile_WriteModelMeshIndex( const AssetFileModelIndex index, AssetFileWriter *output );
bool   AssetFile_WriteModelMeshVertex( const AssetFileModelVertex *vertex, AssetFileWriter *output );
bool   AssetFile_WriteModelNodeChildElements( const AssetFileAssetId *asset_ids, const uint32_t count, AssetFileWriter *output );
bool   AssetFile_WriteShader( const uint8_t *blob, const uint32_t blob_size, AssetFileWriter *output );
bool   AssetFile_WriteTexture( const uint8_t *image, const uint32_t image_size, AssetFileWriter *output );




/*******************************************************************
*
*   AssetFile_MakeAssetIdFromName()
*
*   DESCRIPTION:
*       Create a 32-bit hash asset id given the string asset id
*       name.
*
*******************************************************************/

static uint32_t inline AssetFile_MakeAssetIdFromName( const char *name, const uint32_t name_length )
{
static const uint32_t SEED  = 0x811c9dc5;
static const uint32_t PRIME = 0x01000193;

uint32_t ret = SEED;
for( uint32_t i = 0; i < name_length; i++ )
    {
    ret ^= name[ i ];
    ret *= PRIME;
    }

return( ret );

} /* AssetFile_MakeAssetIdFromName() */