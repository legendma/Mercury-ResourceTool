#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "AssetFileUtilities.hpp"

#define ASSET_FILE_MAX_NAME_STR_LEN ( 60 )

#define ASSET_FILE_MODEL_VERTEX_UV_COUNT \
                                    ( 1 )
#define ASSET_FILE_MODEL_NODE_CHILD_MESH_MAX_COUNT \
                                    ( 10 )
#define ASSET_FILE_MODEL_NODE_CHILD_NODE_MAX_COUNT \
                                    ( 50 )
#define ASSET_FILE_TEXTURE_EXTENT_ASSET_ID \
                                    0xffffffff

#define ASSET_FILE_BINARY_FILENAME   "AllAssets.bin"

#define ASSET_FILE_SOUND_BANK_FILENAME \
                                    "SoundSample.fsb"
#define ASSET_FILE_MUSIC_BANK_FILENAME \
                                    "MusicClips.fsb"
#define ASSET_FILE_MAX_SOUND_NAME_LEN ( 256 )

#define ASSET_FILE_FONT_GLYPH_INVALID_INDEX \
                                      ( 255 )
#define ASSET_FILE_FONT_MAX_GLYPHS    ASSET_FILE_FONT_GLYPH_INVALID_INDEX

typedef struct
    {
    u8                  indices[ ASSET_FILE_FONT_MAX_GLYPHS ];
    } AssetFileGlyphMap;

typedef struct _AssetFileNameString
    {
    char                str[ ASSET_FILE_MAX_NAME_STR_LEN + 1 ];
    } AssetFileNameString;

typedef u32 AssetFileAssetId;
typedef enum _AssetFileAssetKind
    {
    ASSET_FILE_ASSET_KIND_INVALID,
    ASSET_FILE_ASSET_KIND_FONT,
    ASSET_FILE_ASSET_KIND_MODEL,
    ASSET_FILE_ASSET_KIND_SHADER,
    ASSET_FILE_ASSET_KIND_SOUND_SAMPLE,
    ASSET_FILE_ASSET_KIND_SOUND_MUSIC_CLIP,
    ASSET_FILE_ASSET_KIND_TEXTURE,
    ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
    } AssetFileAssetKind;

typedef struct _AssetFileFontGlyph
    {
    u8                  glyph;      /* glyph ascii code             */
    f32                 width;      /* glyph width in pixels        */
    f32                 height;     /* glyph heigh in pixels        */
    f32                 top_left_x; /* pen x offset to top-left     */
    f32                 top_left_y; /* pen y offset to top-left     */
    f32                 bottom_right_x;
                                    /* pen x offset to bottom-right */
    f32                 bottom_right_y;
                                    /* pen y offset to bottom-right */
    f32                 u0;         /* top-left UV x coordinate     */
    f32                 v0;         /* top-left UV y coordinate     */
    f32                 u1;         /* bottom-right UV x coordinate */
    f32                 v1;         /* bottom-right UV y coordinate */
    f32                 h_advance;  /* horizontal pen advancement   */
    } AssetFileFontGlyph;

typedef enum _AssetFileModelElementKind
    {
    ASSET_FILE_MODEL_ELEMENT_KIND_INVALID,
    ASSET_FILE_MODEL_ELEMENT_KIND_NODE,
    ASSET_FILE_MODEL_ELEMENT_KIND_MESH,
    ASSET_FILE_MODEL_ELEMENT_KIND_MATERIAL
    } AssetFileModelElementKind;

typedef enum
    {
    ASSET_FILE_MODEL_TEXTURE_ALBEDO_MAP,            /* t0 */
    ASSET_FILE_MODEL_TEXTURE_NORMAL_MAP,            /* t1 */
    ASSET_FILE_MODEL_TEXTURE_EMISSIVE_MAP,          /* t2 */
    ASSET_FILE_MODEL_TEXTURE_METALLIC_MAP,          /* t3 */
    ASSET_FILE_MODEL_TEXTURE_ROUGHNESS_MAP,         /* t4 */
    ASSET_FILE_MODEL_TEXTURE_DISPLACEMENT_MAP,      /* t5 */
    /* count */
    ASSET_FILE_MODEL_TEXTURE_COUNT
    } AssetFileModelTexture;

