/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#ifndef ENIF_PROTO_H_INCLUDED
#define ENIF_PROTO_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "erl_nif.h"

#define DEBUG       0
#define USE_OS_MEM  0
#define DEBUG_MEM   0

#define RET_OK      0
#define RET_ERROR   1

#define TRUE        1
#define FALSE       0

#define ENC_INIT_SIZE       1024 * 1024
#define STACK_INIT_SIZE     32

#ifdef _MSC_VER
# define inline __inline
#endif

#if 1
#define debug_step() printf("%s:%d:%s()\r\n", __FILE__, __LINE__, __func__)

#define debug_out(fmt) do { \
    fprintf(stderr, "%s:%d:%s(): " fmt "\r\n", __FILE__, __LINE__, __func__); \
} while (0)

#define debug(fmt, ...) do { \
    fprintf(stderr, "%s:%d:%s(): " fmt "\r\n", __FILE__, __LINE__, __func__, __VA_ARGS__); \
} while (0)

#else
#define debug_step() do {} while(0)
#define debug(fmt, ...) do {} while(0)
#endif

#define return_exception(env, term) do {    \
        printf("%s:%d:%s()\r\n", __FILE__, __LINE__, __func__); \
        return enif_raise_exception(env, term);    \
} while(0)

#define return_error(env, term) do {    \
        printf("%s:%d:%s()\r\n", __FILE__, __LINE__, __func__); \
        return enif_make_tuple2(env, ((state_t *) enif_priv_data(env))->atom_error, term);    \
} while(0)

#define check_ret(ret, func) do {               \
    if ((ret = (func)) != RET_OK) {             \
        debug("ret %lu", (unsigned long) ret);  \
        return ret;                             \
    }                                           \
} while(0)

typedef struct field_type_s field_type_t;
typedef struct opts_s opts_t;
typedef struct enum_field_s enum_field_t;
typedef struct field_s field_t;
typedef struct node_id_s node_id_t;
typedef struct node_name_s node_name_t;
typedef struct msg_s msg_t;
typedef struct enum_s enum_t;
typedef struct node_s node_t;

typedef struct state_s state_t;
typedef struct cache_s cache_t;
typedef struct enc_s enc_t;
typedef struct spot_s spot_t;
typedef struct stack_s stack_t;

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

struct stack_s {
    spot_t         *tmp;
    spot_t         *spot;
    spot_t         *end;
    spot_t         *mem;
    size_t          size;
};

struct enc_s {
    char           *tmp;
    char           *p;
    char           *sentinel;
    char           *end;
    char           *mem;
    size_t          size;
    ERL_NIF_TERM    result;
    uint32_t        omit;
};

struct spot_s {
    int32_t         type;

    int8_t          need_length;
    size_t          sentinel_size;

    size_t          pos;
    node_t         *node;
    field_t        *field;

    ERL_NIF_TERM   *array;  // is_tuple
    ERL_NIF_TERM    list;   // is_list
};

typedef struct tdata_s {
    stack_t         stack;
    enc_t           enc;
} tdata_t;

typedef struct lock_s {
    ErlNifTid       tid;
    tdata_t        *tdata;
} lock_t;

struct state_s {
    cache_t        *cache;
    cache_t        *old_cache;

    int32_t         lock_n;
    int32_t         lock_used;
    lock_t         *lock_end;
    lock_t         *locks;
    tdata_t        *tdata;

    ERL_NIF_TERM    int_zero;

    ERL_NIF_TERM    atom_ok;
    ERL_NIF_TERM    atom_error;
	ERL_NIF_TERM    atom_true;
	ERL_NIF_TERM    atom_false;
    ERL_NIF_TERM    atom_undefined;
	ERL_NIF_TERM    atom_field;
    ERL_NIF_TERM    atom_gpb_oneof;
    ERL_NIF_TERM    atom_packed;
    ERL_NIF_TERM    atom_default;
    ERL_NIF_TERM    atom_option;
    ERL_NIF_TERM    atom_allow_alias;
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

#endif
