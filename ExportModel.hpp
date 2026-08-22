#pragma once

#include <unordered_map>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"


bool ExportModel_Export( const AssetFileAssetId id, const char *filename, const float global_scale, const std::unordered_map<std::string, AssetFileAssetId> *texture_map, WriteStats *stats, std::vector<std::string> &out_strs, AssetFileWriter *output );
bool ExportModel_GetTextures( const char *filename, std::vector<std::string> &texture_names );