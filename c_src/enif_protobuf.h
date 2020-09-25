/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#ifndef __EPB_H__
#define __EPB_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "erl_nif.h"

#define IS_DEBUG    0
#define USE_OS_MEM  0
#define DEBUG_MEM   0

#define RET_OK      0
#define RET_ERROR   1

#define TRUE        1
#define FALSE       0

#define ENC_INIT_SIZE       (1024 * 1024)
#define STACK_INIT_SIZE     2
#define ARRAY_INIT_SIZE     32

#ifdef _MSC_VER
#define inline __inline
#define __func__ __FUNCTION__
#define _NAN (-NAN)
#else
#define _NAN NAN
#endif

#ifndef _MSC_VER
#define sprintf_s(p, s, fmt, ...) sprintf(p, fmt, __VA_ARGS__)
#endif

#if IS_DEBUG
#define debug_out(fmt) do { \
    fprintf(stderr, "%s:%d:%s(): " fmt "\r\n", __FILE__, __LINE__, __func__); \
} while (0)

#define debug_step() printf("%s:%d:%s()\r\n", __FILE__, __LINE__, __func__)

#define debug(fmt, ...) do { \
    fprintf(stderr, "%s:%d:%s(): " fmt "\r\n", __FILE__, __LINE__, __func__, __VA_ARGS__); \
} while (0)
#else
#define debug_step() do {} while(0)
#define debug(fmt, ...) do {} while(0)
#endif

#if IS_DEBUG
#define return_exception(env, term) do {    \
        printf("%s:%d:%s()\r\n", __FILE__, __LINE__, __func__); \
        return enif_raise_exception(env, term);    \
} while(0)

#define return_error(env, term) do {    \
        printf("%s:%d:%s()\r\n", __FILE__, __LINE__, __func__); \
        return enif_make_tuple2(env, ((ep_state_t *) enif_priv_data(env))->atom_error, term);    \
} while(0)
#else
#define return_exception(env, term) return enif_raise_exception(env, term)

#define return_error(env, term) return enif_make_tuple2(env, ((ep_state_t *) enif_priv_data(env))->atom_error, term)
#endif

#define check_ret(ret, func) do {               \
    if (((ret) = (func)) != RET_OK) {             \
        debug("ret %lu", (unsigned long) ret);  \
        return ret;                             \
    }                                           \
} while(0)

typedef struct ep_enum_field_s ep_enum_field_t;
typedef struct ep_field_s ep_field_t;
typedef struct ep_fnum_field_s ep_fnum_field_t;
typedef struct ep_node_id_s ep_node_id_t;
typedef struct ep_node_name_s ep_node_name_t;
typedef struct ep_msg_s ep_msg_t;
typedef struct ep_enum_s ep_enum_t;
typedef struct ep_node_s ep_node_t;

typedef struct ep_state_s ep_state_t;
typedef struct ep_cache_s ep_cache_t;
typedef struct ep_enc_s ep_enc_t;
typedef struct ep_dec_s ep_dec_t;
typedef struct ep_spot_s ep_spot_t;
typedef struct ep_stack_s ep_stack_t;

typedef enum {
    //field_unknown = INT_MIN,
    field_unknown,
    field_int32,
    field_int64,
    field_uint32,
    field_uint64,
    field_sint32,
    field_sint64,
    field_fixed32,
    field_fixed64,
    field_sfixed32,
    field_sfixed64,
    field_bool,
    field_float,
    field_double,
    field_string,
    field_bytes,

    field_enum,
    field_msg,
    field_map,
    field_oneof
} field_type_e;

typedef enum {
    occurrence_required,
    occurrence_optional,
    occurrence_repeated
} occurrence_type_e;

typedef enum {
    node_unknown,
    node_msg,
    node_enum,
    node_oneof,
    node_map
} node_type_e;

struct ep_stack_s {
    ep_spot_t      *end;
    ep_spot_t      *spots;
    size_t          size;
};

struct ep_enc_s {
    char           *tmp;
    char           *p;
    char           *sentinel;
    char           *end;
    char           *mem;
    size_t          size;
    ERL_NIF_TERM    result;
    uint32_t        omit;
};

struct ep_dec_s {
    char           *p;
    char           *sentinel;
    char           *end;
    ErlNifBinary    bin;
    ERL_NIF_TERM    term;
    ERL_NIF_TERM    result;
};

struct ep_spot_s {
    int32_t         type;

    int8_t          need_length;
    size_t          sentinel_size;

    size_t          pos;
    ep_node_t      *node;
    ep_field_t     *field;

    ERL_NIF_TERM   *array;  // is_tuple
    ERL_NIF_TERM    list;   // is_list

    ERL_NIF_TERM   *t_arr;
    size_t          t_size;
    size_t          t_used;
    ERL_NIF_TERM    result;
    char           *end_sentinel;
};

typedef struct ep_tdata_s {
    ep_stack_t      stack;
    ep_enc_t        enc;
    ep_dec_t        dec;
} ep_tdata_t;

