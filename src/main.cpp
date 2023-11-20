/*******************************************************************
*  ResourcePackager.exe -d [definition filename and path] -o [output binary filename and path] -r [root to input assets]
*******************************************************************/
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <string>
#include <vector>
#include <io.h>

#include "cjson.h"
#include "model.hpp"

#define MAX_ARGUMENT_LENGTH         ( 500 )
#define ARGUMENT_INPUT_DEFINITION   "-d"
#define ARGUMENT_OUTPUT_BINARY      "-o"
#define ARGUMENT_ASSET_ROOT         "-r"

typedef struct _ProgramArgumentsDefinition
    {
    bool                is_valid;
    char                filename[ MAX_ARGUMENT_LENGTH ];
    } ProgramArgumentsDefinition;

typedef struct _ProgramArgumentsOutput
    {
    bool                is_valid;
    char                filename[ MAX_ARGUMENT_LENGTH ];
    } ProgramArgumentsOutput;

typedef struct _ProgramArgumentsAssetsRoot
    {
    bool                is_valid;
    char                foldername[ MAX_ARGUMENT_LENGTH ];
    } ProgramArgumentsAssetsRoot;

typedef struct _ProgramArguments
    {
    ProgramArgumentsDefinition
                        definition;
    ProgramArgumentsOutput
                        output_binary;
    ProgramArgumentsAssetsRoot
                        assets_folder;
    } ProgramArguments;

typedef struct _ParseDefinitionState
    {
    char               *json_string;
    } ParseDefinitionState;


static bool check_file_exists( const char *filename );
static size_t get_file_char_size( const char *filename );
static void parse_args( int argc, char **argv, ProgramArguments *arguments );
static void print_error( const char *str, ... );
static bool process_args( const ProgramArguments *arguments );
static bool read_json_string( const char *filename, const size_t sz, char *out );


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
static ProgramArguments arguments;

if( argc <= 1 )
    {
    printf( "Usage: ResourcePackager.exe [-d FILENAME]... [-o FILENAME]... [-r PATH]... \n" );
    printf( "\tParse and package assets to binary file.\n\n" );
    printf( "Options:\n" );
    printf( "\t-d FILENAME   Filename with path to input .json file which lists the assets to be packaged.\n" );
    printf( "\t-o FILENAME   Filename with path to binary output file.\n" );
    printf( "\t-r PATH       Folder which is the root of assets defined in definition file.\n" );

    return( 0 );
    }

parse_args( argc, argv, &arguments );

bool has_error = false;
if( !arguments.definition.is_valid )
    {
    has_error = true;
    print_error( "No input definition file provided. ( " ARGUMENT_INPUT_DEFINITION " )\n" );
    }

if( !arguments.output_binary.is_valid )
    {
    has_error = true;
    print_error( "No output filename provided. (" ARGUMENT_OUTPUT_BINARY "\n" );
    }

if( !arguments.assets_folder.is_valid )
    {
    has_error = true;
    print_error( "No asset root folder name. (" ARGUMENT_ASSET_ROOT "\n" );
    }

if( !has_error )
    {
    has_error = process_args( &arguments );
    }

if( has_error )
    {
    return( -1 );
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
typedef struct _ArgumentExpectations
    {
    bool                is_setting_definition;
    bool                is_setting_asset_root;
    bool                is_setting_output_binary;
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
    else
        {
        if( expectation.is_setting_asset_root )
            {
            strcpy_s( arguments->assets_folder.foldername, sizeof( arguments->assets_folder.foldername ), temp_argument );
            arguments->assets_folder.is_valid = ( strlen( temp_argument ) > 0 );
            }
        else if( expectation.is_setting_definition )
            {
            strcpy_s( arguments->definition.filename, sizeof( arguments->definition.filename ), temp_argument );
            arguments->definition.is_valid = ( strlen( temp_argument ) > 0 );
            }
        else if( expectation.is_setting_output_binary )
            {
            strcpy_s( arguments->output_binary.filename, sizeof( arguments->output_binary.filename ), temp_argument );
            arguments->output_binary.is_valid = ( strlen( temp_argument ) > 0 );
            }
        }
    }


} /* parse_args() */


