#include "enif_protobuf.h"

#if defined(_MSC_VER)
#include <intrin.h>
#define EP_THREAD_LOCAL __declspec(thread)
#else
#define EP_THREAD_LOCAL __thread
#endif

static EP_THREAD_LOCAL ep_state_t *ep_tls_state;
static EP_THREAD_LOCAL ep_tdata_t *ep_tls_tdata;

static inline uint32_t
ep_atomic_fetch_add_u32(uint32_t *p, uint32_t add)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_fetch_add(p, add, __ATOMIC_RELAXED);
#elif defined(_MSC_VER)
    return (uint32_t)_InterlockedExchangeAdd((volatile long *)p, (long)add);
#else
    uint32_t old = *p;
    *p += add;
    return old;
#endif
}

ep_tdata_t *
ep_get_tdata(ep_state_t *state)
{
    if (ep_tls_state == state && ep_tls_tdata != NULL) {
        return ep_tls_tdata;
    }

    uint32_t slot = ep_atomic_fetch_add_u32(&state->slot_next, 1);
    if (slot >= state->lock_n) {
        slot = state->lock_n - 1;
    }

    ep_tls_state = state;
    ep_tls_tdata = &state->tdata[slot];
    return ep_tls_tdata;
}

#if defined(EPB_UNIT_TEST)
void
ep_test_reset_tls(void)
{
    ep_tls_state = NULL;
    ep_tls_tdata = NULL;
}
#endif

#if DEBUG_MEM
size_t mem_total = 0;
#endif // DEBUG_MEM

ERL_NIF_TERM
make_atom(ErlNifEnv *env, const char *name)
{
    ERL_NIF_TERM atom;

    if (enif_make_existing_atom(env, name, &atom, ERL_NIF_LATIN1)) {
        return atom;
    }

    return enif_make_atom(env, name);
}

