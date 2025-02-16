/*******************************************************************
*  ResourcePackager.exe -d [definition filename and path] -o [output binary filename and path] -r [root to input assets]
*******************************************************************/
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <windows.h>
#include <io.h>

#include "cjson.h"
#include "AssetFile.hpp"
#include "ExportFont.hpp"
#include "ExportModel.hpp"
#include "ExportSounds.hpp"
//#include "ExportShader.hpp"
#include "ExportTexture.hpp"
#include "ResourceUtilities.hpp"

#define MAX_ARGUMENT_LENGTH         ( 500 )
#define ARGUMENT_INPUT_DEFINITION   "-d"
#define ARGUMENT_OUTPUT_BINARY      "-o"
#define ARGUMENT_ASSET_ROOT         "-r"
#define ARGUMENT_SOUND_BANK_FOLDER  "-sb"
#define ARGUMENT_INPUT_FONTS_FOLDER "-f"

typedef struct
    {
    bool                is_valid;
    char                str[ MAX_ARGUMENT_LENGTH ];
    } GenericArgumentString;

using ProgramArgumentsDefinition      = GenericArgumentString;
using ProgramArgumentsOutput          = GenericArgumentString;
using ProgramArgumentsOutputFolder    = GenericArgumentString;
using ProgramArgumentsAssetsRoot      = GenericArgumentString;
using ProgramArgumentsSoundBankFolder = GenericArgumentString;
using ProgramArgumentsFontsFolder     = GenericArgumentString;

typedef struct _ProgramArguments
    {
    ProgramArgumentsDefinition
                        definition;
    ProgramArgumentsOutput
                        output_binary;
    ProgramArgumentsOutputFolder
                        output_binary_folder;
    ProgramArgumentsAssetsRoot
                        assets_folder;
    ProgramArgumentsSoundBankFolder
                        output_soundbank_folder;
    ProgramArgumentsFontsFolder
                        input_fonts_folder;
    } ProgramArguments;

typedef struct _ParseDefinitionState
    {
    char               *json_string;
    } ParseDefinitionState;

struct _DefinitionVisitor;

static bool check_file_exists( const char *filename );
static size_t get_file_char_size( const char *filename );
static void parse_args( int argc, char **argv, ProgramArguments *arguments );
static bool process_args( const ProgramArguments *arguments );
static bool read_json_as_string( const char *filename, const size_t sz, char *out );
static bool visit_all_definition_assets( const cJSON *assets, const char *asset_folder, const char *input_font_folder, _DefinitionVisitor *visitor );

