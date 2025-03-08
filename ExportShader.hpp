#pragma once

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"


bool ExportShader_Export( const AssetFileAssetId id, const char *filename, const char *target, const char *entry_point, WriteStats *stats, AssetFileWriter *output );