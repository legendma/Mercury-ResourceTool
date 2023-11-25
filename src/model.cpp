#include <cassert>
#include <assimp/importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "AssetFile.hpp"
#include "Model.hpp"
#include "ResourceUtilities.hpp"

static uint32_t CountNodes( const aiNode *node );
static bool WriteNodes( const aiNode *node, const std::unordered_map<uint32_t, uint32_t> *mesh_index_to_element_index, const uint32_t max_element_count, uint32_t *element_count, uint32_t *new_element, AssetFileWriter *output );


/*******************************************************************
*
*   Model_Load()
*
*   DESCRIPTION:
*       Load the given model by filename.
*
*******************************************************************/

bool Model_Load( const AssetFileAssetId id, const char *filename, const std::unordered_map<std::string, AssetFileAssetId> *texture_map, WriteStats *stats, AssetFileWriter *output )
{
*stats = {};
Assimp::Importer importer;
size_t write_start_size = AssetFile_GetWriteSize( output );

const aiScene *scene = importer.ReadFile( std::string( filename ), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded );
if( !scene )
    {
	print_error( "Model_Load() could not read scene from file (%s).", filename );
    return( false );
    }

if( !AssetFile_BeginWritingAsset( id, ASSET_FILE_ASSET_KIND_MODEL, output ) )
	{
	print_error( "Model_Load() could not begin writing asset.  Reason: Asset was not in file table (%s).", filename );
	return( false );
	}

uint32_t node_count = CountNodes( scene->mRootNode );
if( !AssetFile_DescribeModel( node_count, (uint32_t)scene->mNumMeshes, (uint32_t)scene->mNumMaterials, output ) )
	{
	print_error( "Model_Load() could not write model header (%s).", filename );
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
	AssetFileModelMaterialBits maps = 0;
	AssetFileAssetId diffuse_map_asset_id;
	AssetFileAssetId specular_map_asset_id;

	if( material->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
		{
		aiString diffuse_filename;
		if( material->Get( AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE, 0 ), diffuse_filename ) != aiReturn_SUCCESS )
			{
			print_error( "Model_Load() unable to find diffuse texture (%s).", diffuse_filename );
			return( false );
			}

		std::string stripped = strip_filename( diffuse_filename.C_Str() );
		std::unordered_map<std::string, AssetFileAssetId>::const_iterator it = texture_map->find( stripped );
		if( it == texture_map->end() )
			{
			print_error( "Model_Load() encountered a texture (%s) in model (%s) which was not defined in the definition file.", diffuse_filename.C_Str(), filename );
			return( false );
			}
		
		diffuse_map_asset_id = it->second;
		maps |= ASSET_FILE_MODEL_MATERIAL_BIT_DIFFUSE_MAP;
		}

	if( material->GetTextureCount( aiTextureType_SPECULAR ) > 0 )
		{
		aiString specular_filename;
		if( material->Get( AI_MATKEY_TEXTURE( aiTextureType_SPECULAR, 0 ), specular_filename ) != aiReturn_SUCCESS )
			{
			print_error( "Model_Load() unable to find specular texture (%s).", specular_filename );
			return( false );
			}

		std::string stripped = strip_filename( specular_filename.C_Str() );
		std::unordered_map<std::string, AssetFileAssetId>::const_iterator it = texture_map->find( stripped );
		if( it == texture_map->end() )
			{
			print_error( "Model_Load() encountered a texture (%s) in model (%s) which was not defined in the definition file.", specular_filename.C_Str(), filename );
			return( false );
			}
		
		specular_map_asset_id = it->second;
		maps |= ASSET_FILE_MODEL_MATERIAL_BIT_SPECULAR_MAP;
		}

	/* Write it */
	if( !AssetFile_BeginWritingModelElement( ASSET_FILE_MODEL_ELEMENT_KIND_MATERIAL, element_count, output )
	 || !AssetFile_DescribeModelMaterial( maps, output ) )
		{
		print_error( "Model_Load() could not start writing new model material element (%s).", filename );
		return( false );
		}

	AssetFileAssetId map_array[ ASSET_FILE_MODEL_MATERIAL_BIT_SPECULAR_MAP_COUNT ];
	uint8_t map_array_count = 0;
	if( maps & ASSET_FILE_MODEL_MATERIAL_BIT_DIFFUSE_MAP )
		{
		map_array[ map_array_count++ ] = diffuse_map_asset_id;
		}

	if( maps & ASSET_FILE_MODEL_MATERIAL_BIT_SPECULAR_MAP )
		{
		map_array[ map_array_count++ ] = specular_map_asset_id;
		}

	if( !AssetFile_WriteModelMaterialTextureMaps( map_array, map_array_count, output ) )
		{
		print_error( "Model_Load() failed to write the texture maps for a material, for model (%s).", filename );
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
	if( !AssetFile_BeginWritingModelElement( ASSET_FILE_MODEL_ELEMENT_KIND_MESH, element_count, output )
	 || !AssetFile_DescribeModelMesh( map_material_index_to_element_index[ mesh->mMaterialIndex ], (uint32_t)mesh->mNumVertices, output ) )
		{
		print_error( "Model_Load() could not start writing new model mesh element (%s).", filename );
		return( false );
		}

	/* Vertices */
	for( unsigned int j = 0; j < mesh->mNumVertices; j++ )
		{
		AssetFileVertex v = {};
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

	map_mesh_index_to_element_index[ (uint32_t)i ] = element_count;
	assert( element_count <= max_element_count );
	element_count++;

	stats->meshes_written++;
	}

/* Nodes */
uint32_t root_node_element_index = {};
if( !WriteNodes( scene->mRootNode, &map_mesh_index_to_element_index, max_element_count, &element_count, &root_node_element_index, output )
 || !AssetFile_EndWritingModel( root_node_element_index, output ) )
	{
	print_error( "Model_Load failed to write node tree (%s)", filename );
	return( false );
	}

stats->nodes_written += node_count;
size_t write_total_size = AssetFile_GetWriteSize( output ) - write_start_size;
stats->written_sz += write_total_size;
print_info( "[MODEL]     %s     meshes: %d, materials: %d, nodes: %d, %d bytes.", strip_filename( filename ).c_str(), (int)stats->meshes_written, (int)stats->materials_written, (int)stats->nodes_written, (int)write_total_size );

return( true );

} /* Model_Load() */


/*******************************************************************
*
*   CountNodes()
*
*   DESCRIPTION:
*       Walk the node and count it.
*
*******************************************************************/

static uint32_t CountNodes( const aiNode *node )
{
uint32_t ret = 1;
for( unsigned int i = 0; i < node->mNumChildren; i++ )
	{
	ret += CountNodes( node->mChildren[ i ] );
	}

return( ret );

} /* CountNodes() */


/*******************************************************************
*
*   WriteNodes()
*
*   DESCRIPTION:
*       Walk the node (depth first) and write it.
*       Returns the element index of the given node.
*
*******************************************************************/

static bool WriteNodes( const aiNode *node, const std::unordered_map<uint32_t, uint32_t> *mesh_index_to_element_index, const uint32_t max_element_count, uint32_t *element_count, uint32_t *new_element, AssetFileWriter *output )
{
#define MAX_CHILDREN                ( 100 )

uint32_t child_element_indices[ MAX_CHILDREN ] = {};
uint32_t child_count = 0;

for( unsigned int i = 0; i < node->mNumChildren; i++ )
	{
	uint32_t child_element = {};
	if( !WriteNodes( node->mChildren[ i ], mesh_index_to_element_index, max_element_count, element_count, &child_element, output ) )
		{
		return( false );
		}

	child_element_indices[ child_count++ ] = child_element;
	assert( child_count <= MAX_CHILDREN );
	}

for( unsigned int i = 0; i < (uint32_t)node->mNumMeshes; i++ )
	{
	child_element_indices[ child_count++ ] = mesh_index_to_element_index->at( (uint32_t)node->mMeshes[ i ] );
	assert( child_count <= MAX_CHILDREN );
	}

if( !AssetFile_BeginWritingModelElement( ASSET_FILE_MODEL_ELEMENT_KIND_NODE, *element_count, output )
 || !AssetFile_DescribeModelNode( child_count, (uint32_t)node->mNumMeshes, output ) )
	{
	print_error( "Model_Load() could not start writing new model node element." );
	return( false );
	}

if( !AssetFile_WriteModelNodeChildElements( child_element_indices, child_count, output ) )
	{
	print_error( "Model_Load() could not write node's child element indices." );
	return( false );
	}

*new_element = (*element_count)++;
assert( *element_count <= max_element_count );

return( true );

#undef MAX_CHILDREN
} /* WriteNodes() */
