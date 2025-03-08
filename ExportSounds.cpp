#include <fsbank.h>
#include <fsbank_errors.h>
#include <cstdio>
#include <string>
#include <vector>

#include "AssetFile.hpp"
#include "ExportSounds.hpp"
#include "ResourceUtilities.hpp"


using FilenamesList = std::vector<std::vector<const char*>>;
using SubsoundsList = std::vector<FSBANK_SUBSOUND>;

// bank compression level constants.  1 is highest compression, 100 is highest quality, 0 is default
#define SOUND_SAMPLE_BANK_COMPRESSION_LEVEL ( 0 )
#define MUSIC_BANK_COMPRESSION_LEVEL ( 0 ) 
#define BANK_ENCRYPTION_KEY ( "DEFAULT" )


static bool     BuildBank( const std::string &name, const std::vector<FSBANK_SUBSOUND> &subsounds );
static uint32_t LogStats( const std::string &format, std::string &filename, std::vector<ExportSoundPair> &assets, WriteStats &stats );
static void     PrepareBank( const std::vector<ExportSoundPair> &assets, std::vector<AssetFileSoundPair> &pairs, std::vector<FSBANK_SUBSOUND> &subsounds, FilenamesList &filenames );
static bool     WritePairsToBinary( const AssetFileAssetId bank_id, const AssetFileAssetKind kind, const std::vector<AssetFileSoundPair> &pairs, AssetFileWriter *output );


/*******************************************************************
*
*   ExportSounds_CreateBanks()
*
*   DESCRIPTION:
*		creates the sounds banks
*
*******************************************************************/

bool ExportSounds_CreateBanks( std::vector<ExportSoundPair> &samples, WriteStats &samples_stats, std::vector<ExportSoundPair> &music_clips, WriteStats &music_clip_stats, const char *bank_output_folder, AssetFileWriter *output )
{
samples_stats = {};
music_clip_stats = {};

const char *fsbank_cacheDirectory = "FSBANK_CACHE";
const int num_cpu_cores = 1;
// TODO <jw>, pull CPU core count and pass this into the functions to enable multithreading if perf is a problem

FSBANK_RESULT fsbank_error_code = FSBank_Init( FSBANK_FSBVERSION_FSB5, FSBANK_INIT_NORMAL, num_cpu_cores, fsbank_cacheDirectory );
if( fsbank_error_code != FSBANK_OK )
    {
    print_error( "ERROR: fmod FSBank failed to intiailize. %s \n ", FSBank_ErrorString( fsbank_error_code ) );
    fsbank_error_code = FSBank_Release();
    return false;
    }

/* convert the input sound data into FSBank subsound data format. Does not support interleaved subsounds. */
FilenamesList filename_storage;

std::vector<FSBANK_SUBSOUND> sample_subsounds;
std::vector<AssetFileSoundPair> sample_pairs;
PrepareBank( samples, sample_pairs, sample_subsounds, filename_storage );

std::vector<FSBANK_SUBSOUND> music_subsounds;
std::vector<AssetFileSoundPair> music_pairs;
PrepareBank( music_clips, music_pairs, music_subsounds, filename_storage );

/* build the banks! */
std::string samples_filename( bank_output_folder );
samples_filename.append( "\\" ASSET_FILE_SOUND_BANK_FILENAME );
std::string music_filename( bank_output_folder );
music_filename.append( "\\" ASSET_FILE_MUSIC_BANK_FILENAME );
if( !BuildBank( samples_filename, sample_subsounds )
 || !BuildBank( music_filename,   music_subsounds ) )
    {
    return( false );
    }

FSBank_Release();

/* Write the Asset name/index data to the binary. */
AssetFileAssetId sound_bank_asset_id = AssetFile_MakeAssetIdFromName( ASSET_FILE_SOUND_BANK_FILENAME, (uint32_t)strlen( ASSET_FILE_SOUND_BANK_FILENAME ) );
if( !WritePairsToBinary( sound_bank_asset_id, ASSET_FILE_ASSET_KIND_SOUND_SAMPLE, sample_pairs, output ) )
    {
    return( false );
    }

AssetFileAssetId music_bank_asset_id = AssetFile_MakeAssetIdFromName( ASSET_FILE_MUSIC_BANK_FILENAME, (uint32_t)strlen( ASSET_FILE_MUSIC_BANK_FILENAME ) );
if( !WritePairsToBinary( music_bank_asset_id, ASSET_FILE_ASSET_KIND_SOUND_MUSIC_CLIP, music_pairs, output ) )
    {
    return( false );
    }

/* write stats */
samples_stats.sound_samples_written  = LogStats( "[SOUND]     %s", samples_filename, samples, samples_stats );
music_clip_stats.music_clips_written = LogStats( "[MUSIC]     %s", music_filename, music_clips, music_clip_stats );

return true;

}/* ExportSounds_CreateBanks() */


