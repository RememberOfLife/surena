#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "surena/util/base64.h"

#ifdef __cplusplus
extern "C" {
#endif

// https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/
// https://github.com/gaspardpetit/base64/

const char b64_charset_default[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

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
    //TODO put the strlen into the padding detection loop
	size_t len = strlen(data_chars);
	size_t ret = (len / 4) * 3;
	for (size_t i = len - 1; i > 0; i--) {
		if (data_chars[i] == '=') {
			ret--;
		} else {
			break;
		}
	}
	return ret;
}

size_t b64_encode(char* data_chars, char* data_bytes)
{
    //TODO
    return 0;
}

size_t b64_decode(char* data_bytes, char* data_chars)
{
    //TODO
    return 0;
}

#ifdef __cplusplus
}
#endif