typedef struct _DefinitionVisitor
    {
    typedef struct _AssetDescriptor
        {
        AssetFileAssetKind
                        kind = ASSET_FILE_ASSET_KIND_INVALID;
        std::string     filename;
        std::string     stripped_filename;
        std::string     asset_id_str;
        std::string     font_glyphs;
        int             font_point_sz;
        //std::string     shader_entry_point;

        } AssetDescriptor;


    /***************************************************************
    *
    *   VisitFont()
    *
    *   DESCRIPTION:
    *       Tabulate the font asset in the descriptor JSON.
    *
    ***************************************************************/

    virtual void VisitFont( const char *asset_id, const char *filename, const int point_size, const char *glyphs )
    {
    std::string stripped = strip_filename( filename );
    std::stringstream ss;
    ss << point_size
       << "_"
       << stripped;

    std::string point_size_filename = ss.str();
    if( std::find( seen_filenames.begin(), seen_filenames.end(), point_size_filename ) != seen_filenames.end() )
        {
        print_warning( "Found duplicate filename (%s).  This time as FONT.  Ignoring (%s)...", stripped, point_size_filename );
        return;
        }

    seen_filenames.push_back( point_size_filename );

    AssetFileAssetId id = AssetFile_MakeAssetIdFromName( asset_id, (uint32_t)strlen( asset_id ) );
    if( asset_map.find( id ) != asset_map.end() )
        {
        print_warning( "Found duplicate asset name (%s).  This time as FONT.  Overwriting with (%s)...", asset_id, point_size_filename );
        }

    AssetDescriptor descriptor = {};
    descriptor.kind              = ASSET_FILE_ASSET_KIND_FONT;
    descriptor.filename          = std::string( filename );
    descriptor.stripped_filename = stripped;
    descriptor.asset_id_str      = std::string( asset_id );
    descriptor.font_point_sz     = point_size;
    descriptor.font_glyphs       = std::string( glyphs );
    
    asset_map[ id ] = descriptor;

    }   /* VisitFont() */


    /***************************************************************
    *
    *   VisitModel()
    *
    *   DESCRIPTION:
    *       Tabulate the model asset in the descriptor JSON.
    *
    ***************************************************************/

    virtual void VisitModel( const char *asset_id, const char *filename )
    {
    std::string stripped = strip_filename( filename );
    if( std::find( seen_filenames.begin(), seen_filenames.end(), stripped ) != seen_filenames.end() )
        {
        print_warning( "Found duplicate filename (%s).  This time as MODEL.  Ignoring (%s)...", stripped, filename );
        return;
        }

    seen_filenames.push_back( stripped );

    AssetFileAssetId id = AssetFile_MakeAssetIdFromName( asset_id, (uint32_t)strlen( asset_id ) );
    if( asset_map.find( id ) != asset_map.end() )
        {
        print_warning( "Found duplicate asset name (%s).  This time as MODEL.  Overwriting with (%s)...", asset_id, filename );
        }

    AssetDescriptor descriptor = {};
    descriptor.kind              = ASSET_FILE_ASSET_KIND_MODEL;
    descriptor.filename          = std::string( filename );
    descriptor.stripped_filename = stripped;
    descriptor.asset_id_str      = std::string( asset_id );
    
    asset_map[ id ] = descriptor;

    }   /* VisitModel() */


    /***************************************************************
    *
    *   VisitMusicClip()
    *
    *   DESCRIPTION:
    *       Tabulate the music clip asset in the descriptor JSON.
    *
    ***************************************************************/

    virtual void VisitMusicClip( const char *asset_id, const char *filename )
    {
    std::string stripped = strip_filename( filename );
    if( std::find( seen_filenames.begin(), seen_filenames.end(), stripped ) != seen_filenames.end() )
        {
        print_warning( "Found duplicate filename (%s).  This time as MUSIC CLIP.  Ignoring (%s)...", stripped, filename );
        return;
        }

    seen_filenames.push_back( stripped );

    AssetFileAssetId id = AssetFile_MakeAssetIdFromName( asset_id, (uint32_t)strlen( asset_id ) );
    if( asset_map.find( id ) != asset_map.end() )
        {
        print_warning( "Found duplicate asset name (%s).  This time as MUSIC CLIP.  Overwriting with (%s)...", asset_id, filename );
        }

    AssetDescriptor descriptor = {};
    descriptor.kind              = ASSET_FILE_ASSET_KIND_SOUND_MUSIC_CLIP;
    descriptor.filename          = std::string( filename );
    descriptor.stripped_filename = stripped;
    descriptor.asset_id_str       = std::string( asset_id );

    asset_map[ id ] = descriptor;

    }   /* VisitMusicClip() */


    ///***************************************************************
    //*
    //*   VisitShader()
    //*
    //*   DESCRIPTION:
    //*       Tabulate the shader asset in the descriptor JSON.
    //*
    //***************************************************************/

    //virtual void VisitShader( const char *asset_id, const char *filename, const char *target, const char *entry_point )
    //{
    //std::string stripped = strip_filename( filename );
    //if( std::find( seen_filenames.begin(), seen_filenames.end(), stripped ) != seen_filenames.end() )
    //    {
    //    print_warning( "Found duplicate filename (%s).  This time as SHADER.  Ignoring (%s)...", stripped, filename );
    //    return;
    //    }

    //seen_filenames.push_back( stripped );

    //AssetFileAssetId id = AssetFile_MakeAssetIdFromName( asset_id, (uint32_t)strlen( asset_id ) );
    //if( asset_map.find( id ) != asset_map.end() )
    //    {
    //    print_warning( "Found duplicate asset name (%s).  This time as SHADER.  Overwriting with (%s)...", asset_id, filename );
    //    }

    //AssetDescriptor descriptor = {};
    //descriptor.kind               = ASSET_FILE_ASSET_KIND_SHADER;
    //descriptor.filename           = std::string( filename );
    //descriptor.stripped_filename  = stripped;
    //descriptor.shader_target      = std::string( target );
    //descriptor.shader_entry_point = std::string( entry_point );

    //asset_map[ id ] = descriptor;

    //}   /* VisitShader() */


    /***************************************************************
    *
    *   VisitSoundSample()
    *
    *   DESCRIPTION:
    *       Tabulate the sound sample asset in the descriptor JSON.
    *
    ***************************************************************/

    virtual void VisitSoundSample( const char *asset_id, const char *filename )
    {
    std::string stripped = strip_filename( filename );
    if( std::find( seen_filenames.begin(), seen_filenames.end(), stripped ) != seen_filenames.end() )
        {
        print_warning( "Found duplicate filename (%s).  This time as SOUND SAMPLE.  Ignoring (%s)...", stripped, filename );
        return;
        }

    seen_filenames.push_back( stripped );

    AssetFileAssetId id = AssetFile_MakeAssetIdFromName( asset_id, (uint32_t)strlen( asset_id ) );
    if( asset_map.find( id ) != asset_map.end() )
        {
        print_warning( "Found duplicate asset name (%s).  This time as SOUND SAMPLE.  Overwriting with (%s)...", asset_id, filename );
        }

    AssetDescriptor descriptor = {};
    descriptor.kind              = ASSET_FILE_ASSET_KIND_SOUND_SAMPLE;
    descriptor.filename          = std::string( filename );
    descriptor.stripped_filename = stripped;
    descriptor.asset_id_str       = std::string( asset_id );

    asset_map[ id ] = descriptor;

    }   /* VisitSoundSample() */


    /***************************************************************
    *
    *   VisitTexture()
    *
    *   DESCRIPTION:
    *       Tabulate the texture asset in the descriptor JSON.
    *
    ***************************************************************/

    virtual void VisitTexture( const char *asset_id, const char *filename )
    {
    std::string stripped = strip_filename( filename );
    if( std::find( seen_filenames.begin(), seen_filenames.end(), stripped ) != seen_filenames.end() )
        {
        print_warning( "Found duplicate filename (%s).  This time as TEXTURE.  Ignoring (%s)...", stripped, filename );
        return;
        }

    seen_filenames.push_back( stripped );

    AssetFileAssetId id = AssetFile_MakeAssetIdFromName( asset_id, (uint32_t)strlen( asset_id ) );
    if( asset_map.find( id ) != asset_map.end() )
        {
        print_warning( "Found duplicate asset name (%s).  This time as TEXTURE.  Overwriting with (%s)...", asset_id, filename );
        }

    AssetDescriptor descriptor = {};
    descriptor.kind              = ASSET_FILE_ASSET_KIND_TEXTURE;
    descriptor.filename          = std::string( filename );
    descriptor.stripped_filename = stripped;

    asset_map[ id ] = descriptor;

    }   /* VisitTexture() */


    /***************************************************************
    *
    *   ExtractTextureMap()
    *
    *   DESCRIPTION:
    *       Get the texture mapping from filename to asset ID.
    *
    ***************************************************************/

    void ExtractTextureMap( std::unordered_map<std::string, AssetFileAssetId> *out )
    {
    out->clear();
    for( auto &entry : asset_map )
        {
        if( entry.second.kind == ASSET_FILE_ASSET_KIND_TEXTURE )
            {
            (*out)[ entry.second.stripped_filename ] = entry.first;
            }
        }

    } /* ExtractTextureMap() */
    
    std::unordered_map<AssetFileAssetId, AssetDescriptor>
                        asset_map;
    std::vector<std::string>
                        seen_filenames;

    } DefinitionVisitor;


