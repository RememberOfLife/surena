#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "surena/util/base64.h"

#ifdef __cplusplus
extern "C" {
#endif

//TODO should probably switch to: https://github.com/superwills/NibbleAndAHalf

// code adapted for readability from: https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/

const char b64_chars[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

// shift input base64 char by '+' (43) then index into this table to get the 0-63 value represented by that char
const char b64_inv[80] = {62, -1, 63, -1, -1, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

size_t b64_encode_size(size_t size_bytes)
{
    // round up to nearest 3 multiple, div by 3 for 3 byte blocks and times 4 for the characters of each block
	size_t ret = size_bytes;
	if (size_bytes % 3 != 0) {
		ret += 3 - (size_bytes % 3);
    }
	return ((ret / 3) * 4);
}

size_t b64_decode_size(char* data_chars)
{
    if (data_chars == NULL) {
        return 0;
    }
    size_t ret = 0;
    size_t pad = 0;
    char* bp = data_chars;
    while (*bp != '\0') {
        if (*(bp++) == '=') {
            pad++;
        }
        ret++;
    }
    return ((ret / 4) * 3) - pad;
}

size_t b64_encode(char* data_chars, char* data_bytes, size_t len)
{
	if (data_bytes == NULL || len == 0) {
        if (data_chars) {
            *data_chars = '\0';
        }
		return 0;
    }
	size_t bi = 0; // byte triple idx
	size_t ci = 0; // char quartet idx
	while (bi < len) {
        // accumulate 3 bytes of data
		uint32_t acc = (data_bytes[bi] << 16);
        if (bi + 1 < len) {
            acc |= (data_bytes[bi + 1] << 8);
        }
        if (bi + 2 < len) {
            acc |= data_bytes[bi + 2];
        }
        // produces at least 2 chars of data
		data_chars[ci] = b64_chars[(acc >> 18) & 0b111111];
		data_chars[ci+1] = b64_chars[(acc >> 12) & 0b111111];
        // check if data exists or needs padding
        data_chars[ci + 2] = ((bi +1 < len) ? b64_chars[(acc >> 6) & 0b111111] : '=');
        data_chars[ci + 3] = ((bi + 2 < len) ? b64_chars[acc & 0b111111] : '=');
        bi += 3;
        ci += 4;
	}
    data_chars[ci] = '\0';
    return ci + 1;
}

bool b64_is_valid_char(char c)
{
	if (c >= '0' && c <= '9') {
		return 1;
    }
	if (c >= 'A' && c <= 'Z') {
		return 1;
    }
	if (c >= 'a' && c <= 'z') {
		return 1;
    }
	if (c == '+' || c == '-' || c == '=') {
		return 1;
    }
	return 0;
}

size_t b64_decode(char* data_bytes, char* data_chars)
{
	if (data_bytes == NULL || data_chars == NULL) {
		return 0;
    }
    //TODO move strlen and valid char into the decoding loop
    size_t strlen = 0;
    char* cp = data_chars;
    while (*cp != '\0') {
        if (!b64_is_valid_char(*(cp++))) {
            return 0;
        }
        strlen++;
    }
    if (strlen % 4 != 0) {
        return 0;
    }
	size_t ci = 0;
	size_t bi = 0;
	while (ci < strlen) {
        // writeout is interweaved with readin to accomodate fewer checks
        // magic number 43 comes from '+' which is the offset of the b64_inv array
        uint32_t v = (b64_inv[data_chars[ci] - 43] << 18);
		v |= (b64_inv[data_chars[ci + 1] - 43] << 12);
		data_bytes[bi] = ((v >> 16) & 0xFF); // writeout byte1
        if (data_chars[ci + 2] != '=') {
		    v |= (b64_inv[data_chars[ci + 2] - 43] << 6);
			data_bytes[bi + 1] = ((v >> 8) & 0xFF); // writeout byte2 (optional)
        }
        if (data_chars[ci + 3] != '=') {
		    v |= b64_inv[data_chars[ci + 3] - 43];
			data_bytes[bi + 2] = (v & 0xFF); // writeout byte3 (optional)
        }
        ci += 4;
        bi += 3;
	}
	return bi;
}

#ifdef __cplusplus
}
#endif
