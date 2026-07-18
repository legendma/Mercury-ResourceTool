#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"

#define make_fourcc( _a, _b, _c, _d ) \
    ( ( _a << 0 ) | ( _b << 8 ) | ( _c << 16 ) | (_d << 24 ) )
        

static const u32 ASSET_FILE_MAGIC = make_fourcc( 'M', 'e', 'r', 'c' );

typedef struct
    {
    u32                 magic;      /* magic sentinel number        */
    u32                 table_cnt;  /* number entries in table      */
    } AssetFileHeader;

typedef struct
    {
    AssetFileAssetId    id;         /* asset ID hash                */
    AssetFileAssetKind  kind;       /* type of asset                */
    u32                 starts_at;  /* file offset to start of asset*/
    } AssetFileTableRow;

typedef struct
    {
    u8                  oversample_x;
                                    /* horizontal texels/pixels     */
    u8                  oversample_y;
                                    /* vertical texels/pixels       */
    u16                 texture_width;
                                    /* texture extent width         */
    u16                 texture_height;
                                    /* texture extent height        */
    u16                 glyph_cnt;  /* number glyphs in font        */
    u32                 texture_sz; /* texture data byte count      */
    u32                 glyphs_starts_at;
                                    /* file offset to glyph data    */
    u32                 texture_starts_at;
                                    /* file offset to texture data  */
    } FontHeader;

typedef struct
    {
    u8                  glyph;      /* glyph code                   */
    u16                 u0;         /* uv top-left x (pixels)       */
    u16                 v0;         /* uv top-left y (pixels)       */
    u16                 u1;         /* uv bottom-right x (pixels)   */
    u16                 v1;         /* uv bottom-right y (pixels)   */
    f32                 h_advance;  /* pen horizontal advancement   */
    f32                 pen_offset_x;
                                    /* horz offset from pen position*/
    f32                 pen_offset_y;
                                    /* vert offset from pen position*/
    } FontGlyphHeader;

typedef struct
    {
    u32                 node_count; /* number of nodes              */
    u32                 mesh_count; /* number of meshes             */
    u32                 material_cnt;
                                    /* number unique materials      */
    u32                 root_node_element;
                                    /* element index of root node   */
    u32                 total_vertex_count;
                                    /* number of vertices in model  */
    u32                 total_index_count;
                                    /* number of indices in model   */
    } ModelHeader;

typedef struct
    {
    AssetFileModelMaterialBits
                        map_bits;   /* present texture maps         */
    } ModelMaterialHeader;

typedef struct
    {
    u32                 vertex_cnt; /* number of vertices           */
    u32                 index_cnt;  /* number of indices            */
    u32                 material;   /* material element index       */
    } ModelMeshHeader;

typedef struct
    {
    u32                 node_count; /* number of child nodes        */
    u32                 mesh_count; /* number of child meshes       */
    f32                 transform[ 16 ];
                                    /* row major 4x4 matrix         */
    } ModelNodeHeader;

typedef struct
    {
    AssetFileModelElementKind
                        kind;       /* model element kind           */
    u32                 starts_at;  /* file offset to element start */
    } ModelTableRow;

typedef struct
    {
    u32                 byte_size;  /* byte code blob size          */
    } ShaderHeader;

typedef struct
    {
    u32                 channel_cnt;/* number of color channels     */
    u32                 width;      /* image width                  */
    u32                 height;     /* image height                 */
    u32                 byte_size;  /* compressed image blob size   */
    } TextureHeader;

typedef struct
    {
    u16                 texture_cnt;/* number of textures in table  */
    } TextureExtentHeader;


static b8 JumpToAssetInTable( const AssetFileAssetId id, const u32 table_count, fhnd file );
static b8 JumpToModelMaterial( const u32 asset_start, const u32 material_index, fhnd file );
static b8 JumpToModelMesh( const u32 asset_start, const u32 mesh_index, fhnd file );
static b8 JumpToModelNode( const u32 asset_start, const u32 node_index, fhnd file );


/*******************************************************************
*
*   AssetFile_BeginReadingAsset()
*
*   DESCRIPTION:
*       Start reading an asset of the given ID, by discovering its
*       place within the greater asset file.
*
*******************************************************************/

b8 AssetFile_BeginReadingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileReader *input )
{
input->kind        = ASSET_FILE_ASSET_KIND_INVALID;
input->asset_start = 0;

if( !JumpToAssetInTable( id, input->table_cnt, input->hnd ) )
    {
    return( FALSE );
    }

AssetFileTableRow row = {};
if( !file_read_struct( input->hnd, &row ) )
    {
    return( FALSE );
    }

assert( row.id == id );
if( row.kind != kind )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, row.starts_at ) )
    {
    return( FALSE );
    }

input->asset_start = row.starts_at;
input->kind = kind;

return( TRUE );

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

b8 AssetFile_BeginWritingAsset( const AssetFileAssetId id, const AssetFileAssetKind kind, AssetFileWriter *output )
{
output->kind        = ASSET_FILE_ASSET_KIND_INVALID;
output->asset_start = 0;

if( !JumpToAssetInTable( id, output->table_cnt, output->hnd ) )
    {
    return( FALSE );
    }

output->asset_start            = output->caret;
output->kind                   = kind;
output->model_indices_written  = 0;
output->model_vertices_written = 0;

AssetFileTableRow row = {};
row.id        = id;
row.kind      = kind;
row.starts_at = output->caret;

ensure( file_write_struct( output->hnd, &row ) );
if( !file_seek( output->hnd, output->caret ) )
    {
    return( FALSE );
    }

return( TRUE );

} /* AssetFile_BeginWritingAsset() */


