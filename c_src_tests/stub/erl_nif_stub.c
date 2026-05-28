#define _POSIX_C_SOURCE 200809L
#include "../../c_src/enif_protobuf.h"

#include <stdarg.h>
#include <stdio.h>

jmp_buf ep_test_exception_jmp;
int ep_test_exception_active;

enum {
    TERM_NIL = 1,
    TERM_ATOM,
    TERM_INT,
    TERM_UINT,
    TERM_INT64,
    TERM_UINT64,
    TERM_DOUBLE,
    TERM_BINARY,
    TERM_TUPLE,
    TERM_LIST_CELL
};

typedef struct {
    int kind;
    union {
        char *atom;
        int i;
        unsigned u;
        int64_t i64;
        uint64_t u64;
        double d;
        ErlNifBinary bin;
        struct {
            ERL_NIF_TERM *elems;
            int arity;
        } tuple;
        struct {
            ERL_NIF_TERM head;
            ERL_NIF_TERM tail;
        } list;
    } u;
} term_cell_t;

struct enif_environment_t {
    ep_state_t *state;
    term_cell_t *terms;
    size_t terms_cap;
    size_t terms_used;
};

static term_cell_t *
term_at(ErlNifEnv *env, ERL_NIF_TERM t)
{
    size_t idx = (size_t)t - 1;

    if (t == 0 || idx >= env->terms_used) {
        return NULL;
    }
    return &env->terms[idx];
}

static ERL_NIF_TERM
term_new(ErlNifEnv *env, int kind)
{
    term_cell_t *c;

    if (env->terms_used >= env->terms_cap) {
        size_t ncap = env->terms_cap ? env->terms_cap * 2 : 64;

        env->terms = realloc(env->terms, ncap * sizeof(term_cell_t));
        if (env->terms == NULL) {
            return 0;
        }
        env->terms_cap = ncap;
    }

    c = &env->terms[env->terms_used++];
    memset(c, 0, sizeof(*c));
    c->kind = kind;
    return (ERL_NIF_TERM)env->terms_used;
}

ErlNifEnv *
ep_test_env_create(void)
{
    ErlNifEnv *env = calloc(1, sizeof(ErlNifEnv));

    return env;
}

void
ep_test_env_destroy(ErlNifEnv *env)
{
    size_t i;
    term_cell_t *c;

    if (env == NULL) {
        return;
    }

    for (i = 0; i < env->terms_used; i++) {
        c = &env->terms[i];
        if (c->kind == TERM_ATOM && c->u.atom != NULL) {
            free(c->u.atom);
        } else if (c->kind == TERM_BINARY && c->u.bin.data != NULL) {
            free(c->u.bin.data);
        } else if (c->kind == TERM_TUPLE && c->u.tuple.elems != NULL) {
            free(c->u.tuple.elems);
        }
    }

    if (env->state != NULL) {
        ep_cache_destroy(&env->state->cache);
        if (env->state->tdata != NULL) {
            uint32_t j;

            for (j = 0; j < env->state->lock_n; j++) {
                _free(env->state->tdata[j].stack.spots);
                _free(env->state->tdata[j].enc.mem);
            }
            _free(env->state->tdata);
        }
        _free(env->state->locks);
        _free(env->state);
    }

    free(env->terms);
    free(env);
}

void *
enif_priv_data(ErlNifEnv *env)
{
    return env->state;
}

void *
enif_alloc(size_t size)
{
    return malloc(size);
}

void
enif_free(void *ptr)
{
    free(ptr);
}

void *
enif_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

int
enif_alloc_binary(size_t size, ErlNifBinary *bin)
{
    bin->size = size;
    bin->data = malloc(size ? size : 1);
    return bin->data != NULL;
}

ERL_NIF_TERM
enif_make_list2(ErlNifEnv *env, ERL_NIF_TERM t1, ERL_NIF_TERM t2)
{
    return enif_make_list(env, 2, t1, t2);
}

