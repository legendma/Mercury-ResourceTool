#pragma once

#include <unordered_map>

#include "AssetFile.hpp"
#include "Utilities.hpp"


bool Model_Load( const AssetFileAssetId id, const char *filename, const std::unordered_map<std::string, AssetFileAssetId> *texture_map, WriteStats *stats, AssetFileWriter *output );