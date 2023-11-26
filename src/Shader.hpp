#pragma once

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"


bool Shader_Load( const AssetFileAssetId id, const char *filename, const char *target, const char *entry_point, WriteStats *stats, AssetFileWriter *output );