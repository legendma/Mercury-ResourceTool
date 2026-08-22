#include <cassert>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "AssetFile.hpp"
#include "ExportModel.hpp"
#include "ResourceUtilities.hpp"

#define SCALE_COMPONENT_FLOATS      ( 3 )
#define ROTATION_COMPONENT_FLOATS   ( 4 )
#define TRANSLATION_COMPONENT_FLOATS \
                                    ( 3 )

typedef struct
	{
	float                  a[ SCALE_COMPONENT_FLOATS
	                        + ROTATION_COMPONENT_FLOATS
							+ TRANSLATION_COMPONENT_FLOATS ]; /* stored [ s.x, s.y, s.z,
							                                              r.x, r.y, r.z, r.w,
																		  t.x, t.y, t.z ] */
	} LocalTransform;

typedef struct _LocalNode
	{
	const aiNode          *node;
	LocalTransform         transform;
	std::vector<_LocalNode>
	                       children;
	} LocalNode;


/*******************************************************************
*
*   LoadTransform()
*
*******************************************************************/

static inline void LoadTransform( LocalNode *node )
{
aiVector3D scale;
aiQuaternion rotation;
aiVector3D translation;
node->node->mTransformation.Decompose( scale, rotation, translation );

node->transform.a[ 0 ] = scale.x;
node->transform.a[ 1 ] = scale.y;
node->transform.a[ 2 ] = scale.z;
node->transform.a[ 3 ] = rotation.x;
node->transform.a[ 4 ] = rotation.y;
node->transform.a[ 5 ] = rotation.z;
node->transform.a[ 6 ] = rotation.w;
node->transform.a[ 7 ] = translation.x;
node->transform.a[ 8 ] = translation.y;
node->transform.a[ 9 ] = translation.z;

}   /* LoadTransform() */


/*******************************************************************
*
*   Multiply4x4()
*
*******************************************************************/

static inline void Multiply4x4( const float *a, const float *b, float *c )
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

static const unsigned int IMPORT_FLAGS = aiProcess_Triangulate
                                       | aiProcess_RemoveRedundantMaterials
									   | aiProcess_FlipUVs
									   | aiProcess_GlobalScale;


static uint32_t ParseNode( const aiNode *node, LocalNode *parent );
static bool     WriteNode( const LocalNode *node, const AssetFileModelIndex element_id, const std::unordered_map<uint32_t, uint32_t> *mesh_index_to_element_index, uint32_t *element_count, AssetFileWriter *output );


/*******************************************************************
*
*   ExportModel_Export()
*
*   DESCRIPTION:
*       Export the given model by filename.
*
*******************************************************************/

