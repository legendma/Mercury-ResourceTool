#pragma once

#include <unordered_map>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"


bool ExportModel_Export( const AssetFileAssetId id, const char *filename, const std::unordered_map<std::string, AssetFileAssetId> *texture_map, WriteStats *stats, AssetFileWriter *output );