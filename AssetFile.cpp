#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"

#define make_fourcc( _a, _b, _c, _d ) \
    ( ( _a << 0 ) | ( _b << 8 ) | ( _c << 16 ) | (_d << 24 ) )
        

static const uint32_t ASSET_FILE_MAGIC = make_fourcc( 'M', 'e', 'r', 'c' );

typedef struct
    {
    uint32_t            magic;      /* magic sentinel number        */
    uint32_t            table_cnt;  /* number entries in table      */
    } AssetFileHeader;

typedef struct
    {
    AssetFileAssetId    id;         /* asset ID hash                */
    AssetFileAssetKind  kind;       /* type of asset                */
    uint32_t            starts_at;  /* file offset to start of asset*/
    } AssetFileTableRow;

typedef struct
    {
    uint8_t             oversample_x;
                                    /* horizontal texels/pixels     */
    uint8_t             oversample_y;
                                    /* vertical texels/pixels       */
    uint16_t            texture_width;
                                    /* texture extent width         */
    uint16_t            texture_height;
                                    /* texture extent height        */
    uint16_t            glyph_cnt;  /* number glyphs in font        */
    uint32_t            texture_sz; /* texture data byte count      */
    uint32_t            glyphs_starts_at;
                                    /* file offset to glyph data    */
    uint32_t            texture_starts_at;
                                    /* file offset to texture data  */
    } FontHeader;

typedef struct
    {
    uint8_t             glyph;      /* glyph code                   */
    uint16_t            u0;         /* uv top-left x (pixels)       */
    uint16_t            v0;         /* uv top-left y (pixels)       */
    uint16_t            u1;         /* uv bottom-right x (pixels)   */
    uint16_t            v1;         /* uv bottom-right y (pixels)   */
    float               h_advance;  /* pen horizontal advancement   */
    float               pen_offset_x;
                                    /* horz offset from pen position*/
    float               pen_offset_y;
                                    /* vert offset from pen position*/
    } FontGlyphHeader;

typedef struct
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

typedef struct
    {
    AssetFileModelMaterialBits
                        map_bits;   /* present texture maps         */
    } ModelMaterialHeader;

typedef struct
    {
    uint32_t            vertex_cnt; /* number of vertices           */
    uint32_t            index_cnt;  /* number of indices            */
    uint32_t            material;   /* material element index       */
    } ModelMeshHeader;

typedef struct
    {
    uint32_t            node_count; /* number of child nodes        */
    uint32_t            mesh_count; /* number of child meshes       */
    float               transform[ 16 ];
                                    /* row major 4x4 matrix         */
    } ModelNodeHeader;

typedef struct
    {
    AssetFileModelElementKind
                        kind;       /* model element kind           */
    uint32_t            starts_at;  /* file offset to element start */
    } ModelTableRow;

typedef struct
    {
    uint32_t            byte_size;  /* byte code blob size          */
    } ShaderHeader;

typedef struct
    {
    uint32_t            channel_cnt;/* number of color channels     */
    uint32_t            width;      /* image width                  */
    uint32_t            height;     /* image height                 */
    uint32_t            byte_size;  /* compressed image blob size   */
    } TextureHeader;

typedef struct
    {
    uint16_t            texture_cnt;/* number of textures in table  */
    } TextureExtentHeader;


static bool JumpToAssetInTable( const AssetFileAssetId id, const uint32_t table_count, FILE *file );
static bool JumpToModelMaterial( const uint32_t asset_start, const uint32_t material_index, FILE *file );
static bool JumpToModelMesh( const uint32_t asset_start, const uint32_t mesh_index, FILE *file );
static bool JumpToModelNode( const uint32_t asset_start, const uint32_t node_index, FILE *file );


/*******************************************************************
*
*   AssetFile_BeginReadingAsset()
*
*   DESCRIPTION:
*       Start reading an asset of the given ID, by discovering its
*       place within the greater asset file.
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

bool AssetFile_BeginWritingModelElement( const AssetFileModelElementKind kind, const AssetFileModelIndex element_index, AssetFileWriter *output )
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

ModelTableRow row = {};
row.starts_at = output->caret;
row.kind      = kind;
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

errno_t err = fopen_s( &output->fhnd, filename, "w+b" );
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
*   AssetFile_DescribeFont()
*
*   DESCRIPTION:
*       Provide the details about a font being written.
*
*******************************************************************/

