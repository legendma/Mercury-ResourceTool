#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>

#define ASSET_FILE_MAX_NAME_STR_LEN ( 60 )

#define ASSET_FILE_MODEL_VERTEX_UV_COUNT \
                                    ( 1 )
#define ASSET_FILE_MODEL_NODE_CHILD_MESH_MAX_COUNT \
                                    ( 10 )
#define ASSET_FILE_MODEL_NODE_CHILD_NODE_MAX_COUNT \
                                    ( 50 )
#define ASSET_FILE_TEXTURE_EXTENT_ASSET_ID \
                                    0xffffffff

typedef struct _AssetFileNameString
    {
    char                str[ ASSET_FILE_MAX_NAME_STR_LEN + 1 ];
    } AssetFileNameString;

typedef uint32_t AssetFileAssetId;
typedef enum _AssetFileAssetKind
    {
    ASSET_FILE_ASSET_KIND_INVALID,
    ASSET_FILE_ASSET_KIND_MODEL,
    ASSET_FILE_ASSET_KIND_SHADER,
    ASSET_FILE_ASSET_KIND_TEXTURE,
    ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
    } AssetFileAssetKind;

typedef enum _AssetFileModelElementKind
    {
    ASSET_FILE_MODEL_ELEMENT_KIND_INVALID,
    ASSET_FILE_MODEL_ELEMENT_KIND_NODE,
    ASSET_FILE_MODEL_ELEMENT_KIND_MESH,
    ASSET_FILE_MODEL_ELEMENT_KIND_MATERIAL
    } AssetFileModelElementKind;

typedef enum _AssetFileModelTextures
    {
    ASSET_FILE_MODEL_TEXTURES_ALBEDO_MAP,            /* t0 */
    ASSET_FILE_MODEL_TEXTURES_NORMAL_MAP,            /* t1 */
    ASSET_FILE_MODEL_TEXTURES_EMISSIVE_MAP,          /* t2 */
    ASSET_FILE_MODEL_TEXTURES_METALLIC_MAP,          /* t3 */
    ASSET_FILE_MODEL_TEXTURES_ROUGHNESS_MAP,         /* t4 */
    ASSET_FILE_MODEL_TEXTURES_DISPLACEMENT_MAP,      /* t5 */
    /* count */
    ASSET_FILE_MODEL_TEXTURES_COUNT
    } AssetFileModelTextures;

typedef uint8_t AssetFileModelMaterialBits;
enum
    {
    /* textures */
    ASSET_FILE_MODEL_MATERIAL_BIT_ALBEDO_MAP       = ( 1 << ASSET_FILE_MODEL_TEXTURES_ALBEDO_MAP       ),
    ASSET_FILE_MODEL_MATERIAL_BIT_NORMAL_MAP       = ( 1 << ASSET_FILE_MODEL_TEXTURES_NORMAL_MAP       ),
    ASSET_FILE_MODEL_MATERIAL_BIT_EMISSIVE_MAP     = ( 1 << ASSET_FILE_MODEL_TEXTURES_EMISSIVE_MAP     ),
    ASSET_FILE_MODEL_MATERIAL_BIT_METALLIC_MAP     = ( 1 << ASSET_FILE_MODEL_TEXTURES_METALLIC_MAP     ),
    ASSET_FILE_MODEL_MATERIAL_BIT_ROUGHNESS_MAP    = ( 1 << ASSET_FILE_MODEL_TEXTURES_ROUGHNESS_MAP    ),
    ASSET_FILE_MODEL_MATERIAL_BIT_DISPLACEMENT_MAP = ( 1 << ASSET_FILE_MODEL_TEXTURES_DISPLACEMENT_MAP ),
    /* markers */
    ASSET_FILE_MODEL_MATERIAL_BIT_TRANSPARENCY     = ( 1 << ( ASSET_FILE_MODEL_TEXTURES_COUNT + 0 ) )
    };

typedef struct _AssetFileModelVertex
    {
    float               x;          /* vertex position              */
    float               y;          /* vertex position              */
    float               z;          /* vertex position              */
    float               u0;         /* first texture coordinate     */
    float               v0;         /* first texture coordinate     */
    } AssetFileModelVertex;

typedef uint32_t AssetFileModelIndex; /* used to index vertices/
                                         meshes/nodes/materials     */

typedef struct _AssetFileModelNode
    {
    float               transform[ 4 * 4 ];
                                    /* row major index              */
    AssetFileModelIndex child_meshes[ ASSET_FILE_MODEL_NODE_CHILD_MESH_MAX_COUNT ];
    AssetFileModelIndex child_nodes[ ASSET_FILE_MODEL_NODE_CHILD_NODE_MAX_COUNT ];
    uint16_t            child_mesh_count;
    uint16_t            child_node_count;
    } AssetFileModelNode;

typedef struct _AssetFileModelMaterial
    {
    AssetFileModelMaterialBits
                        bits;       /* valid material parameters    */
    AssetFileAssetId    textures[ ASSET_FILE_MODEL_TEXTURES_COUNT ];
                                    /* material texture maps        */
    } AssetFileModelMaterial;
    
