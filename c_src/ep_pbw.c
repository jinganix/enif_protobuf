/*
 * Experimental pbwire-backed codec: gpb record term <-> pbw_message tree, with
 * pbwire doing schema linking, wire encoding and wire decoding.
 *
 * Enabled with -DEPB_USE_PBW=1. The default build does not compile this path
 * into the NIF dispatch (the file is still compiled to keep the build simple).
 *
 * Supported: records, all scalar types, repeated (packed and unpacked),
 * nested messages, enums, oneofs, maps (gpb's default list-of-{K,V} shape),
 * proto2/proto3 defaulty fields. Not supported: groups (pbwire has no group
 * field codec), unknown-field collection (pbwire keeps unknown bytes but does
 * not expose them), and gpb's map-record (`maps` option) shape.
 */
#include "ep_pbw.h"

#include <stdio.h>

typedef struct ep_pbw_node_s ep_pbw_node_t;

typedef struct {
    ep_field_t *field;
    uint32_t pf_index; /* linked-descriptor index, avoids number lookups */
    ep_pbw_node_t *sub;
    uint32_t slot;  /* record tuple index (1-based) */
    uint32_t oneof; /* 0 = not in a oneof, else the parent slot */
} ep_pbw_fmeta_t;

struct ep_pbw_node_s {
    ep_node_t *node;
    pbw_descriptor *desc;
    char name[24];
    uint32_t arity;
    ep_pbw_fmeta_t *slots; /* arity entries, [0] unused */
    ep_pbw_fmeta_t **by_fnum;
    size_t by_fnum_n;
};

struct ep_pbw_cache_s {
    pbw_schema *schema;
    ep_pbw_node_t **nodes;
    size_t n;
    size_t cap;
};

static ERL_NIF_TERM
ep_pbw_raise(ErlNifEnv *env, const char *what)
{
    return enif_raise_exception(env, enif_make_atom(env, what));
}

static int
ep_pbw_is_node(const ep_node_t *node)
{
    return node != NULL &&
           (node->n_type == node_msg || node->n_type == node_map || node->n_type == node_group);
}

static int
ep_pbw_push_node(ep_pbw_cache_t *cache, ep_node_t *node)
{
    ep_pbw_node_t **nodes;
    ep_pbw_node_t *meta;

    if (cache->n == cache->cap) {
        size_t cap = cache->cap ? cache->cap * 2 : 16;

        nodes = (ep_pbw_node_t **)realloc(cache->nodes, cap * sizeof(*nodes));
        if (nodes == NULL) {
            return 0;
        }
        cache->nodes = nodes;
        cache->cap = cap;
    }
    meta = (ep_pbw_node_t *)calloc(1, sizeof(*meta));
    if (meta == NULL) {
        return 0;
    }
    meta->node = node;
    snprintf(meta->name, sizeof(meta->name), "n%zu", cache->n);
    cache->nodes[cache->n++] = meta;
    node->pbw = meta;
    return 1;
}

/* Depth-first over the schema graph: cache nodes plus sub_nodes that are not
 * in the cache (map entries and oneof groups). Idempotent by pointer. */
static int
ep_pbw_collect(ep_pbw_cache_t *cache, ep_node_t *node)
{
    uint32_t i;

    if (node == NULL || node->pbw != NULL) {
        return 1;
    }
    if (!ep_pbw_push_node(cache, node)) {
        return 0;
    }
    if (node->n_type == node_msg || node->n_type == node_map || node->n_type == node_group ||
        node->n_type == node_oneof) {
        for (i = 0; i < node->size; i++) {
            ep_field_t *f = (ep_field_t *)node->fields + i;

            if (!ep_pbw_collect(cache, f->sub_node)) {
                return 0;
            }
        }
    }
    return 1;
}

static pbw_type
ep_pbw_type(field_type_e type)
{
    switch (type) {
    case field_double:
        return PBW_TYPE_DOUBLE;
    case field_float:
        return PBW_TYPE_FLOAT;
    case field_int64:
        return PBW_TYPE_INT64;
    case field_uint64:
        return PBW_TYPE_UINT64;
    case field_int32:
        return PBW_TYPE_INT32;
    case field_fixed64:
        return PBW_TYPE_FIXED64;
    case field_fixed32:
        return PBW_TYPE_FIXED32;
    case field_bool:
        return PBW_TYPE_BOOL;
    case field_string:
        return PBW_TYPE_STRING;
    case field_group:
        return PBW_TYPE_GROUP;
    case field_msg:
    case field_map:
        return PBW_TYPE_MESSAGE;
    case field_bytes:
        return PBW_TYPE_BYTES;
    case field_uint32:
        return PBW_TYPE_UINT32;
    case field_enum:
        return PBW_TYPE_ENUM;
    case field_sfixed32:
        return PBW_TYPE_SFIXED32;
    case field_sfixed64:
        return PBW_TYPE_SFIXED64;
    case field_sint32:
        return PBW_TYPE_SINT32;
    case field_sint64:
        return PBW_TYPE_SINT64;
    default:
        return PBW_TYPE_INT32;
    }
}

