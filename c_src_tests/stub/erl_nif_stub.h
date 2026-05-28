#ifndef ERL_NIF_STUB_H
#define ERL_NIF_STUB_H

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>

typedef uintptr_t ERL_NIF_TERM;
typedef struct enif_environment_t ErlNifEnv;
typedef unsigned char ErlNifUInt8;
typedef unsigned int ErlNifUInt;
typedef long ErlNifSInt;
typedef int64_t ErlNifSInt64;
typedef uint64_t ErlNifUInt64;

typedef struct {
    size_t size;
    unsigned char *data;
} ErlNifBinary;

#define ERL_NIF_LATIN1 1

typedef struct enif_rwlock_t ErlNifRWLock;
typedef unsigned long ErlNifTid;

extern jmp_buf ep_test_exception_jmp;
extern int ep_test_exception_active;

ErlNifEnv *ep_test_env_create(void);
void ep_test_env_destroy(ErlNifEnv *env);
void *enif_priv_data(ErlNifEnv *env);

int enif_alloc_binary(size_t size, ErlNifBinary *bin);
ERL_NIF_TERM enif_make_list2(ErlNifEnv *env, ERL_NIF_TERM t1, ERL_NIF_TERM t2);
void enif_release_binary(ErlNifBinary *bin);
void *enif_alloc(size_t size);
void enif_free(void *ptr);
void *enif_realloc(void *ptr, size_t size);

int enif_make_existing_atom(ErlNifEnv *env, const char *name, ERL_NIF_TERM *atom, int flags);
ERL_NIF_TERM enif_make_atom(ErlNifEnv *env, const char *name);
ERL_NIF_TERM enif_make_badarg(ErlNifEnv *env);
ERL_NIF_TERM enif_raise_exception(ErlNifEnv *env, ERL_NIF_TERM reason);

int enif_get_uint(ErlNifEnv *env, ERL_NIF_TERM term, unsigned int *ip);
int enif_get_int(ErlNifEnv *env, ERL_NIF_TERM term, int *ip);
int enif_get_int64(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifSInt64 *ip);
int enif_get_uint64(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifUInt64 *ip);
int enif_get_double(ErlNifEnv *env, ERL_NIF_TERM term, double *dp);
int enif_get_string(ErlNifEnv *env, ERL_NIF_TERM term, char *buf, unsigned len, int flags);

ERL_NIF_TERM enif_make_uint(ErlNifEnv *env, unsigned int i);
ERL_NIF_TERM enif_make_int(ErlNifEnv *env, int i);
ERL_NIF_TERM enif_make_int64(ErlNifEnv *env, ErlNifSInt64 i);
ERL_NIF_TERM enif_make_uint64(ErlNifEnv *env, ErlNifUInt64 i);
ERL_NIF_TERM enif_make_double(ErlNifEnv *env, double d);
ERL_NIF_TERM enif_make_binary(ErlNifEnv *env, ErlNifBinary *bin);
ERL_NIF_TERM enif_make_sub_binary(ErlNifEnv *env, ERL_NIF_TERM bin_term, size_t pos, size_t size);
ERL_NIF_TERM enif_make_string(ErlNifEnv *env, const char *string, int flags);
ERL_NIF_TERM enif_make_list(ErlNifEnv *env, unsigned n, ...);
ERL_NIF_TERM enif_make_list_cell(ErlNifEnv *env, ERL_NIF_TERM head, ERL_NIF_TERM tail);
ERL_NIF_TERM enif_make_tuple(ErlNifEnv *env, unsigned arity, ...);
ERL_NIF_TERM enif_make_tuple2(ErlNifEnv *env, ERL_NIF_TERM e1, ERL_NIF_TERM e2);
ERL_NIF_TERM enif_make_tuple3(ErlNifEnv *env, ERL_NIF_TERM e1, ERL_NIF_TERM e2, ERL_NIF_TERM e3);
ERL_NIF_TERM enif_make_tuple4(ErlNifEnv *env, ERL_NIF_TERM e1, ERL_NIF_TERM e2, ERL_NIF_TERM e3, ERL_NIF_TERM e4);
ERL_NIF_TERM enif_make_tuple7(ErlNifEnv *env, ERL_NIF_TERM e1, ERL_NIF_TERM e2,
                              ERL_NIF_TERM e3, ERL_NIF_TERM e4, ERL_NIF_TERM e5, ERL_NIF_TERM e6, ERL_NIF_TERM e7);
ERL_NIF_TERM enif_make_tuple_from_array(ErlNifEnv *env, const ERL_NIF_TERM arr[], unsigned n);

int enif_get_tuple(ErlNifEnv *env, ERL_NIF_TERM tpl, int *arity, const ERL_NIF_TERM **array);
int enif_is_atom(ErlNifEnv *env, ERL_NIF_TERM term);
int enif_is_list(ErlNifEnv *env, ERL_NIF_TERM term);
int enif_is_tuple(ErlNifEnv *env, ERL_NIF_TERM term);
int enif_is_binary(ErlNifEnv *env, ERL_NIF_TERM term);
int enif_is_empty_list(ErlNifEnv *env, ERL_NIF_TERM term);
int enif_get_list_cell(ErlNifEnv *env, ERL_NIF_TERM list, ERL_NIF_TERM *head, ERL_NIF_TERM *tail);
int enif_get_list_length(ErlNifEnv *env, ERL_NIF_TERM list, unsigned *len);
int enif_make_reverse_list(ErlNifEnv *env, ERL_NIF_TERM list, ERL_NIF_TERM *list_out);

int enif_inspect_binary(ErlNifEnv *env, ERL_NIF_TERM bin_term, ErlNifBinary *bin);
int enif_inspect_iolist_as_binary(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifBinary *bin);
int enif_compare(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs);

ErlNifRWLock *enif_rwlock_create(char *name);
void enif_rwlock_rwlock(ErlNifRWLock *lock);
void enif_rwlock_rwunlock(ErlNifRWLock *lock);
void enif_rwlock_rlock(ErlNifRWLock *lock);
void enif_rwlock_runlock(ErlNifRWLock *lock);
ErlNifTid enif_thread_self(void);

void ep_test_reset_tls(void);
int ep_test_init_state(ErlNifEnv *env, unsigned lock_n);
int ep_test_build_int32_msg(ErlNifEnv *env, const char *msg_name, uint32_t fnum);
void ep_test_prepare_decode_stack(ErlNifEnv *env);
void ep_test_set_opts(ErlNifEnv *env, int with_utf8, int string_as_list);

#endif
