/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#include "enif_protobuf.h"

#if DEBUG_MEM
size_t mem_total = 0;
#endif // DEBUG_MEM

ERL_NIF_TERM
make_atom(ErlNifEnv *env, const char *name)
{
    ERL_NIF_TERM    atom;

    if (enif_make_existing_atom(env, name, &atom, ERL_NIF_LATIN1)) {
        return atom;
    }

    return enif_make_atom(env, name);
}

/*
 * nif library callbacks
 */
static int
load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
    enc_t          *enc;
    int32_t         lock_n, i;
    stack_t        *stack;
    state_t        *state;

    if (*priv == NULL) {

        if (!enif_get_int(env, info, &lock_n)) {
            return RET_ERROR;
        }

        state = _calloc(sizeof(state_t), 1);
        if (state == NULL) {
            return RET_ERROR;
        }

        state->lock_n = lock_n;

        /*
         * init state->tdata
         */
        state->tdata = _calloc(sizeof(tdata_t), state->lock_n);

        if (state->tdata == NULL) {
            return RET_ERROR;
        }

        for (i = 0; i < state->lock_n; i++) {

            stack = &(state->tdata[i].stack);
            stack->size = STACK_INIT_SIZE;
            stack->mem = _calloc(sizeof(spot_t), stack->size);
            if (stack->mem == NULL) {
                return RET_ERROR;
            }
            stack->end = stack->mem + stack->size;

            enc = &(state->tdata[i].enc);
            enc->mem = _calloc(ENC_INIT_SIZE, 1);
            enc->size = ENC_INIT_SIZE;
        }

        /*
         * init state->locks
         */
        state->locks = _calloc(sizeof(lock_t), state->lock_n);
        if (state->locks == NULL) {
            return RET_ERROR;
        }
        state->lock_end = state->locks + state->lock_n;

        for (i = 0; i < state->lock_n; i++) {

            state->locks[i].tdata = &(state->tdata[i]);
        }

#define EP_MAKE_ATOM(env, state, name) (state)->atom_##name = make_atom(env, #name)

        state->int_zero = enif_make_int(env, 0);

        EP_MAKE_ATOM(env, state, ok);
        EP_MAKE_ATOM(env, state, error);
        EP_MAKE_ATOM(env, state, true);
        EP_MAKE_ATOM(env, state, false);
        EP_MAKE_ATOM(env, state, undefined);
        EP_MAKE_ATOM(env, state, field);
        EP_MAKE_ATOM(env, state, gpb_oneof);
        EP_MAKE_ATOM(env, state, packed);
        EP_MAKE_ATOM(env, state, default);
        EP_MAKE_ATOM(env, state, option);
        EP_MAKE_ATOM(env, state, allow_alias);
        EP_MAKE_ATOM(env, state, infinity);
        (state)->atom_min_infinity = make_atom(env, "-infinity");
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
        EP_MAKE_ATOM(env, state, repeated);

        *priv = (void *) state;
    }
    return RET_OK;
}

static int
reload(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
    return RET_OK;
}

static int
upgrade(ErlNifEnv *env, void **priv, void **old_priv, ERL_NIF_TERM info)
{

    *priv = *old_priv;
    return load(env, priv, info);
}

static void
unload(ErlNifEnv *env, void *priv)
{
#if 0
    if (priv != NULL) {
        state = (state_t *) enif_priv_data(env);
        _free(state->stack.mem);
        cache_destroy(&(state->cache));
        _free(state);
    }
#endif
    return;
}