static pbw_label
ep_pbw_label(const ep_field_t *f)
{
    if (f->o_type == occurrence_repeated) {
        return PBW_LABEL_REPEATED;
    }
    if (f->o_type == occurrence_required) {
        return PBW_LABEL_REQUIRED;
    }
    return PBW_LABEL_OPTIONAL;
}

static pbw_field *
ep_pbw_add_field(ep_pbw_node_t *meta, ep_field_t *f, uint32_t oneof)
{
    char name[24];
    pbw_field *pf;
    pbw_type type = ep_pbw_type(f->type);

    snprintf(name, sizeof(name), "f%u", f->fnum);
    pf = pbw_message_add_field(meta->desc, name, f->fnum, type, ep_pbw_label(f));
    if (pf == NULL) {
        return NULL;
    }
    if (f->sub_node != NULL &&
        (type == PBW_TYPE_MESSAGE || type == PBW_TYPE_ENUM || type == PBW_TYPE_GROUP)) {
        pbw_field_set_type_name(pf, ((ep_pbw_node_t *)f->sub_node->pbw)->name);
    }
    if (oneof != 0) {
        pbw_field_set_oneof(pf, oneof);
    }
    if (ep_type_is_packable(f->type) && f->o_type == occurrence_repeated &&
        (f->packed || f->proto_v == 3)) {
        pbw_field_set_packed(pf, 1);
    }
    return pf;
}

static int
ep_pbw_build_meta(ep_pbw_cache_t *cache, ep_pbw_node_t *meta)
{
    ep_node_t *node = meta->node;
    uint32_t i;

    meta->arity = node->size + 1;
    if (node->n_type == node_enum) {
        meta->desc = pbw_schema_add_enum(cache->schema, meta->name);
        return meta->desc != NULL;
    }
    if (node->n_type == node_oneof) {
        return 1; /* its fields are flattened into the parent message */
    }
    meta->desc = pbw_schema_add_message(cache->schema, meta->name);
    if (meta->desc == NULL) {
        return 0;
    }
    meta->slots = (ep_pbw_fmeta_t *)calloc(meta->arity, sizeof(ep_pbw_fmeta_t));
    meta->by_fnum = (ep_pbw_fmeta_t **)calloc(node->v_size ? node->v_size : 1, sizeof(*meta->by_fnum));
    if (meta->slots == NULL || meta->by_fnum == NULL) {
        return 0;
    }

    for (i = 0; i < node->size; i++) {
        ep_field_t *f = (ep_field_t *)node->fields + i;
        ep_pbw_fmeta_t *slot = &meta->slots[i + 1];

        if (f->type == field_unknown) {
            continue;
        }
        slot->field = f;
        slot->slot = i + 1;
        if (f->sub_node != NULL) {
            slot->sub = (ep_pbw_node_t *)f->sub_node->pbw;
        }
        if (f->type == field_oneof) {
            uint32_t j;

            for (j = 0; j < f->sub_node->size; j++) {
                ep_field_t *sf = (ep_field_t *)f->sub_node->fields + j;
                ep_pbw_fmeta_t *entry;

                entry = (ep_pbw_fmeta_t *)calloc(1, sizeof(*entry));
                if (entry == NULL) {
                    return 0;
                }
                if (ep_pbw_add_field(meta, sf, i + 1) == NULL) {
                    free(entry);
                    return 0;
                }
                entry->field = sf;
                entry->slot = i + 1;
                entry->oneof = i + 1;
                entry->sub = sf->sub_node != NULL ? (ep_pbw_node_t *)sf->sub_node->pbw : NULL;
                meta->by_fnum[meta->by_fnum_n++] = entry;
            }
        } else {
            ep_pbw_fmeta_t *entry;

            if (ep_pbw_add_field(meta, f, 0) == NULL) {
                return 0;
            }
            entry = (ep_pbw_fmeta_t *)calloc(1, sizeof(*entry));
            if (entry == NULL) {
                return 0;
            }
            *entry = *slot;
            meta->by_fnum[meta->by_fnum_n++] = entry;
        }
    }
    /* Sort by field number for decode lookup. */
    {
        size_t a, b;

        for (a = 1; a < meta->by_fnum_n; a++) {
            ep_pbw_fmeta_t *key = meta->by_fnum[a];

            for (b = a; b > 0 && meta->by_fnum[b - 1]->field->fnum > key->field->fnum; b--) {
                meta->by_fnum[b] = meta->by_fnum[b - 1];
            }
            meta->by_fnum[b] = key;
        }
    }
    return 1;
}