/*******************************************************************
*
*   main()
*
*   DESCRIPTION:
*       Program entry point
*
*******************************************************************/

int main( int argc, char **argv )
{
print_info( "Starting...\n" );

try
    {
    clock_t start_time = clock();

    static ProgramArguments arguments;
    if( argc <= 1 )
        {
        printf( "Usage: ResourcePackager.exe [-d FILENAME]... [-o FILENAME]... [-r PATH]... \n" );
        printf( "\tParse and package assets to binary file.\n\n" );
        printf( "Options:\n" );
        printf( "\t-d FILENAME   Filename with path to input .json file which lists the assets to be packaged.\n" );
        printf( "\t-o PATH       Folder to write binary output file.\n" );
        printf( "\t-r PATH       Folder which is the root of assets defined in definition file.\n" );
        printf( "\t-sb PATH      Folder which to output the sound bank files.\n" );

        return( 0 );
        }

    parse_args( argc, argv, &arguments );

    bool has_error = false;
    if( !arguments.definition.is_valid )
        {
        has_error = true;
        print_error( "No input definition file provided. (%s)", ARGUMENT_INPUT_DEFINITION );
        }

    if( !arguments.output_binary.is_valid )
        {
        has_error = true;
        print_error( "No output filename provided. (%s)", ARGUMENT_OUTPUT_BINARY );
        }

    if( !arguments.assets_folder.is_valid )
        {
        has_error = true;
        print_error( "No asset root folder name. (%s)", ARGUMENT_ASSET_ROOT );
        }

    if( !arguments.output_soundbank_folder.is_valid )
        {
        has_error = true;
        print_error( "No soundbank folder name. (%s)", ARGUMENT_SOUND_BANK_FOLDER );
        }

    if( !arguments.input_fonts_folder.is_valid )
        {
        has_error = true;
        print_error( "No input font folder name. (%s)", ARGUMENT_INPUT_FONTS_FOLDER );
        }

    if( !has_error )
        {
        has_error = !process_args( &arguments );
        }

    if( has_error )
        {
        return( -1 );
        }

    clock_t end_time = clock();
    float final_elapsed_time = ( (float)end_time - (float)start_time ) / (float)CLOCKS_PER_SEC;
    print_info( "Finished!  Elapsed time: %.2f seconds.", final_elapsed_time );
    }
catch( ... )
    {
    print_error( "An unknown exception occurred!!!  Try putting Sleep( 20000 ) at the top of main() and attaching a debugger to the running process." );
    }

return( 0 );

} /* main() */


/*******************************************************************
*
*   check_file_exists()
*
*   DESCRIPTION:
*       Does the given file exist?
*
*******************************************************************/