bool AssetFile_DescribeFont( const uint8_t oversample_x, const uint8_t oversample_y, const uint16_t texture_width, const uint16_t texture_height, const uint32_t texture_sz, const uint8_t *pixels, const uint16_t glyph_cnt, const uint8_t *glyph_codes, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_FONT
|| !output->asset_start
|| fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
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

ensure( fwrite( &header, 1, sizeof( header ), output->fhnd ) == sizeof( header ) );
ensure( fwrite( pixels, 1, texture_sz, output->fhnd ) == texture_sz );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

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

bool AssetFile_DescribeModelNode( const uint32_t node_count, const float *mat4x4, const uint32_t mesh_count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start
 || node_count > ASSET_FILE_MODEL_NODE_CHILD_NODE_MAX_COUNT 
 || mesh_count > ASSET_FILE_MODEL_NODE_CHILD_MESH_MAX_COUNT )
    {
    return( false );
    }

ModelNodeHeader header = {};
header.node_count = node_count;
header.mesh_count = mesh_count;
memcpy( header.transform, mat4x4, _countof( header.transform ) * sizeof( *header.transform ) );

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

bool AssetFile_DescribeTexture( const uint32_t byte_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !output->asset_start
 || fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }

TextureHeader header = {};
header.byte_size  = byte_size;

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
output->caret = (uint32_t)ftell( output->fhnd );

return( true );

} /* AssetFile_DescribeTexture() */


/*******************************************************************
*
*   AssetFile_DescribeTexture2()
*
*   DESCRIPTION:
*       Provide the dimensions of the texture under write.
*
*******************************************************************/

bool AssetFile_DescribeTexture2( const uint32_t channel_cnt, const uint32_t width, const uint32_t height, const uint32_t byte_size, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !output->asset_start
 || fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }

TextureHeader header = {};
header.byte_size   = byte_size;
header.width       = width;
header.height      = height;
header.channel_cnt = channel_cnt;

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
output->caret = (uint32_t)ftell( output->fhnd );

return( true );

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

bool AssetFile_DescribeTextureExtents( const uint16_t element_cnt, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
 || !output->asset_start
 || fseek( output->fhnd, output->asset_start, SEEK_SET ) )
    {
    return( false );
    }

TextureExtentHeader header = {};
header.texture_cnt = element_cnt;

ensure( fwrite( &header, 1, sizeof(header), output->fhnd ) == sizeof(header) );
output->caret = (uint32_t)ftell( output->fhnd );

return( true );

}   /* AssetFile_DescribeTextureExtents() */


/*******************************************************************
*
*   AssetFile_EndReadingAsset()
*
*   DESCRIPTION:
*       Finish reading an asset.
*
*******************************************************************/

bool AssetFile_EndReadingAsset( AssetFileReader *input )
{
if( input->kind == ASSET_FILE_ASSET_KIND_INVALID
 || !input->asset_start )
    {
    return( false );
    }

input->asset_start = 0;
input->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( true );

} /* AssetFile_EndReadingAsset() */


/*******************************************************************
*
*   AssetFile_EndWritingAsset()
*
*   DESCRIPTION:
*       Finish writing a model by setting its root node.
*
*******************************************************************/

bool AssetFile_EndWritingAsset( AssetFileWriter *output )
{
output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( true );

}   /* AssetFile_EndWritingAsset() */


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
*   AssetFile_EndWritingTextureExtents()
*
*   DESCRIPTION:
*       Finish writing texture extents.
*
*******************************************************************/

bool AssetFile_EndWritingTextureExtents( AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
 || !output->asset_start )
    {
    return( false );
    }

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return( true );

}   /* AssetFile_EndWritingTextureExtents() */


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
errno_t err = fopen_s( &input->fhnd, filename, "rb" );
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
*   AssetFile_ReadFontGlyphs()
*
*   DESCRIPTION:
*       Read and output a font's glyph data.
*
*******************************************************************/

bool AssetFile_ReadFontGlyphs( const uint16_t glyph_capacity, AssetFileFontGlyph *glyphs, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_FONT
 || !input->asset_start
 || glyphs == NULL )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

FontHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

if( glyph_capacity < header.glyph_cnt )
    {
    return( false );
    }
    
if( fseek( input->fhnd, header.glyphs_starts_at, SEEK_SET ) )
    {
    return( false );
    }

float width_scale  = 1.0f / (float)header.oversample_x;
float height_scale = 1.0f / (float)header.oversample_y;

