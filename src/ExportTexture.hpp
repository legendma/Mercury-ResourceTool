#pragma once

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"


bool ExportTexture_Export( const AssetFileAssetId id, const char *filename, WriteStats *stats, AssetFileWriter *output );