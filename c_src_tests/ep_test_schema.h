#ifndef EP_TEST_SCHEMA_H
#define EP_TEST_SCHEMA_H

#include "../c_src/enif_protobuf.h"
#include "stub/erl_nif_stub.h"

int ep_test_load_m1_cache(ErlNifEnv *env);
int ep_test_load_person_cache(ErlNifEnv *env);
int ep_test_load_single_field(ErlNifEnv *env, const char *msg_name, const char *field_name,
                              unsigned fnum, ERL_NIF_TERM type, ERL_NIF_TERM occ, ERL_NIF_TERM *extra_defs, size_t extra_n);
int ep_test_load_packed_field(ErlNifEnv *env, const char *msg_name, const char *field_name,
                              unsigned fnum, ERL_NIF_TERM type);
int ep_test_load_oneof_cache(ErlNifEnv *env);
int ep_test_load_nested_cache(ErlNifEnv *env);
int ep_test_load_enum_cache(ErlNifEnv *env);
ERL_NIF_TERM ep_test_mk_field(ErlNifEnv *env, ep_state_t *st, const char *name, unsigned fnum,
                              unsigned rnum, ERL_NIF_TERM type, ERL_NIF_TERM occ);
ERL_NIF_TERM ep_test_mk_msg_def(ErlNifEnv *env, const char *name, ERL_NIF_TERM fields);
ERL_NIF_TERM ep_test_make_m1_message(ErlNifEnv *env, ep_state_t *state);
ERL_NIF_TERM ep_test_make_m1_message_prefix(ErlNifEnv *env, ep_state_t *state, unsigned n_set);
ERL_NIF_TERM ep_test_make_m1_message_field(ErlNifEnv *env, ep_state_t *state, unsigned field_idx);
ERL_NIF_TERM ep_test_make_person_message(ErlNifEnv *env);
int ep_test_roundtrip(ErlNifEnv *env, ERL_NIF_TERM msg, const char *msg_name);
int ep_test_encode_only(ErlNifEnv *env, ERL_NIF_TERM msg);
int ep_test_encode_only_small_buf(ErlNifEnv *env, ERL_NIF_TERM msg, size_t buf_size);
int ep_test_decode_wire(ErlNifEnv *env, const char *msg_name, const unsigned char *buf, size_t len);

#endif
