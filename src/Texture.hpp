#pragma once

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"


bool Texture_Load( const AssetFileAssetId id, const char *filename, WriteStats *stats, AssetFileWriter *output );