/*******************************************************************
*
*   print_error()
*
*   DESCRIPTION:
*       Print an error message.
*
*******************************************************************/

static void print_error( const char *str, ... )
{
va_list args;
va_start( args, str );

printf( "ERROR - [ResourcePackager] - " );
vprintf( str, args );

va_end( args );

} /* read_json_string() */


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
if( !check_file_exists( arguments->definition.filename ) )
    {
    print_error( "Input definition file was provided, but did not exist.\n" );
    return( false );
    }

size_t json_size = get_file_char_size( arguments->definition.filename );
if( json_size == 0 )
    {
    print_error( "Input definition file was empty.\n" );
    }

static ParseDefinitionState parse = {};
parse.json_string = (char*)malloc( json_size );
read_json_string( arguments->definition.filename, json_size, parse.json_string );

cJSON *json = cJSON_ParseWithLength( parse.json_string, json_size );
const char *error = cJSON_GetErrorPtr();
if( error )
    {
    print_error( "Parsing error when processing input definition file. (%s)\n", error );
    return( false );
    }

bool success = true;
std::vector<std::string> seen_asset_ids;
const cJSON *models = NULL;
const cJSON *assets = cJSON_GetObjectItemCaseSensitive( json, "assets" );
if( !assets )
    {
    print_error( "Parsing error.  Unable to find 'assets' node in definition JSON file.\n" );
    success = false;
    goto error_cleanup;
    }

/* Models */
models = cJSON_GetObjectItemCaseSensitive( assets, "models" );
if( models )
    {
    std::string basefolder( arguments->assets_folder.foldername );
    basefolder.append( "/" );
    
    const cJSON *folder_object = cJSON_GetObjectItemCaseSensitive( models, "basefolder" );
    if( cJSON_IsString( folder_object ) )
        {
        basefolder.append( folder_object->valuestring );
        basefolder.append( "/" );
        }

    const cJSON *model_list = cJSON_GetObjectItemCaseSensitive( models, "list" );
    const cJSON *model = NULL;
    cJSON_ArrayForEach( model_list, model )
        {
        const cJSON *model_filename = cJSON_GetObjectItemCaseSensitive( model, "filename" );
        const cJSON *model_asset_id = cJSON_GetObjectItemCaseSensitive( model, "assetid" );

        if( !model_filename
         || !cJSON_IsString( model_filename ) )
            {
            success = false;
            print_error( "Could not find filename for model (%s).\n", cJSON_Print( model ) );
            goto error_cleanup;
            }
        else if( !model_asset_id
              || !cJSON_IsString( model_asset_id ) )
            {
            success = false;
            print_error( "Could not find asset ID for model (%s)\n", cJSON_Print( model ) );
            goto error_cleanup;
            }

        std::string asset_id_str( model_asset_id->valuestring );
        if( asset_id_str.length() < 1 )
            {
            success = false;
            print_error( "Could not find asset ID for model (%s)\n", cJSON_Print( model ) );
            goto error_cleanup;
            }
        else if( std::find( seen_asset_ids.begin(), seen_asset_ids.end(), asset_id_str ) != seen_asset_ids.end() )
            {
            success = false;
            print_error( "Parser Error.  Found duplicate asset ID of %s (%s)\n", asset_id_str.c_str(), cJSON_Print( model ) );
            goto error_cleanup;
            }
        
        std::string model_filename_str( basefolder );
        model_filename_str.append( model_filename->valuestring );
        if( !check_file_exists( model_filename_str.c_str() ) )
            {
            success = false;
            print_error( "Could not model for filename %s (%s)\n", model_filename_str.c_str(), cJSON_Print( model ) );
            goto error_cleanup;
            }

        Model_Load( model_filename_str.c_str() );
        }

    }


error_cleanup:
cJSON_Delete( json );

return( false );

} /* process_args() */


/*******************************************************************
*
*   read_json_string()
*
*   DESCRIPTION:
*       Read the JSON string into the output character buffer.
*
*******************************************************************/

static bool read_json_string( const char *filename, const size_t sz, char *out )
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

} /* read_json_string() */