static ERL_NIF_TERM
load_cache_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    char            buf[16];
    node_t         *node;
    int32_t         arity;
    state_t        *state = (state_t *) enif_priv_data(env);
    uint32_t        len = 0, proto_v = 2;
    ERL_NIF_TERM    term, head, tail, ret, syn_list = 0;
    ERL_NIF_TERM   *array;

    if (argc != 1) {
        return enif_make_badarg(env);
    }

    term = argv[0];
    while (enif_get_list_cell(env, term, &head, &tail)) {

        if (!enif_get_tuple(env, head, &arity, to_const(array)) || arity != 2) {
            return_error(env, head);
        }

        if (array[0] == make_atom(env, "syntax")) {

            if (!enif_get_string(env, array[1], buf, sizeof(buf), ERL_NIF_LATIN1)) {
                return_error(env, head);
            }

            if (!strncmp(buf, "proto2", sizeof("proto2"))) {
                proto_v = 2;

            } else if (!strncmp(buf, "proto3", sizeof("proto3"))) {
                proto_v = 3;

            } else {
                return_error(env, head);
            }

            term = tail;
            continue;
        }

        if (array[0] == make_atom(env, "proto3_msgs")) {

            if (enif_is_list(env, array[1])) {
                syn_list = array[1];

            } else {
                return_error(env, head);
            }
            term = tail;
            continue;
        }

        len++;
        term = tail;
    }

    if (len == 0) {
        return_error(env, argv[0]);
    }

    if (state->old_cache != NULL) {
        cache_destroy(&(state->old_cache));
    }

    state->old_cache = state->cache;

    if (cache_create(len, &(state->cache)) != RET_OK) {
        return_exception(env, state->atom_error);
    }

    term = argv[0];
    while (enif_get_list_cell(env, term, &head, &tail)) {

        if (!enif_get_tuple(env, head, &arity, to_const(array)) || arity != 2) {
            return_error(env, head);
        }

        if (array[0] == make_atom(env, "syntax") || array[0] == make_atom(env, "proto3_msgs")) {
            term = tail;
            continue;
        }

        check_ret(ret, parse_node(env, head, &node, proto_v, syn_list));

        cache_insert(node, state->cache);
        term = tail;
    }
    cache_sort(state->cache);

    check_ret(ret, prelink_nodes(env));

    return state->atom_ok;
}

static ERL_NIF_TERM
purge_cache_0(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    state_t        *state = (state_t *) enif_priv_data(env);

    if(argc != 0) {
        return enif_make_badarg(env);
    }

    if (state->old_cache != NULL) {
        cache_destroy(&(state->old_cache));
    }

    if (state->cache != NULL) {
        cache_destroy(&(state->cache));
    }

    return state->atom_ok;
}

static int
search_compare_lock(const void *a, const void *b)
{
    return (int) ((size_t) *((ErlNifTid *) a) - (size_t) ((lock_t *) b)->tid);
}

static int
sort_compare_lock(const void *a, const void *b)
{
    return (int) ((size_t) ((lock_t *) a)->tid - (size_t) ((lock_t *) b)->tid);
}

static ERL_NIF_TERM
encode_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    lock_t         *lock;
    tdata_t        *tdata;
    state_t        *state = (state_t *) enif_priv_data(env);
    ErlNifTid       tid;
    ERL_NIF_TERM    ret;

    if (argc != 1 || !enif_is_tuple(env, argv[0])) {
        return enif_make_badarg(env);
    }

    if (state->cache == NULL) {
        return_error(env, make_atom(env, "cache_not_exists"));
    }

    tid = enif_thread_self();
    if (state->lock_used < state->lock_n) {

        //debug("used: %d, lock_n: %d", state->lock_used, state->lock_n);
        lock = bsearch(&tid, state->locks, state->lock_used, sizeof(lock_t), search_compare_lock);
        if (lock == NULL) {
            lock = &state->locks[(state->lock_used)++];
            lock->tid = tid;
            tdata = lock->tdata;
            qsort(state->locks, state->lock_used, sizeof(lock_t), sort_compare_lock);

        } else {

            tdata = lock->tdata;
        }

    } else {

        lock = bsearch(&tid, state->locks, state->lock_used, sizeof(node_id_t), search_compare_lock);
        if (lock == NULL) {
            return_error(env, make_atom(env, "tid_not_found"));
        }
        tdata = lock->tdata;
    }

    //debug("used: %d, lock_n: %d, lock: 0x%016lx", state->lock_used, state->lock_n, (size_t) lock);
    tdata->enc.p = tdata->enc.mem;
    tdata->enc.end = tdata->enc.mem + tdata->enc.size;

    check_ret(ret, encode(env, argv[0], tdata));

    return tdata->enc.result;
}

static ERL_NIF_TERM
decode_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    return make_atom(env, "not_available");
}

#if DEBUG
static ERL_NIF_TERM
debug_term_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM    term;

    if (argc != 1) {
        return enif_make_badarg(env);
    }

    term = argv[0];
    if (enif_is_atom(env, term)) {
        printf("atom v:%lu\r\n", (unsigned long) term);
    }

    return term;
}
#endif

static ErlNifFunc funcs[] =
{
#if DEBUG
        {"debug_term", 1, debug_term_1},
#endif
        {"load_cache", 1, load_cache_1},
        {"purge_cache", 0, purge_cache_0},
        {"encode", 1, encode_1},
        {"decode", 1, decode_1}
};

ERL_NIF_INIT(enif_protobuf, funcs, &load, &reload, &upgrade, &unload);