void
enif_release_binary(ErlNifBinary *bin)
{
    free(bin->data);
    bin->data = NULL;
    bin->size = 0;
}

ERL_NIF_TERM
enif_raise_exception(ErlNifEnv *env, ERL_NIF_TERM reason)
{
    (void)env;
    (void)reason;
    ep_test_exception_active = 1;
    longjmp(ep_test_exception_jmp, 1);
    return 0;
}

ERL_NIF_TERM
enif_make_badarg(ErlNifEnv *env)
{
    return enif_raise_exception(env, 0);
}

int
enif_make_existing_atom(ErlNifEnv *env, const char *name, ERL_NIF_TERM *atom, int flags)
{
    (void)flags;
    *atom = enif_make_atom(env, name);
    return 1;
}

static ERL_NIF_TERM
find_atom(ErlNifEnv *env, const char *name)
{
    size_t i;

    for (i = 0; i < env->terms_used; i++) {
        if (env->terms[i].kind == TERM_ATOM && strcmp(env->terms[i].u.atom, name) == 0) {
            return (ERL_NIF_TERM)(i + 1);
        }
    }
    return 0;
}

ERL_NIF_TERM
enif_make_atom(ErlNifEnv *env, const char *name)
{
    term_cell_t *c;
    ERL_NIF_TERM existing = find_atom(env, name);
    ERL_NIF_TERM t;

    if (existing != 0) {
        return existing;
    }

    t = term_new(env, TERM_ATOM);
    c = term_at(env, t);
    c->u.atom = strdup(name);
    return t;
}

int
enif_is_atom(ErlNifEnv *env, ERL_NIF_TERM term)
{
    term_cell_t *c = term_at(env, term);

    return c != NULL && c->kind == TERM_ATOM;
}

int
enif_get_int(ErlNifEnv *env, ERL_NIF_TERM term, int *ip)
{
    term_cell_t *c = term_at(env, term);

    if (c == NULL) {
        return 0;
    }
    if (c->kind == TERM_INT) {
        *ip = c->u.i;
        return 1;
    }
    if (c->kind == TERM_UINT) {
        *ip = (int)c->u.u;
        return 1;
    }
    return 0;
}

int
enif_get_uint(ErlNifEnv *env, ERL_NIF_TERM term, unsigned int *ip)
{
    term_cell_t *c = term_at(env, term);

    if (c == NULL) {
        return 0;
    }
    if (c->kind == TERM_UINT) {
        *ip = c->u.u;
        return 1;
    }
    if (c->kind == TERM_INT && c->u.i >= 0) {
        *ip = (unsigned)c->u.i;
        return 1;
    }
    return 0;
}

int
enif_get_int64(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifSInt64 *ip)
{
    term_cell_t *c = term_at(env, term);

    if (c == NULL) {
        return 0;
    }
    if (c->kind == TERM_INT64) {
        *ip = c->u.i64;
        return 1;
    }
    if (c->kind == TERM_INT) {
        *ip = c->u.i;
        return 1;
    }
    return 0;
}

int
enif_get_uint64(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifUInt64 *ip)
{
    term_cell_t *c = term_at(env, term);

    if (c == NULL) {
        return 0;
    }
    if (c->kind == TERM_UINT64) {
        *ip = c->u.u64;
        return 1;
    }
    if (c->kind == TERM_UINT) {
        *ip = c->u.u;
        return 1;
    }
    return 0;
}

int
enif_get_string(ErlNifEnv *env, ERL_NIF_TERM term, char *buf, unsigned len, int flags)
{
    ErlNifBinary bin;

    (void)flags;
    if (!enif_inspect_binary(env, term, &bin)) {
        return 0;
    }
    if (bin.size >= len) {
        return 0;
    }
    memcpy(buf, bin.data, bin.size);
    buf[bin.size] = '\0';
    return 1;
}

