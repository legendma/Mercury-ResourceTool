#include <cassert>
#include <assimp/importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "AssetFile.hpp"
#include "ExportModel.hpp"
#include "ResourceUtilities.hpp"


typedef struct
	{
	float                  a[ 16 ];
	} LocalMatrix4x4;

static const LocalMatrix4x4 IDENTITY_4x4 = 
	{
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
	};

typedef struct _LocalNode
	{
	const aiNode          *node;
	LocalMatrix4x4         transform;
	std::vector<_LocalNode>
	                       children;
	} LocalNode;


/*******************************************************************
*
*   LoadMatrix()
*
*******************************************************************/

static __inline void LoadMatrix( const aiMatrix4x4 *a, float *b )
{
b[  0 ] = a->a1;
b[  1 ] = a->a2;
b[  2 ] = a->a3;
b[  3 ] = a->a4;
b[  4 ] = a->b1;
b[  5 ] = a->b2;
b[  6 ] = a->b3;
b[  7 ] = a->b4;
b[  8 ] = a->c1;
b[  9 ] = a->c2;
b[ 10 ] = a->c3;
b[ 11 ] = a->c4;
b[ 12 ] = a->d1;
b[ 13 ] = a->d2;
b[ 14 ] = a->d3;
b[ 15 ] = a->d4;

}   /* LoadMatrix() */


/*******************************************************************
*
*   Multiply4x4()
*
*******************************************************************/