static bool check_file_exists( const char *filename )
{
#define ACCESS_CHECK_EXISTANCE      ( 0x00 )
int result = _access( filename, ACCESS_CHECK_EXISTANCE );

return( result != -1 );

#undef ACCESS_CHECK_EXISTANCE
} /* check_file_exists() */


/*******************************************************************
*
*   get_file_char_size()
*
*   DESCRIPTION:
*       How large is the given file (in characters)?
*
*******************************************************************/

static size_t get_file_char_size( const char *filename )
{
FILE *fhnd = NULL;
errno_t err = fopen_s( &fhnd, filename, "r" );
if( err )
    {
    return( 0 );
    }

if( fseek( fhnd, 0, SEEK_END ) != 0 )
    {
    fclose( fhnd );
    return( 0 );
    }

size_t ret = (size_t)ftell( fhnd );
fclose( fhnd );

return( ret );

} /* get_file_char_size() */


/*******************************************************************
*
*   parse_args()
*
*   DESCRIPTION:
*       Parse the program arguments.
*
*******************************************************************/

static void parse_args( int argc, char **argv, ProgramArguments *arguments )
{
typedef struct
    {
    bool                is_setting_definition;
    bool                is_setting_asset_root;
    bool                is_setting_input_fonts_folder;
    bool                is_setting_output_binary;
    bool                is_setting_output_bank_folder;
    } ArgumentExpectations;


static char temp_argument[ MAX_ARGUMENT_LENGTH ];

memset( arguments, 0, sizeof( *arguments ) );
ArgumentExpectations expectation = {};

for( int i = 0; i < argc; i++ )
    {
    strcpy_s( temp_argument, sizeof( temp_argument ), argv[ i ] );

    if( strcmp( temp_argument, ARGUMENT_INPUT_DEFINITION ) == 0 )
        {
        expectation = {};
        expectation.is_setting_definition = true;
        }
    else if( strcmp( temp_argument, ARGUMENT_OUTPUT_BINARY ) == 0 )
        {
        expectation = {};
        expectation.is_setting_output_binary = true;
        }
    else if( strcmp( temp_argument, ARGUMENT_ASSET_ROOT ) == 0 )
        {
        expectation = {};
        expectation.is_setting_asset_root = true;
        }
    else if( strcmp( temp_argument, ARGUMENT_SOUND_BANK_FOLDER ) == 0 )
        {
        expectation = {};
        expectation.is_setting_output_bank_folder = true;
        }
    else if( strcmp( temp_argument, ARGUMENT_INPUT_FONTS_FOLDER ) == 0 )
        {
        expectation = {};
        expectation.is_setting_input_fonts_folder = true;
        }
    else
        {
        if( expectation.is_setting_asset_root )
            {
            strcpy_s( arguments->assets_folder.str, sizeof( arguments->assets_folder.str ), temp_argument );
            arguments->assets_folder.is_valid = ( strlen( temp_argument ) > 0 );
            }
        else if( expectation.is_setting_definition )
            {
            strcpy_s( arguments->definition.str, sizeof( arguments->definition.str ), temp_argument );
            arguments->definition.is_valid = ( strlen( temp_argument ) > 0 );
            }
        else if( expectation.is_setting_output_binary )
            {
            sprintf_s( arguments->output_binary.str, sizeof( arguments->output_binary.str ), "%s\\%s", temp_argument, ASSET_FILE_BINARY_FILENAME );
            strcpy_s( arguments->output_binary_folder.str, sizeof( arguments->output_binary_folder.str ), temp_argument );
            arguments->output_binary.is_valid = ( strlen( temp_argument ) > 0 );
            }
        else if( expectation.is_setting_output_bank_folder )
            {
            strcpy_s( arguments->output_soundbank_folder.str, sizeof( arguments->output_soundbank_folder.str ), temp_argument );
            arguments->output_soundbank_folder.is_valid = ( strlen( temp_argument ) > 0 );
            }
        else if( expectation.is_setting_input_fonts_folder )
            {
            strcpy_s( arguments->input_fonts_folder.str, sizeof( arguments->input_fonts_folder.str ), temp_argument );
            arguments->input_fonts_folder.is_valid = ( strlen( temp_argument ) > 0 );
            }
        }
    }

} /* parse_args() */


/*******************************************************************
*
*   process_args()
*
*   DESCRIPTION:
*       Run the program with the given arguments.
*
*******************************************************************/