/*******************************************************************
*
*   AssetFile_CloseForRead()
*
*   DESCRIPTION:
*       Complete reading of the asset file.
*
*******************************************************************/

b8 AssetFile_CloseForRead( AssetFileReader *input )
{
b8 ret = file_close( input->hnd );
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

b8 AssetFile_CloseForWrite( AssetFileWriter *output )
{
b8 ret = file_close( output->hnd );
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

b8 AssetFile_BeginWritingModelElement( const AssetFileModelElementKind kind, const AssetFileModelIndex element_index, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( FALSE );
    }

u32 row_location = output->asset_start
                 + (u32)sizeof(ModelHeader)
                 + element_index * (u32)sizeof(ModelTableRow);

if( !file_seek( output->hnd, row_location ) )
    {
    return( FALSE );
    }

ModelTableRow row = {};
row.starts_at = output->caret;
row.kind      = kind;
ensure( file_write_struct( output->hnd, &row ) );

if( !file_seek( output->hnd, output->caret ) )
    {
    return( FALSE );
    }

return( TRUE );

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

b8 AssetFile_CreateForWrite( const char *filename, const AssetFileAssetId *ids, const u32 ids_count, AssetFileWriter *output )
{
*output = {};
if( !file_open( filename, "w+b", &output->hnd ) )
    {
    return( FALSE );
    }

AssetFileHeader header = {};
header.magic     = ASSET_FILE_MAGIC;
header.table_cnt = ids_count;

ensure( file_write_struct( output->hnd, &header ) );

AssetFileTableRow row;
for( u32 i = 0; i < ids_count; i++ )
    {
    row = {};
    row.id = ids[ i ];

    ensure( file_write_struct( output->hnd, &row ) );
    }

output->table_cnt = header.table_cnt;
output->caret     = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_CreateForWrite() */


/*******************************************************************
*
*   AssetFile_DescribeFont()
*
*   DESCRIPTION:
*       Provide the details about a font being written.
*
*******************************************************************/

b8 AssetFile_DescribeFont( const u8 oversample_x, const u8 oversample_y, const u16 texture_width, const u16 texture_height, const u32 texture_sz, const u8 *pixels, const u16 glyph_cnt, const u8 *glyph_codes, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_FONT
|| !output->asset_start
|| !file_seek( output->hnd, output->asset_start ) )
    {
    return( FALSE );
    }

FontHeader header = {};
header.oversample_x      = oversample_x;
header.oversample_y      = oversample_y;
header.texture_width     = texture_width;
header.texture_height    = texture_height;
header.glyph_cnt         = glyph_cnt;
header.texture_sz        = texture_sz;
header.texture_starts_at = output->caret + sizeof( FontHeader );
header.glyphs_starts_at  = header.texture_starts_at + texture_sz;

ensure( file_write_struct( output->hnd, &header ) );
ensure( file_write( output->hnd, texture_sz, pixels ) );

output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_DescribeFont() */


/*******************************************************************
*
*   AssetFile_DescribeModel()
*
*   DESCRIPTION:
*       Provide the number of table entries for the model under
*       write.
*
*******************************************************************/

b8 AssetFile_DescribeModel( const u32 node_count, const u32 mesh_count, const u32 material_count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start
 || !file_seek( output->hnd, output->asset_start ) )
    {
    return( FALSE );
    }

ModelHeader header = {};
header.node_count   = node_count;
header.mesh_count   = mesh_count;
header.material_cnt = material_count;

ensure( file_write_struct( output->hnd, &header ) );

u32 row_count = node_count + mesh_count + material_count;
ModelTableRow row = {};
for( u32 i = 0; i < row_count; i++ )
    {
    ensure( file_write_struct( output->hnd, &row ) );
    }

output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_DescribeModel() */


/*******************************************************************
*
*   AssetFile_DescribeModelMaterial()
*
*   DESCRIPTION:
*       Provide the details of the material about to be written.
*
*******************************************************************/

b8 AssetFile_DescribeModelMaterial( const AssetFileModelMaterialBits maps, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( FALSE );
    }

ModelMaterialHeader header = {};
header.map_bits = maps;

ensure( file_write_struct( output->hnd, &header ) );
output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_DescribeModelMaterial() */


/*******************************************************************
*
*   AssetFile_DescribeModelMesh()
*
*   DESCRIPTION:
*       Provide the details of the mesh about to be written.
*
*******************************************************************/

b8 AssetFile_DescribeModelMesh( const u32 material_element_index, const u32 vertex_cnt, const u32 index_cnt, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( FALSE );
    }

ModelMeshHeader header = {};
header.vertex_cnt = vertex_cnt;
header.index_cnt  = index_cnt;
header.material   = material_element_index;

ensure( file_write_struct( output->hnd, &header ) );

output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_DescribeModelMesh() */


/*******************************************************************
*
*   AssetFile_DescribeModelNode()
*
*   DESCRIPTION:
*       Provide the details of the model node about to be written.
*
*******************************************************************/

b8 AssetFile_DescribeModelNode( const u32 node_count, const f32 *mat4x4, const u32 mesh_count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start
 || node_count > ASSET_FILE_MODEL_NODE_CHILD_NODE_MAX_COUNT 
 || mesh_count > ASSET_FILE_MODEL_NODE_CHILD_MESH_MAX_COUNT )
    {
    return( FALSE );
    }

ModelNodeHeader header = {};
header.node_count = node_count;
header.mesh_count = mesh_count;
memcpy( header.transform, mat4x4, _countof( header.transform ) * sizeof( *header.transform ) );

ensure( file_write_struct( output->hnd, &header ) );

output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_DescribeModelNode() */


/*******************************************************************
*
*   AssetFile_DescribeShader()
*
*   DESCRIPTION:
*       Provide the byte code size of the shader under write.
*
*******************************************************************/

b8 AssetFile_DescribeShader( const u32 byte_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_SHADER
 || !output->asset_start
 || !file_seek( output->hnd, output->asset_start ) )
    {
    return( FALSE );
    }

ShaderHeader header = {};
header.byte_size = byte_size;

ensure( file_write_struct( output->hnd, &header ) );
output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_DescribeShader() */


/*******************************************************************
*
*   AssetFile_DescribeTexture()
*
*   DESCRIPTION:
*       Provide the dimensions of the texture under write.
*
*******************************************************************/

b8 AssetFile_DescribeTexture( const u32 byte_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !output->asset_start
 || !file_seek( output->hnd, output->asset_start ) )
    {
    return( FALSE );
    }

TextureHeader header = {};
header.byte_size  = byte_size;

ensure( file_write_struct( output->hnd, &header ) );
output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_DescribeTexture() */


/*******************************************************************
*
*   AssetFile_DescribeTexture2()
*
*   DESCRIPTION:
*       Provide the dimensions of the texture under write.
*
*******************************************************************/

b8 AssetFile_DescribeTexture2( const u32 channel_cnt, const u32 width, const u32 height, const u32 byte_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !output->asset_start
 || !file_seek( output->hnd, output->asset_start ) )
    {
    return( FALSE );
    }

TextureHeader header = {};
header.byte_size   = byte_size;
header.width       = width;
header.height      = height;
header.channel_cnt = channel_cnt;

ensure( file_write_struct( output->hnd, &header ) );
output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_DescribeTexture2() */


/*******************************************************************
*
*   AssetFile_DescribeTextureExtents()
*
*   DESCRIPTION:
*       Provide the number of elements in the texture extent map
*       table.
*
*******************************************************************/

b8 AssetFile_DescribeTextureExtents( const u16 element_cnt, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
 || !output->asset_start
 || !file_seek( output->hnd, output->asset_start ) )
    {
    return( FALSE );
    }

TextureExtentHeader header = {};
header.texture_cnt = element_cnt;

ensure( file_write_struct( output->hnd, &header ) );
output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

}   /* AssetFile_DescribeTextureExtents() */


/*******************************************************************
*
*   AssetFile_EndReadingAsset()
*
*   DESCRIPTION:
*       Finish reading an asset.
*
*******************************************************************/

b8 AssetFile_EndReadingAsset( AssetFileReader *input )
{
if( input->kind == ASSET_FILE_ASSET_KIND_INVALID
 || !input->asset_start )
    {
    return( FALSE );
    }

input->asset_start = 0;
input->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( TRUE );

} /* AssetFile_EndReadingAsset() */


/*******************************************************************
*
*   AssetFile_EndWritingAsset()
*
*   DESCRIPTION:
*       Finish writing a model by setting its root node.
*
*******************************************************************/

b8 AssetFile_EndWritingAsset( AssetFileWriter *output )
{
output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( TRUE );

}   /* AssetFile_EndWritingAsset() */


/*******************************************************************
*
*   AssetFile_EndWritingModel()
*
*   DESCRIPTION:
*       Finish writing a model by setting its root node.
*
*******************************************************************/

b8 AssetFile_EndWritingModel( const u32 root_node_element, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( FALSE );
    }

if( !file_seek( output->hnd, output->asset_start ) )
    {
    return( FALSE );
    }

ModelHeader header = {};
ensure( file_read_struct( output->hnd, &header ) );

header.root_node_element  = root_node_element;
header.total_index_count  = output->model_indices_written;
header.total_vertex_count = output->model_vertices_written;
if( !file_seek( output->hnd, output->asset_start ) )
    {
    return( FALSE );
    }
    
ensure( file_write_struct( output->hnd, &header ) );
if( !file_seek( output->hnd, output->caret ) )
    {
    return( FALSE );
    }

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;
output->model_indices_written = 0;
output->model_vertices_written = 0;

return( TRUE );

} /* AssetFile_EndWritingModel() */


/*******************************************************************
*
*   AssetFile_EndWritingTextureExtents()
*
*   DESCRIPTION:
*       Finish writing texture extents.
*
*******************************************************************/

b8 AssetFile_EndWritingTextureExtents( AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
 || !output->asset_start )
    {
    return( FALSE );
    }

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( TRUE );

}   /* AssetFile_EndWritingTextureExtents() */