static __inline void Multiply4x4( const float *a, const float *b, float *c )
{
c[  0 ] = a[ 0 + 0 * 4 ] * b[ 0 + 0 * 4 ]
        + a[ 1 + 0 * 4 ] * b[ 0 + 1 * 4 ]
        + a[ 2 + 0 * 4 ] * b[ 0 + 2 * 4 ]
        + a[ 3 + 0 * 4 ] * b[ 0 + 3 * 4 ];

c[  1 ] = a[ 0 + 0 * 4 ] * b[ 1 + 0 * 4 ]
        + a[ 1 + 0 * 4 ] * b[ 1 + 1 * 4 ]
        + a[ 2 + 0 * 4 ] * b[ 1 + 2 * 4 ]
        + a[ 3 + 0 * 4 ] * b[ 1 + 3 * 4 ];

c[  2 ] = a[ 0 + 0 * 4 ] * b[ 2 + 0 * 4 ]
        + a[ 1 + 0 * 4 ] * b[ 2 + 1 * 4 ]
        + a[ 2 + 0 * 4 ] * b[ 2 + 2 * 4 ]
        + a[ 3 + 0 * 4 ] * b[ 2 + 3 * 4 ];

c[  3 ] = a[ 0 + 0 * 4 ] * b[ 3 + 0 * 4 ]
        + a[ 1 + 0 * 4 ] * b[ 3 + 1 * 4 ]
        + a[ 2 + 0 * 4 ] * b[ 3 + 2 * 4 ]
        + a[ 3 + 0 * 4 ] * b[ 3 + 3 * 4 ];

c[  4 ] = a[ 0 + 1 * 4 ] * b[ 0 + 0 * 4 ]
        + a[ 1 + 1 * 4 ] * b[ 0 + 1 * 4 ]
        + a[ 2 + 1 * 4 ] * b[ 0 + 2 * 4 ]
        + a[ 3 + 1 * 4 ] * b[ 0 + 3 * 4 ];

c[  5 ] = a[ 0 + 1 * 4 ] * b[ 1 + 0 * 4 ]
        + a[ 1 + 1 * 4 ] * b[ 1 + 1 * 4 ]
        + a[ 2 + 1 * 4 ] * b[ 1 + 2 * 4 ]
        + a[ 3 + 1 * 4 ] * b[ 1 + 3 * 4 ];

c[  6 ] = a[ 0 + 1 * 4 ] * b[ 2 + 0 * 4 ]
        + a[ 1 + 1 * 4 ] * b[ 2 + 1 * 4 ]
        + a[ 2 + 1 * 4 ] * b[ 2 + 2 * 4 ]
        + a[ 3 + 1 * 4 ] * b[ 2 + 3 * 4 ];

c[  7 ] = a[ 0 + 1 * 4 ] * b[ 3 + 0 * 4 ]
        + a[ 1 + 1 * 4 ] * b[ 3 + 1 * 4 ]
        + a[ 2 + 1 * 4 ] * b[ 3 + 2 * 4 ]
        + a[ 3 + 1 * 4 ] * b[ 3 + 3 * 4 ];

c[  8 ] = a[ 0 + 2 * 4 ] * b[ 0 + 0 * 4 ]
        + a[ 1 + 2 * 4 ] * b[ 0 + 1 * 4 ]
        + a[ 2 + 2 * 4 ] * b[ 0 + 2 * 4 ]
        + a[ 3 + 2 * 4 ] * b[ 0 + 3 * 4 ];

c[  9 ] = a[ 0 + 2 * 4 ] * b[ 1 + 0 * 4 ]
        + a[ 1 + 2 * 4 ] * b[ 1 + 1 * 4 ]
        + a[ 2 + 2 * 4 ] * b[ 1 + 2 * 4 ]
        + a[ 3 + 2 * 4 ] * b[ 1 + 3 * 4 ];

c[ 10 ] = a[ 0 + 2 * 4 ] * b[ 2 + 0 * 4 ]
        + a[ 1 + 2 * 4 ] * b[ 2 + 1 * 4 ]
        + a[ 2 + 2 * 4 ] * b[ 2 + 2 * 4 ]
        + a[ 3 + 2 * 4 ] * b[ 2 + 3 * 4 ];

c[ 11 ] = a[ 0 + 2 * 4 ] * b[ 3 + 0 * 4 ]
        + a[ 1 + 2 * 4 ] * b[ 3 + 1 * 4 ]
        + a[ 2 + 2 * 4 ] * b[ 3 + 2 * 4 ]
        + a[ 3 + 2 * 4 ] * b[ 3 + 3 * 4 ];

c[ 12 ] = a[ 0 + 3 * 4 ] * b[ 0 + 0 * 4 ]
        + a[ 1 + 3 * 4 ] * b[ 0 + 1 * 4 ]
        + a[ 2 + 3 * 4 ] * b[ 0 + 2 * 4 ]
        + a[ 3 + 3 * 4 ] * b[ 0 + 3 * 4 ];

c[ 13 ] = a[ 0 + 3 * 4 ] * b[ 1 + 0 * 4 ]
        + a[ 1 + 3 * 4 ] * b[ 1 + 1 * 4 ]
        + a[ 2 + 3 * 4 ] * b[ 1 + 2 * 4 ]
        + a[ 3 + 3 * 4 ] * b[ 1 + 3 * 4 ];

c[ 14 ] = a[ 0 + 3 * 4 ] * b[ 2 + 0 * 4 ]
        + a[ 1 + 3 * 4 ] * b[ 2 + 1 * 4 ]
        + a[ 2 + 3 * 4 ] * b[ 2 + 2 * 4 ]
        + a[ 3 + 3 * 4 ] * b[ 2 + 3 * 4 ];

c[ 15 ] = a[ 0 + 3 * 4 ] * b[ 3 + 0 * 4 ]
        + a[ 1 + 3 * 4 ] * b[ 3 + 1 * 4 ]
        + a[ 2 + 3 * 4 ] * b[ 3 + 2 * 4 ]
        + a[ 3 + 3 * 4 ] * b[ 3 + 3 * 4 ];

}   /* Multiply4x4() */


static uint32_t ParseNode( const aiNode *node, const LocalMatrix4x4 *transform, LocalNode *parent );
static bool     WriteNode( const LocalNode *node, const AssetFileModelIndex element_id, const std::unordered_map<uint32_t, uint32_t> *mesh_index_to_element_index, uint32_t *element_count, AssetFileWriter *output );


/*******************************************************************
*
*   ExportModel_Export()
*
*   DESCRIPTION:
*       Export the given model by filename.
*
*******************************************************************/