typedef struct ep_lock_s {
    ErlNifTid       tid;
    ep_tdata_t     *tdata;
} ep_lock_t;

struct ep_state_s {
    ep_cache_t     *cache;

    uint32_t        lock_n;
    uint32_t        lock_used;
    ep_lock_t      *locks;
    ep_tdata_t     *tdata;
    ErlNifRWLock   *cache_lock;
    ErlNifRWLock   *local_lock;

    struct opts_s {
        uint32_t    with_utf8;
        uint32_t    string_as_list;
    } opts;

    ERL_NIF_TERM    integer_zero;
    ERL_NIF_TERM    double_zero;
    ERL_NIF_TERM    binary_nil;
    ERL_NIF_TERM    nil;

    ERL_NIF_TERM    atom_ok;
    ERL_NIF_TERM    atom_error;
	ERL_NIF_TERM    atom_true;
	ERL_NIF_TERM    atom_false;
    ERL_NIF_TERM    atom_undefined;
	ERL_NIF_TERM    atom_field;
    ERL_NIF_TERM    atom_option;
    ERL_NIF_TERM    atom_infinity;
    ERL_NIF_TERM    atom_min_infinity;
    ERL_NIF_TERM    atom_nan;

    ERL_NIF_TERM    atom_int32;
    ERL_NIF_TERM    atom_int64;
    ERL_NIF_TERM    atom_uint32;
    ERL_NIF_TERM    atom_uint64;
    ERL_NIF_TERM    atom_sint32;
    ERL_NIF_TERM    atom_sint64;
    ERL_NIF_TERM    atom_fixed32;
    ERL_NIF_TERM    atom_fixed64;
    ERL_NIF_TERM    atom_sfixed32;
    ERL_NIF_TERM    atom_sfixed64;
    ERL_NIF_TERM    atom_bool;
    ERL_NIF_TERM    atom_float;
    ERL_NIF_TERM    atom_double;
    ERL_NIF_TERM    atom_string;
    ERL_NIF_TERM    atom_bytes;

    ERL_NIF_TERM    atom_enum;
    ERL_NIF_TERM    atom_msg;
    ERL_NIF_TERM    atom_map;

    ERL_NIF_TERM    atom_required;
    ERL_NIF_TERM    atom_optional;
    ERL_NIF_TERM    atom_repeated;
};

#define to_const(p)     (const ERL_NIF_TERM **) (&(p))

#if USE_OS_MEM

#define _alloc(size)            malloc(size)
#define _free(ptr)              free(ptr)
#define _realloc(ptr, size)     realloc(ptr, size)
#define _calloc(nmemb, size)    calloc(nmemb, size)

#elif DEBUG_MEM

extern size_t mem_total;

#define _alloc(size) __alloc(size, __FILE__, __LINE__, __func__)

static inline void *
__alloc(size_t size, const char *file, int line, const char *func)
{
	size_t	   *p;

	p = enif_alloc(size + sizeof(size_t));
	*p++ = size;
	mem_total += size;
	printf("%s:%d:%s() alloc: %ld, total: %ld\r\n", file, line, func, (long) size, (long)mem_total);

	return p;
}

#define _free(ptr) __free(ptr, __FILE__, __LINE__, __func__)

static inline void
__free(void *ptr, const char *file, int line, const char *func)
{
	size_t	   *p;

	p = (size_t *) ptr;
	p--;
	mem_total = mem_total - (*p);
	printf("%s:%d:%s() free: %ld, total: %ld\r\n", file, line, func, (long) *p, (long) mem_total);

	enif_free(p);
}

#define _realloc(ptr, size) __realloc(ptr, size, __FILE__, __LINE__, __func__)

static inline void *
__realloc(void *ptr, size_t size, const char *file, int line, const char *func)
{
	void	   *p;

	p = __alloc(size, file, line, func);
	if (p != NULL) {
		__free(ptr, file, line, func);
		return p;
	}

	return NULL;
}

#define _calloc(nmemb, size) __calloc(nmemb, size, __FILE__, __LINE__, __func__)

static inline void *
__calloc(size_t nmemb, size_t size, const char *file, int line, const char *func)
{
	void   *ptr;

	size *= nmemb;
	ptr = __alloc(size, file, line, func);
	return ptr == NULL ? ptr : memset(ptr, 0x00, size);
}
#else

#define _alloc(size)        enif_alloc(size)
#define _free(ptr)          enif_free(ptr)
#define _realloc(ptr, size) enif_realloc(ptr, size)

static inline void *
_calloc(size_t nmemb, size_t size)
{
	void   *ptr;

	size *= nmemb;
	ptr = _alloc(size);
	return ptr == NULL ? ptr : memset(ptr, 0x00, size);
}

#endif

#include "ep_codec.h"
#include "ep_cache.h"
#include "ep_node.h"

ERL_NIF_TERM
make_atom(ErlNifEnv *env, const char *name);

char*
get_atom(ErlNifEnv *env, ERL_NIF_TERM term, char *buf, unsigned size);

#endif