static bool process_args( const ProgramArguments *arguments )
{
if( !check_file_exists( arguments->definition.str ) )
    {
    print_error( "Input definition file was provided, but did not exist." );
    return( false );
    }

size_t json_size = get_file_char_size( arguments->definition.str );
if( json_size == 0 )
    {
    print_error( "Input definition file was empty." );
    }

static ParseDefinitionState parse = {};
parse.json_string = (char*)malloc( json_size );
read_json_as_string( arguments->definition.str, json_size, parse.json_string );

cJSON *json = cJSON_ParseWithLength( parse.json_string, json_size );
const char *error = cJSON_GetErrorPtr();
if( error )
    {
    print_error( "Parsing error when processing input definition file. (%s)", error );
    return( false );
    }

bool success = false;
DefinitionVisitor visitor;
std::vector<AssetFileAssetId> asset_ids;
AssetFileWriter output_file = {};
std::unordered_map<std::string, AssetFileAssetId> texture_map;
WriteStats fonts_stats = {};
WriteStats models_stats = {};
WriteStats textures_stats = {};
//WriteStats shaders_stats = {};
AssetIdToExtentMap texture_extent_map;
WriteStats sound_sample_stats = {};
WriteStats music_clip_stats = {};
std::vector<ExportSoundPair> sound_sample_pairs;
std::vector<ExportSoundPair> music_clip_pairs;
const cJSON *assets = cJSON_GetObjectItemCaseSensitive( json, "assets" );
if( !assets )
    {
    print_error( "Parsing error.  Unable to find 'assets' node in definition JSON file." );
    goto error_cleanup;
    }

visit_all_definition_assets( assets, arguments->assets_folder.str, arguments->input_fonts_folder.str, &visitor );
for( auto &entry : visitor.asset_map )
    {
    asset_ids.push_back( entry.first );
    }

asset_ids.push_back( ASSET_FILE_TEXTURE_EXTENT_ASSET_ID ); /* <MPA> This is a nasty hack to get the texture extents table to be a valid asset ID - THIS MEANS IT MUST BE PROCESSED LAST */

if( asset_ids.empty() )
    {
    print_warning( "No assets found in definition.  Quitting..." );
    success = true;
    goto error_cleanup;
    }

std::sort( asset_ids.begin(), asset_ids.end() );
CreateDirectoryA( arguments->output_binary_folder.str, 0 );
if( !AssetFile_CreateForWrite( arguments->output_binary.str, &asset_ids[ 0 ], (uint32_t)asset_ids.size(), &output_file ) )
    {
    std::vector<char> cwd_buffer;
    cwd_buffer.resize( 1000 );
    GetCurrentDirectoryA( (DWORD)cwd_buffer.size(), cwd_buffer.data() );
    print_error( "Could not create output file at the path requested (%s), working directory = (%s).", arguments->output_binary.str, cwd_buffer.data() );
    goto error_cleanup;
    }

visitor.ExtractTextureMap( &texture_map );
for( auto &entry : visitor.asset_map )
    {
    WriteStats this_stats = {};
    switch( entry.second.kind )
        {
        case ASSET_FILE_ASSET_KIND_FONT:
            this_stats = {};
            if( !ExportFont_Export( entry.first, entry.second.asset_id_str.c_str(), entry.second.filename.c_str(), entry.second.font_point_sz, entry.second.font_glyphs.c_str(), this_stats, &output_file) )
                {
                print_error( "Failed to load font (%s).  Exiting...", entry.second.filename.c_str() );
                goto error_cleanup;
                }

            fonts_stats.fonts_written++;
            fonts_stats.written_sz += this_stats.written_sz;
            break;

        case ASSET_FILE_ASSET_KIND_MODEL:
            this_stats = {};
            if( !ExportModel_Export( entry.first, entry.second.filename.c_str(), &texture_map, &this_stats, &output_file ) )
                {
                print_error( "Failed to load model (%s).  Exiting...", entry.second.filename.c_str() );
                goto error_cleanup;
                }

            models_stats.models_written++;
            models_stats.written_sz += this_stats.written_sz;
            break;

        case ASSET_FILE_ASSET_KIND_SOUND_MUSIC_CLIP:
            music_clip_pairs.push_back( { entry.second.filename.c_str(), entry.second.asset_id_str.c_str() });
            break;

        //case ASSET_FILE_ASSET_KIND_SHADER:
        //    this_stats = {};
        //    if( !ExportShader_Export( entry.first, entry.second.filename.c_str(), entry.second.shader_target.c_str(), entry.second.shader_entry_point.c_str(), &this_stats, &output_file) )
        //        {
        //        print_error( "Failed to load shader (%s).  Exiting...", entry.second.filename.c_str() );
        //        }

        //    shaders_stats.shaders_written++;
        //    shaders_stats.written_sz += this_stats.written_sz;
        //    break;

        case ASSET_FILE_ASSET_KIND_SOUND_SAMPLE:
            sound_sample_pairs.push_back( { entry.second.filename.c_str(), entry.second.asset_id_str.c_str() } );
            break;

        case ASSET_FILE_ASSET_KIND_TEXTURE:
            this_stats = {};
            if( !ExportTexture_Export( entry.first, entry.second.filename.c_str(), texture_extent_map, &this_stats, &output_file ) )
                {
                print_error( "Failed to load texture (%s).  Exiting...", entry.second.filename.c_str() );
                }

            textures_stats.textures_written++;
            textures_stats.written_sz += this_stats.written_sz;
            break;

        default:
            print_warning( "Encountered unknown asset kind (%d).  Ignoring...", entry.second.kind );
            break;
        }

    }

if( sound_sample_pairs.size()
 || music_clip_pairs.size() )
    {
    ExportSounds_CreateBanks( sound_sample_pairs, sound_sample_stats, music_clip_pairs, music_clip_stats, arguments->output_soundbank_folder.str );
    }
	
success = ExportTexture_WriteTextureExtents( texture_extent_map, &output_file );

success = AssetFile_CloseForWrite( &output_file );
printf( "\n" );
print_info( "<" ASSET_FILE_BINARY_FILENAME ">     %d Models (%d bytes), %d Textures (%d bytes), %d Fonts (%d bytes)",
            (int)models_stats.models_written,     (int)models_stats.written_sz,
            (int)textures_stats.textures_written, (int)textures_stats.written_sz,
            (int)fonts_stats.fonts_written,       (int)fonts_stats.written_sz );
print_info( "<" ASSET_FILE_SOUND_BANK_FILENAME ">  %d samples (%0.1f MB).", (int)sound_sample_stats.sound_samples_written, ( (float)sound_sample_stats.written_sz ) / ( 1024 * 1024 ) );
print_info( "<" ASSET_FILE_MUSIC_BANK_FILENAME ">   %d clips (%0.1f MB).",   (int)music_clip_stats.music_clips_written,     ( (float)music_clip_stats.written_sz ) / ( 1024 * 1024) );

error_cleanup:
cJSON_Delete( json );

return( success );

} /* process_args() */


