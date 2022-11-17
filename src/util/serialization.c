#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "surena/util/raw_stream.h"
#include "surena/util/serialization.h"

#ifdef __cplusplus
extern "C" {
#endif

/////
// utils for event serialization

void blob_create(blob* b, size_t len)
{
    b->len = len;
    b->data = len > 0 ? malloc(len) : NULL;
}

void blob_resize(blob* b, size_t len, bool preserve_data)
{
    if (b->len == len) {
        return;
    }
    blob new_blob;
    blob_create(&new_blob, len);
    if (preserve_data == true && b->len > 0 && len > 0) {
        memcpy(new_blob.data, b->data, len);
    }
    blob_destroy(b);
    *b = new_blob;
}

void blob_destroy(blob* b)
{
    if (b->data) {
        free(b->data);
    }
    b->len = 0;
    b->data = NULL;
}

/////
// general purpose serialization backend

//TODO can inline these, but instantiate https://stackoverflow.com/a/16245669

void* ptradd(void* p, size_t v)
{
    return (char*)p + v;
}

void* ptrsub(void* p, size_t v)
{
    return (char*)p - v;
}

size_t ptrdiff(const void* p_end, const void* p_start)
{
    return (const char*)p_end - (const char*)p_start;
}

// primitive serialization functions

size_t ls_primitive_bool_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    bool* cin_p = (bool*)obj_in;
    bool* cout_p = (bool*)obj_out;
    switch (itype) {
        case GSIT_NONE: {
            assert(0);
        } break;
        case GSIT_INITZERO: {
            // pass
        } break;
        case GSIT_SIZE: {
            return 1;
        } break;
        case GSIT_SERIALIZE: {
            raw_stream rs = rs_init(buf);
            rs_w_bool(&rs, *cin_p);
            return 1;
        } break;
        case GSIT_DESERIALIZE: {
            if (ptrdiff(buf_end, buf) < 1) {
                return LS_ERR;
            }
            raw_stream rs = rs_init(buf);
            *cout_p = rs_r_bool(&rs);
            return 1;
        } break;
        case GSIT_COPY: {
            *cout_p = *cin_p;
        } break;
        case GSIT_DESTROY: {
            // pass
        } break;
        case GSIT_COUNT:
        case GSIT_SIZE_MAX: {
            assert(0);
        } break;
    }
    return 0;
}

size_t ls_primitive_u8_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    uint8_t* cin_p = (uint8_t*)obj_in;
    uint8_t* cout_p = (uint8_t*)obj_out;
    switch (itype) {
        case GSIT_NONE: {
            assert(0);
        } break;
        case GSIT_INITZERO: {
            // pass
        } break;
        case GSIT_SIZE: {
            return 1;
        } break;
        case GSIT_SERIALIZE: {
            raw_stream rs = rs_init(buf);
            rs_w_uint8(&rs, *cin_p);
            return 1;
        } break;
        case GSIT_DESERIALIZE: {
            if (ptrdiff(buf_end, buf) < 1) {
                return LS_ERR;
            }
            raw_stream rs = rs_init(buf);
            *cout_p = rs_r_uint8(&rs);
            return 1;
        } break;
        case GSIT_COPY: {
            *cout_p = *cin_p;
        } break;
        case GSIT_DESTROY: {
            // pass
        } break;
        case GSIT_COUNT:
        case GSIT_SIZE_MAX: {
            assert(0);
        } break;
    }
    return 0;
}

size_t ls_primitive_u32_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    uint32_t* cin_p = (uint32_t*)obj_in;
    uint32_t* cout_p = (uint32_t*)obj_out;
    switch (itype) {
        case GSIT_NONE: {
            assert(0);
        } break;
        case GSIT_INITZERO: {
            // pass
        } break;
        case GSIT_SIZE: {
            return 4;
        } break;
        case GSIT_SERIALIZE: {
            raw_stream rs = rs_init(buf);
            rs_w_uint32(&rs, *cin_p);
            return 4;
        } break;
        case GSIT_DESERIALIZE: {
            if (ptrdiff(buf_end, buf) < 4) {
                return LS_ERR;
            }
            raw_stream rs = rs_init(buf);
            *cout_p = rs_r_uint32(&rs);
            return 4;
        } break;
        case GSIT_COPY: {
            *cout_p = *cin_p;
        } break;
        case GSIT_DESTROY: {
            // pass
        } break;
        case GSIT_COUNT:
        case GSIT_SIZE_MAX: {
            assert(0);
        } break;
    }
    return 0;
}

