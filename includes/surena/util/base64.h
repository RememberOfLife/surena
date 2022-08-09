#pragma once

// simple base64 alike implementation

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// returns the size (including null character) of the char buffer required to encode this many bytes
size_t b64_encode_size(size_t size_bytes);

// returns the size (byte count) produced by decoding this char buffer
size_t b64_decode_size(char* data_chars);

// encode data_bytes as base64 into data_chars and returns the number of bytes written
size_t b64_encode(char* data_chars, char* data_bytes);

// decode data_chars base64 into data_bytes and returns the number of bytes decoded
size_t b64_decode(char* data_bytes, char* data_chars);

extern const char b64_charset[64];

#ifdef __cplusplus
}
#endif