bool ExportModel_Export( const AssetFileAssetId id, const char *filename, const float global_scale, const std::unordered_map<std::string, AssetFileAssetId> *texture_map, WriteStats *stats, std::vector<std::string> &out_strs, AssetFileWriter *output )
{
*stats = {};
Assimp::Importer importer;
size_t write_start_size = AssetFile_GetWriteSize( output );

importer.SetPropertyFloat( AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, global_scale );
const aiScene *scene = importer.ReadFile( std::string( filename ), IMPORT_FLAGS );
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
root_node.node = scene->mRootNode;
LoadTransform( &root_node );
uint32_t node_count = 1;

for( unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++ )
	{
	node_count += ParseNode( root_node.node, &root_node );
	}

//for( unsigned int i = 0; i < scene->mNumMaterials; i++ )
//	{
//    aiString name;
//    scene->mMaterials[i]->Get(AI_MATKEY_NAME, name);
//    printf( "Material %d: '%s'\n", i, name.C_Str() );
//    
//    // Also print how many meshes reference this material
//	}

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
	aiString mat_name;
	material->Get( AI_MATKEY_NAME, mat_name );
	std::string material_and_special = std::string( mat_name.C_Str() );
	uint32_t colon = (uint32_t)material_and_special.find( ':' );
	if( colon == material_and_special.npos )
		{
		print_error( "ExportModel_Export() found malformed material name (%s).", material_and_special.c_str() );
		return( false );
		}

	std::string material_name = material_and_special.substr( 0, colon );
	std::string special_name = material_and_special.substr( colon + 1 );

	/* Texture Maps */
	AssetFileModelMaterialBits mat_props = 0;
	AssetFileAssetId diffuse_map_asset_id;
	std::vector<std::string> texture_str_names;

	if( material->GetTextureCount( aiTextureType_BASE_COLOR ) > 0 )
		{
		/* albedo */
		aiString diffuse_filename;
		if( material->GetTexture( aiTextureType_BASE_COLOR, 0, &diffuse_filename ) != aiReturn_SUCCESS )
			{
			print_error( "ExportModel_Export() unable to find diffuse texture (%s).", diffuse_filename.C_Str() );
			return( false );
			}

		std::string stripped = strip_filename( diffuse_filename.C_Str() );
		std::unordered_map<std::string, AssetFileAssetId>::const_iterator it = texture_map->find( stripped );
		if( it == texture_map->end() )
			{
			print_error( "ExportModel_Export() encountered a texture (%s) in model (%s) which was not added to the texture table.", diffuse_filename.C_Str(), filename );
			return( false );
			}
		
		texture_str_names.push_back( std::string( diffuse_filename.C_Str() ) );
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
	 || !AssetFile_DescribeModelMaterial( mat_props, material_name.c_str(), special_name.c_str(), output ) )
		{
		print_error( "ExportModel_Export() could not start writing new model material element (%s).", filename );
		return( false );
		}

	AssetFileAssetId map_array[ ASSET_FILE_MODEL_TEXTURE_COUNT ];
	const char *name_array[ ASSET_FILE_MODEL_TEXTURE_COUNT ];
	uint8_t map_array_count = 0;
	if( mat_props & ASSET_FILE_MODEL_MATERIAL_BIT_ALBEDO_MAP )
		{
		std::string strip_file_extension = texture_str_names[ map_array_count ];
		strip_file_extension = strip_file_extension.substr( 0, strip_file_extension.find_first_of( '.' ) );
		texture_str_names[ map_array_count ] = strip_file_extension;

		name_array[ map_array_count ] = texture_str_names[ map_array_count ].c_str();
		map_array[ map_array_count++ ] = diffuse_map_asset_id;
		}

	if( !AssetFile_WriteModelMaterialTextureMaps( map_array, name_array, map_array_count, output ) )
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
	std::string mesh_name = strip_filename( filename );
	mesh_name = mesh_name.substr( 0, mesh_name.find_first_of( '.' ) ) + std::string( "/" ) + std::string( mesh->mName.C_Str() );

	std::string mesh_hash_name = std::string( "msh/" ) + mesh_name;
	AssetFileAssetId mesh_id = AssetFile_MakeAssetIdFromName2( mesh_hash_name.c_str() );

	uint32_t index_count = 0;
	for( unsigned int j = 0; j < mesh->mNumFaces; j++ )
		{
		index_count += (uint32_t)mesh->mFaces[ j ].mNumIndices;
		}

	aiAABB abb = mesh->mAABB;
	aiVector3D half_extent = 0.5f * ( abb.mMax - abb.mMin );
	aiVector3D center = abb.mMin + half_extent;

	f32 abb_center[ 3 ];
	abb_center[ 0 ] = center.x;
	abb_center[ 1 ] = center.y;
	abb_center[ 2 ] = center.z;

	f32 abb_half_extents[ 3 ];
	abb_half_extents[ 0 ] = half_extent.x;
	abb_half_extents[ 1 ] = half_extent.y;
	abb_half_extents[ 2 ] = half_extent.z;

	if( !AssetFile_BeginWritingModelElement( ASSET_FILE_MODEL_ELEMENT_KIND_MESH, element_count, output )
	 || !AssetFile_DescribeModelMesh( map_material_index_to_element_index[ mesh->mMaterialIndex ], (uint32_t)mesh->mNumVertices, index_count, abb_center, abb_half_extents, mesh_id, mesh_name.c_str(), output ) )
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
		v.z = -mesh->mVertices[ j ].z;

		if( mesh->mTextureCoords[ 0 ] )
			{
			v.u0 = mesh->mTextureCoords[ 0 ][ j ].x;
			v.v0 = mesh->mTextureCoords[ 0 ][ j ].y;
			}

		if( mesh->mNormals )
			{
			v.nx = mesh->mNormals[ j ].x;
			v.ny = mesh->mNormals[ j ].y;
			v.nz = -mesh->mNormals[ j ].z;
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

std::ostringstream os;
os << "meshes: " << (int)stats->meshes_written
   << ", materials: " << (int)stats->materials_written
   << ", nodes: " << (int)stats->nodes_written
   << ", " << (int)write_total_size << " bytes";
out_strs.push_back( sprint_info( ASSET_STR_FORMAT_STRING, "[MODEL]", strip_filename( filename ).c_str(), os.str().c_str() ) );

return( true );

} /* ExportModel_Export() */


/*******************************************************************
*
*   ExportModel_GetTextures()
*
*   DESCRIPTION:
*       Get all the texture filenames in the given model.
*
*******************************************************************/

bool ExportModel_GetTextures( const char *filename, std::vector<std::string> &texture_names )
{
Assimp::Importer importer;
const aiScene *scene = importer.ReadFile( std::string( filename ), IMPORT_FLAGS );
if( !scene )
    {
	print_error( "ExportModel_GetTextures() unable to read textures (%s).", filename );
    return( false );
    }

for( unsigned int i = 0; i < scene->mNumMaterials; i++ )
	{
	const aiMaterial *material = scene->mMaterials[ i ];
	if( material->GetTextureCount( aiTextureType_BASE_COLOR ) > 0 )
		{
		/* albedo */
		aiString diffuse_filename;
		if( material->GetTexture( aiTextureType_BASE_COLOR, 0, &diffuse_filename ) != aiReturn_SUCCESS )
			{
			print_error( "ExportModel_Export() unable to find diffuse texture (%s).", diffuse_filename.C_Str() );
			return( false );
			}

	    texture_names.push_back( std::string( diffuse_filename.C_Str() ) );
        }
    }

return( true );

} /* ExportModel_GetTextures() */


/*******************************************************************
*
*   ParseNode()
*
*   DESCRIPTION:
*       Parse the node and its children into local form.
*
*******************************************************************/

static uint32_t ParseNode( const aiNode *node, LocalNode *parent )
{
aiVector3D scale;
aiQuaternion rotation;
aiVector3D translation;
node->mTransformation.Decompose( scale, rotation, translation );

LocalNode &local = parent->children.emplace_back();
local.node = node;
LoadTransform( &local );

uint32_t ret = 1;
for( unsigned int i = 0; i < node->mNumChildren; i++ )
	{
	ret += ParseNode( node->mChildren[ i ], &local);
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