size_t ls_primitive_u64_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    uint64_t* cin_p = (uint64_t*)obj_in;
    uint64_t* cout_p = (uint64_t*)obj_out;
    switch (itype) {
        case GSIT_NONE: {
            assert(0);
        } break;
        case GSIT_INITZERO: {
            // pass
        } break;
        case GSIT_SIZE: {
            return 8;
        } break;
        case GSIT_SERIALIZE: {
            raw_stream rs = rs_init(buf);
            rs_w_uint64(&rs, *cin_p);
            return 8;
        } break;
        case GSIT_DESERIALIZE: {
            if (ptrdiff(buf_end, buf) < 8) {
                return LS_ERR;
            }
            raw_stream rs = rs_init(buf);
            *cout_p = rs_r_uint64(&rs);
            return 8;
        } break;
        case GSIT_COPY: {
            *cout_p = *cin_p;
        } break;
        case GSIT_DESTROY: {
            // pass
        } break;
        case GSIT_COUNT:
        case GSIT_SIZE_MAX: {
            assert(0);
        } break;
    }
    return 0;
}

size_t ls_primitive_size_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    size_t* cin_p = (size_t*)obj_in;
    size_t* cout_p = (size_t*)obj_out;
    switch (itype) {
        case GSIT_NONE: {
            assert(0);
        } break;
        case GSIT_INITZERO: {
            // pass
        } break;
        case GSIT_SIZE: {
            return 8;
        } break;
        case GSIT_SERIALIZE: {
            raw_stream rs = rs_init(buf);
            rs_w_size(&rs, *cin_p);
            return 8;
        } break;
        case GSIT_DESERIALIZE: {
            if (ptrdiff(buf_end, buf) < 8) {
                return LS_ERR;
            }
            raw_stream rs = rs_init(buf);
            *cout_p = rs_r_size(&rs);
            return 8;
        } break;
        case GSIT_COPY: {
            *cout_p = *cin_p;
        } break;
        case GSIT_DESTROY: {
            // pass
        } break;
        case GSIT_COUNT:
        case GSIT_SIZE_MAX: {
            assert(0);
        } break;
    }
    return 0;
}

size_t ls_primitive_string_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    char** cin_p = (char**)obj_in;
    char** cout_p = (char**)obj_out;
    switch (itype) {
        case GSIT_NONE: {
            assert(0);
        } break;
        case GSIT_INITZERO: {
            *cin_p = NULL;
        } break;
        case GSIT_SIZE: {
            size_t string_size = (*cin_p == NULL ? 2 : strlen(*cin_p) + 1);
            return (string_size < 2 ? 2 : string_size);
        } break;
        case GSIT_SERIALIZE: {
            size_t string_size = (*cin_p == NULL ? 0 : strlen(*cin_p) + 1);
            // str_len is 1 if NULL or empty"", otherwise strlen+1
            if (string_size <= 1) {
                *(char*)buf = '\0';
                if (string_size == 0) {
                    *((uint8_t*)buf + 1) = 0x00; // NULL becomes 0x0000
                } else {
                    *((uint8_t*)buf + 1) = 0xFF; // empty"" becomes 0x00FF
                }
                string_size = 2;
            } else {
                memcpy(buf, *cin_p, string_size);
            }
            return string_size;
        } break;
        case GSIT_DESERIALIZE: {
            if (ptrdiff(buf_end, buf) < 1) {
                return LS_ERR;
            }
            size_t max_string_size = ptrdiff(buf_end, buf);
            const void* found = memchr(buf, '\0', max_string_size);
            if (!found || max_string_size < 2) {
                return LS_ERR;
            }
            size_t string_size = ptrdiff(found, buf) + 1;
            if (ptrdiff(buf_end, buf) < string_size) {
                return LS_ERR;
            }
            if (string_size == 1 && *(uint8_t*)ptradd(buf, 1) == 0x00) {
                *cout_p = NULL;
                string_size = 2;
            } else {
                *cout_p = (char*)malloc(string_size);
                memcpy(*cout_p, buf, string_size);
                if (string_size == 1) {
                    string_size += 1;
                }
            }
            return string_size;
        } break;
        case GSIT_COPY: {
            *cout_p = (*cin_p != NULL ? strdup(*cin_p) : NULL);
        } break;
        case GSIT_DESTROY: {
            if (*cin_p != NULL) {
                free(*cin_p);
            }
        } break;
        case GSIT_COUNT:
        case GSIT_SIZE_MAX: {
            assert(0);
        } break;
    }
    return 0;
}

