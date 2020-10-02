/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#ifndef __EP_NODE_H__
#define __EP_NODE_H__

#include "enif_protobuf.h"

struct ep_enum_field_s {
    ERL_NIF_TERM    name;
    int32_t         value;
    uint32_t        proto_v;
};

struct ep_field_s {
    ERL_NIF_TERM        name;
    occurrence_type_e   o_type;
    field_type_e        type;
    ep_node_t          *sub_node;
    ERL_NIF_TERM        sub_name;
    ERL_NIF_TERM        defaut_value;
    uint32_t            id;
    uint32_t            fnum;
    uint32_t            rnum;
    uint32_t            proto_v;
    uint32_t            is_oneof;
    uint32_t            packed;
};

struct ep_fnum_field_s {
    uint32_t        fnum;
    ep_field_t     *field;
};

struct ep_node_s {
    node_type_e     n_type;
    ERL_NIF_TERM    name;
    uint32_t        id;
    uint32_t        proto_v;
    uint32_t        size;
    uint32_t        v_size;
    void           *fields;
    void           *v_fields;
};

struct ep_node_id_s {
    uint32_t        id;
    ep_node_t      *node;
};

struct ep_node_name_s {
    ERL_NIF_TERM    name;
    ep_node_t      *node;
};

void
free_node(ep_node_t *node);

ERL_NIF_TERM
parse_node(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t **node, uint32_t proto_v, ERL_NIF_TERM syn_list);

ERL_NIF_TERM
prelink_nodes(ErlNifEnv *env, ep_cache_t *cache);

void
stack_ensure_all(ErlNifEnv *env, ep_cache_t *cache);

ERL_NIF_TERM
stack_ensure(ErlNifEnv *env, ep_stack_t *stack, ep_spot_t **spot);

int
get_field_compare_name(const void *a, const void *b);

int
get_field_compare_sub_name(const void *a, const void *b);

int
get_map_field_compare_fnum(const void *a, const void *b);

int
get_field_compare_fnum(const void *a, const void *b);

int
get_enum_compare_name(const void *a, const void *b);

int
get_enum_compare_value(const void *a, const void *b);

#endif
