#ifndef SrTileRenderer_h__
#define SrTileRenderer_h__

#include "SrTileTypes.h"

class SrRasterizer;

void SrBuildTileBins(const SrTileLayout& layout,
	const std::vector<SrPreparedTriangleBounds>& bounds,
	SrTileBins& bins);

#endif