int
enif_get_double(ErlNifEnv *env, ERL_NIF_TERM term, double *dp)
{
    term_cell_t *c = term_at(env, term);

    if (c != NULL && c->kind == TERM_DOUBLE) {
        *dp = c->u.d;
        return 1;
    }
    return 0;
}

ERL_NIF_TERM
enif_make_int(ErlNifEnv *env, int i)
{
    term_cell_t *c;
    ERL_NIF_TERM t = term_new(env, TERM_INT);

    c = term_at(env, t);
    c->u.i = i;
    return t;
}

ERL_NIF_TERM
enif_make_uint(ErlNifEnv *env, unsigned int i)
{
    term_cell_t *c;
    ERL_NIF_TERM t = term_new(env, TERM_UINT);

    c = term_at(env, t);
    c->u.u = i;
    return t;
}

ERL_NIF_TERM
enif_make_int64(ErlNifEnv *env, ErlNifSInt64 i)
{
    term_cell_t *c;
    ERL_NIF_TERM t = term_new(env, TERM_INT64);

    c = term_at(env, t);
    c->u.i64 = i;
    return t;
}

ERL_NIF_TERM
enif_make_uint64(ErlNifEnv *env, ErlNifUInt64 i)
{
    term_cell_t *c;
    ERL_NIF_TERM t = term_new(env, TERM_UINT64);

    c = term_at(env, t);
    c->u.u64 = i;
    return t;
}

ERL_NIF_TERM
enif_make_double(ErlNifEnv *env, double d)
{
    term_cell_t *c;
    ERL_NIF_TERM t = term_new(env, TERM_DOUBLE);

    c = term_at(env, t);
    c->u.d = d;
    return t;
}

ERL_NIF_TERM
enif_make_binary(ErlNifEnv *env, ErlNifBinary *bin)
{
    term_cell_t *c;
    ERL_NIF_TERM t = term_new(env, TERM_BINARY);

    c = term_at(env, t);
    c->u.bin.size = bin->size;
    c->u.bin.data = malloc(bin->size ? bin->size : 1);
    if (bin->size > 0) {
        memcpy(c->u.bin.data, bin->data, bin->size);
    }
    return t;
}

ERL_NIF_TERM
enif_make_string(ErlNifEnv *env, const char *string, int flags)
{
    ErlNifBinary bin;
    ERL_NIF_TERM t;

    (void)flags;
    bin.size = strlen(string);
    bin.data = (unsigned char *)string;
    t = enif_make_binary(env, &bin);
    return t;
}

ERL_NIF_TERM
enif_make_list(ErlNifEnv *env, unsigned n, ...)
{
    ERL_NIF_TERM tail = term_new(env, TERM_NIL);
    va_list ap;
    unsigned i;

    (void)n;
    va_start(ap, n);
    for (i = 0; i < n; i++) {
        ERL_NIF_TERM head = va_arg(ap, ERL_NIF_TERM);

        tail = enif_make_list_cell(env, head, tail);
    }
    va_end(ap);
    return tail;
}

ERL_NIF_TERM
enif_make_list_cell(ErlNifEnv *env, ERL_NIF_TERM head, ERL_NIF_TERM tail)
{
    term_cell_t *c;
    ERL_NIF_TERM t = term_new(env, TERM_LIST_CELL);

    c = term_at(env, t);
    c->u.list.head = head;
    c->u.list.tail = tail;
    return t;
}

static ERL_NIF_TERM
make_tuple_n(ErlNifEnv *env, unsigned arity, const ERL_NIF_TERM *elems)
{
    term_cell_t *c;
    ERL_NIF_TERM t = term_new(env, TERM_TUPLE);
    unsigned i;

    c = term_at(env, t);
    c->u.tuple.arity = (int)arity;
    c->u.tuple.elems = malloc(arity * sizeof(ERL_NIF_TERM));
    for (i = 0; i < arity; i++) {
        c->u.tuple.elems[i] = elems[i];
    }
    return t;
}