bool ExportModel_Export( const AssetFileAssetId id, const char *filename, const std::unordered_map<std::string, AssetFileAssetId> *texture_map, WriteStats *stats, AssetFileWriter *output )
{
*stats = {};
Assimp::Importer importer;
size_t write_start_size = AssetFile_GetWriteSize( output );

const aiScene *scene = importer.ReadFile( std::string( filename ), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded );
if( !scene )
    {
	print_error( "ExportModel_Export() could not read scene from file (%s).", filename );
    return( false );
    }

if( !AssetFile_BeginWritingAsset( id, ASSET_FILE_ASSET_KIND_MODEL, output ) )
	{
	print_error( "ExportModel_Export() could not begin writing asset.  Reason: Asset was not in file table (%s).", filename );
	return( false );
	}

LocalNode root_node = {};
LoadMatrix( &scene->mRootNode->mTransformation, root_node.transform.a );
root_node.node = scene->mRootNode;
uint32_t node_count = 1;

for( unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++ )
	{
	node_count += ParseNode( root_node.node, &root_node.transform, &root_node );
	}

if( !AssetFile_DescribeModel( node_count, (uint32_t)scene->mNumMeshes, (uint32_t)scene->mNumMaterials, output ) )
	{
	print_error( "ExportModel_Export() could not write model header (%s).", filename );
	return( false );
	}

uint32_t max_element_count = node_count + (uint32_t)scene->mNumMeshes + (uint32_t)scene->mNumMaterials;
uint32_t element_count = 0;

/* Materials */
std::unordered_map<uint32_t, uint32_t> map_material_index_to_element_index;
for( unsigned int i = 0; i < scene->mNumMaterials; i++ )
	{
	const aiMaterial *material = scene->mMaterials[ i ];

	/* Texture Maps */
	AssetFileModelMaterialBits mat_props = 0;
	AssetFileAssetId diffuse_map_asset_id;

	if( material->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
		{
		/* albedo */
		aiString diffuse_filename;
		if( material->Get( AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE, 0 ), diffuse_filename ) != aiReturn_SUCCESS )
			{
			print_error( "ExportModel_Export() unable to find diffuse texture (%s).", diffuse_filename );
			return( false );
			}

		std::string stripped = strip_filename( diffuse_filename.C_Str() );
		std::unordered_map<std::string, AssetFileAssetId>::const_iterator it = texture_map->find( stripped );
		if( it == texture_map->end() )
			{
			print_error( "ExportModel_Export() encountered a texture (%s) in model (%s) which was not defined in the definition file.", diffuse_filename.C_Str(), filename );
			return( false );
			}
		
		diffuse_map_asset_id = it->second;
		mat_props |= ASSET_FILE_MODEL_MATERIAL_BIT_ALBEDO_MAP;
		}

	/* Transparency */
	ai_real opacity;
	if( material->Get( AI_MATKEY_OPACITY, opacity )
     && opacity < 1.0f )
		{
		mat_props |= ASSET_FILE_MODEL_MATERIAL_BIT_TRANSPARENCY;
		}

	/* Write it */
	if( !AssetFile_BeginWritingModelElement( ASSET_FILE_MODEL_ELEMENT_KIND_MATERIAL, element_count, output )
	 || !AssetFile_DescribeModelMaterial( mat_props, output ) )
		{
		print_error( "ExportModel_Export() could not start writing new model material element (%s).", filename );
		return( false );
		}

	AssetFileAssetId map_array[ ASSET_FILE_MODEL_TEXTURE_COUNT ];
	uint8_t map_array_count = 0;
	if( mat_props & ASSET_FILE_MODEL_MATERIAL_BIT_ALBEDO_MAP )
		{
		map_array[ map_array_count++ ] = diffuse_map_asset_id;
		}

	if( !AssetFile_WriteModelMaterialTextureMaps( map_array, map_array_count, output ) )
		{
		print_error( "ExportModel_Export() failed to write the texture maps for a material, for model (%s).", filename );
		}

	map_material_index_to_element_index[ (uint32_t)i ] = element_count;
	element_count++;
	assert( element_count <= max_element_count );

	stats->materials_written++;
	}

/* Meshes */
std::unordered_map<uint32_t, uint32_t> map_mesh_index_to_element_index;
for( unsigned int i = 0; i < scene->mNumMeshes; i++ )
	{
	aiMesh *mesh = scene->mMeshes[ i ];

	uint32_t index_count = 0;
	for( unsigned int j = 0; j < mesh->mNumFaces; j++ )
		{
		index_count += (uint32_t)mesh->mFaces[ j ].mNumIndices;
		}

	if( !AssetFile_BeginWritingModelElement( ASSET_FILE_MODEL_ELEMENT_KIND_MESH, element_count, output )
	 || !AssetFile_DescribeModelMesh( map_material_index_to_element_index[ mesh->mMaterialIndex ], (uint32_t)mesh->mNumVertices, index_count, output ) )
		{
		print_error( "ExportModel_Export() could not start writing new model mesh element (%s).", filename );
		return( false );
		}

	/* Vertices */
	for( unsigned int j = 0; j < mesh->mNumVertices; j++ )
		{
		AssetFileModelVertex v = {};
		v.x = mesh->mVertices[ j ].x;
		v.y = mesh->mVertices[ j ].y;
		v.z = mesh->mVertices[ j ].z;

		if( mesh->mTextureCoords[ 0 ] )
			{
			v.u0 = mesh->mTextureCoords[ 0 ][ j ].x;
			v.v0 = mesh->mTextureCoords[ 0 ][ j ].y;
			}

		AssetFile_WriteModelMeshVertex( &v, output );
		}

	/* Indices */
	for( unsigned int j = 0; j < mesh->mNumFaces; j++ )
		{
		aiFace *face = &mesh->mFaces[ j ];
		for( unsigned int k = 0; k < face->mNumIndices; k++ )
			{
			AssetFile_WriteModelMeshIndex( (AssetFileModelIndex)face->mIndices[ k ], output );
			}
		}

	map_mesh_index_to_element_index[ (uint32_t)i ] = element_count;
	assert( element_count <= max_element_count );
	element_count++;

	stats->meshes_written++;
	}

/* Nodes */
uint32_t root_node_element_index = {};
if( !WriteNode( &root_node, element_count++, &map_mesh_index_to_element_index, &element_count, output )
 || !AssetFile_EndWritingModel( root_node_element_index, output ) )
	{
	print_error( "ExportModel_Export failed to write node tree (%s)", filename );
	return( false );
	}

stats->nodes_written += node_count;
size_t write_total_size = AssetFile_GetWriteSize( output ) - write_start_size;
stats->written_sz += write_total_size;
print_info( "[MODEL]     %s     meshes: %d, materials: %d, nodes: %d, %d bytes.", strip_filename( filename ).c_str(), (int)stats->meshes_written, (int)stats->materials_written, (int)stats->nodes_written, (int)write_total_size );

return( true );

} /* ExportModel_Export() */