/*******************************************************************
*
*   AssetFile_GetWriteSize()
*
*   DESCRIPTION:
*       Get the current file written size.
*
*******************************************************************/

u64 AssetFile_GetWriteSize( const AssetFileWriter *output )
{
return( output->caret );

} /* AssetFile_GetWriteSize() */


/*******************************************************************
*
*   AssetFile_OpenForRead()
*
*   DESCRIPTION:
*       Open the asset file for read-only.
*
*******************************************************************/

b8 AssetFile_OpenForRead( const char *filename, AssetFileReader *input )
{
*input = {};
if( !file_open( filename, "rb", &input->hnd ) )
    {
    return( FALSE );
    }

AssetFileHeader file_header = {};
if( !file_read_struct( input->hnd, &file_header ) )
    {
    ensure( file_close( input->hnd ) );
    input->hnd = 0;
    return( FALSE );
    }

if( file_header.magic != ASSET_FILE_MAGIC )
    {
    ensure( file_close( input->hnd ) );
    input->hnd = 0;
    return( FALSE );
    }

input->table_cnt = file_header.table_cnt;

return( TRUE );

} /* AssetFile_OpenForRead() */


/*******************************************************************
*
*   AssetFile_ReadFontGlyphs()
*
*   DESCRIPTION:
*       Read and output a font's glyph data.
*
*******************************************************************/

