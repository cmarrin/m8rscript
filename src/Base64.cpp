/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Base64.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Program.h"

using namespace m8r;

static const uint32_t BASE64_STACK_ALLOC_LIMIT = 32;

static const char RODATA_ATTR _base64enc_tab[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static ROMString base64enc_tab(_base64enc_tab);

static const uint8_t RODATA_ATTR _base64dec_tab[256]= {
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
	 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
	255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
	255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};
static const ROMString base64dec_tab(reinterpret_cast<const char*>(_base64dec_tab));


/* decode a base64 string in one shot */
int Base64::decode(uint16_t in_len, const char *in, uint16_t out_len, unsigned char *out)
{
	unsigned ii, io;
	uint_least32_t v;
	unsigned rem;

	for(io = 0, ii = 0, v = 0, rem = 0; ii < in_len; ii++) {
		unsigned char ch;
		if(isspace(in[ii])) continue;
		if(in[ii]=='=') break; /* stop at = */
		ch = readRomByte(base64dec_tab + static_cast<unsigned>(in[ii]));
		if(ch==255) break; /* stop at a parse error */
		v=(v<<6)|ch;
		rem+=6;
		if(rem>=8) {
			rem-=8;
			if(io>=out_len) return -1; /* truncation is failure */
			out[io++]=(v>>rem)&255;
		}
	}
	if(rem>=8) {
		rem-=8;
		if(io>=out_len) return -1; /* truncation is failure */
		out[io++]=(v>>rem)&255;
	}
	return io;
}

int Base64::encode(uint16_t in_len, const unsigned char *in, uint16_t out_len, char *out)
{
	unsigned ii, io;
	uint_least32_t v;
	unsigned rem;

	for(io=0,ii=0,v=0,rem=0;ii<in_len;ii++) {
		unsigned char ch;
		ch=in[ii];
		v=(v<<8)|ch;
		rem+=8;
		while(rem>=6) {
			rem-=6;
			if(io>=out_len) return -1; /* truncation is failure */
			out[io++] = readRomByte(base64enc_tab + ((v >> rem) & 63));
		}
	}
	if(rem) {
		v<<=(6-rem);
		if(io>=out_len) return -1; /* truncation is failure */
		out[io++]=readRomByte(base64enc_tab + (v & 63));
	}
	while(io&3) {
		if(io>=out_len) return -1; /* truncation is failure */
		out[io++]='=';
	}
	if(io>=out_len) return -1; /* no room for null terminator */
	out[io]=0;
	return io;
}

Base64::Base64(Mad<Program> program, ObjectFactory* parent)
    : ObjectFactory(program, SA::Base64, parent)
{
    addProperty(program, SA::encode, encodeFunc);
    addProperty(program, SA::decode, decodeFunc);
}

CallReturnValue Base64::encodeFunc(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    String inString = eu->stack().top().toStringValue(eu);
    uint16_t inLength = inString.size();
    uint16_t outLength = (inLength * 4 + 2) / 3 + 1;
    if (outLength <= BASE64_STACK_ALLOC_LIMIT) {
        char outString[BASE64_STACK_ALLOC_LIMIT];
        int actualLength = encode(inLength, reinterpret_cast<const uint8_t*>(inString.c_str()), 
                                         BASE64_STACK_ALLOC_LIMIT, outString);
        Mad<String> string = Mad<String>::create(outString, actualLength);
        eu->stack().push(Value(string));
    } else {
        Mad<char> outString = Mad<char>::create(outLength);
        int actualLength = encode(inLength, reinterpret_cast<const uint8_t*>(inString.c_str()),
                                         BASE64_STACK_ALLOC_LIMIT, outString.get());
        Mad<String> string = Mad<String>::create(outString.get(), actualLength);
        eu->stack().push(Value(string));
        outString.destroy(outLength);
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Base64::decodeFunc(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    String inString = eu->stack().top().toStringValue(eu);
    uint16_t inLength = inString.size();
    uint16_t outLength = (inLength * 3 + 3) / 4 + 1;
    if (outLength <= BASE64_STACK_ALLOC_LIMIT) {
        unsigned char outString[BASE64_STACK_ALLOC_LIMIT];
        int actualLength = decode(inLength, inString.c_str(), BASE64_STACK_ALLOC_LIMIT, outString);
        Mad<String> string = Mad<String>::create(reinterpret_cast<char*>(outString), actualLength);
        eu->stack().push(Value(string));
    } else {
        Mad<unsigned char> outString = Mad<uint8_t>::create(outLength);
        int actualLength = decode(inLength, inString.c_str(), BASE64_STACK_ALLOC_LIMIT, outString.get());
        Mad<String> string = Mad<String>::create(reinterpret_cast<char*>(outString.get()), actualLength);
        eu->stack().push(Value(string));
        outString.destroy(outLength);
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