/*******************************************************************
*
*   ParseNode()
*
*   DESCRIPTION:
*       Parse the node and its children into local form.
*
*******************************************************************/

static uint32_t ParseNode( const aiNode *node, const LocalMatrix4x4 *transform, LocalNode *parent )
{
uint32_t ret = 0;
LocalNode *use_parent = parent;
LocalMatrix4x4 use_transform;
LocalMatrix4x4 local_matrix;
LoadMatrix( &node->mTransformation, local_matrix.a );
Multiply4x4( local_matrix.a, transform->a, use_transform.a );

if( node->mNumMeshes > 0 )
	{
	parent->children.emplace_back();
	ret += 1;

	use_parent = &parent->children.back();
	use_parent->node = node;
	use_parent->transform = local_matrix;

	use_transform = IDENTITY_4x4;
	}

for( unsigned int i = 0; i < node->mNumChildren; i++ )
	{
	ret += ParseNode( node->mChildren[ i ], &use_transform, use_parent );
	}

return( ret );

}   /* ParseNode() */


/*******************************************************************
*
*   WriteNode()
*
*   DESCRIPTION:
*       Write the given node.
*
*******************************************************************/

static bool WriteNode( const LocalNode *node, const AssetFileModelIndex element_id, const std::unordered_map<uint32_t, uint32_t> *mesh_index_to_element_index, uint32_t *element_count, AssetFileWriter *output )
{
/* establish the child node element indices */
std::vector<AssetFileModelIndex> child_element_indices;
for( auto i = 0; i < node->children.size(); i++ )
	{
	child_element_indices.push_back( (*element_count)++ );
	}

/* add the child indices */
for( unsigned int i = 0; i < node->node->mNumMeshes; i++ )
	{
	child_element_indices.push_back( mesh_index_to_element_index->at( (uint32_t)node->node->mMeshes[ i ] ) );
	}
	
/* write this node */
if( !AssetFile_BeginWritingModelElement( ASSET_FILE_MODEL_ELEMENT_KIND_NODE, element_id, output )
 || !AssetFile_DescribeModelNode( (uint32_t)node->children.size(), node->transform.a, (uint32_t)node->node->mNumMeshes, output ) )
	{
	print_error( "ExportModel_Export() could not start writing new model node element." );
	return( false );
	}

if( !AssetFile_WriteModelNodeChildElements( child_element_indices.data(), (uint32_t)child_element_indices.size(), output ) )
	{
	print_error( "ExportModel_Export() could not write node's child element indices." );
	return( false );
	}

/* write each child node */
for( auto i = 0; i < node->children.size(); i++ )
	{
	if( !WriteNode( &node->children[ i ], child_element_indices[ i ], mesh_index_to_element_index, element_count, output ) )
		{
		print_error( "ExportModel_Export() could not write node's child node." );
		return( false );
		}
	}

return( true );

} /* WriteNode() */