b8 AssetFile_ReadFontGlyphs( const u16 glyph_capacity, AssetFileFontGlyph *glyphs, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_FONT
 || !input->asset_start
 || glyphs == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

FontHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
    }

if( glyph_capacity < header.glyph_cnt )
    {
    return( FALSE );
    }
    
if( !file_seek( input->hnd, header.glyphs_starts_at ) )
    {
    return( FALSE );
    }

f32 width_scale  = 1.0f / (f32)header.oversample_x;
f32 height_scale = 1.0f / (f32)header.oversample_y;

for( u32 i = 0; i < header.glyph_cnt; i++ )
    {
    FontGlyphHeader glyph = {};
    if( !file_read_struct( input->hnd, &glyph ) )
        {
        return( FALSE );
        }

    AssetFileFontGlyph *out = glyphs + i;
    
    out->glyph          = glyph.glyph;
    out->width          = width_scale  * ( glyph.u1 - glyph.u0 );
    out->height         = height_scale * ( glyph.v1 - glyph.v0 );
    out->top_left_x     = glyph.pen_offset_x;
    out->top_left_y     = glyph.pen_offset_y;
    out->bottom_right_x = out->top_left_x + out->width;
    out->bottom_right_y = out->top_left_y + out->height;
    out->u0             = (f32)glyph.u0 / header.texture_width;
    out->v0             = (f32)glyph.v0 / header.texture_height;
    out->u1             = (f32)glyph.u1 / header.texture_width;
    out->v1             = (f32)glyph.v1 / header.texture_height;
    out->h_advance      = glyph.h_advance;
    }

return( TRUE );

}   /* AssetFile_ReadFontGlyphs() */


/*******************************************************************
*
*   AssetFile_ReadFontTexture()
*
*   DESCRIPTION:
*       Read the font's texture dimensions and pixel data.
*
*******************************************************************/

b8 AssetFile_ReadFontTexture( const u32 buffer_sz, u8 *pixels, u16 *width, u16 *height, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_FONT
 || !input->asset_start
 || pixels == NULL
 || width == NULL
 || height == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

FontHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
    }

*width  = header.texture_width;
*height = header.texture_height;

if( !file_seek( input->hnd, header.texture_starts_at ) )
    {
    return( FALSE );
    }
    
if( !file_read( input->hnd, header.texture_sz, pixels ) )
    {
    return( FALSE );
    }

return( TRUE );

}   /* AssetFile_ReadFontTexture() */


/*******************************************************************
*
*   AssetFile_ReadFontStorageRequirements()
*
*   DESCRIPTION:
*       Read the storage needed to query a font.
*
*******************************************************************/

b8 AssetFile_ReadFontStorageRequirements( u16 *glyph_cnt, u32 *texture_sz, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_FONT
 || !input->asset_start
 || glyph_cnt == NULL
 || texture_sz == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

FontHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
    }

*glyph_cnt  = header.glyph_cnt;
*texture_sz = header.texture_sz;

return( TRUE );

}   /* AssetFile_ReadFontStorageRequirements() */


/*******************************************************************
*
*   AssetFile_ReadModelMaterials()
*
*   DESCRIPTION:
*       Read and output the given model's materials.
*
*******************************************************************/

b8 AssetFile_ReadModelMaterials( const u32 material_capacity, u32 *material_count, AssetFileModelMaterial *materials, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start
 || materials == NULL
 || material_count == NULL )
    {
    return( FALSE );
    }

*material_count = 0;

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

ModelHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
    }

if( header.material_cnt > material_capacity )
    {
    return( FALSE );
    }

for( u32 i = 0; i < header.material_cnt; i++ )
    {
    materials[ i ] = {};

    if( !JumpToModelMaterial( input->asset_start, i, input->hnd ) )
        {
        return( FALSE );
        }

    ModelMaterialHeader material = {};
    if( !file_read_struct( input->hnd, &material ) )
        {
        return( FALSE );
        }

    materials[ i ].bits = material.map_bits;
    for( u32 j = 0; j < ASSET_FILE_MODEL_TEXTURE_COUNT; j++ )
        {
        if( !( material.map_bits & ( 1 << j ) ) )
            {
            continue;
            }

        AssetFileAssetId element;
        if( !file_read_struct( input->hnd, &element ) )
            {
            return( FALSE );
            }

        materials[ i ].textures[ j ] = element;
        }

    }

