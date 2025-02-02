#pragma once
#include <map>

#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"

typedef struct
    {
    uint16_t            width;
    uint16_t            height;
    } TextureExtent;

using AssetIdToExtentMap = std::map<AssetFileAssetId, TextureExtent>;

bool ExportTexture_Export( const AssetFileAssetId id, const char *filename, AssetIdToExtentMap &extent_map, WriteStats *stats, AssetFileWriter *output );
bool ExportTexture_WriteTextureExtents( AssetIdToExtentMap &extent_map, AssetFileWriter *output );