ERL_NIF_TERM
ep_load_cache(ErlNifEnv *env, ERL_NIF_TERM list)
{
    char buf[16];
    ep_spot_t *spot;
    ep_node_t *node = NULL;
    ep_cache_t *cache, *old_cache;
    ep_stack_t *stack;
    int32_t arity;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    uint32_t i, len = 0, proto_v = 2, max_fields = 2;
    ERL_NIF_TERM term, head, tail, ret, proto3_list = 0;
    ERL_NIF_TERM *array;

    term = list;
    while (enif_get_list_cell(env, term, &head, &tail)) {
        if (!enif_get_tuple(env, head, &arity, to_const(array)) || arity != 2) {
            raise_exception(env, head);
        }

        if (array[0] == make_atom(env, "syntax")) {
            if (!enif_get_string(env, array[1], buf, sizeof(buf), ERL_NIF_LATIN1)) {
                raise_exception(env, head);
            }

            if (!strncmp(buf, "proto2", sizeof("proto2"))) {
                proto_v = 2;
            } else if (!strncmp(buf, "proto3", sizeof("proto3"))) {
                proto_v = 3;
            } else {
                raise_exception(env, head);
            }

            term = tail;
            continue;
        }

        if (array[0] == make_atom(env, "proto3_msgs")) {
            if (enif_is_list(env, array[1])) {
                proto3_list = array[1];
            } else {
                raise_exception(env, head);
            }
            term = tail;
            continue;
        }

        len++;
        term = tail;
    }

    if (len == 0) {
        raise_exception(env, list);
    }

    if (ep_cache_create(len, &cache) != RET_OK) {
        raise_exception(env, list);
    }

    term = list;
    while (enif_get_list_cell(env, term, &head, &tail)) {
        if (!enif_get_tuple(env, head, &arity, to_const(array)) || arity != 2) {
            ep_cache_destroy(&cache);
            raise_exception(env, head);
        }

        if (array[0] == make_atom(env, "syntax") || array[0] == make_atom(env, "proto3_msgs")) {
            term = tail;
            continue;
        }

        if ((ret = parse_node(env, head, &node, proto_v, proto3_list)) != RET_OK) {
            if (node != NULL) {
                free_node(node);
            }
            ep_cache_destroy(&cache);
            raise_exception(env, ret);
        }

        if (node != NULL) {
            if (node->n_type == node_msg || node->n_type == node_map) {
                max_fields = max_fields >= node->size ? max_fields : node->size;
            }
            ep_cache_insert(node, cache);
        }
        term = tail;
    }

    if (!cache->used) {
        ep_cache_destroy(&cache);
        raise_exception(env, list);
    }
    ep_cache_sort(cache);

    if ((ret = prelink_nodes(env, cache)) != RET_OK) {
        ep_cache_destroy(&cache);
        return ret;
    }

    /*
     * Swap cache under write lock so readers never observe a half-updated
     * pointer and old cache lifetime stays valid for in-flight readers.
     */
    enif_rwlock_rwlock(state->cache_lock);

    stack_ensure_all(env, cache);
    for (i = 0; i < state->lock_n; i++) {
        stack = &(state->tdata[i].stack);
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
    old_cache = state->cache;
    state->cache = cache;

    enif_rwlock_rwunlock(state->cache_lock);

    if (old_cache != NULL) {
        ep_cache_destroy(&old_cache);
    }

    return state->atom_ok;
}

#ifndef EPB_UNIT_TEST
/*
 * nif library callbacks
 */
static int
load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
    ep_enc_t *enc;
    uint32_t lock_n, i;
    ep_stack_t *stack;
    ep_state_t *state;

    if (*priv == NULL) {
        if (!enif_get_uint(env, info, &lock_n)) {
            return RET_ERROR;
        }

        state = _calloc(sizeof(ep_state_t), 1);
        if (state == NULL) {
            return RET_ERROR;
        }
        state->lock_n = lock_n;
        state->cache_lock = enif_rwlock_create("CACHE_LOCK");
        state->slot_next = 0;

        /*
         * init state->tdata
         */
        state->tdata = _calloc(sizeof(ep_tdata_t), state->lock_n);
        if (state->tdata == NULL) {
            return RET_ERROR;
        }

        for (i = 0; i < state->lock_n; i++) {

            stack = &(state->tdata[i].stack);
            stack->size = STACK_INIT_SIZE;
            stack->spots = _calloc(sizeof(ep_spot_t), stack->size);
            if (stack->spots == NULL) {
                return RET_ERROR;
            }
            stack->end = stack->spots + stack->size;

            enc = &(state->tdata[i].enc);
            enc->mem = _calloc(ENC_INIT_SIZE, 1);
            enc->size = ENC_INIT_SIZE;
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
        EP_MAKE_ATOM(env, state, defaulty);
        EP_MAKE_ATOM(env, state, repeated);

        *priv = (void *)state;
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
        state = (ep_state_t *) enif_priv_data(env);
        _free(state->stack.mem);
        ep_cache_destroy(&(state->cache));
        _free(state);
    }
    return;
#endif
}

static ERL_NIF_TERM
load_cache_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    if (argc != 1) {
        return enif_make_badarg(env);
    }

    return ep_load_cache(env, argv[0]);
}

static ERL_NIF_TERM
purge_cache_0(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ep_cache_t *old_cache = NULL;

    if (argc != 0) {
        return enif_make_badarg(env);
    }

    /* Same protection scope as load_cache: pointer swap + object lifetime. */
    enif_rwlock_rwlock(state->cache_lock);
    old_cache = state->cache;
    state->cache = NULL;
    enif_rwlock_rwunlock(state->cache_lock);
    if (old_cache != NULL) {
        ep_cache_destroy(&old_cache);
    }

    return state->atom_ok;
}