*material_count = header.material_cnt;
return( TRUE );

} /* AssetFile_ReadModelMaterials() */


/*******************************************************************
*
*   AssetFile_ReadModelMeshIndices()
*
*   DESCRIPTION:
*       Read and output the given model mesh's indices.
*
*******************************************************************/

b8 AssetFile_ReadModelMeshIndices( const u32 mesh_index, const u32 index_capacity, u32 *index_count, AssetFileModelIndex *indices, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start
 || indices == NULL
 || index_count == NULL )
    {
    return( FALSE );
    }

*index_count = 0;

if( !JumpToModelMesh( input->asset_start, mesh_index, input->hnd ) )
    {
    return( FALSE );
    }

ModelMeshHeader mesh = {};
if( !file_read_struct( input->hnd, &mesh ) )
    {
    return( FALSE );
    }

if( index_capacity < mesh.index_cnt )
    {
    return( FALSE );
    }
    
/* Geometry order is... 
 a) VERTICES
 b) INDICES <-- Look here */
if( !file_seek_rel( input->hnd, sizeof(AssetFileModelVertex) * mesh.vertex_cnt ) )
    {
    return( FALSE );
    }

if( !file_read_array( input->hnd, mesh.index_cnt, indices ) )
    {
    return( FALSE );
    }

*index_count = mesh.index_cnt;
return( TRUE );

} /* AssetFile_ReadModelMeshIndices() */


/*******************************************************************
*
*   AssetFile_ReadModelMeshVertices()
*
*   DESCRIPTION:
*       Read and output the given model mesh's vertices.       
*
*******************************************************************/

b8 AssetFile_ReadModelMeshVertices( const u32 mesh_index, const u32 vertex_capacity, AssetFileModelIndex *material_index, u32 *vertex_count, AssetFileModelVertex *vertices, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start
 || vertices == NULL
 || vertex_count == NULL )
    {
    return( FALSE );
    }

*vertex_count = 0;

if( !JumpToModelMesh( input->asset_start, mesh_index, input->hnd ) )
    {
    return( FALSE );
    }

ModelMeshHeader mesh = {};
if( !file_read_struct( input->hnd, &mesh ) )
    {
    return( FALSE );
    }

if( vertex_capacity < mesh.vertex_cnt )
    {
    return( FALSE );
    }

if( material_index )
    {
    *material_index = (AssetFileModelIndex)mesh.material;
    }
    
/* Geometry order is... 
 a) VERTICES <-- Look here
 b) INDICES */
if( !file_read_array( input->hnd, mesh.vertex_cnt, vertices ) )
    {
    return( FALSE );
    }

*vertex_count = mesh.vertex_cnt;
return( TRUE );

} /* AssetFile_ReadModelMeshVertices() */


/*******************************************************************
*
*   AssetFile_ReadModelNodes()
*
*   DESCRIPTION:
*       Read and output the model-under-read's node tree.
*
*******************************************************************/

b8 AssetFile_ReadModelNodes( const u32 node_capacity, u32 *node_count, AssetFileModelNode *nodes, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start
 || nodes == NULL )
    {
    return( FALSE );
    }

*node_count = 0;

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

ModelHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
    }

if( header.node_count > node_capacity )
    {
    return( FALSE );
    }

for( u32 i = 0; i < header.node_count; i++ )
    {
    nodes[ i ] = {};

    if( !JumpToModelNode( input->asset_start, i, input->hnd ) )
        {
        return( FALSE );
        }

    ModelNodeHeader node = {};
    if( !file_read_struct( input->hnd, &node ) )
        {
        return( FALSE );
        }

    AssetFileModelIndex element;

    /* transform */
    memcpy( nodes[ i ].transform, node.transform, _countof( nodes->transform ) * sizeof( *nodes->transform ) );

    /* nodes */
    for( u32 j = 0; j < node.node_count; j++ )
        {
        if( !file_read_struct( input->hnd, &element ) )
            {
            return( FALSE );
            }

        nodes[ i ].child_nodes[ nodes[ i ].child_node_count++ ] = element - ( header.material_cnt + header.mesh_count );
        }

    /* meshes */
    for( u32 j = 0; j < node.mesh_count; j++ )
        {
        if( !file_read_struct( input->hnd, &element ) )
            {
            return( FALSE );
            }

        nodes[ i ].child_meshes[ nodes[ i ].child_mesh_count++ ] = element - header.material_cnt;
        }
    }

*node_count = header.node_count;

return( TRUE );

} /* AssetFile_ReadModelNodes() */


/*******************************************************************
*
*   AssetFile_ReadModelStorageRequirements()
*
*   DESCRIPTION:
*       Read the count of each of the model's elements.
*
*******************************************************************/

b8 AssetFile_ReadModelStorageRequirements( u32 *vertex_count, u32 *index_count, u32 *mesh_count, u32 *node_count, u32 *material_count, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

ModelHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
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

if( material_count )
    {
    *material_count = header.material_cnt;
    }

return( TRUE );

} /* AssetFile_ReadModelStorageRequirements() */