static ep_pbw_fmeta_t *
ep_pbw_find_fnum(ep_pbw_node_t *meta, uint32_t fnum)
{
    size_t lo = 0, hi = meta->by_fnum_n;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        uint32_t value = meta->by_fnum[mid]->field->fnum;

        if (value == fnum) {
            return meta->by_fnum[mid];
        }
        if (value < fnum) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return NULL;
}

ep_pbw_cache_t *
ep_pbw_cache_build(ep_cache_t *cache)
{
    ep_pbw_cache_t *out;
    size_t i;

    out = (ep_pbw_cache_t *)calloc(1, sizeof(*out));
    if (out == NULL) {
        return NULL;
    }
    out->schema = pbw_schema_new();
    if (out->schema == NULL) {
        free(out);
        return NULL;
    }
    for (i = 0; i < cache->used; i++) {
        if (!ep_pbw_collect(out, cache->ids[i].node)) {
            goto fail;
        }
    }
    for (i = 0; i < out->n; i++) {
        if (!ep_pbw_build_meta(out, out->nodes[i])) {
            goto fail;
        }
    }
    if (pbw_schema_link(out->schema) != PBW_OK) {
        goto fail;
    }
    for (i = 0; i < out->n; i++) {
        ep_pbw_node_t *meta = out->nodes[i];
        size_t j;

        if (meta->desc == NULL || meta->slots == NULL) {
            continue;
        }
        for (j = 1; j < meta->arity; j++) {
            ep_pbw_fmeta_t *fm = &meta->slots[j];
            size_t index;

            if (fm->field == NULL || fm->field->type == field_oneof) {
                continue;
            }
            index = pbw_descriptor_field_index(meta->desc, fm->field->fnum);
            fm->pf_index = index == (size_t)-1 ? 0 : (uint32_t)index;
        }
        for (j = 0; j < meta->by_fnum_n; j++) {
            ep_pbw_fmeta_t *fm = meta->by_fnum[j];
            size_t index;

            if (fm->field == NULL) {
                continue;
            }
            index = pbw_descriptor_field_index(meta->desc, fm->field->fnum);
            fm->pf_index = index == (size_t)-1 ? 0 : (uint32_t)index;
        }
    }
    return out;

fail:
    ep_pbw_cache_destroy(out);
    return NULL;
}

void
ep_pbw_cache_destroy(ep_pbw_cache_t *cache)
{
    size_t i;

    if (cache == NULL) {
        return;
    }
    for (i = 0; i < cache->n; i++) {
        size_t j;
        ep_pbw_node_t *meta = cache->nodes[i];

        for (j = 0; j < meta->by_fnum_n; j++) {
            free(meta->by_fnum[j]);
        }
        free(meta->slots);
        free(meta->by_fnum);
        if (meta->node != NULL) {
            meta->node->pbw = NULL;
        }
        free(meta);
    }
    free(cache->nodes);
    pbw_schema_free(cache->schema);
    free(cache);
}

/* ---------------------------------------------------------------- encode */

static ep_field_t *
ep_pbw_oneof_field(ep_node_t *oneof, ERL_NIF_TERM name)
{
    uint32_t i;

    for (i = 0; i < oneof->size; i++) {
        ep_field_t *f = (ep_field_t *)oneof->fields + i;

        if (f->name == name) {
            return f;
        }
    }
    return NULL;
}

static int32_t
ep_pbw_enum_value(ep_node_t *node, ERL_NIF_TERM term)
{
    uint32_t i;

    for (i = 0; i < node->size; i++) {
        ep_enum_field_t *ef = (ep_enum_field_t *)node->fields + i;

        if (ef->name == term) {
            return ef->value;
        }
    }
    return 0;
}