size_t ls_primitive_blob_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    blob* cin_p = (blob*)obj_in;
    blob* cout_p = (blob*)obj_out;
    switch (itype) {
        case GSIT_NONE: {
            assert(0);
        } break;
        case GSIT_INITZERO: {
            *cin_p = (blob){
                .len = 0,
                .data = NULL,
            };
        } break;
        case GSIT_SIZE: {
            return 8 + cin_p->len;
        } break;
        case GSIT_SERIALIZE: {
            raw_stream rs = rs_init(buf);
            rs_w_size(&rs, cin_p->len);
            if (cin_p->len > 0) {
                memcpy(rs.end, cin_p->data, cin_p->len);
            }
            return 8 + cin_p->len;
        } break;
        case GSIT_DESERIALIZE: {
            if (ptrdiff(buf_end, buf) < 8) {
                return LS_ERR;
            }
            raw_stream rs = rs_init(buf);
            size_t csize = rs_r_size(&rs);
            blob_create(cout_p, csize);
            if (csize > 0) {
                if (ptrdiff(buf_end, ptradd(buf, 8)) < csize) {
                    return LS_ERR;
                }
                memcpy(cout_p->data, ptradd(buf, 8), csize);
            }
            return csize + 8;
        } break;
        case GSIT_COPY: {
            blob_create(cout_p, cin_p->len);
            memcpy(cout_p->data, cin_p->data, cin_p->len);
        } break;
        case GSIT_DESTROY: {
            blob_destroy(cin_p);
        } break;
        case GSIT_COUNT:
        case GSIT_SIZE_MAX: {
            assert(0);
        } break;
    }
    return 0;
}

custom_serializer_t* ls_primitive_serializers[] = {
    [SL_TYPE_BOOL] = ls_primitive_bool_serializer,
    [SL_TYPE_U8] = ls_primitive_u8_serializer,
    [SL_TYPE_U32] = ls_primitive_u32_serializer,
    [SL_TYPE_U64] = ls_primitive_u64_serializer,
    [SL_TYPE_SIZE] = ls_primitive_size_serializer,
    [SL_TYPE_STRING] = ls_primitive_string_serializer,
    [SL_TYPE_BLOB] = ls_primitive_blob_serializer,
};