for( uint32_t i = 0; i < header.glyph_cnt; i++ )
    {
    FontGlyphHeader glyph = {};
    if( fread_s( &glyph, sizeof(glyph), 1, sizeof(glyph), input->fhnd ) != sizeof(glyph) )
        {
        return( false );
        }

    AssetFileFontGlyph *out = &glyphs[ i ];
    
    out->glyph          = glyph.glyph;
    out->width          = width_scale  * ( glyph.u1 - glyph.u0 );
    out->height         = height_scale * ( glyph.v1 - glyph.v0 );
    out->top_left_x     = glyph.pen_offset_x;
    out->top_left_y     = glyph.pen_offset_y;
    out->bottom_right_x = out->top_left_x + out->width;
    out->bottom_right_y = out->top_left_y + out->height;
    out->u0             = (float)glyph.u0 / header.texture_width;
    out->v0             = (float)glyph.v0 / header.texture_height;
    out->u1             = (float)glyph.u1 / header.texture_width;
    out->v1             = (float)glyph.v1 / header.texture_height;
    out->h_advance      = glyph.h_advance;
    }

return( true );

}   /* AssetFile_ReadFontGlyphs() */


/*******************************************************************
*
*   AssetFile_ReadFontTexture()
*
*   DESCRIPTION:
*       Read the font's texture dimensions and pixel data.
*
*******************************************************************/

bool AssetFile_ReadFontTexture( const uint32_t buffer_sz, uint8_t *pixels, uint16_t *width, uint16_t *height, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_FONT
 || !input->asset_start
 || pixels == NULL
 || width == NULL
 || height == NULL )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

FontHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

*width  = header.texture_width;
*height = header.texture_height;

if( fseek( input->fhnd, header.texture_starts_at, SEEK_SET ) )
    {
    return( false );
    }
    
if( fread_s( pixels, header.texture_sz, 1, header.texture_sz, input->fhnd ) != header.texture_sz )
    {
    return( false );
    }

return( true );

}   /* AssetFile_ReadFontTexture() */


/*******************************************************************
*
*   AssetFile_ReadFontStorageRequirements()
*
*   DESCRIPTION:
*       Read the storage needed to query a font.
*
*******************************************************************/

bool AssetFile_ReadFontStorageRequirements( uint16_t *glyph_cnt, uint32_t *texture_sz, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_FONT
 || !input->asset_start
 || glyph_cnt == NULL
 || texture_sz == NULL )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

FontHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

*glyph_cnt  = header.glyph_cnt;
*texture_sz = header.texture_sz;

return( true );

}   /* AssetFile_ReadFontStorageRequirements() */


/*******************************************************************
*
*   AssetFile_ReadModelMaterials()
*
*   DESCRIPTION:
*       Read and output the given model's materials.
*
*******************************************************************/

bool AssetFile_ReadModelMaterials( const uint32_t material_capacity, uint32_t *material_count, AssetFileModelMaterial *materials, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start
 || materials == NULL
 || material_count == NULL )
    {
    return( false );
    }

*material_count = 0;

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

ModelHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

if( header.material_cnt > material_capacity )
    {
    return( false );
    }

for( uint32_t i = 0; i < header.material_cnt; i++ )
    {
    materials[ i ] = {};

    if( !JumpToModelMaterial( input->asset_start, i, input->fhnd ) )
        {
        return( false );
        }

    ModelMaterialHeader material = {};
    if( fread_s( &material, sizeof( material ), 1, sizeof( material ), input->fhnd ) != sizeof( material ) )
        {
        return( false );
        }

    materials[ i ].bits = material.map_bits;
    for( uint32_t j = 0; j < ASSET_FILE_MODEL_TEXTURES_COUNT; j++ )
        {
        if( !( material.map_bits & ( 1 << j ) ) )
            {
            continue;
            }

        AssetFileAssetId element;
        if( fread_s( &element, sizeof(element), 1, sizeof(element), input->fhnd ) != sizeof(element) )
            {
            return( false );
            }

        materials[ i ].textures[ j ] = element;
        }

    }

*material_count = header.material_cnt;
return( true );

} /* AssetFile_ReadModelMaterials() */


/*******************************************************************
*
*   AssetFile_ReadModelMeshIndices()
*
*   DESCRIPTION:
*       Read and output the given model mesh's indices.
*
*******************************************************************/

