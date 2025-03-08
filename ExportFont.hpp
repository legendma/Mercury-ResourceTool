#pragma once
#include "AssetFile.hpp"
#include "ResourceUtilities.hpp"

bool ExportFont_Export( const AssetFileAssetId id, const char *asset_id_str, const char *filename, const int point_size, const char *glyphs, WriteStats &stats, AssetFileWriter *output );
