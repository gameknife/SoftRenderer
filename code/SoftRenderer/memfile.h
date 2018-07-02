/**
  @file IMemFile.h
  
  @brief �ڴ��ļ���װ�������ļ���ȡ

  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#ifndef IMemFile_h__
#define IMemFile_h__

	struct SrMemFile
	{
		SrMemFile():m_open(false),
			m_data(NULL),
			m_size(0),
			m_binary(false),
			m_unicode(false)
		{

		}
		~SrMemFile()
		{
			Close();
		}


		SrMemFile( const SrMemFile& other ) :m_open(false),
			m_data(NULL),
			m_size(0),
			m_binary(false),
			m_unicode(false),
			m_encoding(0)
		{
			m_size = other.m_size;
			m_open = other.m_open;
			m_binary = other.m_binary;
			m_unicode = other.m_unicode;
			m_encoding = other.m_encoding;


			if (other.m_data)
			{
				m_data = new char[m_size];
				memcpy( m_data, other.m_data, m_size );
			}
		}

		SrMemFile& operator=( const SrMemFile& rhs )
		{
			Close();

			m_size = rhs.m_size;
			m_open = rhs.m_open;
			m_binary = rhs.m_binary;
			m_unicode = rhs.m_unicode;
			m_encoding = rhs.m_encoding;

			if (rhs.m_data)
			{
				m_data = new char[m_size];
				memcpy( m_data, rhs.m_data, m_size );
			}

			return *this;
		}

		void Open( const char* diskPath )
		{
			m_size = 0;
			m_open = false;
			m_unicode = false;

			FILE* pFile = NULL;
			pFile = fopen( diskPath, "rb");

			if (pFile)
			{
				// Get the file size
				fseek(pFile, 0, SEEK_END);
				m_size = ftell(pFile);
				fseek(pFile, 0, SEEK_SET);

				// read the data
				if ( m_size == -1)
				{
					fclose(pFile);
					return;
				}

				if ( m_size > 1024 * 1024 * 1024)
				{
					fclose(pFile);
					return;
				}

				char* pTmp = new char[m_size];
				if (!pTmp)
				{
					m_size = 0;
					fclose(pFile);
					return;
				}
				size_t BytesRead = fread(pTmp, 1, m_size, pFile);

				if (BytesRead != m_size)
				{
					delete [] pTmp;
					m_size = 0;
				}
				else
				{
					m_data = pTmp;
					m_open = true;
				}

				fclose(pFile);				
			}
			else
			{

			}
		}


		virtual void Close()
		{
			if(m_data)
			{
				delete[] m_data;
				m_data = NULL;
			}
		}

		virtual bool IsOpen() const
		{
			return m_open;
		}

		virtual int Size() const
		{
			if(m_unicode)
			{
				return m_size - 2;
			}
			return m_size;
		}

		virtual const char* Data() const
		{
			if(m_unicode)
			{
				return m_data + 2;
			}
			return m_data;
		}

		char* m_data;
		int m_size;
		bool m_open;
		bool m_binary;
		bool m_unicode;
		int m_encoding;
	};

#endif  //_IMEMFILE_H
