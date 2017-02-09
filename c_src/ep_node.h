/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#ifndef EP_NODE_H_INCLUDED
#define EP_NODE_H_INCLUDED

#include "enif_protobuf.h"

struct field_type_s {
    field_type_e    type;
    node_t         *node;
    ERL_NIF_TERM    name;
};

struct opts_s {
    ERL_NIF_TERM    defaut_value;
    int32_t         packed;
};

struct enum_field_s {
    ERL_NIF_TERM    name;
    int32_t         value;
    uint32_t        proto_v;
};

struct field_s {
    ERL_NIF_TERM        name;
    uint32_t            proto_v;
    int32_t             id;
    int32_t             fnum;
    int32_t             rnum;
    field_type_t        f_type;
    occurrence_type_e   o_type;
    opts_t              opts;
};

struct node_s {
    node_type_e     n_type;
    ERL_NIF_TERM    name;
    int32_t         id;
    uint32_t        proto_v;
    int32_t         size;
    void           *fields;
    int32_t         v_size;
    void           *v_fields;
};

struct node_id_s {
    uint32_t        id;
    node_t         *node;
};

struct node_name_s {
    ERL_NIF_TERM    name;
    node_t         *node;
};

ERL_NIF_TERM
parse_node(ErlNifEnv *env, ERL_NIF_TERM term, node_t **node, uint32_t proto_v, ERL_NIF_TERM syn_list);

ERL_NIF_TERM
prelink_nodes(ErlNifEnv *env);

int
get_field_compare_name(const void *a, const void *b);

int
get_enum_compare_name(const void *a, const void *b);

#endif