typedef u8 AssetFileModelMaterialBits;
enum
    {
    /* textures */
    ASSET_FILE_MODEL_MATERIAL_BIT_ALBEDO_MAP       = ( 1 << ASSET_FILE_MODEL_TEXTURE_ALBEDO_MAP ),
    ASSET_FILE_MODEL_MATERIAL_BIT_NORMAL_MAP       = ( 1 << ASSET_FILE_MODEL_TEXTURE_NORMAL_MAP       ),
    ASSET_FILE_MODEL_MATERIAL_BIT_EMISSIVE_MAP     = ( 1 << ASSET_FILE_MODEL_TEXTURE_EMISSIVE_MAP     ),
    ASSET_FILE_MODEL_MATERIAL_BIT_METALLIC_MAP     = ( 1 << ASSET_FILE_MODEL_TEXTURE_METALLIC_MAP     ),
    ASSET_FILE_MODEL_MATERIAL_BIT_ROUGHNESS_MAP    = ( 1 << ASSET_FILE_MODEL_TEXTURE_ROUGHNESS_MAP    ),
    ASSET_FILE_MODEL_MATERIAL_BIT_DISPLACEMENT_MAP = ( 1 << ASSET_FILE_MODEL_TEXTURE_DISPLACEMENT_MAP ),
    /* markers */
    ASSET_FILE_MODEL_MATERIAL_BIT_TRANSPARENCY     = ( 1 << ( ASSET_FILE_MODEL_TEXTURE_COUNT + 0 ) )
    };

typedef struct _AssetFileModelVertex
    {
    f32                 x;          /* vertex position              */
    f32                 y;          /* vertex position              */
    f32                 z;          /* vertex position              */
    f32                 u0;         /* first texture coordinate     */
    f32                 v0;         /* first texture coordinate     */
    } AssetFileModelVertex;

typedef u32 AssetFileModelIndex; /* used to index vertices/
                                         meshes/nodes/materials     */

typedef struct _AssetFileModelNode
    {
    f32                 transform[ 4 * 4 ];
                                    /* row major index              */
    AssetFileModelIndex child_meshes[ ASSET_FILE_MODEL_NODE_CHILD_MESH_MAX_COUNT ];
    AssetFileModelIndex child_nodes[ ASSET_FILE_MODEL_NODE_CHILD_NODE_MAX_COUNT ];
    u16                 child_mesh_count;
    u16                 child_node_count;
    } AssetFileModelNode;

typedef struct _AssetFileModelMaterial
    {
    AssetFileModelMaterialBits
                        bits;       /* valid material parameters    */
    AssetFileAssetId    textures[ ASSET_FILE_MODEL_TEXTURE_COUNT ];
                                    /* material texture maps        */
    } AssetFileModelMaterial;

typedef struct _AssetFileSoundPair
    {
    AssetFileAssetId    asset_id;       /* ID of the sound          */
    u32                 subsound_index; /* index within bank        */
    } AssetFileSoundPair; 

typedef struct _AssetFileTextureExtent
    {
    AssetFileAssetId    texture_id; /* ID of the texture            */
    u16                 width;      /* texture width                */
    u16                 height;     /* texture height               */
    } AssetFileTextureExtent;

typedef struct _AssetFileWriter
    {
    fhnd                hnd;        /* file handle                  */
    u32                 caret;      /* working write location       */
    u32                 table_cnt;  /* number entries in table      */
    AssetFileAssetKind  kind;       /* asset kind under write       */
    u32                 asset_start;/* start of asset under write   */
    u32                 model_vertices_written;
    u32                 model_indices_written;
    } AssetFileWriter;

typedef struct _AssetFileReader
    {
    fhnd                hnd;        /* file handle                  */
    AssetFileAssetKind  kind;       /* asset kind under read        */
    u32                 asset_start;/* start of asset under read    */
    u32                 table_cnt;  /* number entries in table      */
    } AssetFileReader;