/*******************************************************************
*
*   AssetFile_ReadShaderBinary()
*
*   DESCRIPTION:
*       Read the binary code for the shader under read, and copy it
*       into the given buffer.
*
*******************************************************************/

b8 AssetFile_ReadShaderBinary( const u32 buffer_sz, u32 *read_sz, u8 *buffer, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_SHADER
 || !input->asset_start
 || buffer == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

ShaderHeader header = {};
if( !file_read_struct( input->hnd, &header )
 || buffer_sz < header.byte_size )
    {
    return( FALSE );
    }
    
if( !file_read_buffer( input->hnd, buffer_sz, header.byte_size, buffer ) )
    {
    return( FALSE );
    }

if( read_sz != NULL )
    {
    *read_sz = header.byte_size;
    }

return( TRUE );

} /* AssetFile_ReadShaderBinary() */


/*******************************************************************
*
*   AssetFile_ReadShaderStorageRequirements()
*
*   DESCRIPTION:
*       Read the buffer size required for the shader under read.
*
*******************************************************************/

b8 AssetFile_ReadShaderStorageRequirements( u32 *byte_count, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_SHADER
 || !input->asset_start
 || byte_count == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

ShaderHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
    }

*byte_count = header.byte_size;
return( TRUE );

} /* AssetFile_ReadShaderStorageRequirements() */


/*******************************************************************
*
*   AssetFile_ReadSoundPairs()
*
*   DESCRIPTION:
*       Read the binary and return the sound pairs data. 
*       Works for both sample and music pairs.
*       
*******************************************************************/

b8 AssetFile_ReadSoundPairs( u16 num_pairs, AssetFileSoundPair *sound_pairs, AssetFileReader *input )
{
u16 num_elements;
if( !AssetFile_ReadSoundPairsStorageRequirements( &num_elements, input )
 || num_pairs < num_elements
 || sound_pairs == NULL )
    {
    return( FALSE );
    }

ensure( file_read_array( input->hnd, num_elements, sound_pairs ) );

return( TRUE );
   
} /* AssetFile_ReadSoundPairs() */


/*******************************************************************
*
*   AssetFile_ReadSoundPairsStorageRequirements()
*
*   DESCRIPTION:
*       Read the array size required for the sound paired asset ID/index data.
*
*******************************************************************/

b8 AssetFile_ReadSoundPairsStorageRequirements( u16 *num_elements, AssetFileReader *input )
{
if( ( input->kind != ASSET_FILE_ASSET_KIND_SOUND_SAMPLE
   && input->kind != ASSET_FILE_ASSET_KIND_SOUND_MUSIC_CLIP )
 || !input->asset_start
 || num_elements == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

if( !file_read_struct( input->hnd, num_elements ) )
    {
    return( FALSE );
    }

return( TRUE );

} /* AssetFile_ReadSoundPairsStorageRequirements() */


/*******************************************************************
*
*   AssetFile_ReadTextureBinary()
*
*   DESCRIPTION:
*       Read the binary compressed image data for the texture under
*       read, and copy it into the given buffer.
*
*******************************************************************/

b8 AssetFile_ReadTextureBinary( const u32 buffer_sz, u32 *read_sz, byte *buffer, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !input->asset_start
 || buffer == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

TextureHeader header = {};
if( !file_read_struct( input->hnd, &header )
 || buffer_sz < header.byte_size )
    {
    return( FALSE );
    }
    
if( !file_read( input->hnd, header.byte_size, buffer ) )
    {
    return( FALSE );
    }

if( read_sz != NULL )
    {
    *read_sz = header.byte_size;
    }

return( TRUE );

} /* AssetFile_ReadTextureBinary() */


/*******************************************************************
*
*   AssetFile_ReadTextureStorageRequirements()
*
*   DESCRIPTION:
*       Read the buffer size required for the texture under read.
*
*******************************************************************/

b8 AssetFile_ReadTextureStorageRequirements( u32 *channel_cnt, u32 *width, u32 *height, u32 *byte_count, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !input->asset_start
 || channel_cnt == NULL
 || width == NULL
 || height == NULL
 || byte_count == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

TextureHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
    }

*channel_cnt = header.channel_cnt;
*width = header.width;
*height = header.height;
*byte_count = header.byte_size;
return( TRUE );

} /* AssetFile_ReadTextureStorageRequirements() */


/*******************************************************************
*
*   AssetFile_ReadTextureExtents()
*
*   DESCRIPTION:
*       Read the texture extent table array.
*
*******************************************************************/

b8 AssetFile_ReadTextureExtents( const u16 output_cnt, AssetFileTextureExtent *out_elements, AssetFileReader *input )
{
u16 element_cnt;
if( !AssetFile_ReadTextureExtentsStorageRequirements( &element_cnt, input )
 || output_cnt < element_cnt )
    {
    return( FALSE );
    }

for( u16 i = 0; i < element_cnt; i++ )
    {
    AssetFileTextureExtent *element = &out_elements[ i ];

    if( !file_read_struct( input->hnd, &element->texture_id )
     || !file_read_struct( input->hnd, &element->width )
     || !file_read_struct( input->hnd, &element->height ) )
        {
        return( FALSE );
        }
    }

return( TRUE );

} /* AssetFile_ReadTextureExtents() */


/*******************************************************************
*
*   AssetFile_ReadTextureExtentsStorageRequirements()
*
*   DESCRIPTION:
*       Read the array size required for the texture extent table.
*
*******************************************************************/

