#include <fmod_common.h>
#include <fmod_errors.h>
#include <fmod.h>
#include <fsbank.h>
#include <fsbank_errors.h>
#include <cstdio>
#include <string>
#include <vector>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"
#include "ExportSounds.hpp"

using FilenamesList = std::vector<std::vector<const char*>>;
using FiledatumList = std::vector<std::vector<const void*>>;
using FileLengthsList = std::vector<unsigned int>;
using SubsoundsList = std::vector<FSBANK_SUBSOUND>;

// bank compression level constants.  1 is highest compression, 100 is highest quality, 0 is default
#define SOUND_SAMPLE_BANK_COMPRESSION_LEVEL ( 0 )
#define MUSIC_BANK_COMPRESSION_LEVEL ( 0 ) 
#define BANK_ENCRYPTION_KEY ( "DEFAULT" )

static bool FileErrorChecker( const char *filepath, FILE *file_handle, const uint32_t seek_error_code, const uint32_t read_error_code, const uint32_t expected_file_length );
static bool CreateSubsound( const ExportSoundPair &input, FilenamesList &filenames, FiledatumList &file_datas, FileLengthsList &file_lengths, FSBANK_SUBSOUND &output );


/*******************************************************************
*
*   ExportSounds_CreateBanks()
*
*   DESCRIPTION:
*		creates the sounds banks
*
*******************************************************************/

bool ExportSounds_CreateBanks( std::vector<ExportSoundPair> &samples, WriteStats &samples_stats, std::vector<ExportSoundPair> &music_clips, WriteStats &music_clip_stats, const char *bank_output_folder)
{
FSBANK_RESULT fsbank_error_code;
const char *fsbank_cacheDirectory = "FSBANK_CACHE";
const int num_cpu_cores = 1;
// TODO <jw>, pull CPU core count and pass this into the functions to enable multithreading if perf is a problem

fsbank_error_code = FSBank_Init( FSBANK_FSBVERSION_FSB5, FSBANK_INIT_NORMAL, num_cpu_cores, fsbank_cacheDirectory );

if( fsbank_error_code != FSBANK_OK )
    {
    print_error( "ERROR: fmod FSBank failed to intiailize. %s ", FSBank_ErrorString( fsbank_error_code ) );
    fsbank_error_code = FSBank_Release();
    return false;
    }

// convert the input sound data into FSBank subsound data format. Does not support interleaved subsounds.
FilenamesList file_name_dummy;  //dummy
FiledatumList file_data_dummy;  //dummy
FileLengthsList file_length_dummy;

std::vector<FSBANK_SUBSOUND> sample_subsounds_array;
//create sound sample subsound array
for( auto &sample : samples )
    {
    sample_subsounds_array.push_back({});
    FSBANK_SUBSOUND &subsound = sample_subsounds_array.back();

    CreateSubsound( sample, file_name_dummy, file_data_dummy, file_length_dummy, subsound );
    }

std::vector<FSBANK_SUBSOUND> music_subsounds_array;
    //create music subsound array
for( auto &clip : music_clips )
    {
    music_subsounds_array.push_back( {} );
    FSBANK_SUBSOUND &subsound = music_subsounds_array.back();
    CreateSubsound( clip, file_name_dummy, file_data_dummy, file_length_dummy, subsound );
    }

std::string sound_bank_name( bank_output_folder );
sound_bank_name.append( "\\" ASSET_FILE_SOUND_BANK_FILENAME );
fsbank_error_code = FSBank_Build( sample_subsounds_array.data(), 1, FSBANK_FORMAT_PCM, FSBANK_BUILD_DEFAULT, SOUND_SAMPLE_BANK_COMPRESSION_LEVEL, BANK_ENCRYPTION_KEY, sound_bank_name.c_str() );


std::string music_bank_name( bank_output_folder );
music_bank_name.append( "\\" ASSET_FILE_MUSIC_BANK_FILENAME );
fsbank_error_code = FSBank_Build( music_subsounds_array.data(), 1, FSBANK_FORMAT_PCM, FSBANK_BUILD_DEFAULT, SOUND_SAMPLE_BANK_COMPRESSION_LEVEL, BANK_ENCRYPTION_KEY, music_bank_name.c_str() );

return true;

}/* ExportSounds_CreateBanks() */


/*******************************************************************
*
*   CreateSubsound()
*
*   DESCRIPTION:
*		creates a single subsound with no interleaving
*
*******************************************************************/

static bool CreateSubsound( const ExportSoundPair &input, FilenamesList &filenames, FiledatumList &file_datas, FileLengthsList &file_lengths, FSBANK_SUBSOUND &output )
{
output = {};
output.numFiles = 1;

// get file names
filenames.push_back({});
auto &files = filenames.back();
files.push_back( input.str_filename_w_path.c_str() );
output.fileNames = filenames.back().data();

// get file data
const char *filepath = input.str_filename_w_path.c_str();
FILE *sound_file = std::fopen( filepath, "rb" );
uint32_t seek_error = std::fseek( sound_file, 0, SEEK_END );
uint32_t file_length = std::ftell( sound_file );
std::rewind( sound_file );
char *file_data = (char *)malloc( file_length );
uint32_t read_error = (uint32_t)std::fread( file_data, 1, file_length, sound_file );

if( FileErrorChecker( filepath, sound_file, seek_error, read_error, file_length ) == false )
    {
    return false;
    }

file_datas.push_back({});
auto &data = file_datas.back();
data.push_back( file_data );
output.fileData = file_datas.back().data();

file_lengths.push_back( file_length );
output.fileDataLengths = &file_lengths.back();

std::fclose( sound_file );

}/* CreateSubsound() */


/*******************************************************************
*
*   FileErrorChecker()
*
*   DESCRIPTION:
*		reports errors openining and reading sound files. returns true if no error
*
*******************************************************************/

static bool FileErrorChecker(const char * filepath, FILE * file_handle, const uint32_t seek_error_code, const uint32_t read_error_code, const uint32_t expected_file_length )
{
    if( file_handle == NULL || seek_error_code != 0 || read_error_code != expected_file_length || feof(file_handle) != 0 )
    {
        print_error( "The following sound or music asset had an error:  %s ", filepath );

        if( file_handle == NULL )
        {
            print_error( "The file was unable to be opened. Please check the path");
            return false;
        }
        if( seek_error_code != 0 )
        {
            print_error( "There was an error seeking the file. The error code is: %s", ferror( file_handle ) );
            return false;
        }
        if( read_error_code != 0 )
        {
            print_error( "There was an error reading the file or the end of file was reached." );

            if( ferror( file_handle ) != NULL )
            {
                print_error("The file read error: %s", ferror( file_handle ));
            }
            else if (feof( file_handle ) != 0 )
            {
                print_error( "The end of file was reached prematurely" );
                print_error( "%d %s %d %s", read_error_code, " out of ", expected_file_length, " bytes were read." );
            }
            else print_error( "An unknown error occured. good luck!" );
            return false;         
        }
    }

    return true;
}/* FileErrorChecker() */