b8  AssetFile_BeginReadingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileReader *input );
b8  AssetFile_BeginWritingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileWriter *output );
b8  AssetFile_BeginWritingModelElement( const AssetFileModelElementKind kind, const AssetFileModelIndex element_index, AssetFileWriter *output );
b8  AssetFile_CloseForRead( AssetFileReader *input );
b8  AssetFile_CloseForWrite( AssetFileWriter *output );
b8  AssetFile_CreateForWrite( const char *filename, const AssetFileAssetId *ids, const u32 ids_count, AssetFileWriter *output );
b8  AssetFile_DescribeFont( const u8 oversample_x, const u8 oversample_y, const u16 texture_width, const u16 texture_height, const u32 texture_sz, const u8 *pixels, const u16 glyph_cnt, const u8 *glyph_codes, AssetFileWriter *output );
b8  AssetFile_DescribeModel( const u32 node_count, const u32 mesh_count, const u32 material_count, AssetFileWriter *output );
b8  AssetFile_DescribeModelMaterial( const AssetFileModelMaterialBits maps, AssetFileWriter *output );
b8  AssetFile_DescribeModelMesh( const u32 material_element_index, const u32 vertex_cnt, const u32 index_cnt, AssetFileWriter *output );
b8  AssetFile_DescribeModelNode( const u32 node_count, const f32 *mat4x4, const u32 mesh_count, AssetFileWriter *output );
b8  AssetFile_DescribeShader( const u32 byte_size, AssetFileWriter *output );
b8  AssetFile_DescribeTexture( const u32 byte_size, AssetFileWriter *output );
b8  AssetFile_DescribeTexture2( const u32 channel_cnt, const u32 width, const u32 height, const u32 byte_size, AssetFileWriter *output );
b8  AssetFile_DescribeTextureExtents( const u16 element_cnt, AssetFileWriter *output );
b8  AssetFile_EndReadingAsset( AssetFileReader *input );
b8  AssetFile_EndWritingAsset( AssetFileWriter *output );
b8  AssetFile_EndWritingModel( const u32 root_node_element, AssetFileWriter *output );
u64 AssetFile_GetWriteSize( const AssetFileWriter *output );
b8  AssetFile_OpenForRead( const char *filename, AssetFileReader *input );
b8  AssetFile_ReadFontGlyphs( const u16 glyph_capacity, AssetFileFontGlyph *glyphs, AssetFileReader *input );
b8  AssetFile_ReadFontTexture( const u32 buffer_sz, u8 *pixels, u16 *width, u16 *height, AssetFileReader *input );
b8  AssetFile_ReadFontStorageRequirements( u16 *glyph_cnt, u32 *texture_sz, AssetFileReader *input );
b8  AssetFile_ReadModelMaterials( const u32 material_capacity, u32 *material_count, AssetFileModelMaterial *materials, AssetFileReader *input );
b8  AssetFile_ReadModelMeshIndices( const u32 mesh_index, const u32 index_capacity, u32 *index_count, AssetFileModelIndex *indices, AssetFileReader *input );
b8  AssetFile_ReadModelMeshVertices( const u32 mesh_index, const u32 vertex_capacity, AssetFileModelIndex *material_index, u32 *vertex_count, AssetFileModelVertex *vertices, AssetFileReader *input );
b8  AssetFile_ReadModelNodes( const u32 node_capacity, u32 *node_count, AssetFileModelNode *nodes, AssetFileReader *input );
b8  AssetFile_ReadModelStorageRequirements( u32 *vertex_count, u32 *index_count, u32 *mesh_count, u32 *node_count, u32 *material_count, AssetFileReader *input );
b8  AssetFile_ReadShaderBinary( const u32 buffer_sz, u32 *read_sz, byte *buffer, AssetFileReader *input );
b8  AssetFile_ReadSoundPairs( u16 num_pairs, AssetFileSoundPair *sound_pairs, AssetFileReader *input );
b8  AssetFile_ReadSoundPairsStorageRequirements( u16 *num_elements, AssetFileReader *input );
b8  AssetFile_ReadShaderStorageRequirements( u32 *byte_count, AssetFileReader *input );
b8  AssetFile_ReadTextureExtentsStorageRequirements( u16 *num_elements, AssetFileReader *input );
b8  AssetFile_ReadTextureBinary( const u32 buffer_sz, u32 *read_sz, byte *buffer, AssetFileReader *input );
b8  AssetFile_ReadTextureStorageRequirements( u32 *channel_cnt, u32 *width, u32 *height, u32 *byte_count, AssetFileReader *input );
b8  AssetFile_ReadTextureExtents( const u16 output_cnt, AssetFileTextureExtent *out_elements, AssetFileReader *input );
b8  AssetFile_ReadTextureExtentsStorageRequirements( u16 *element_cnt, AssetFileReader *input );
b8  AssetFile_WriteFontGlyph( const u8 glyph, const u16 u0, const u16 v0, const u16 u1, const u16 v1, const f32 pen_dx, const f32 pen_dy, const f32 pen_xadvance, AssetFileWriter *output );
b8  AssetFile_WriteModelMaterialTextureMaps( const AssetFileAssetId *asset_ids, const u8 count, AssetFileWriter *output );
b8  AssetFile_WriteModelMeshIndex( const AssetFileModelIndex index, AssetFileWriter *output );
b8  AssetFile_WriteModelMeshVertex( const AssetFileModelVertex *vertex, AssetFileWriter *output );
b8  AssetFile_WriteModelNodeChildElements( const AssetFileModelIndex *element_ids, const u32 count, AssetFileWriter *output );
b8  AssetFile_WriteShader( const byte *blob, const u32 blob_size, AssetFileWriter *output );
b8  AssetFile_WriteSoundPairs( const AssetFileSoundPair *sound_pair, const u16 num_pairs, AssetFileWriter *output );
b8  AssetFile_WriteTexture( const byte *image, const u32 image_size, AssetFileWriter *output );
b8  AssetFile_WriteTextureExtent( const AssetFileAssetId id, const u16 width, const u16 height, AssetFileWriter *output );