ERL_NIF_TERM
enif_make_tuple(ErlNifEnv *env, unsigned arity, ...)
{
    ERL_NIF_TERM buf[16];
    va_list ap;
    unsigned i;

    va_start(ap, arity);
    for (i = 0; i < arity && i < 16; i++) {
        buf[i] = va_arg(ap, ERL_NIF_TERM);
    }
    va_end(ap);
    return make_tuple_n(env, arity, buf);
}

ERL_NIF_TERM
enif_make_tuple2(ErlNifEnv *env, ERL_NIF_TERM e1, ERL_NIF_TERM e2)
{
    ERL_NIF_TERM buf[2] = {e1, e2};

    return make_tuple_n(env, 2, buf);
}

ERL_NIF_TERM
enif_make_tuple3(ErlNifEnv *env, ERL_NIF_TERM e1, ERL_NIF_TERM e2, ERL_NIF_TERM e3)
{
    ERL_NIF_TERM buf[3] = {e1, e2, e3};

    return make_tuple_n(env, 3, buf);
}

ERL_NIF_TERM
enif_make_tuple4(ErlNifEnv *env, ERL_NIF_TERM e1, ERL_NIF_TERM e2, ERL_NIF_TERM e3, ERL_NIF_TERM e4)
{
    ERL_NIF_TERM buf[4] = {e1, e2, e3, e4};

    return make_tuple_n(env, 4, buf);
}

ERL_NIF_TERM
enif_make_tuple7(ErlNifEnv *env, ERL_NIF_TERM e1, ERL_NIF_TERM e2, ERL_NIF_TERM e3,
                 ERL_NIF_TERM e4, ERL_NIF_TERM e5, ERL_NIF_TERM e6, ERL_NIF_TERM e7)
{
    ERL_NIF_TERM buf[7] = {e1, e2, e3, e4, e5, e6, e7};

    return make_tuple_n(env, 7, buf);
}

ERL_NIF_TERM
enif_make_tuple_from_array(ErlNifEnv *env, const ERL_NIF_TERM arr[], unsigned n)
{
    return make_tuple_n(env, n, arr);
}

int
enif_get_tuple(ErlNifEnv *env, ERL_NIF_TERM tpl, int *arity, const ERL_NIF_TERM **array)
{
    term_cell_t *c = term_at(env, tpl);

    if (c == NULL || c->kind != TERM_TUPLE) {
        return 0;
    }
    *arity = c->u.tuple.arity;
    *array = c->u.tuple.elems;
    return 1;
}

int
enif_is_list(ErlNifEnv *env, ERL_NIF_TERM term)
{
    term_cell_t *c = term_at(env, term);

    return c != NULL && (c->kind == TERM_LIST_CELL || c->kind == TERM_NIL);
}

int
enif_is_tuple(ErlNifEnv *env, ERL_NIF_TERM term)
{
    term_cell_t *c = term_at(env, term);

    return c != NULL && c->kind == TERM_TUPLE;
}

int
enif_is_binary(ErlNifEnv *env, ERL_NIF_TERM term)
{
    term_cell_t *c = term_at(env, term);

    return c != NULL && c->kind == TERM_BINARY;
}

int
enif_is_empty_list(ErlNifEnv *env, ERL_NIF_TERM term)
{
    term_cell_t *c = term_at(env, term);

    return c != NULL && c->kind == TERM_NIL;
}

int
enif_get_list_cell(ErlNifEnv *env, ERL_NIF_TERM list, ERL_NIF_TERM *head, ERL_NIF_TERM *tail)
{
    term_cell_t *c = term_at(env, list);

    if (c == NULL || c->kind != TERM_LIST_CELL) {
        return 0;
    }
    *head = c->u.list.head;
    *tail = c->u.list.tail;
    return 1;
}