bool AssetFile_ReadModelMeshIndices( const uint32_t mesh_index, const uint32_t index_capacity, uint32_t *index_count, AssetFileModelIndex *indices, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start
 || indices == NULL
 || index_count == NULL )
    {
    return( false );
    }

*index_count = 0;

if( !JumpToModelMesh( input->asset_start, mesh_index, input->fhnd ) )
    {
    return( false );
    }

ModelMeshHeader mesh = {};
if( fread_s( &mesh, sizeof(mesh), 1, sizeof(mesh), input->fhnd ) != sizeof(mesh) )
    {
    return( false );
    }

if( index_capacity < mesh.index_cnt )
    {
    return( false );
    }
    
/* Geometry order is... 
 a) VERTICES
 b) INDICES <-- Look here */
if( fseek( input->fhnd, sizeof(AssetFileModelVertex) * mesh.vertex_cnt, SEEK_CUR ) )
    {
    return( false );
    }

if( fread_s( indices, index_capacity * sizeof(AssetFileModelIndex), sizeof(AssetFileModelIndex), mesh.index_cnt, input->fhnd ) != mesh.index_cnt )
    {
    return( false );
    }

*index_count = mesh.index_cnt;
return( true );

} /* AssetFile_ReadModelMeshIndices() */


/*******************************************************************
*
*   AssetFile_ReadModelMeshVertices()
*
*   DESCRIPTION:
*       Read and output the given model mesh's vertices.       
*
*******************************************************************/

bool AssetFile_ReadModelMeshVertices( const uint32_t mesh_index, const uint32_t vertex_capacity, AssetFileModelIndex *material_index, uint32_t *vertex_count, AssetFileModelVertex *vertices, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start
 || vertices == NULL
 || vertex_count == NULL )
    {
    return( false );
    }

*vertex_count = 0;

if( !JumpToModelMesh( input->asset_start, mesh_index, input->fhnd ) )
    {
    return( false );
    }

ModelMeshHeader mesh = {};
if( fread_s( &mesh, sizeof(mesh), 1, sizeof(mesh), input->fhnd ) != sizeof(mesh) )
    {
    return( false );
    }

if( vertex_capacity < mesh.vertex_cnt )
    {
    return( false );
    }

if( material_index )
    {
    *material_index = (AssetFileModelIndex)mesh.material;
    }
    
/* Geometry order is... 
 a) VERTICES <-- Look here
 b) INDICES */
if( fread_s( vertices, vertex_capacity * sizeof(AssetFileModelVertex), sizeof(AssetFileModelVertex), mesh.vertex_cnt, input->fhnd ) != mesh.vertex_cnt )
    {
    return( false );
    }

*vertex_count = mesh.vertex_cnt;
return( true );

} /* AssetFile_ReadModelMeshVertices() */


/*******************************************************************
*
*   AssetFile_ReadModelNodes()
*
*   DESCRIPTION:
*       Read and output the model-under-read's node tree.
*
*******************************************************************/

bool AssetFile_ReadModelNodes( const uint32_t node_capacity, uint32_t *node_count, AssetFileModelNode *nodes, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !input->asset_start
 || nodes == NULL )
    {
    return( false );
    }

*node_count = 0;

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

ModelHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

if( header.node_count > node_capacity )
    {
    return( false );
    }

for( uint32_t i = 0; i < header.node_count; i++ )
    {
    nodes[ i ] = {};

    if( !JumpToModelNode( input->asset_start, i, input->fhnd ) )
        {
        return( false );
        }

    ModelNodeHeader node = {};
    if( fread_s( &node, sizeof( node ), 1, sizeof( node ), input->fhnd ) != sizeof( node ) )
        {
        return( false );
        }

    AssetFileModelIndex element;

    /* transform */
    memcpy( nodes[ i ].transform, node.transform, _countof( nodes->transform ) * sizeof( *nodes->transform ) );

    /* nodes */
    for( uint32_t j = 0; j < node.node_count; j++ )
        {
        if( fread_s( &element, sizeof(element), 1, sizeof(element), input->fhnd ) != sizeof(element) )
            {
            return( false );
            }

        nodes[ i ].child_nodes[ nodes[ i ].child_node_count++ ] = element - ( header.material_cnt + header.mesh_count );
        }

    /* meshes */
    for( uint32_t j = 0; j < node.mesh_count; j++ )
        {
        if( fread_s( &element, sizeof(element), 1, sizeof(element), input->fhnd ) != sizeof(element) )
            {
            return( false );
            }

        nodes[ i ].child_meshes[ nodes[ i ].child_mesh_count++ ] = element - header.material_cnt;
        }
    }

