//////////////////////////////////////////////////////////////////////

#include "pch.h"

//////////////////////////////////////////////////////////////////////

const uint8 utf8d[] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  070,070,070,070,070,070,070,070,070,070,070,070,070,070,070,070,
  050,050,050,050,050,050,050,050,050,050,050,050,050,050,050,050,
  030,030,030,030,030,030,030,030,030,030,030,030,030,030,030,030,
  030,030,030,030,030,030,030,030,030,030,030,030,030,030,030,030,
  204,204,188,188,188,188,188,188,188,188,188,188,188,188,188,188,
  188,188,188,188,188,188,188,188,188,188,188,188,188,188,188,188,
  174,158,158,158,158,158,158,158,158,158,158,158,158,142,126,126,
  111, 95, 95, 95, 79,207,207,207,207,207,207,207,207,207,207,207,
  0,1,1,1,8,7,6,4,5,4,3,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,4,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,8,7,6,4,5,4,3,2,1,1,1,1
};

//////////////////////////////////////////////////////////////////////

bool UTF8ToWide(uint8 *input, size_t inputSize, size_t *decodedLength, wchar *decodeBuffer)
{
	if(decodedLength == null || input == null)
	{
		return false;
	}

	uint8 b;
	uint8 stat = 9;
	uint32 unic = 0;
	size_t length = 0;

	for(; inputSize > 0 && (b = *input++) != 0; --inputSize)
	{
		uint8 data = utf8d[b];
		stat = utf8d[256 + (stat << 4) + (data >> 4)];
		b = (b ^ (uint8)(data << 4));
		unic = (unic << 6) | b;

		if (stat == 0)
		{
			++length;
			if(decodeBuffer != null)
			{
				*decodeBuffer++ = unic;
			}
			unic = 0;
		}

		if (stat == 1)
		{
			return false;
		}
	}
	*decodedLength = length;
	return true;
}

//////////////////////////////////////////////////////////////////////

void AsciiToWide(char const *input, wchar *output)
{
	char b;
	while((b = *input++) != 0)
	{
		*output++ = (wchar)b;
	}
	*output++ = (wchar)0;
}

//////////////////////////////////////////////////////////////////////
