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

//TODO on deserialization error, need to destroy the partial object backwards
//TODO place (pseudo-)primitive types in separate functions
//TODO replace surena/rawstream uses by something cheaper
// recursive object serializer
size_t layout_serializer(GSIT itype, const serialization_layout* layout, void* obj_in, void* obj_out, void* buf, void* buf_end)
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
        assert(!((is_ptr == true || is_array == true) && pl->typesize == 0)); // detect missing typesize if required
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
                *(void**)out_p = (arr_len > 0 ? malloc(pl->typesize * arr_len) : NULL);
                out_p = *(void**)out_p;
            }
        }
        for (size_t idx = 0; idx < arr_len; idx++) {
            switch (pl->type & SL_TYPE_TYPEMASK) {
                default: {
                    assert(0);
                } break;
                case SL_TYPE_BOOL: {
                    bool* cin_p = (bool*)in_p;
                    bool* cout_p = (bool*)out_p;
                    switch (itype) {
                        default: {
                            assert(0);
                        } break;
                        case GSIT_SIZE: {
                            rsize += 1;
                        } break;
                        case GSIT_SERIALIZE: {
                            raw_stream rs = rs_init(cbuf);
                            rs_w_bool(&rs, *cin_p);
                            rsize += 1;
                            cbuf = ptradd(cbuf, 1);
                        } break;
                        case GSIT_DESERIALIZE: {
                            if (ptrdiff(ebuf, cbuf) < 1) {
                                return LS_ERR;
                            }
                            raw_stream rs = rs_init(cbuf);
                            *cout_p = rs_r_bool(&rs);
                            rsize += 1;
                            cbuf = ptradd(cbuf, 1);
                        } break;
                        case GSIT_COPY: {
                            *cout_p = *cin_p;
                        } break;
                        case GSIT_DESTROY:
                            break;
                    }
                } break;
                case SL_TYPE_U32: {
                    uint32_t* cin_p = (uint32_t*)in_p;
                    uint32_t* cout_p = (uint32_t*)out_p;
                    switch (itype) {
                        default: {
                            assert(0);
                        } break;
                        case GSIT_SIZE: {
                            rsize += 4;
                        } break;
                        case GSIT_SERIALIZE: {
                            raw_stream rs = rs_init(cbuf);
                            rs_w_uint32(&rs, *cin_p);
                            rsize += 4;
                            cbuf = ptradd(cbuf, 4);
                        } break;
                        case GSIT_DESERIALIZE: {
                            if (ptrdiff(ebuf, cbuf) < 4) {
                                return LS_ERR;
                            }
                            raw_stream rs = rs_init(cbuf);
                            *cout_p = rs_r_uint32(&rs);
                            rsize += 4;
                            cbuf = ptradd(cbuf, 4);
                        } break;
                        case GSIT_COPY: {
                            *cout_p = *cin_p;
                        } break;
                        case GSIT_DESTROY:
                            break;
                    }
                } break;
                case SL_TYPE_U64: {
                    uint64_t* cin_p = (uint64_t*)in_p;
                    uint64_t* cout_p = (uint64_t*)out_p;
                    switch (itype) {
                        default: {
                            assert(0);
                        } break;
                        case GSIT_SIZE: {
                            rsize += 8;
                        } break;
                        case GSIT_SERIALIZE: {
                            raw_stream rs = rs_init(cbuf);
                            rs_w_uint64(&rs, *cin_p);
                            rsize += 8;
                            cbuf = ptradd(cbuf, 8);
                        } break;
                        case GSIT_DESERIALIZE: {
                            if (ptrdiff(ebuf, cbuf) < 8) {
                                return LS_ERR;
                            }
                            raw_stream rs = rs_init(cbuf);
                            *cout_p = rs_r_uint64(&rs);
                            rsize += 8;
                            cbuf = ptradd(cbuf, 8);
                        } break;
                        case GSIT_COPY: {
                            *cout_p = *cin_p;
                        } break;
                        case GSIT_DESTROY:
                            break;
                    }
                } break;
                case SL_TYPE_SIZE: {
                    size_t* cin_p = (size_t*)in_p;
                    size_t* cout_p = (size_t*)out_p;
                    switch (itype) {
                        default: {
                            assert(0);
                        } break;
                        case GSIT_SIZE: {
                            rsize += 8;
                        } break;
                        case GSIT_SERIALIZE: {
                            raw_stream rs = rs_init(cbuf);
                            rs_w_size(&rs, *cin_p);
                            rsize += 8;
                            cbuf = ptradd(cbuf, 8);
                        } break;
                        case GSIT_DESERIALIZE: {
                            if (ptrdiff(ebuf, cbuf) < 8) {
                                return LS_ERR;
                            }
                            raw_stream rs = rs_init(cbuf);
                            *cout_p = rs_r_size(&rs);
                            rsize += 8;
                            cbuf = ptradd(cbuf, 8);
                        } break;
                        case GSIT_COPY: {
                            *cout_p = *cin_p;
                        } break;
                        case GSIT_DESTROY:
                            break;
                    }
                } break;
                case SL_TYPE_STRING: {
                    char** cin_p = (char**)in_p;
                    char** cout_p = (char**)out_p;
                    switch (itype) {
                        default: {
                            assert(0);
                        } break;
                        case GSIT_SIZE: {
                            size_t string_size = (*cin_p == NULL ? 2 : strlen(*cin_p) + 1);
                            rsize += (string_size < 2 ? 2 : string_size);
                        } break;
                        case GSIT_SERIALIZE: {
                            size_t string_size = (*cin_p == NULL ? 0 : strlen(*cin_p) + 1);
                            // str_len is 1 if NULL or empty"", otherwise strlen+1
                            if (string_size <= 1) {
                                *(char*)cbuf = '\0';
                                if (string_size == 0) {
                                    *((uint8_t*)cbuf + 1) = 0x00; // NULL becomes 0x0000
                                } else {
                                    *((uint8_t*)cbuf + 1) = 0xFF; // empty"" becomes 0x00FF
                                }
                                string_size = 2;
                            } else {
                                memcpy(cbuf, *cin_p, string_size);
                            }
                            rsize += string_size;
                            cbuf = ptradd(cbuf, string_size);
                        } break;
                        case GSIT_DESERIALIZE: {
                            if (ptrdiff(ebuf, cbuf) < 1) {
                                return LS_ERR;
                            }
                            size_t max_string_size = ptrdiff(ebuf, cbuf);
                            const void* found = memchr(cbuf, '\0', max_string_size);
                            if (!found || max_string_size < 2) {
                                return LS_ERR;
                            }
                            size_t string_size = ptrdiff(found, cbuf) + 1;
                            if (ptrdiff(ebuf, cbuf) < string_size) {
                                return LS_ERR;
                            }
                            if (string_size == 1 && *(uint8_t*)ptradd(cbuf, 1) == 0x00) {
                                *cout_p = NULL;
                                string_size = 2;
                            } else {
                                *cout_p = (char*)malloc(string_size);
                                memcpy(*cout_p, cbuf, string_size);
                                if (string_size == 1) {
                                    string_size += 1;
                                }
                            }
                            rsize += string_size;
                            cbuf = ptradd(cbuf, string_size);
                        } break;
                        case GSIT_COPY: {
                            *cout_p = (*cin_p != NULL ? strdup(*cin_p) : NULL);
                        } break;
                        case GSIT_DESTROY: {
                            if (*cin_p != NULL) {
                                free(*cin_p);
                            }
                        } break;
                    }
                } break;
                case SL_TYPE_BLOB: {
                    blob* cin_p = (blob*)in_p;
                    blob* cout_p = (blob*)out_p;
                    switch (itype) {
                        default: {
                            assert(0);
                        } break;
                        case GSIT_SIZE: {
                            rsize += 8 + cin_p->len;
                        } break;
                        case GSIT_SERIALIZE: {
                            raw_stream rs = rs_init(cbuf);
                            rs_w_size(&rs, cin_p->len);
                            if (cin_p->len > 0) {
                                memcpy(rs.end, cin_p->data, cin_p->len);
                            }
                            rsize += 8 + cin_p->len;
                            cbuf = ptradd(cbuf, 8 + cin_p->len);
                        } break;
                        case GSIT_DESERIALIZE: {
                            if (ptrdiff(ebuf, cbuf) < 8) {
                                return LS_ERR;
                            }
                            raw_stream rs = rs_init(cbuf);
                            size_t csize = rs_r_size(&rs);
                            rsize += 8;
                            cbuf = ptradd(cbuf, 8);
                            blob_create(cout_p, csize);
                            if (csize > 0) {
                                if (ptrdiff(ebuf, cbuf) < csize) {
                                    return LS_ERR;
                                }
                                memcpy(cout_p->data, cbuf, csize);
                            }
                            rsize += csize;
                            cbuf = ptradd(cbuf, csize);
                        } break;
                        case GSIT_COPY: {
                            blob_create(cout_p, cin_p->len);
                            memcpy(cout_p->data, cin_p->data, cin_p->len);
                        } break;
                        case GSIT_DESTROY: {
                            blob_destroy(cin_p);
                        } break;
                    }
                } break;
                case SL_TYPE_COMPLEX: {
                    size_t csize = layout_serializer(itype, pl->ext.layout, in_p, out_p, cbuf, ebuf);
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
                in_p = ptradd(in_p, pl->typesize);
            }
            if (out_ex == true) {
                out_p = ptradd(out_p, pl->typesize);
            }
        }
        if (itype == GSIT_DESTROY && is_ptr == true) {
            free(*(void**)ptradd(obj_in, pl->data_offset));
        }
        pl++;
    }
    return rsize;
}

#ifdef __cplusplus
}
#endif