/*******************************************************************
*
*   read_json_as_string()
*
*   DESCRIPTION:
*       Read the JSON string into the output character buffer.
*
*******************************************************************/

static bool read_json_as_string( const char *filename, const size_t sz, char *out )
{
FILE *fhnd = NULL;
errno_t err = fopen_s( &fhnd, filename, "r" );
if( err )
    {
    return( false );
    }

size_t size_read = fread( out, sz, 1, fhnd );
fclose( fhnd );

return( size_read == sz );

} /* read_json_as_string() */


/*******************************************************************
*
*   visit_all_definition_assets()
*
*   DESCRIPTION:
*       Visit all the assets in the definition JSON file.
*
*******************************************************************/

static bool visit_all_definition_assets( const cJSON *assets, const char *asset_folder, const char *input_font_folder, _DefinitionVisitor *visitor )
{
/* Fonts */
const cJSON *fonts = cJSON_GetObjectItemCaseSensitive( assets, "font" );
if( fonts )
    {    
    const cJSON *font_list = cJSON_GetObjectItemCaseSensitive( fonts, "list" );
    const cJSON *font = NULL;
    cJSON_ArrayForEach( font, font_list )
        {
        const cJSON *font_filename = cJSON_GetObjectItemCaseSensitive( font, "filename" );
        const cJSON *font_asset_id = cJSON_GetObjectItemCaseSensitive( font, "assetid" );
        const cJSON *font_point_size = cJSON_GetObjectItemCaseSensitive( font, "pt" );
        const cJSON *font_glyphs = cJSON_GetObjectItemCaseSensitive( font, "glyphs" );

        if( !font_filename
         || !cJSON_IsString( font_filename ) )
            {
            print_error( "Could not find filename for font (%s).", cJSON_Print( font ) );
            return( false );
            }
        else if( !font_asset_id
              || !cJSON_IsString( font_asset_id ) )
            {
            print_error( "Could not find asset ID for font (%s)", cJSON_Print( font ) );
            return( false );
            }
        else if( !font_point_size
              || !cJSON_IsNumber( font_point_size ) )
            {
            print_error( "Could not find point size for font (%s)", cJSON_Print( font ) );
            return( false );
            }
        else if( !font_glyphs
              || !cJSON_IsString( font_glyphs ) )
            {
            print_error( "Could not find glyphs for font (%s)", cJSON_Print( font ) );
            return( false );
            }
      
        std::string font_filename_str( input_font_folder );
        font_filename_str.append( "/" );
        font_filename_str.append( font_filename->valuestring );
        font_filename_str = resolve_environments( font_filename_str.c_str() );

        if( !check_file_exists( font_filename_str.c_str() ) )
            {
            print_error( "Could not find font for filename %s (%s)", font_filename_str.c_str(), cJSON_Print( font ) );
            return( false );
            }

        std::ostringstream os;
        os << fonts->string << "/" << font_asset_id->valuestring;        
        visitor->VisitFont( os.str().c_str(), font_filename_str.c_str(), font_point_size->valueint, font_glyphs->valuestring );
        }

    }

/* Models */
const cJSON *models = cJSON_GetObjectItemCaseSensitive( assets, "model" );
if( models )
    {
    std::string basefolder( asset_folder );
    basefolder.append( "/" );
    
    const cJSON *folder_object = cJSON_GetObjectItemCaseSensitive( models, "basefolder" );
    if( cJSON_IsString( folder_object ) )
        {
        basefolder.append( folder_object->valuestring );
        basefolder.append( "/" );
        }

    const cJSON *model_list = cJSON_GetObjectItemCaseSensitive( models, "list" );
    const cJSON *model = NULL;
    cJSON_ArrayForEach( model, model_list )
        {
        const cJSON *model_filename = cJSON_GetObjectItemCaseSensitive( model, "filename" );
        const cJSON *model_asset_id = cJSON_GetObjectItemCaseSensitive( model, "assetid" );

        if( !model_filename
         || !cJSON_IsString( model_filename ) )
            {
            print_error( "Could not find filename for model (%s).", cJSON_Print( model ) );
            return( false );
            }
        else if( !model_asset_id
              || !cJSON_IsString( model_asset_id ) )
            {
            print_error( "Could not find asset ID for model (%s)", cJSON_Print( model ) );
            return( false );
            }
      
        std::string model_filename_str( basefolder );
        model_filename_str.append( model_filename->valuestring );
        if( !check_file_exists( model_filename_str.c_str() ) )
            {
            print_error( "Could not find model for filename %s (%s)", model_filename_str.c_str(), cJSON_Print( model ) );
            return( false );
            }

        std::ostringstream os;
        os << models->string << "/" << model_asset_id->valuestring;
        visitor->VisitModel( os.str().c_str(), model_filename_str.c_str() );
        }

    }

///* Shaders */
//const cJSON *shaders = cJSON_GetObjectItemCaseSensitive( assets, "shaders" );
//if( shaders )
//    {
//    std::string basefolder( asset_folder );
//    basefolder.append( "/" );
//    
//    const cJSON *folder_object = cJSON_GetObjectItemCaseSensitive( shaders, "basefolder" );
//    if( cJSON_IsString( folder_object ) )
//        {
//        basefolder.append( folder_object->valuestring );
//        basefolder.append( "/" );
//        }
//
//    const cJSON *shader_list = cJSON_GetObjectItemCaseSensitive( shaders, "list" );
//    const cJSON *shader = NULL;
//    cJSON_ArrayForEach( shader, shader_list )
//        {
//        const cJSON *shader_filename    = cJSON_GetObjectItemCaseSensitive( shader, "filename" );
//        const cJSON *shader_asset_id    = cJSON_GetObjectItemCaseSensitive( shader, "assetid" );
//        const cJSON *shader_target      = cJSON_GetObjectItemCaseSensitive( shader, "target" );
//        const cJSON *shader_entry_point = cJSON_GetObjectItemCaseSensitive( shader, "entry_point" );
//
//        if( !shader_filename
//         || !cJSON_IsString( shader_filename ) )
//            {
//            print_error( "Could not find filename for model (%s).", cJSON_Print( shader ) );
//            return( false );
//            }
//        else if( !shader_asset_id
//              || !cJSON_IsString( shader_asset_id ) )
//            {
//            print_error( "Could not find asset ID for model (%s)", cJSON_Print( shader ) );
//            return( false );
//            }
//        else if( !shader_target
//              || !cJSON_IsString( shader_target ) )
//            {
//            print_error( "Shader definition was missing target string (%s)", cJSON_Print( shader ) );
//            return( false );
//            }
//        else if( !shader_entry_point
//              || !cJSON_IsString( shader_entry_point ) )
//            {
//            print_error( "Shader definition was missing entry point string (%s)", cJSON_Print( shader ) );
//            return( false );
//            }
//      
//        std::string shader_filename_str( basefolder );
//        shader_filename_str.append( shader_filename->valuestring );
//        if( !check_file_exists( shader_filename_str.c_str() ) )
//            {
//            print_error( "Could not find shader for filename %s (%s)", shader_filename_str.c_str(), cJSON_Print( shader ) );
//            return( false );
//            }
//
//        std::ostringstream os;
//        os << shaders->string << "/" << shader_asset_id->valuestring;
//        visitor->VisitShader( os.str().c_str(), shader_filename_str.c_str(), shader_target->valuestring, shader_entry_point->valuestring );
//        }
//
//    }

/* Textures */
const cJSON *textures = cJSON_GetObjectItemCaseSensitive( assets, "texture" );
if( textures )
    {
    std::string basefolder( asset_folder );
    basefolder.append( "/" );
    
    const cJSON *folder_object = cJSON_GetObjectItemCaseSensitive( textures, "basefolder" );
    if( cJSON_IsString( folder_object ) )
        {
        basefolder.append( folder_object->valuestring );
        basefolder.append( "/" );
        }

    const cJSON *texture_list = cJSON_GetObjectItemCaseSensitive( textures, "list" );
    const cJSON *texture = NULL;
    cJSON_ArrayForEach( texture, texture_list )
        {
        const cJSON *texture_filename = cJSON_GetObjectItemCaseSensitive( texture, "filename" );
        const cJSON *texture_asset_id = cJSON_GetObjectItemCaseSensitive( texture, "assetid" );

        if( !texture_filename
         || !cJSON_IsString( texture_filename ) )
            {
            print_error( "Could not find filename for texture (%s).", cJSON_Print( texture ) );
            return( false );
            }
        else if( !texture_asset_id
              || !cJSON_IsString( texture_asset_id ) )
            {
            print_error( "Could not find asset ID for texture (%s)", cJSON_Print( texture ) );
            return( false );
            }
      
        std::string texture_filename_str( basefolder );
        texture_filename_str.append( texture_filename->valuestring );
        if( !check_file_exists( texture_filename_str.c_str() ) )
            {
            print_error( "Could not find texture for filename %s (%s)", texture_filename_str.c_str(), cJSON_Print( texture ) );
            return( false );
            }

        std::ostringstream os;
        os << textures->string << "/" << texture_asset_id->valuestring;
        visitor->VisitTexture( os.str().c_str(), texture_filename_str.c_str());
        }

    }

/* Sound samples */
const cJSON *sound_samples = cJSON_GetObjectItemCaseSensitive( assets, "sound_sample" );
if( sound_samples )
    {
    std::string basefolder( asset_folder );
    basefolder.append( "/" );
    
    const cJSON *folder_object = cJSON_GetObjectItemCaseSensitive( sound_samples, "basefolder" );
    if( cJSON_IsString( folder_object ) )
        {
        basefolder.append( folder_object->valuestring );
        basefolder.append( "/" );
        }

    const cJSON *sound_sample_list = cJSON_GetObjectItemCaseSensitive( sound_samples, "list" );
    const cJSON *sound_sample = NULL;
    cJSON_ArrayForEach( sound_sample, sound_sample_list )
        {
        const cJSON *sound_sample_filename = cJSON_GetObjectItemCaseSensitive( sound_sample, "filename" );
        const cJSON *sound_sample_asset_id = cJSON_GetObjectItemCaseSensitive( sound_sample, "assetid" );

        if( !sound_sample_filename
         || !cJSON_IsString( sound_sample_filename ) )
            {
            print_error( "Could not find filename for sound sample (%s).", cJSON_Print( sound_sample ) );
            return( false );
            }
        else if( !sound_sample_asset_id
              || !cJSON_IsString( sound_sample_asset_id ) )
            {
            print_error( "Could not find asset ID for sound sample (%s)", cJSON_Print( sound_sample ) );
            return( false );
            }
      
        std::string sound_sample_filename_str( basefolder );
        sound_sample_filename_str.append( sound_sample_filename->valuestring );
        if( !check_file_exists( sound_sample_filename_str.c_str() ) )
            {
            print_error( "Could not find sound sample for filename %s (%s)", sound_sample_filename_str.c_str(), cJSON_Print( sound_sample ) );
            return( false );
            }

        std::ostringstream os;
        os << sound_samples->string << "/" << sound_sample_asset_id->valuestring;
        visitor->VisitSoundSample( os.str().c_str(), sound_sample_filename_str.c_str());
        }

    }

/* Music clips */
const cJSON *music_clips = cJSON_GetObjectItemCaseSensitive( assets, "sound_music" );
if( music_clips )
    {
    std::string basefolder( asset_folder );
    basefolder.append( "/" );
    
    const cJSON *folder_object = cJSON_GetObjectItemCaseSensitive( music_clips, "basefolder" );
    if( cJSON_IsString( folder_object ) )
        {
        basefolder.append( folder_object->valuestring );
        basefolder.append( "/" );
        }

    const cJSON *music_clip_list = cJSON_GetObjectItemCaseSensitive( music_clips, "list" );
    const cJSON *music_clip = NULL;
    cJSON_ArrayForEach( music_clip, music_clip_list )
        {
        const cJSON *music_clip_filename = cJSON_GetObjectItemCaseSensitive( music_clip, "filename" );
        const cJSON *music_clip_asset_id = cJSON_GetObjectItemCaseSensitive( music_clip, "assetid" );

        if( !music_clip_filename
         || !cJSON_IsString( music_clip_filename ) )
            {
            print_error( "Could not find filename for music clip (%s).", cJSON_Print( music_clip ) );
            return( false );
            }
        else if( !music_clip_asset_id
              || !cJSON_IsString( music_clip_asset_id ) )
            {
            print_error( "Could not find asset ID for music clip (%s)", cJSON_Print( music_clip ) );
            return( false );
            }
      
        std::string music_clip_filename_str( basefolder );
        music_clip_filename_str.append( music_clip_filename->valuestring );
        if( !check_file_exists( music_clip_filename_str.c_str() ) )
            {
            print_error( "Could not find music clip for filename %s (%s)", music_clip_filename_str.c_str(), cJSON_Print( music_clip ) );
            return( false );
            }

        std::ostringstream os;
        os << music_clips->string << "/" << music_clip_asset_id->valuestring;
        visitor->VisitMusicClip( os.str().c_str(), music_clip_filename_str.c_str());
        }

    }

return( true );

} /* visit_all_definition_assets() */