int
enif_get_list_length(ErlNifEnv *env, ERL_NIF_TERM list, unsigned *len)
{
    unsigned n = 0;
    ERL_NIF_TERM head, tail = list;

    while (enif_get_list_cell(env, tail, &head, &tail)) {
        n++;
    }
    if (!enif_is_empty_list(env, tail)) {
        return 0;
    }
    *len = n;
    return 1;
}

int
enif_make_reverse_list(ErlNifEnv *env, ERL_NIF_TERM list, ERL_NIF_TERM *list_out)
{
    ERL_NIF_TERM rev = term_new(env, TERM_NIL);
    ERL_NIF_TERM head, tail = list;

    while (enif_get_list_cell(env, tail, &head, &tail)) {
        rev = enif_make_list_cell(env, head, rev);
    }
    *list_out = rev;
    return 1;
}

int
enif_inspect_binary(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifBinary *bin)
{
    term_cell_t *c = term_at(env, term);

    if (c == NULL || c->kind != TERM_BINARY) {
        return 0;
    }
    bin->size = c->u.bin.size;
    bin->data = c->u.bin.data;
    return 1;
}

int
enif_inspect_iolist_as_binary(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifBinary *bin)
{
    return enif_inspect_binary(env, term, bin);
}

int
enif_compare(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs)
{
    if (lhs < rhs) {
        return -1;
    }
    if (lhs > rhs) {
        return 1;
    }
    return 0;
}

ErlNifRWLock *
enif_rwlock_create(char *name)
{
    (void)name;
    return (ErlNifRWLock *)1;
}

void
enif_rwlock_rwlock(ErlNifRWLock *lock)
{
    (void)lock;
}
void
enif_rwlock_rwunlock(ErlNifRWLock *lock)
{
    (void)lock;
}
void
enif_rwlock_rlock(ErlNifRWLock *lock)
{
    (void)lock;
}
void
enif_rwlock_runlock(ErlNifRWLock *lock)
{
    (void)lock;
}

ErlNifTid
enif_thread_self(void)
{
    return 0;
}

int
ep_test_init_state(ErlNifEnv *env, unsigned lock_n)
{
    ep_state_t *state;
    uint32_t i;

    state = _calloc(sizeof(ep_state_t), 1);
    if (state == NULL) {
        return RET_ERROR;
    }

    state->lock_n = lock_n;
    state->cache_lock = enif_rwlock_create("CACHE_LOCK");
    state->local_lock = enif_rwlock_create("LOCAL_LOCK");
    state->tdata = _calloc(sizeof(ep_tdata_t), lock_n);
    if (state->tdata == NULL) {
        return RET_ERROR;
    }

    for (i = 0; i < lock_n; i++) {
        ep_stack_t *stack = &state->tdata[i].stack;
        ep_enc_t *enc = &state->tdata[i].enc;

        stack->size = STACK_INIT_SIZE;
        stack->spots = _calloc(sizeof(ep_spot_t), stack->size);
        stack->end = stack->spots + stack->size;

        enc->mem = _calloc(ENC_INIT_SIZE, 1);
        enc->size = ENC_INIT_SIZE;
        enc->p = enc->mem;
        enc->end = enc->mem + enc->size;
    }

    state->locks = _calloc(sizeof(ep_lock_t), lock_n);
    for (i = 0; i < lock_n; i++) {
        state->locks[i].tdata = &state->tdata[i];
    }

#define EP_MAKE_ATOM(env, state, name) (state)->atom_##name = make_atom(env, #name)
    EP_MAKE_ATOM(env, state, ok);
    EP_MAKE_ATOM(env, state, error);
    EP_MAKE_ATOM(env, state, true);
    EP_MAKE_ATOM(env, state, false);
    EP_MAKE_ATOM(env, state, undefined);
    EP_MAKE_ATOM(env, state, field);
    EP_MAKE_ATOM(env, state, option);
    EP_MAKE_ATOM(env, state, infinity);
    state->atom_min_infinity = make_atom(env, "-infinity");
    EP_MAKE_ATOM(env, state, nan);
    EP_MAKE_ATOM(env, state, int32);
    EP_MAKE_ATOM(env, state, int64);
    EP_MAKE_ATOM(env, state, uint32);
    EP_MAKE_ATOM(env, state, uint64);
    EP_MAKE_ATOM(env, state, sint32);
    EP_MAKE_ATOM(env, state, sint64);
    EP_MAKE_ATOM(env, state, fixed32);
    EP_MAKE_ATOM(env, state, fixed64);
    EP_MAKE_ATOM(env, state, sfixed32);
    EP_MAKE_ATOM(env, state, sfixed64);
    EP_MAKE_ATOM(env, state, bool);
    EP_MAKE_ATOM(env, state, float);
    EP_MAKE_ATOM(env, state, double);
    EP_MAKE_ATOM(env, state, string);
    EP_MAKE_ATOM(env, state, bytes);
    EP_MAKE_ATOM(env, state, enum);
    EP_MAKE_ATOM(env, state, msg);
    EP_MAKE_ATOM(env, state, map);
    EP_MAKE_ATOM(env, state, required);
    EP_MAKE_ATOM(env, state, optional);
    EP_MAKE_ATOM(env, state, defaulty);
    EP_MAKE_ATOM(env, state, repeated);
#undef EP_MAKE_ATOM

    env->state = state;
    return RET_OK;
}

