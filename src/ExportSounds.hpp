#include <string>
#include <vector>

#include "ResourceUtilities.hpp"

typedef struct
    {
    std::string         str_filename_w_path;
    std::string         str_asset_id;
    } ExportSoundPair;


bool ExportSounds_CreateBanks( std::vector<ExportSoundPair> &samples, WriteStats &samples_stats, std::vector<ExportSoundPair> &music_clips, WriteStats &music_clip_stats, const char *bank_output_folder );