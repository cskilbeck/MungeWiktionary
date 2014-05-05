//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

void AsciiToWide(char const *input, wchar *output);
bool UTF8ToWide(uint8 *input, size_t inputSize, size_t *decodedLength, wchar *decodeBuffer);

//////////////////////////////////////////////////////////////////////

extern const uint8 utf8d[];

struct UTF8Decoder
{
	uint32	unic;
	uint8 *	inputBuffer;
	size_t	inputSize;
	uint8	stat;

	UTF8Decoder(uint8 *input, size_t inputBytesLength)
	{
		stat = 9;
		unic = 0;
		inputBuffer = input;
		inputSize = inputBytesLength;
	}

	// returns: 0 = end of stream, 0xffffffff = decode error, else decoded unicode character
	// calling Next() after it has returned 0 or -1 is undefined (crash)
	uint32 Next()
	{
		if(inputSize <= 0)
		{
			return 0;
		}
		uint8 b;
		while(true)
		{
			if(inputSize == 0)
			{
				return 0;
			}

			--inputSize;
			b = *inputBuffer++;

			uint8 data = utf8d[b];
			stat = utf8d[256 + (stat << 4) + (data >> 4)];
			b = (b ^ (uint8)(data << 4));
			unic = (unic << 6) | b;
			if (stat == 0)
			{
				uint32 temp = unic;
				unic = 0;
				return temp;
			}
			if(stat == 1)
			{
				return 0xffffffff;
			}
		}
	}
};