int
ep_test_build_int32_msg(ErlNifEnv *env, const char *msg_name, uint32_t fnum)
{
    ep_state_t *state = env->state;
    ep_node_t *node;
    ep_field_t *field;
    ep_fnum_field_t *ff;
    ERL_NIF_TERM name;

    if (ep_cache_create(4, &state->cache) != RET_OK) {
        return RET_ERROR;
    }

    node = make_node(1, node_msg);
    if (node == NULL) {
        return RET_ERROR;
    }

    name = enif_make_atom(env, msg_name);
    node->name = name;
    node->n_type = node_msg;
    node->size = 1;
    node->v_size = 1;
    node->id = 1;
    node->proto_v = 2;

    field = node->fields;
    field->name = enif_make_atom(env, "a");
    field->type = field_int32;
    field->fnum = fnum;
    field->rnum = 1;
    field->o_type = occurrence_optional;
    field->proto_v = 2;

    node->v_fields = _calloc(sizeof(ep_fnum_field_t), 1);
    ff = node->v_fields;
    ff->fnum = fnum;
    ff->field = field;

    if (ep_cache_insert(node, state->cache) != RET_OK) {
        free_node(node);
        return RET_ERROR;
    }
    ep_cache_sort(state->cache);
    return RET_OK;
}

void
ep_test_set_opts(ErlNifEnv *env, int with_utf8, int string_as_list)
{
    ep_state_t *state = env->state;

    state->opts.with_utf8 = with_utf8 ? 1 : 0;
    state->opts.string_as_list = string_as_list ? 1 : 0;
}

void
ep_test_prepare_decode_stack(ErlNifEnv *env)
{
    ep_state_t *state = env->state;
    ep_cache_t *cache = state->cache;
    ep_stack_t *stack;
    ep_spot_t *spot;
    uint32_t i, max_fields = 0;

    for (i = 0; i < cache->used; i++) {
        if (cache->names[i].node->n_type == node_msg || cache->names[i].node->n_type == node_map) {
            if (cache->names[i].node->size > max_fields) {
                max_fields = cache->names[i].node->size;
            }
        }
    }

    stack_ensure_all(env, cache);
    stack = &state->tdata[0].stack;
    spot = stack->spots;
    while (spot < stack->end) {
        spot->t_size = max_fields + 1;
        if (spot->t_arr == NULL) {
            spot->t_arr = _calloc(sizeof(ERL_NIF_TERM), spot->t_size);
        } else {
            spot->t_arr = _realloc(spot->t_arr, sizeof(ERL_NIF_TERM) * spot->t_size);
        }
        spot++;
    }
}
