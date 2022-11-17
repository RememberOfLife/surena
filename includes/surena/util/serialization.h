#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/////
// utils for serialization

void* ptradd(void* p, size_t v);
void* ptrsub(void* p, size_t v);
size_t ptrdiff(const void* p_end, const void* p_start);

typedef struct blob_s {
    size_t len;
    void* data;
} blob;

void blob_create(blob* b, size_t len);

void blob_resize(blob* b, size_t len, bool preserve_data);

void blob_destroy(blob* b);

/////
// event serialization entries

//NOTE:
// complex types like uint32_t** ptr_to_ptrarray; require a separate layout for the uint32_t* serialization of the uint32_t* elems in the ptrarray

typedef enum __attribute__((__packed__)) SL_TYPE_E {
    SL_TYPE_NULL = 0,

    SL_TYPE_BOOL,
    SL_TYPE_U8,
    SL_TYPE_U32,
    SL_TYPE_U64,
    SL_TYPE_SIZE,
    //TODO more primitives

    // pseudo primitives
    SL_TYPE_STRING, // char* member;
    SL_TYPE_BLOB, // blob member;

    // extension types, must supply typesize
    SL_TYPE_COMPLEX, // // use ext.layout to specify the type
    SL_TYPE_CUSTOM, // use ext.serializer to supply the serializer to use

    SL_TYPE_STOP, // signals the end of serialization items in this sl

    SL_TYPE_COUNT,

    SL_TYPE_TYPEMASK = 0b00111111,
    // these are flags applicable to all serializable types
    SL_TYPE_PTR = 0b01000000,
    SL_TYPE_ARRAY = 0b10000000, // if ptr: use len.offset else: len.immediate, to get size of the array, len.offset always locates a size_t
    SL_TYPE_PTRARRAY = SL_TYPE_PTR | SL_TYPE_ARRAY, // convenience type for sl definitions

    SL_TYPE_SIZE_MAX = UINT8_MAX,
} SL_TYPE;

// general serializer invocation type
typedef enum __attribute__((__packed__)) GSIT_E {
    GSIT_NONE,
    GSIT_INITZERO, // supply obj_in as object to initalize to zero (similar to destroy)
    GSIT_SIZE, // size from obj_in
    GSIT_SERIALIZE, // serialize obj_in
    GSIT_DESERIALIZE, // deserialize to obj_out
    GSIT_COPY, // copy from obj_in to obj_out
    GSIT_DESTROY, // destroy obj_in
    GSIT_COUNT,
    GSIT_SIZE_MAX = UINT8_MAX,
} GSIT;

typedef struct serialization_layout_s serialization_layout;

// must provide guarantee that any zero initialized object will be destructible from any partial deserialized state *as left by this function*
typedef size_t custom_serializer_t(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end);

struct serialization_layout_s {
    SL_TYPE type;
    size_t data_offset;

    union {
        const serialization_layout* layout;
        // size_t serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
        custom_serializer_t* serializer;
    } ext;

    size_t typesize; // req'd only if PTR || ARRAY

    union {
        size_t immediate;
        size_t offset;
    } len; // req'd only if PTR || ARRAY
};

static const size_t LS_ERR = SIZE_MAX;

// returned size_t is only valid if itype SIZE/SERIALIZE/DESERIALIZE
// if return value is LS_ERR then an error occured, can not happen during init or destroy
// deserialization errors are automatically cleaned up and do not leak memory (assuming all used custom functions do this as well)
size_t layout_serializer(GSIT itype, const serialization_layout* layout, void* obj_in, void* obj_out, void* buf, void* buf_end);

//TODO rather give lookup array?
// primitive serializers
custom_serializer_t ls_primitive_bool_serializer;
custom_serializer_t ls_primitive_u8_serializer;
custom_serializer_t ls_primitive_u32_serializer;
custom_serializer_t ls_primitive_u64_serializer;
custom_serializer_t ls_primitive_size_serializer;
custom_serializer_t ls_primitive_string_serializer;
custom_serializer_t ls_primitive_blob_serializer;

#ifdef __cplusplus
}
#endif
