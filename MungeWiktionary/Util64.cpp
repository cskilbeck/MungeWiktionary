//////////////////////////////////////////////////////////////////////

#include "pch.h"

//////////////////////////////////////////////////////////////////////

void TRACE(char const *strMsg, ...)
{
	char strBuffer[16384];
	va_list args;
	va_start(args, strMsg);
	vsprintf_s(strBuffer, ARRAYSIZE(strBuffer), strMsg, args);
	va_end(args);
	OutputDebugStringA(strBuffer);
}

//////////////////////////////////////////////////////////////////////

void TRACE(wchar const *strMsg, ...)
{
	wchar strBuffer[16384];
	va_list args;
	va_start(args, strMsg);
	_vsnwprintf_s(strBuffer, ARRAYSIZE(strBuffer), strMsg, args);
	va_end(args);
	OutputDebugStringW(strBuffer);
}

//////////////////////////////////////////////////////////////////////

uint8 *LoadFile(char const *filename, size_t *size)
{
	uint8 *buf = null;
	FILE *f = null;

	if(fopen_s(&f, filename, "rb") == 0)
	{
		_fseeki64(f, 0, SEEK_END);
		size_t len = _ftelli64(f);
		_fseeki64(f, 0, SEEK_SET);

		buf = new uint8[len + sizeof(wchar)];

		if(buf != null)
		{
			size_t s = fread_s(buf, len, 1, len, f);

			if(s != len)
			{
				delete [] buf;
				buf = null;
			}
			else
			{
				*((wchar *)(((char *)buf) + len)) = L'\0';
				if(size != null)
				{
					*size = len;
				}
			}
		}

		fclose(f);
	}
	else
	{
		MessageBoxA(null, Format("File not found: %s", filename).c_str(), "LoadFile", MB_ICONERROR);
	}
	return buf;
}

//////////////////////////////////////////////////////////////////////

std::wstring Format(wchar const *fmt, ...)
{
	wchar buffer[512];

	va_list v;
	va_start(v, fmt);
	_vsnwprintf_s(buffer, 512, fmt, v);
	return std::wstring(buffer);
}

//////////////////////////////////////////////////////////////////////

std::string Format(char const *fmt, ...)
{
	char buffer[512];

	va_list v;
	va_start(v, fmt);
	_vsnprintf_s(buffer, 512, fmt, v);
	return std::string(buffer);
}
