#pragma once
#include <cstdint>
#include <cstdio>

typedef uint32_t AssetFileAssetId;
typedef enum _AssetFileAssetKind
    {
    ASSET_FILE_ASSET_KIND_INVALID,
    ASSET_FILE_ASSET_KIND_MODEL,
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
    /* count */
    ASSET_FILE_MODEL_MATERIAL_BIT_SPECULAR_MAP_COUNT = 2
    };

typedef struct _AssetFileVertex
    {
    float               x;          /* vertex position              */
    float               y;          /* vertex position              */
    float               z;          /* vertex position              */
    float               u0;         /* first texture coordinate     */
    float               v0;         /* first texture coordinate     */
    } AssetFileVertex;

typedef struct _AssetFileWriter
    {
    FILE               *fhnd;
    uint32_t            caret;      /* working write location       */
    uint32_t            table_cnt;  /* number entries in table      */
    AssetFileAssetKind  kind;       /* asset kind under write       */
    uint32_t            asset_start;/* start of asset under write   */
    } AssetFileWriter;


bool AssetFile_BeginWritingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileWriter *output );
bool AssetFile_BeginWritingModelElement( const AssetFileModelElementKind kind, const uint32_t element_index, AssetFileWriter *output );
bool AssetFile_CreateForWrite( const char *filename, const AssetFileAssetId *ids, const uint32_t ids_count, AssetFileWriter *output );
bool AssetFile_DescribeModel( const uint32_t node_count, const uint32_t mesh_count, const uint32_t material_count, AssetFileWriter *output );
bool AssetFile_DescribeModelMaterial( const AssetFileModelMaterialBits maps, AssetFileWriter *output );
bool AssetFile_DescribeModelMesh( const uint32_t material_element_index, const uint32_t vertex_cnt, AssetFileWriter *output );
bool AssetFile_DescribeModelNode( const uint32_t node_count, const uint32_t mesh_count, AssetFileWriter *output );
bool AssetFile_DescribeTexture( const uint32_t width, const uint32_t height, AssetFileWriter *output );
bool AssetFile_EndWritingModel( const uint32_t root_node_element, AssetFileWriter *output );
size_t AssetFile_GetWriteSize( const AssetFileWriter *output );
bool AssetFile_WriteModelMaterialTextureMaps( const AssetFileAssetId *asset_ids, const uint8_t count, AssetFileWriter *output );
bool AssetFile_WriteModelMeshVertex( const AssetFileVertex *vertex, AssetFileWriter *output );
bool AssetFile_WriteModelNodeChildElements( const AssetFileAssetId *asset_ids, const uint32_t count, AssetFileWriter *output );
bool AssetFile_WriteTexture( const uint8_t *image, const uint32_t image_size, AssetFileWriter *output );


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