*node_count = header.node_count;

return( true );

} /* AssetFile_ReadModelNodes() */


/*******************************************************************
*
*   AssetFile_ReadModelStorageRequirements()
*
*   DESCRIPTION:
*       Read the count of each of the model's elements.
*
*******************************************************************/

bool AssetFile_ReadModelStorageRequirements( uint32_t *vertex_count, uint32_t *index_count, uint32_t *mesh_count, uint32_t *node_count, uint32_t *material_count, AssetFileReader *input )
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

if( material_count )
    {
    *material_count = header.material_cnt;
    }

return( true );

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

bool AssetFile_ReadShaderBinary( const uint32_t buffer_sz, uint32_t *read_sz, uint8_t *buffer, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_SHADER
 || !input->asset_start
 || buffer == NULL )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

ShaderHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header)
 || buffer_sz < header.byte_size )
    {
    return( false );
    }
    
if( fread_s( buffer, buffer_sz, 1, header.byte_size, input->fhnd ) != header.byte_size )
    {
    return( false );
    }

if( read_sz != NULL )
    {
    *read_sz = header.byte_size;
    }

return( true );

} /* AssetFile_ReadShaderBinary() */


/*******************************************************************
*
*   AssetFile_ReadShaderStorageRequirements()
*
*   DESCRIPTION:
*       Read the buffer size required for the shader under read.
*
*******************************************************************/

bool AssetFile_ReadShaderStorageRequirements( uint32_t *byte_count, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_SHADER
 || !input->asset_start
 || byte_count == NULL )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

ShaderHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

*byte_count = header.byte_size;
return( true );

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

bool AssetFile_ReadSoundPairs(  uint16_t num_pairs, AssetFileSoundPair *sound_pairs, AssetFileReader *input )
{
uint16_t num_elements;
if( !AssetFile_ReadSoundPairsStorageRequirements( &num_elements, input )
 || num_pairs < num_elements
 || sound_pairs == NULL )
    {
    return(false);
    }

uint32_t read_size = sizeof(*sound_pairs) * num_elements;
ensure ( fread_s( sound_pairs, read_size, 1, read_size,input->fhnd ) == read_size );

return true;
   
} /* AssetFile_ReadSoundPairs() */


/*******************************************************************
*
*   AssetFile_ReadSoundPairsStorageRequirements()
*
*   DESCRIPTION:
*       Read the array size required for the sound paired asset ID/index data.
*
*******************************************************************/

bool AssetFile_ReadSoundPairsStorageRequirements( uint16_t *num_elements, AssetFileReader *input )
{
if( ( input->kind != ASSET_FILE_ASSET_KIND_SOUND_SAMPLE
   && input->kind != ASSET_FILE_ASSET_KIND_SOUND_MUSIC_CLIP )
 || !input->asset_start
 || num_elements == NULL)
    {
    return(false);
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return(false);
    }

if( fread_s( num_elements, sizeof( *num_elements ), 1, sizeof( *num_elements ), input->fhnd ) != sizeof( *num_elements ) )
    {
    return(false);
    }

return(true);

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

bool AssetFile_ReadTextureBinary( const uint32_t buffer_sz, uint32_t *read_sz, uint8_t *buffer, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !input->asset_start
 || buffer == NULL )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

TextureHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header)
 || buffer_sz < header.byte_size )
    {
    return( false );
    }
    
if( fread_s( buffer, buffer_sz, 1, header.byte_size, input->fhnd ) != header.byte_size )
    {
    return( false );
    }

if( read_sz != NULL )
    {
    *read_sz = header.byte_size;
    }

return( true );

} /* AssetFile_ReadTextureBinary() */


/*******************************************************************
*
*   AssetFile_ReadTextureStorageRequirements()
*
*   DESCRIPTION:
*       Read the buffer size required for the texture under read.
*
*******************************************************************/

bool AssetFile_ReadTextureStorageRequirements( uint32_t *channel_cnt, uint32_t *width, uint32_t *height, uint32_t *byte_count, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_TEXTURE
 || !input->asset_start
 || channel_cnt == NULL
 || width == NULL
 || height == NULL
 || byte_count == NULL )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

TextureHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