static int
ep_pbw_is_default(ErlNifEnv *env, ep_field_t *f, ERL_NIF_TERM term)
{
    if (f->proto_v != 3 || f->o_type != occurrence_defaulty) {
        return 0;
    }
    switch (f->type) {
    case field_int32:
    case field_int64:
    case field_uint32:
    case field_uint64:
    case field_sint32:
    case field_sint64:
    case field_fixed32:
    case field_fixed64:
    case field_sfixed32:
    case field_sfixed64:
    case field_bool: {
        ErlNifSInt64 value = 0;

        if (!enif_get_int64(env, term, &value)) {
            return 0;
        }
        return value == 0;
    }
    case field_float:
    case field_double: {
        double value = 0;

        if (!enif_get_double(env, term, &value)) {
            return 0;
        }
        return value == 0.0;
    }
    case field_string:
    case field_bytes: {
        ErlNifBinary bin;

        return enif_inspect_binary(env, term, &bin) && bin.size == 0;
    }
    case field_enum:
        return ep_pbw_enum_value(f->sub_node, term) == 0;
    default:
        return 0;
    }
}

static int ep_pbw_encode_record(ErlNifEnv *env, ep_state_t *state, pbw_ctx *ctx, pbw_message *msg,
                                ERL_NIF_TERM term, ep_pbw_node_t *meta);