//TODO place (pseudo-)primitive types in separate functions
//TODO replace surena/rawstream uses by something cheaper
// recursive object serializer
size_t layout_serializer_impl(GSIT itype, const serialization_layout* layout, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    assert(layout != NULL);
    const bool in_ex = (itype != GSIT_DESERIALIZE);
    assert(!(in_ex == true && obj_in == NULL));
    const bool out_ex = (itype == GSIT_DESERIALIZE || itype == GSIT_COPY);
    assert(!(out_ex == true && obj_out == NULL));
    const bool use_buf = (itype == GSIT_SERIALIZE || itype == GSIT_DESERIALIZE);
    assert(!(use_buf == true && buf == NULL));
    assert(!(itype == GSIT_DESERIALIZE && buf_end == NULL));
    size_t rsize = 0;
    const serialization_layout* pl = layout;
    void* cbuf = buf;
    void* ebuf = buf_end;
    while ((pl->type & SL_TYPE_TYPEMASK) != SL_TYPE_STOP) {
        const bool is_ptr = (pl->type & SL_TYPE_PTR);
        const bool is_array = (pl->type & SL_TYPE_ARRAY);
        size_t typesize = pl->typesize;
        switch (pl->type & SL_TYPE_TYPEMASK) {
            case SL_TYPE_BOOL: {
                typesize = sizeof(bool);
            } break;
            case SL_TYPE_U32: {
                typesize = sizeof(uint32_t);
            } break;
            case SL_TYPE_U64: {
                typesize = sizeof(uint64_t);
            } break;
            case SL_TYPE_SIZE: {
                typesize = sizeof(size_t);
            } break;
            case SL_TYPE_STRING: {
                typesize = sizeof(char*);
            } break;
            case SL_TYPE_BLOB: {
                typesize = sizeof(blob);
            } break;
        }
        assert(!((is_ptr == true || is_array == true) && typesize == 0)); // detect missing typesize if required
        assert(!(((pl->type & SL_TYPE_TYPEMASK) == SL_TYPE_COMPLEX && pl->ext.layout == NULL))); // detect missing layout info if required
        assert(!(((pl->type & SL_TYPE_TYPEMASK) == SL_TYPE_CUSTOM && pl->ext.serializer == NULL))); // detect missing serializer function if required
        assert(!(is_ptr == false && is_array == true && pl->len.immediate == 0)); // detect missing immediate array length
        void* in_p = NULL;
        void* out_p = NULL;
        if (in_ex == true) {
            in_p = ptradd(obj_in, pl->data_offset);
        }
        if (out_ex == true) {
            out_p = ptradd(obj_out, pl->data_offset);
        }
        size_t arr_len = 1;
        if (is_array == true) {
            if (is_ptr == true) {
                if (itype == GSIT_DESERIALIZE) {
                    if (ptrdiff(ebuf, cbuf) < 8) {
                        return LS_ERR;
                    }
                    raw_stream rs = rs_init(cbuf);
                    arr_len = rs_r_size(&rs);
                    cbuf = ptradd(cbuf, 8);
                } else {
                    arr_len = *(size_t*)ptradd(obj_in, pl->len.offset);
                    if (use_buf == true) {
                        raw_stream rs = rs_init(cbuf);
                        rs_w_size(&rs, arr_len);
                        cbuf = ptradd(cbuf, 8);
                    }
                }
                if (out_ex) {
                    *(size_t*)ptradd(obj_out, pl->len.offset) = arr_len;
                }
                rsize += 8;
            } else {
                arr_len = pl->len.immediate;
            }
        } else if (is_ptr == true) {
            if (itype == GSIT_DESERIALIZE) {
                if (ptrdiff(ebuf, cbuf) < 1) {
                    return LS_ERR;
                }
                raw_stream rs = rs_init(cbuf);
                arr_len = (rs_r_uint8(&rs) == 0xFF ? 1 : 0);
                cbuf = ptradd(cbuf, 1);
            } else {
                arr_len = ((*(void**)in_p) == NULL ? 0 : 1);
                if (use_buf == true) {
                    raw_stream rs = rs_init(cbuf);
                    rs_w_uint8(&rs, arr_len == 0 ? 0x00 : 0xFF);
                    cbuf = ptradd(cbuf, 1);
                }
            }
            rsize += 1;
        }
        if (is_ptr == true) {
            if (in_ex == true && arr_len > 0) {
                in_p = *(void**)in_p;
            }
            if (out_ex == true) {
                *(void**)out_p = (arr_len > 0 ? malloc(typesize * arr_len) : NULL);
                out_p = *(void**)out_p;
            }
        }
        for (size_t idx = 0; idx < arr_len; idx++) {
            SL_TYPE serialize_type = pl->type & SL_TYPE_TYPEMASK;
            switch (serialize_type) {
                default: {
                    assert(0); // cannot serialize unknown type
                } break;
                case SL_TYPE_BOOL:
                case SL_TYPE_U32:
                case SL_TYPE_U64:
                case SL_TYPE_SIZE:
                case SL_TYPE_STRING:
                case SL_TYPE_BLOB: {
                    //TODO
                    // use primitive serializer func
                    size_t csize = ls_primitive_serializers[serialize_type](itype, in_p, out_p, cbuf, ebuf);
                    if (csize == LS_ERR) {
                        return LS_ERR;
                    }
                    rsize += csize;
                    if (use_buf == true) {
                        cbuf = ptradd(cbuf, csize);
                    }
                } break;
                case SL_TYPE_COMPLEX: {
                    size_t csize = layout_serializer_impl(itype, pl->ext.layout, in_p, out_p, cbuf, ebuf);
                    if (csize == LS_ERR) {
                        return LS_ERR;
                    }
                    rsize += csize;
                    if (use_buf == true) {
                        cbuf = ptradd(cbuf, csize);
                    }
                } break;
                case SL_TYPE_CUSTOM: {
                    size_t csize = pl->ext.serializer(itype, in_p, out_p, cbuf, ebuf);
                    if (csize == LS_ERR) {
                        return LS_ERR;
                    }
                    rsize += csize;
                    if (use_buf == true) {
                        cbuf = ptradd(cbuf, csize);
                    }
                } break;
            }
            if (in_ex == true) {
                in_p = ptradd(in_p, typesize);
            }
            if (out_ex == true) {
                out_p = ptradd(out_p, typesize);
            }
        }
        if (itype == GSIT_DESTROY && is_ptr == true) {
            free(*(void**)ptradd(obj_in, pl->data_offset));
        }
        pl++;
    }
    return rsize;
}

size_t layout_serializer(GSIT itype, const serialization_layout* layout, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    // deserializing, first zero init the obj, and on copy/deserialization error, destroy it, so we don't leak memory
    if (itype == GSIT_COPY || itype == GSIT_DESERIALIZE) {
        layout_serializer_impl(GSIT_INITZERO, layout, obj_out, NULL, buf, buf_end);
    }
    size_t ret = layout_serializer_impl(itype, layout, obj_in, obj_out, buf, buf_end);
    if ((itype == GSIT_COPY || itype == GSIT_DESERIALIZE) && ret == LS_ERR) {
        layout_serializer_impl(GSIT_DESTROY, layout, obj_out, NULL, buf, buf_end);
    }
    return ret;
}

#ifdef __cplusplus
}
#endif