*channel_cnt = header.channel_cnt;
*width = header.width;
*height = header.height;
*byte_count = header.byte_size;
return( true );

} /* AssetFile_ReadTextureStorageRequirements() */


/*******************************************************************
*
*   AssetFile_ReadTextureExtents()
*
*   DESCRIPTION:
*       Read the texture extent table array.
*
*******************************************************************/

bool AssetFile_ReadTextureExtents( const uint16_t output_cnt, AssetFileTextureExtent *out_elements, AssetFileReader *input )
{
uint16_t element_cnt;
if( !AssetFile_ReadTextureExtentsStorageRequirements( &element_cnt, input )
 || output_cnt < element_cnt )
    {
    return( false );
    }

for( uint16_t i = 0; i < element_cnt; i++ )
    {
    AssetFileTextureExtent *element = &out_elements[ i ];

    if( fread_s( &element->texture_id, sizeof( element->texture_id ), 1, sizeof( element->texture_id ), input->fhnd ) != sizeof( element->texture_id )
     || fread_s( &element->width,      sizeof( element->width ),      1, sizeof( element->width ),      input->fhnd ) != sizeof( element->width )
     || fread_s( &element->height,     sizeof( element->height ),     1, sizeof( element->height ),     input->fhnd ) != sizeof( element->height ) )
        {
        return( false );
        }
    }

return( true );

} /* AssetFile_ReadTextureExtents() */


/*******************************************************************
*
*   AssetFile_ReadTextureExtentsStorageRequirements()
*
*   DESCRIPTION:
*       Read the array size required for the texture extent table.
*
*******************************************************************/

bool AssetFile_ReadTextureExtentsStorageRequirements( uint16_t *element_cnt, AssetFileReader *input )
{
if( input->kind != ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
 || !input->asset_start
 || element_cnt == NULL )
    {
    return( false );
    }

if( fseek( input->fhnd, input->asset_start, SEEK_SET ) )
    {
    return( false );
    }

TextureExtentHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), input->fhnd ) != sizeof(header) )
    {
    return( false );
    }

*element_cnt = header.texture_cnt;

return( true );

} /* AssetFile_ReadTextureExtentsStorageRequirements() */


/*******************************************************************
*
*   AssetFile_WriteFontGlyph()
*
*   DESCRIPTION:
*       Write a font glyph's character data.
*
*******************************************************************/

bool AssetFile_WriteFontGlyph( const uint8_t glyph, const uint16_t u0, const uint16_t v0, const uint16_t u1, const uint16_t v1, const float pen_dx, const float pen_dy, const float pen_xadvance, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_FONT
 || !output->asset_start )
    {
    return( false );
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

ensure( fwrite( &header, 1, sizeof( header ), output->fhnd ) == sizeof( header ) );
output->caret = (uint32_t)ftell( output->fhnd );

return( true );

}   /* AssetFile_WriteFontGlyph() */


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

bool AssetFile_WriteModelNodeChildElements( const AssetFileModelIndex *element_ids, const uint32_t count, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_MODEL
 || !output->asset_start )
    {
    return( false );
    }

ensure( fwrite( element_ids, sizeof(*element_ids), count, output->fhnd) == count );

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
*   AssetFile_WriteSoundPairs()
*
*   DESCRIPTION:
*       Write the sound asset ID/index pair data to the asset binary.
*       This also ends the asset writing session.
*
*******************************************************************/