static int
ep_pbw_encode_one(ErlNifEnv *env, ep_state_t *state, pbw_ctx *ctx, pbw_message *msg, ep_pbw_fmeta_t *fm, ERL_NIF_TERM term)
{
    ep_field_t *f = fm->field;
    pbw_value *value = NULL;
    pbw_status status;

    if (f->type == field_group) {
        ep_pbw_raise(env, "pbw_unsupported_group");
        return -1;
    }
    if (term == state->atom_undefined || ep_pbw_is_default(env, f, term)) {
        return 0;
    }
    switch (f->type) {
    case field_int32:
    case field_sint32:
    case field_sfixed32: {
        ErlNifSInt64 v = 0;

        if (!enif_get_int64(env, term, &v)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        value = pbw_value_new_int_ctx(ctx, (int32_t)v);
        break;
    }
    case field_int64:
    case field_sint64:
    case field_sfixed64: {
        ErlNifSInt64 v = 0;

        if (!enif_get_int64(env, term, &v)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        value = pbw_value_new_int_ctx(ctx, v);
        break;
    }
    case field_uint32:
    case field_fixed32: {
        unsigned int v = 0;

        if (!enif_get_uint(env, term, &v)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        value = pbw_value_new_uint_ctx(ctx, v);
        break;
    }
    case field_uint64:
    case field_fixed64: {
        ErlNifUInt64 v = 0;

        if (!enif_get_uint64(env, term, &v)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        value = pbw_value_new_uint_ctx(ctx, v);
        break;
    }
    case field_bool:
        if (term == state->atom_true) {
            value = pbw_value_new_bool_ctx(ctx, 1);
        } else if (term == state->atom_false) {
            value = pbw_value_new_bool_ctx(ctx, 0);
        } else {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        break;
    case field_float: {
        double v = 0;

        if (term == state->atom_infinity) {
            v = INFINITY;
        } else if (term == state->atom_min_infinity) {
            v = -INFINITY;
        } else if (term == state->atom_nan) {
            v = _NAN;
        } else if (!enif_get_double(env, term, &v)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        value = pbw_value_new_float_ctx(ctx, (float)v);
        break;
    }
    case field_double: {
        double v = 0;

        if (term == state->atom_infinity) {
            v = INFINITY;
        } else if (term == state->atom_min_infinity) {
            v = -INFINITY;
        } else if (term == state->atom_nan) {
            v = _NAN;
        } else if (!enif_get_double(env, term, &v)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        value = pbw_value_new_double_ctx(ctx, v);
        break;
    }
    case field_enum:
        value = pbw_value_new_int_ctx(ctx, ep_pbw_enum_value(f->sub_node, term));
        break;
    case field_string:
    case field_bytes: {
        ErlNifBinary bin;

        if (!enif_inspect_binary(env, term, &bin)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        value = pbw_value_new_bytes_ctx(ctx, bin.data, bin.size);
        break;
    }
    case field_msg:
    case field_map: {
        pbw_message *nested;

        if (fm->sub == NULL || fm->sub->desc == NULL) {
            ep_pbw_raise(env, "pbw_unknown_type");
            return -1;
        }
        nested = pbw_message_new_ctx(ctx, fm->sub->desc);
        if (nested == NULL) {
            ep_pbw_raise(env, "alloc");
            return -1;
        }
        if (ep_pbw_encode_record(env, state, ctx, nested, term, fm->sub) != 0) {
            pbw_message_free(nested);
            return -1;
        }
        value = pbw_value_new_message_ctx(ctx, nested);
        break;
    }
    default:
        ep_pbw_raise(env, "badarg");
        return -1;
    }
    if (value == NULL) {
        ep_pbw_raise(env, "alloc");
        return -1;
    }
    status = pbw_message_set_index(msg, fm->pf_index, value);
    if (status != PBW_OK) {
        pbw_value_free(value);
        ep_pbw_raise(env, status == PBW_ERR_NOMEM ? "alloc" : "badarg");
        return -1;
    }
    return 0;
}

static int
ep_pbw_encode_slot(ErlNifEnv *env, ep_state_t *state, pbw_ctx *ctx, pbw_message *msg, ep_pbw_node_t *meta, uint32_t slot,
                   ERL_NIF_TERM term)
{
    ep_pbw_fmeta_t *fm;

    if (slot >= meta->arity) {
        return 0;
    }
    fm = &meta->slots[slot];
    if (fm->field == NULL) {
        return 0;
    }
    return ep_pbw_encode_one(env, state, ctx, msg, fm, term);
}

static int
ep_pbw_encode_field(ErlNifEnv *env, ep_state_t *state, pbw_ctx *ctx, pbw_message *msg, ep_pbw_fmeta_t *fm, ERL_NIF_TERM term)
{
    ep_field_t *f = fm->field;
    ERL_NIF_TERM head, tail;

    if (term == state->atom_undefined) {
        return 0;
    }
    if (f->type == field_map) {
        if (!enif_is_list(env, term)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        while (enif_get_list_cell(env, term, &head, &tail)) {
            int32_t arity;
            ERL_NIF_TERM *kv;
            pbw_message *entry;
            pbw_value *value;

            if (!enif_get_tuple(env, head, &arity, to_const(kv)) || arity != 2) {
                ep_pbw_raise(env, "badarg");
                return -1;
            }
            entry = pbw_message_new_ctx(ctx, fm->sub->desc);
            if (entry == NULL) {
                ep_pbw_raise(env, "alloc");
                return -1;
            }
            if (ep_pbw_encode_slot(env, state, ctx, entry, fm->sub, 1, kv[0]) != 0 ||
                ep_pbw_encode_slot(env, state, ctx, entry, fm->sub, 2, kv[1]) != 0) {
                pbw_message_free(entry);
                return -1;
            }
            value = pbw_value_new_message_ctx(ctx, entry);
            if (value == NULL || pbw_message_set_index(msg, fm->pf_index, value) != PBW_OK) {
                pbw_value_free(value);
                ep_pbw_raise(env, "alloc");
                return -1;
            }
            term = tail;
        }
        if (!enif_is_empty_list(env, term)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        return 0;
    }
    if (f->o_type == occurrence_repeated) {
        if (!enif_is_list(env, term)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        while (enif_get_list_cell(env, term, &head, &tail)) {
            if (ep_pbw_encode_one(env, state, ctx, msg, fm, head) != 0) {
                return -1;
            }
            term = tail;
        }
        if (!enif_is_empty_list(env, term)) {
            ep_pbw_raise(env, "badarg");
            return -1;
        }
        return 0;
    }
    return ep_pbw_encode_one(env, state, ctx, msg, fm, term);
}

static int
ep_pbw_encode_record(ErlNifEnv *env, ep_state_t *state, pbw_ctx *ctx, pbw_message *msg, ERL_NIF_TERM term,
                     ep_pbw_node_t *meta)
{
    int32_t arity;
    const ERL_NIF_TERM *array;
    uint32_t i;

    if (!enif_get_tuple(env, term, &arity, to_const(array))) {
        ep_pbw_raise(env, "badarg");
        return -1;
    }
    for (i = 1; i < meta->arity && i < (uint32_t)arity; i++) {
        ep_pbw_fmeta_t *fm = &meta->slots[i];
        ep_field_t *f = fm->field;

        if (f == NULL) {
            continue;
        }
        if (f->type == field_oneof) {
            ERL_NIF_TERM oneof_term = array[i];
            int32_t oneof_arity;
            const ERL_NIF_TERM *oneof_array;
            ep_field_t *sf;
            ep_pbw_fmeta_t *sfm;

            if (oneof_term == state->atom_undefined) {
                continue;
            }
            if (!enif_get_tuple(env, oneof_term, &oneof_arity, to_const(oneof_array)) || oneof_arity != 2) {
                ep_pbw_raise(env, "badarg");
                return -1;
            }
            sf = ep_pbw_oneof_field(f->sub_node, oneof_array[0]);
            if (sf == NULL) {
                ep_pbw_raise(env, "badarg");
                return -1;
            }
            sfm = ep_pbw_find_fnum(meta, sf->fnum);
            if (sfm == NULL || ep_pbw_encode_one(env, state, ctx, msg, sfm, oneof_array[1]) != 0) {
                return -1;
            }
        } else if (ep_pbw_encode_field(env, state, ctx, msg, fm, array[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

ERL_NIF_TERM
ep_pbw_encode(ErlNifEnv *env, ep_state_t *state, ep_pbw_cache_t *cache, pbw_ctx *ctx, ERL_NIF_TERM term)
{
    int32_t arity;
    const ERL_NIF_TERM *array;
    ep_node_t *node;
    ep_pbw_node_t *meta;
    pbw_message *msg;
    const uint8_t *buf = NULL;
    size_t len = 0;
    ERL_NIF_TERM out;

    (void)cache;
    if (!enif_get_tuple(env, term, &arity, to_const(array)) || arity < 1) {
        return enif_make_badarg(env);
    }
    node = get_node_by_name(array[0], state->cache);
    if (node == NULL || node->pbw == NULL || !ep_pbw_is_node(node)) {
        return ep_pbw_raise(env, "unknown_msg");
    }
    meta = (ep_pbw_node_t *)node->pbw;
    msg = pbw_message_new_ctx(ctx, meta->desc);
    if (msg == NULL) {
        return ep_pbw_raise(env, "alloc");
    }
    if (ep_pbw_encode_record(env, state, ctx, msg, term, meta) != 0) {
        pbw_message_free(msg);
        return 0;
    }
    if (pbw_encode_ctx(ctx, msg, &buf, &len) != PBW_OK) {
        pbw_ctx_reset(ctx);
        return ep_pbw_raise(env, "encode_failed");
    }
    {
        ERL_NIF_TERM bin_term;
        unsigned char *dst = enif_make_new_binary(env, len, &bin_term);

        if (len > 0) {
            memcpy(dst, buf, len);
        }
        out = bin_term;
    }
    pbw_ctx_reset(ctx);
    return out;
}

/* ---------------------------------------------------------------- decode */

static ERL_NIF_TERM
ep_pbw_default(ErlNifEnv *env, ep_state_t *state, ep_field_t *f, ep_node_t *node)
{
    if (f->o_type == occurrence_repeated) {
        return enif_make_list(env, 0);
    }
    if (f->o_type != occurrence_required && !(f->proto_v == 3 && f->o_type == occurrence_defaulty) &&
        node->n_type != node_map) {
        return state->atom_undefined;
    }
    switch (f->type) {
    case field_int32:
    case field_int64:
    case field_uint32:
    case field_uint64:
    case field_sint32:
    case field_sint64:
    case field_fixed32:
    case field_fixed64:
    case field_sfixed32:
    case field_sfixed64:
        return enif_make_int(env, 0);
    case field_bool:
        return state->atom_false;
    case field_float:
    case field_double:
        return enif_make_double(env, 0.0);
    case field_string:
        return enif_make_string(env, "", ERL_NIF_LATIN1);
    case field_bytes: {
        ErlNifBinary bin;

        if (!enif_alloc_binary(0, &bin)) {
            return state->atom_undefined;
        }
        return enif_make_binary(env, &bin);
    }
    case field_enum: {
        ep_node_t *en = f->sub_node;
        ep_enum_field_t *zero = NULL;
        uint32_t i;

        for (i = 0; i < en->size; i++) {
            ep_enum_field_t *ef = (ep_enum_field_t *)en->fields + i;

            if (ef->value == 0) {
                zero = ef;
                break;
            }
        }
        return zero != NULL ? zero->name : ((ep_enum_field_t *)en->fields)->name;
    }
    default:
        return state->atom_undefined;
    }
}

static ERL_NIF_TERM
ep_pbw_utf8_list(ErlNifEnv *env, const unsigned char *data, size_t len)
{
    ERL_NIF_TERM list = enif_make_list(env, 0);
    size_t i = 0;

    while (i < len) {
        uint32_t cp;
        unsigned char c = data[i++];

        if (c < 0x80) {
            cp = c;
        } else if ((c & 0xe0) == 0xc0 && i < len) {
            cp = ((uint32_t)(c & 0x1f) << 6) | (data[i++] & 0x3f);
        } else if ((c & 0xf0) == 0xe0 && i + 1 < len) {
            cp = ((uint32_t)(c & 0x0f) << 12) | ((uint32_t)(data[i] & 0x3f) << 6) | (data[i + 1] & 0x3f);
            i += 2;
        } else if ((c & 0xf8) == 0xf0 && i + 2 < len) {
            cp = ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(data[i] & 0x3f) << 12) |
                 ((uint32_t)(data[i + 1] & 0x3f) << 6) | (data[i + 2] & 0x3f);
            i += 3;
        } else {
            cp = c;
        }
        list = enif_make_list_cell(env, enif_make_uint(env, cp), list);
    }
    enif_make_reverse_list(env, list, &list);
    return list;
}

static ERL_NIF_TERM
ep_pbw_value_term(ErlNifEnv *env, ep_state_t *state, ep_field_t *f, pbw_value *v)
{
    switch (f->type) {
    case field_int32:
    case field_sint32:
    case field_sfixed32:
        return enif_make_int(env, pbw_value_get_int32(v));
    case field_int64:
    case field_sint64:
    case field_sfixed64:
        return enif_make_int64(env, pbw_value_get_int(v));
    case field_uint32:
    case field_fixed32:
        return enif_make_uint(env, pbw_value_get_uint32(v));
    case field_uint64:
    case field_fixed64:
        return enif_make_uint64(env, pbw_value_get_uint(v));
    case field_bool:
        return pbw_value_get_bool(v) ? state->atom_true : state->atom_false;
    case field_float: {
        float number = pbw_value_get_float(v);

        if (number != number) {
            return state->atom_nan;
        }
        if (number == INFINITY) {
            return state->atom_infinity;
        }
        if (number == -INFINITY) {
            return state->atom_min_infinity;
        }
        return enif_make_double(env, (double)number);
    }
    case field_double: {
        double number = pbw_value_get_double(v);

        if (number != number) {
            return state->atom_nan;
        }
        if (number == INFINITY) {
            return state->atom_infinity;
        }
        if (number == -INFINITY) {
            return state->atom_min_infinity;
        }
        return enif_make_double(env, number);
    }
    case field_enum: {
        int32_t number = pbw_value_get_int32(v);
        ep_node_t *en = f->sub_node;
        uint32_t i;

        for (i = 0; i < en->v_size; i++) {
            ep_enum_field_t *ef = (ep_enum_field_t *)en->v_fields + i;

            if (ef->value == number) {
                return ef->name;
            }
        }
        return enif_make_int(env, number);
    }
    case field_string:
    case field_bytes: {
        size_t len = 0;
        const void *data = pbw_value_get_bytes(v, &len);

        if (f->type == field_string && state->opts.string_as_list) {
            if (state->opts.with_utf8) {
                return ep_pbw_utf8_list(env, (const unsigned char *)data, len);
            } else {
                size_t i;
                ERL_NIF_TERM list = enif_make_list(env, 0);

                for (i = len; i > 0; i--) {
                    list = enif_make_list_cell(env, enif_make_int(env, ((const unsigned char *)data)[i - 1]), list);
                }
                return list;
            }
        }
        {
            ERL_NIF_TERM term;
            unsigned char *dst = enif_make_new_binary(env, len, &term);

            if (len > 0) {
                memcpy(dst, data, len);
            }
            return term;
        }
    }
    default:
        return state->atom_undefined;
    }
}

static ERL_NIF_TERM
ep_pbw_decode_record(ErlNifEnv *env, ep_state_t *state, pbw_message *msg, ep_pbw_node_t *meta);

static ERL_NIF_TERM
ep_pbw_decode_slot(ErlNifEnv *env, ep_state_t *state, pbw_message *msg, ep_pbw_node_t *meta, uint32_t slot)
{
    ep_pbw_fmeta_t *fm;
    pbw_value *v;

    if (meta == NULL || slot >= meta->arity) {
        return state->atom_undefined;
    }
    fm = &meta->slots[slot];
    if (fm->field == NULL) {
        return state->atom_undefined;
    }
    v = pbw_message_get_index(msg, fm->pf_index);
    if (v == NULL) {
        return ep_pbw_default(env, state, fm->field, meta->node);
    }
    if (fm->field->type == field_msg) {
        pbw_message *nested = pbw_value_get_message(v);

        return nested != NULL ? ep_pbw_decode_record(env, state, nested, fm->sub) : state->atom_undefined;
    }
    return ep_pbw_value_term(env, state, fm->field, v);
}

static ERL_NIF_TERM
ep_pbw_decode_record(ErlNifEnv *env, ep_state_t *state, pbw_message *msg, ep_pbw_node_t *meta)
{
    ERL_NIF_TERM small[64];
    ERL_NIF_TERM *arr = meta->arity <= 64 ? small : NULL;
    ERL_NIF_TERM result;
    size_t i;
    uint32_t slot;

    if (arr == NULL) {
        arr = (ERL_NIF_TERM *)calloc(meta->arity, sizeof(ERL_NIF_TERM));
        if (arr == NULL) {
            return ep_pbw_raise(env, "alloc");
        }
    }
    arr[0] = meta->node->name;
    for (slot = 0; slot < meta->node->size; slot++) {
        ep_field_t *f = (ep_field_t *)meta->node->fields + slot;

        arr[slot + 1] = ep_pbw_default(env, state, f, meta->node);
    }

    /* Iterating the known field table is O(fields); the reflection API
     * (field_count + field_at) rescans the descriptor for every field. */
    for (i = 0; i < meta->by_fnum_n; i++) {
        ep_pbw_fmeta_t *fm = meta->by_fnum[i];
        ep_field_t *f = fm->field;
        pbw_value *value;

        if (f == NULL || f->type == field_group) {
            continue;
        }
        value = pbw_message_get_index(msg, fm->pf_index);
        if (value == NULL) {
            continue;
        }
        if (f->type == field_map || f->o_type == occurrence_repeated) {
            size_t n = pbw_value_count(value);
            ERL_NIF_TERM small_items[64];
            ERL_NIF_TERM *items = n <= 64 ? small_items : NULL;

            if (items == NULL) {
                items = (ERL_NIF_TERM *)calloc(n ? n : 1, sizeof(ERL_NIF_TERM));
                if (items == NULL) {
                    if (arr != small) {
                        free(arr);
                    }
                    return ep_pbw_raise(env, "alloc");
                }
            }
            for (size_t k = 0; k < n; k++) {
                pbw_value *item = pbw_value_at(value, k);

                if (f->type == field_map) {
                    pbw_message *entry = pbw_value_get_message(item);
                    ERL_NIF_TERM kv[2];

                    kv[0] = entry != NULL ? ep_pbw_decode_slot(env, state, entry, fm->sub, 1)
                                          : state->atom_undefined;
                    kv[1] = entry != NULL ? ep_pbw_decode_slot(env, state, entry, fm->sub, 2)
                                          : state->atom_undefined;
                    items[k] = enif_make_tuple_from_array(env, kv, 2);
                } else if (f->type == field_msg) {
                    pbw_message *nested = pbw_value_get_message(item);

                    items[k] = nested != NULL ? ep_pbw_decode_record(env, state, nested, fm->sub)
                                              : state->atom_undefined;
                } else {
                    items[k] = ep_pbw_value_term(env, state, f, item);
                }
            }
            arr[fm->slot] = enif_make_list_from_array(env, items, (unsigned int)n);
            if (items != small_items) {
                free(items);
            }
        } else if (f->type == field_msg) {
            pbw_message *nested = pbw_value_get_message(value);

            arr[fm->slot] = nested != NULL ? ep_pbw_decode_record(env, state, nested, fm->sub)
                                           : state->atom_undefined;
        } else if (fm->oneof != 0) {
            ERL_NIF_TERM pair[2];

            pair[0] = f->name;
            pair[1] = ep_pbw_value_term(env, state, f, value);
            arr[fm->slot] = enif_make_tuple_from_array(env, pair, 2);
        } else {
            arr[fm->slot] = ep_pbw_value_term(env, state, f, value);
        }
    }

    result = enif_make_tuple_from_array(env, arr, meta->arity);
    if (arr != small) {
        free(arr);
    }
    return result;
}

ERL_NIF_TERM
ep_pbw_decode(ErlNifEnv *env, ep_state_t *state, ep_pbw_cache_t *cache, pbw_ctx *ctx, ERL_NIF_TERM bin, ep_node_t *node)
{
    ErlNifBinary data;
    pbw_message *msg = NULL;
    ep_pbw_node_t *meta;
    ERL_NIF_TERM out;

    (void)cache;
    if (node == NULL || node->pbw == NULL || !ep_pbw_is_node(node)) {
        return ep_pbw_raise(env, "unknown_msg");
    }
    if (!enif_inspect_binary(env, bin, &data)) {
        return enif_make_badarg(env);
    }
    meta = (ep_pbw_node_t *)node->pbw;
    pbw_ctx_reset(ctx);
    if (pbw_decode_ctx(ctx, meta->desc, data.data, data.size, &msg) != PBW_OK) {
        return ep_pbw_raise(env, "decode_failed");
    }
    out = ep_pbw_decode_record(env, state, msg, meta);
    pbw_ctx_reset(ctx);
    return out;
}