typedef struct _AssetFileTextureExtent
    {
    AssetFileAssetId    texture_id; /* ID of the texture            */
    uint16_t            width;      /* texture width                */
    uint16_t            height;     /* texture height               */
    } AssetFileTextureExtent;

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
bool   AssetFile_BeginWritingModelElement( const AssetFileModelElementKind kind, const AssetFileModelIndex element_index, AssetFileWriter *output );
bool   AssetFile_CloseForRead( AssetFileReader *input );
bool   AssetFile_CloseForWrite( AssetFileWriter *output );
bool   AssetFile_CreateForWrite( const char *filename, const AssetFileAssetId *ids, const uint32_t ids_count, AssetFileWriter *output );
bool   AssetFile_DescribeModel( const uint32_t node_count, const uint32_t mesh_count, const uint32_t material_count, AssetFileWriter *output );
bool   AssetFile_DescribeModelMaterial( const AssetFileModelMaterialBits maps, AssetFileWriter *output );
bool   AssetFile_DescribeModelMesh( const uint32_t material_element_index, const uint32_t vertex_cnt, const uint32_t index_cnt, AssetFileWriter *output );
bool   AssetFile_DescribeModelNode( const uint32_t node_count, const float *mat4x4, const uint32_t mesh_count, AssetFileWriter *output );
bool   AssetFile_DescribeShader( const uint32_t byte_size, AssetFileWriter *output );
bool   AssetFile_DescribeTexture( const uint32_t byte_size, AssetFileWriter *output );
bool   AssetFile_DescribeTexture2( const uint32_t channel_cnt, const uint32_t width, const uint32_t height, const uint32_t byte_size, AssetFileWriter *output );
bool   AssetFile_DescribeTextureExtents( const uint16_t element_cnt, AssetFileWriter *output );
bool   AssetFile_EndReadingAsset( AssetFileReader *input );
bool   AssetFile_EndWritingModel( const uint32_t root_node_element, AssetFileWriter *output );
bool   AssetFile_EndWritingTextureExtents( AssetFileWriter *output );
size_t AssetFile_GetWriteSize( const AssetFileWriter *output );
bool   AssetFile_OpenForRead( const char *filename, AssetFileReader *input );
bool   AssetFile_ReadModelMaterials( const uint32_t material_capacity, uint32_t *material_count, AssetFileModelMaterial *materials, AssetFileReader *input );
bool   AssetFile_ReadModelMeshIndices( const uint32_t mesh_index, const uint32_t index_capacity, uint32_t *index_count, AssetFileModelIndex *indices, AssetFileReader *input );
bool   AssetFile_ReadModelMeshVertices( const uint32_t mesh_index, const uint32_t vertex_capacity, AssetFileModelIndex *material_index, uint32_t *vertex_count, AssetFileModelVertex *vertices, AssetFileReader *input );
bool   AssetFile_ReadModelNodes( const uint32_t node_capacity, uint32_t *node_count, AssetFileModelNode *nodes, AssetFileReader *input );
bool   AssetFile_ReadModelStorageRequirements( uint32_t *vertex_count, uint32_t *index_count, uint32_t *mesh_count, uint32_t *node_count, uint32_t *material_count, AssetFileReader *input );
bool   AssetFile_ReadShaderBinary( const uint32_t buffer_sz, uint32_t *read_sz, uint8_t *buffer, AssetFileReader *input );
bool   AssetFile_ReadShaderStorageRequirements( uint32_t *byte_count, AssetFileReader *input );
bool   AssetFile_ReadTextureBinary( const uint32_t buffer_sz, uint32_t *read_sz, uint8_t *buffer, AssetFileReader *input );
bool   AssetFile_ReadTextureStorageRequirements( uint32_t *channel_cnt, uint32_t *width, uint32_t *height, uint32_t *byte_count, AssetFileReader *input );
bool   AssetFile_ReadTextureExtents( const uint16_t output_cnt, AssetFileTextureExtent *out_elements, AssetFileReader *input );
bool   AssetFile_ReadTextureExtentsStorageRequirements( uint16_t *element_cnt, AssetFileReader *input );
bool   AssetFile_WriteModelMaterialTextureMaps( const AssetFileAssetId *asset_ids, const uint8_t count, AssetFileWriter *output );
bool   AssetFile_WriteModelMeshIndex( const AssetFileModelIndex index, AssetFileWriter *output );
bool   AssetFile_WriteModelMeshVertex( const AssetFileModelVertex *vertex, AssetFileWriter *output );
bool   AssetFile_WriteModelNodeChildElements( const AssetFileModelIndex *element_ids, const uint32_t count, AssetFileWriter *output );
bool   AssetFile_WriteShader( const uint8_t *blob, const uint32_t blob_size, AssetFileWriter *output );
bool   AssetFile_WriteTexture( const uint8_t *image, const uint32_t image_size, AssetFileWriter *output );
bool   AssetFile_WriteTextureExtent( const AssetFileAssetId id, const uint16_t width, const uint16_t height, AssetFileWriter *output );


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


/*******************************************************************
*
*   AssetFile_MakeAssetIdFromNameString()
*
*   DESCRIPTION:
*       Create a 32-bit hash asset id given the string asset id
*       name string.
*
*******************************************************************/

static uint32_t inline AssetFile_MakeAssetIdFromNameString( const AssetFileNameString *name )
{
return( AssetFile_MakeAssetIdFromName( name->str, (uint32_t)strlen( name->str ) ) );

} /* AssetFile_MakeAssetIdFromNameString() */


/*******************************************************************
*
*   AssetFile_CopyNameString()
*
*   DESCRIPTION:
*       Copy the given asset name to a string buffer.
*
*******************************************************************/

static inline AssetFileNameString AssetFile_CopyNameString( const char *name )
{
AssetFileNameString     ret = {};
strcpy_s( ret.str, sizeof( ret.str ), name );

return( ret );

} /* AssetFile_CopyNameString() */