/*******************************************************************
*
*   BuildBank()
*
*******************************************************************/

static bool BuildBank( const std::string &name, const std::vector<FSBANK_SUBSOUND> &subsounds )
{
FSBANK_RESULT fsbank_error_code = FSBank_Build( subsounds.data(), (unsigned int)subsounds.size(), FSBANK_FORMAT_FADPCM, FSBANK_BUILD_DEFAULT, SOUND_SAMPLE_BANK_COMPRESSION_LEVEL, NULL, name.c_str() );
if( fsbank_error_code != FSBANK_OK )
    {
    print_error( "ERROR: fmod FSBank %s failed to build with error code: %s ", name.c_str(), FSBank_ErrorString( fsbank_error_code ) );
    fsbank_error_code = FSBank_Release();
    return false;
    }

return( true );

}   /* BuildBank() */


/*******************************************************************
*
*   LogStats()
*
*******************************************************************/

static uint32_t LogStats( const std::string &format, std::string &filename, std::vector<ExportSoundPair> &assets, WriteStats &stats )
{
uint32_t asset_cnt = 0;
for( auto &asset : assets )
    {
    print_info( format.c_str(), strip_filename( asset.str_filename_w_path.c_str() ).c_str() );
    asset_cnt++;
    }

FILE *fhnd = std::fopen( filename.c_str(), "rb" );
std::fseek( fhnd, 0, SEEK_END );
stats.written_sz = std::ftell( fhnd );
fclose( fhnd );

return( asset_cnt );

}   /* LogStats() */


/*******************************************************************
*
*   PrepareBank()
*
*******************************************************************/

static void PrepareBank( const std::vector<ExportSoundPair> &assets, std::vector<AssetFileSoundPair> &pairs, std::vector<FSBANK_SUBSOUND> &subsounds, FilenamesList &filenames )
{
for( auto &asset : assets )
    {
    pairs.push_back( {} );
    auto &pair = pairs.back();
    pair.asset_id = AssetFile_MakeAssetIdFromName( asset.str_asset_id.c_str(), (uint32_t)asset.str_asset_id.length() );
    pair.subsound_index = (uint32_t)subsounds.size();

    /* build subsound */
    filenames.push_back( {} );
    auto &files = filenames.back();
    files.push_back( asset.str_filename_w_path.c_str() );

    subsounds.push_back( {} );
    FSBANK_SUBSOUND &subsound = subsounds.back();
    subsound.numFiles = 1;
    subsound.fileNames = filenames.back().data();
    }

}   /* PrepareBank() */


/*******************************************************************
*
*   WritePairsToBinary()
*
*******************************************************************/

static bool WritePairsToBinary( const AssetFileAssetId bank_id, const AssetFileAssetKind kind, const std::vector<AssetFileSoundPair> &pairs, AssetFileWriter *output )
{
if( !AssetFile_BeginWritingAsset( bank_id, kind, output ) )
    {
    print_error("ERROR: The Sound bank pair data had an error in AssetFile_BeginWritingAsset. \n");
    return false;
    }

if( !AssetFile_WriteSoundPairs( pairs.data(), (uint16_t)pairs.size(), output ) ) 
    {
    print_error( "ERROR: The Sound bank pair data had an error in AssetFile_WriteSoundPairs. \n" );
    return false;
    }

if( !AssetFile_EndWritingAsset( output ) )
    {
    print_error( "ERROR: The Sound bank pair data had an error in AssetFile_EndWritingAsset. \n" );
    return false;
    }

return( true );

}   /* WritePairsToBinary() */
