#include "stdafx.h"
#include "SrTexture.h"

#include <fstream>

void SrTexture::WriteToFile(const char* filename)
{
	std::ofstream file(filename, std::ios::out);

	bmpfile_magic magic;
	magic.magic[0] = 'B';
	magic.magic[1] = 'M';
	file.write((char*)(&magic), sizeof(magic));

	bmpfile_header header;
	memset( &header, 0, sizeof(header) );
	header.bmp_offset = sizeof(bmpfile_magic)
		+ sizeof(bmpfile_header) + sizeof(bmpfile_dib_info);
	header.file_size = header.bmp_offset
		+ m_width * m_height * m_bpp;
	file.write((char*)(&header), sizeof(header));

	bmpfile_dib_info dib_info;
	memset( &dib_info, 0, sizeof(dib_info) );
	dib_info.header_size = sizeof(bmpfile_dib_info);
	dib_info.width = m_width;
	dib_info.height = m_height;
	dib_info.num_planes = 1;
	dib_info.bits_per_pixel = m_bpp * 8;
	dib_info.compression = 0;
	dib_info.bmp_byte_size = 0;
	dib_info.hres = 2834;
	dib_info.vres = 2834;
	dib_info.num_colors = 0;
	dib_info.num_important_colors = 0;
	file.write((char*)(&dib_info), sizeof(dib_info));

	// write data
	//fout << m_rawData;
	for (int i = m_height - 1; i >= 0; --i)
	{
		for (int j = 0; j < m_width; ++j)
		{
			file.write((char*)m_data + m_width * i * m_bpp + j * m_bpp, m_bpp);
		}

		unsigned int padding = (4 - ((m_bpp * m_width) % 4)) % 4;
		char padding_data[4] = { 0x00, 0x00, 0x00, 0x00 };
		file.write(padding_data, padding);
	}

	file.close();
}