b8 AssetFile_ReadTextureExtentsStorageRequirements( u16 *element_cnt, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
 || !input->asset_start
 || element_cnt == NULL )
    {
    return( FALSE );
    }

if( !file_seek( input->hnd, input->asset_start ) )
    {
    return( FALSE );
    }

TextureExtentHeader header = {};
if( !file_read_struct( input->hnd, &header ) )
    {
    return( FALSE );
    }

*element_cnt = header.texture_cnt;

return( TRUE );

} /* AssetFile_ReadTextureExtentsStorageRequirements() */


/*******************************************************************
*
*   AssetFile_WriteFontGlyph()
*
*   DESCRIPTION:
*       Write a font glyph's character data.
*
*******************************************************************/

b8 AssetFile_WriteFontGlyph( const u8 glyph, const u16 u0, const u16 v0, const u16 u1, const u16 v1, const f32 pen_dx, const f32 pen_dy, const f32 pen_xadvance, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_FONT
 || !output->asset_start )
    {
    return( FALSE );
    }

FontGlyphHeader header = {};
header.glyph        = glyph;
header.u0           = u0;
header.v0           = v0;
header.u1           = u1;
header.v1           = v1;
header.h_advance    = pen_xadvance;
header.pen_offset_x = pen_dx;
header.pen_offset_y = pen_dy;

ensure( file_write_struct( output->hnd, &header ) );
output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

}   /* AssetFile_WriteFontGlyph() */


/*******************************************************************
*
*   AssetFile_WriteModelMaterialTextureMaps()
*
*   DESCRIPTION:
*       Write the given mesh material texture map asset IDs.
*
*******************************************************************/

b8 AssetFile_WriteModelMaterialTextureMaps( const AssetFileAssetId *asset_ids, const u8 count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( FALSE );
    }

ensure( file_write_array( output->hnd, count, asset_ids ) );

output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_WriteModelMaterialTextureMaps() */


/*******************************************************************
*
*   AssetFile_WriteModelMeshIndex()
*
*   DESCRIPTION:
*       Write the given mesh index.
*
*******************************************************************/

b8 AssetFile_WriteModelMeshIndex( const AssetFileModelIndex index, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( FALSE );
    }

ensure( file_write_struct( output->hnd, &index ) );

output->caret = (u32)file_get_pos( output->hnd );
output->model_indices_written++;

return( TRUE );

} /* AssetFile_WriteModelMeshIndex() */


/*******************************************************************
*
*   AssetFile_WriteModelMeshVertex()
*
*   DESCRIPTION:
*       Write the given mesh vertex.
*
*******************************************************************/

b8 AssetFile_WriteModelMeshVertex( const AssetFileModelVertex *vertex, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( FALSE );
    }

ensure( file_write_struct( output->hnd, vertex ) );

output->caret = (u32)file_get_pos( output->hnd );
output->model_vertices_written++;

return( TRUE );

} /* AssetFile_WriteModelMeshVertex() */


/*******************************************************************
*
*   AssetFile_WriteModelNodeChildElements()
*
*   DESCRIPTION:
*       Write the child node/mesh element indices for a node.
*
*******************************************************************/

b8 AssetFile_WriteModelNodeChildElements( const AssetFileModelIndex *element_ids, const u32 count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( FALSE );
    }

ensure( file_write_array( output->hnd, count, element_ids ) );

output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

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

b8 AssetFile_WriteShader( const byte *blob, const u32 blob_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_SHADER
 || !output->asset_start )
    {
    return( FALSE );
    }

ensure( file_write( output->hnd, blob_size, blob ) );

output->caret = (u32)file_get_pos( output->hnd );

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( TRUE );

} /* AssetFile_WriteShader() */


/*******************************************************************
*
*   AssetFile_WriteSoundPairs()
*
*   DESCRIPTION:
*       Write the sound asset ID/index pair data to the asset binary.
*       This also ends the asset writing session.
*
*******************************************************************/

b8 AssetFile_WriteSoundPairs( const AssetFileSoundPair *sound_pair, const u16 num_pairs, AssetFileWriter *output )
{
if( !output->asset_start
 || ( output->kind != ASSET_FILE_ASSET_KIND_SOUND_SAMPLE
   && output->kind != ASSET_FILE_ASSET_KIND_SOUND_MUSIC_CLIP ) )
    {
    return( FALSE );
    }

u32 size_write = sizeof( *sound_pair ) * num_pairs;
ensure( file_write_struct( output->hnd, &num_pairs ) );
ensure( file_write_array( output->hnd, num_pairs, sound_pair ) );

output->caret = (u32)file_get_pos( output->hnd );

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( TRUE );

} /* AssetFile_WriteSoundPairs() */


/*******************************************************************
*
*   AssetFile_WriteTexture()
*
*   DESCRIPTION:
*       Write the texture data to the asset binary.  This also ends
*       the asset writing session.
*
*******************************************************************/

b8 AssetFile_WriteTexture( const byte *image, const u32 image_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !output->asset_start )
    {
    return( FALSE );
    }

ensure( file_write( output->hnd, image_size, image ) );

output->caret = (u32)file_get_pos( output->hnd );

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( TRUE );

} /* AssetFile_WriteTexture() */


