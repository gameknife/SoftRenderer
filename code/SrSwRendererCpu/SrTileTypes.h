#ifndef SrTileTypes_h__
#define SrTileTypes_h__

#include <algorithm>
#include <cstdint>
#include <vector>

struct SrTileRect
{
	uint32_t x0, y0, x1, y1;

	SrTileRect() : x0(0), y0(0), x1(0), y1(0) {}
	SrTileRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
		: x0(left), y0(top), x1(right), y1(bottom) {}
};

class SrTileLayout
{
public:
	SrTileLayout() : m_width(0), m_height(0), m_tileSize(32), m_columns(0), m_rows(0) {}
	SrTileLayout(uint32_t width, uint32_t height, uint32_t tileSize) { Reset(width, height, tileSize); }

	void Reset(uint32_t width, uint32_t height, uint32_t tileSize)
	{
		m_width = width;
		m_height = height;
		m_tileSize = (std::max<uint32_t>)(1, tileSize);
		m_columns = (m_width + m_tileSize - 1) / m_tileSize;
		m_rows = (m_height + m_tileSize - 1) / m_tileSize;
	}

	uint32_t GetTileCount() const { return m_columns * m_rows; }
	uint32_t GetColumns() const { return m_columns; }
	uint32_t GetRows() const { return m_rows; }
	uint32_t GetTileSize() const { return m_tileSize; }

	SrTileRect GetRect(uint32_t tileIndex) const
	{
		const uint32_t tx = tileIndex % m_columns;
		const uint32_t ty = tileIndex / m_columns;
		const uint32_t x0 = tx * m_tileSize;
		const uint32_t y0 = ty * m_tileSize;
		return SrTileRect(x0, y0, (std::min)(x0 + m_tileSize, m_width), (std::min)(y0 + m_tileSize, m_height));
	}

	uint32_t GetTileIndex(uint32_t x, uint32_t y) const
	{
		return (y / m_tileSize) * m_columns + x / m_tileSize;
	}

private:
	uint32_t m_width, m_height, m_tileSize, m_columns, m_rows;
};

struct SrTileBins
{
	std::vector<uint32_t> offsets;
	std::vector<uint32_t> triangleRefs;
};

struct SrPreparedTriangleBounds
{
	uint32_t minX, minY, maxX, maxY;
};

#endif