bool AssetFile_WriteSoundPairs( const AssetFileSoundPair *sound_pair, const uint16_t num_pairs, AssetFileWriter *output )
{
if( !output->asset_start
 || ( output->kind != ASSET_FILE_ASSET_KIND_SOUND_SAMPLE
   && output->kind != ASSET_FILE_ASSET_KIND_SOUND_MUSIC_CLIP ) )
    {
    return( false );
    }

uint32_t size_write = sizeof( *sound_pair ) * num_pairs;
ensure( fwrite( &num_pairs, 1, sizeof( num_pairs ), output->fhnd) == sizeof( num_pairs ) ); 
ensure( fwrite( sound_pair, 1, size_write, output->fhnd ) == size_write );

output->caret = (uint32_t)ftell( output->fhnd );

output->asset_start = 0;
output->kind = ASSET_FILE_ASSET_KIND_INVALID;

return(true);

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
*   AssetFile_WriteTextureExtent()
*
*   DESCRIPTION:
*       Write the texture data to the asset binary.  This also ends
*       the asset writing session.
*
*******************************************************************/

bool AssetFile_WriteTextureExtent( const AssetFileAssetId id, const uint16_t width, const uint16_t height, AssetFileWriter *output )
{
if( output->kind != ASSET_FILE_ASSET_KIND_TEXTURE_EXTENTS
 || !output->asset_start )
    {
    return( false );
    }

ensure( fwrite( &id, 1, sizeof( id ), output->fhnd ) == sizeof( id ) );
ensure( fwrite( &width, 1, sizeof( width ), output->fhnd ) == sizeof( width ) );
ensure( fwrite( &height, 1, sizeof( height ), output->fhnd ) == sizeof( height ) );

output->caret = (uint32_t)ftell( output->fhnd );

return( true );

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
    
    if( fseek( file, table_start + middle * row_stride, SEEK_SET ) )
        {
        return( false );
        }

    row = {};
    if( fread( &row, 1, sizeof(row), file ) != sizeof(row) )
        {
        return( false );
        }

    if( id == row.id )
        {
        found_it = true;
        if( fseek( file, -row_stride, SEEK_CUR ) )
            {
            return( false );
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

static bool JumpToModelMaterial( const uint32_t asset_start, const uint32_t material_index, FILE *file )
{
if( fseek( file, asset_start, SEEK_SET ) )
    {
    return( false );
    }

ModelHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), file ) != sizeof(header) )
    {
    return( false );
    }

/* Element table order is...  
 a) MATERIALS <-- Look here
 b) MESHES
 c) NODES */

uint32_t element_location = asset_start
                          + (uint32_t)sizeof(ModelHeader)
                          + material_index * (uint32_t)sizeof(ModelTableRow);

if( fseek( file, element_location, SEEK_SET ) )
    {
    return( false );
    }

ModelTableRow element = {};
if( fread_s( &element, sizeof(element), 1, sizeof(element), file ) != sizeof(element)
 || element.kind != ASSET_FILE_MODEL_ELEMENT_KIND_MATERIAL )
    {
    return( false );
    }

if( fseek( file, element.starts_at, SEEK_SET ) )
    {
    return( false );
    }

return( true );

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

static bool JumpToModelMesh( const uint32_t asset_start, const uint32_t mesh_index, FILE *file )
{
if( fseek( file, asset_start, SEEK_SET ) )
    {
    return( false );
    }

ModelHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), file ) != sizeof(header) )
    {
    return( false );
    }

if( mesh_index >= header.mesh_count )
    {
    return( false );
    }

/* Element table order is...  
 a) MATERIALS
 b) MESHES <-- Look here
 c) NODES */
uint32_t element_location = asset_start
                          + (uint32_t)sizeof(ModelHeader)
                          + ( header.material_cnt + mesh_index ) * (uint32_t)sizeof(ModelTableRow);

if( fseek( file, element_location, SEEK_SET ) )
    {
    return( false );
    }

ModelTableRow element = {};
if( fread_s( &element, sizeof(element), 1, sizeof(element), file ) != sizeof(element)
 || element.kind != ASSET_FILE_MODEL_ELEMENT_KIND_MESH )
    {
    return( false );
    }

if( fseek( file, element.starts_at, SEEK_SET ) )
    {
    return( false );
    }

return( true );

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

static bool JumpToModelNode( const uint32_t asset_start, const uint32_t node_index, FILE *file )
{
if( fseek( file, asset_start, SEEK_SET ) )
    {
    return( false );
    }

ModelHeader header = {};
if( fread_s( &header, sizeof(header), 1, sizeof(header), file ) != sizeof(header) )
    {
    return( false );
    }

/* Element table order is...  
 a) MATERIALS
 b) MESHES
 c) NODES  <-- Look here */
uint32_t element_location = asset_start
                          + (uint32_t)sizeof(ModelHeader)
                          + ( header.material_cnt + header.mesh_count + node_index ) * (uint32_t)sizeof(ModelTableRow);

if( fseek( file, element_location, SEEK_SET ) )
    {
    return( false );
    }

ModelTableRow element = {};
if( fread_s( &element, sizeof(element), 1, sizeof(element), file ) != sizeof(element)
 || element.kind != ASSET_FILE_MODEL_ELEMENT_KIND_NODE )
    {
    return( false );
    }

if( fseek( file, element.starts_at, SEEK_SET ) )
    {
    return( false );
    }

return( true );

} /* JumpToModelNode() */