/*******************************************************************
*
*   AssetFile_WriteTextureExtent()
*
*   DESCRIPTION:
*       Write the texture data to the asset binary.  This also ends
*       the asset writing session.
*
*******************************************************************/

b8 AssetFile_WriteTextureExtent( const AssetFileAssetId id, const u16 width, const u16 height, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
 || !output->asset_start )
    {
    return( FALSE );
    }

ensure( file_write_struct( output->hnd, &id ) );
ensure( file_write_struct( output->hnd, &width ) );
ensure( file_write_struct( output->hnd, &height ) );

output->caret = (u32)file_get_pos( output->hnd );

return( TRUE );

} /* AssetFile_WriteTextureExtent() */


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

static b8 JumpToAssetInTable( const AssetFileAssetId id, const u32 table_count, fhnd file )
{
long table_start = (long)sizeof(AssetFileHeader);
long row_stride  = (long)sizeof(AssetFileTableRow);

b8 found_it = FALSE;
long remain = (long)table_count;
long middle;
long top = 0;
AssetFileTableRow row;
while( remain > 0 )
    {
    middle = top + remain / 2;
    
    if( !file_seek( file, table_start + middle * row_stride ) )
        {
        return( FALSE );
        }

    row = {};
    if( !file_read_struct( file, &row ) )
        {
        return( FALSE );
        }

    if( id == row.id )
        {
        found_it = TRUE;
        if( !file_seek_rel( file, -row_stride ) )
            {
            return( FALSE );
            }

        break;
        }
    else if( id > row.id )
        {
        top = middle + 1;
        remain -= 1 - remain % 2;
        }

    remain /= 2;
    }

return( found_it );

} /* JumpToAssetInTable() */


/*******************************************************************
*
*   JumpToModelMaterials()
*
*   DESCRIPTION:
*       Jump the file caret to the start of the current model's
*       material, given the material index.
*
*******************************************************************/

static b8 JumpToModelMaterial( const u32 asset_start, const u32 material_index, fhnd file )
{
if( !file_seek( file, asset_start ) )
    {
    return( FALSE );
    }

ModelHeader header = {};
if( !file_read_struct( file, &header ) )
    {
    return( FALSE );
    }

/* Element table order is...  
 a) MATERIALS <-- Look here
 b) MESHES
 c) NODES */

u32 element_location = asset_start
                     + (u32)sizeof(ModelHeader)
                     + material_index * (u32)sizeof(ModelTableRow);

if( !file_seek( file, element_location ) )
    {
    return( FALSE );
    }

ModelTableRow element = {};
if( !file_read_struct( file, &element )
 || element.kind != ASSET_FILE_MODEL_ELEMENT_KIND_MATERIAL )
    {
    return( FALSE );
    }

if( !file_seek( file, element.starts_at ) )
    {
    return( FALSE );
    }

return( TRUE );

} /* JumpToModelMaterials() */


/*******************************************************************
*
*   JumpToModelMesh()
*
*   DESCRIPTION:
*       Jump the file caret to the start of the current model's
*       mesh, given the mesh index.
*
*******************************************************************/

static b8 JumpToModelMesh( const u32 asset_start, const u32 mesh_index, fhnd file )
{
if( !file_seek( file, asset_start ) )
    {
    return( FALSE );
    }

ModelHeader header = {};
if( !file_read_struct( file, &header ) )
    {
    return( FALSE );
    }

if( mesh_index >= header.mesh_count )
    {
    return( FALSE );
    }

/* Element table order is...  
 a) MATERIALS
 b) MESHES <-- Look here
 c) NODES */
u32 element_location = asset_start
                     + (u32)sizeof(ModelHeader)
                     + ( header.material_cnt + mesh_index ) * (u32)sizeof(ModelTableRow);

if( !file_seek( file, element_location ) )
    {
    return( FALSE );
    }

ModelTableRow element = {};
if( !file_read_struct( file, &element )
 || element.kind != ASSET_FILE_MODEL_ELEMENT_KIND_MESH )
    {
    return( FALSE );
    }

if( !file_seek( file, element.starts_at ) )
    {
    return( FALSE );
    }

return( TRUE );

} /* JumpToModelMesh() */


/*******************************************************************
*
*   JumpToModelNode()
*
*   DESCRIPTION:
*       Jump the file caret to the start of the current model's
*       node, given the node index.
*
*******************************************************************/

static b8 JumpToModelNode( const u32 asset_start, const u32 node_index, fhnd file )
{
if( !file_seek( file, asset_start ) )
    {
    return( FALSE );
    }

ModelHeader header = {};
if( !file_read_struct( file, &header ) )
    {
    return( FALSE );
    }

/* Element table order is...  
 a) MATERIALS
 b) MESHES
 c) NODES  <-- Look here */
u32 element_location = asset_start
                     + (u32)sizeof(ModelHeader)
                     + ( header.material_cnt + header.mesh_count + node_index ) * (u32)sizeof(ModelTableRow);

if( !file_seek( file, element_location ) )
    {
    return( FALSE );
    }

ModelTableRow element = {};
if( !file_read_struct( file, &element )
 || element.kind != ASSET_FILE_MODEL_ELEMENT_KIND_NODE )
    {
    return( FALSE );
    }

if( !file_seek( file, element.starts_at ) )
    {
    return( FALSE );
    }

return( TRUE );

} /* JumpToModelNode() */