static ERL_NIF_TERM
encode_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    ep_tdata_t *tdata;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM ret;

    if (argc != 1 || !enif_is_tuple(env, argv[0])) {
        return enif_make_badarg(env);
    }

    if (state->cache == NULL) {
        raise_exception(env, make_atom(env, "cache_not_exists"));
    }

    tdata = ep_get_tdata(state);
    enif_rwlock_rlock(state->cache_lock);
    tdata->enc.p = tdata->enc.mem;
    tdata->enc.end = tdata->enc.mem + tdata->enc.size;

    if ((ret = (encode(env, argv[0], tdata))) == RET_OK) {
        ret = tdata->enc.result;
    }
    enif_rwlock_runlock(state->cache_lock);

    return ret;
}

static ERL_NIF_TERM
decode_2(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    ep_node_t *node;
    ep_tdata_t *tdata;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM ret;

    if (argc != 2 || !enif_is_binary(env, argv[0]) || !enif_is_atom(env, argv[1])) {
        return enif_make_badarg(env);
    }

    if (state->cache == NULL) {
        raise_exception(env, make_atom(env, "cache_not_exists"));
    }

    tdata = ep_get_tdata(state);
    enif_rwlock_rlock(state->cache_lock);
    if (!enif_inspect_binary(env, argv[0], &(tdata->dec.bin))) {
        enif_rwlock_runlock(state->cache_lock);
        raise_exception(env, argv[0]);
    }

    tdata->dec.p = (char *)(tdata->dec.bin.data);
    tdata->dec.end = tdata->dec.p + tdata->dec.bin.size;
    tdata->dec.term = argv[0];

    node = get_node_by_name(argv[1], state->cache);
    if (node == NULL) {
        enif_rwlock_runlock(state->cache_lock);
        raise_exception(env, argv[1]);
    }
    if ((ret = (decode(env, tdata, node))) == RET_OK) {
        ret = tdata->dec.result;
    }
    enif_rwlock_runlock(state->cache_lock);

    return ret;
}

static ERL_NIF_TERM
set_opts_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    int32_t arity;
    ERL_NIF_TERM head, tail, *array;
    ERL_NIF_TERM opt_with_utf8, opt_string_as_list;

    if (argc != 1 || !enif_is_list(env, argv[0])) {
        return enif_make_badarg(env);
    }

    /*
     * Resolve option atoms once per call; avoids repeated atom lookups
     * when callers pass longer option lists.
     */
    opt_with_utf8 = make_atom(env, "with_utf8");
    opt_string_as_list = make_atom(env, "string_as_list");

    head = argv[0];
    while (enif_get_list_cell(env, head, &head, &tail)) {
        if (enif_get_tuple(env, head, &arity, to_const(array)) && arity == 2) {
            if (array[0] == opt_with_utf8) {
                if (array[1] == state->atom_true) {
                    state->opts.with_utf8 = 1;
                } else if (array[1] == state->atom_false) {
                    state->opts.with_utf8 = 0;
                } else {
                    return enif_make_badarg(env);
                }
            } else if (array[0] == opt_string_as_list) {
                if (array[1] == state->atom_true) {
                    state->opts.string_as_list = 1;
                } else if (array[1] == state->atom_false) {
                    state->opts.string_as_list = 0;
                } else {
                    return enif_make_badarg(env);
                }
            } else {
                return enif_make_badarg(env);
            }
        } else {
            return enif_make_badarg(env);
        }

        head = tail;
    }

    return state->atom_ok;
}

#if DEBUG
static ERL_NIF_TERM
debug_term_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM term;

    if (argc != 1) {
        return enif_make_badarg(env);
    }

    term = argv[0];
    if (enif_is_atom(env, term)) {
        printf("atom v:%lu\r\n", (unsigned long)term);
    }

    return term;
}
#endif

static ErlNifFunc funcs[] =
    {
#if DEBUG
        {"debug_term", 1, debug_term_1},
#endif
        {"set_opts", 1, set_opts_1},
        {"load_cache", 1, load_cache_1},
        {"purge_cache", 0, purge_cache_0},
        {"encode", 1, encode_1},
        {"decode", 2, decode_2}};

ERL_NIF_INIT(enif_protobuf, funcs, &load, &reload, &upgrade, &unload);
#endif /* EPB_UNIT_TEST */