/*******************************************************************
*
*   AssetFile_GlyphMapInit()
*
*******************************************************************/

#define AssetFile_GlyphMapInit( _pglyph_map ) \
    memset( _pglyph_map, ASSET_FILE_FONT_GLYPH_INVALID_INDEX, sizeof( *(_pglyph_map) ) )


/*******************************************************************
*
*   AssetFile_FNV1a()
*
*   DESCRIPTION:
*       Compute a FNV-1a hash.
*
*******************************************************************/

static inline u32 AssetFile_FNV1a( const void *data, const u32 sz )
{
static const u32 SEED  = 0x811c9dc5;
static const u32 PRIME = 0x01000193;

const byte *bytes = (byte*)data;

u32 ret = SEED;
for( u32 i = 0; i < sz; i++ )
    {
    ret ^= bytes[ i ];
    ret *= PRIME;
    }

return( ret );

} /* AssetFile_FNV1a() */


/*******************************************************************
*
*   AssetFile_MakeAssetIdFromName()
*
*   DESCRIPTION:
*       Create an asset id given the string asset id name.
*
*******************************************************************/

static inline AssetFileAssetId AssetFile_MakeAssetIdFromName( const char *name, const u32 name_length )
{
return( AssetFile_FNV1a( name, name_length ) );

} /* AssetFile_MakeAssetIdFromName() */


/*******************************************************************
*
*   AssetFile_MakeAssetIdFromName2()
*
*   DESCRIPTION:
*       Create a 32-bit hash asset id given the string asset id
*       name.
*
*******************************************************************/

static inline u32 AssetFile_MakeAssetIdFromName2( const char *name )
{
return( AssetFile_MakeAssetIdFromName( name, (u32)strlen( name ) ) );

} /* AssetFile_MakeAssetIdFromName2() */


/*******************************************************************
*
*   AssetFile_MakeAssetIdFromNameString()
*
*   DESCRIPTION:
*       Create an asset id given the string asset id name string.
*
*******************************************************************/

static AssetFileAssetId inline AssetFile_MakeAssetIdFromNameString( const AssetFileNameString *name )
{
return( AssetFile_MakeAssetIdFromName( name->str, (u32)strlen( name->str ) ) );

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
strncpy( ret.str, name, sizeof( ret.str ) - 1 );
ret.str[ sizeof( ret.str ) - 1 ] = '\0';

return( ret );

} /* AssetFile_CopyNameString() */
