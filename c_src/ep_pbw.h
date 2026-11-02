#ifndef __EP_PBW_H__
#define __EP_PBW_H__

#include "enif_protobuf.h"
#include "pbwire/pbwire.h"

/*
 * Experimental pbwire-backed codec (EPB_USE_PBW).
 *
 * Builds a pbw_schema that mirrors the ep_node cache parsed from gpb's
 * get_msg_defs/0, then converts gpb record terms to pbw_message trees and
 * back. Kept behind a compile-time switch so the default build is unchanged.
 */
ep_pbw_cache_t *ep_pbw_cache_build(ep_cache_t *cache);
void ep_pbw_cache_destroy(ep_pbw_cache_t *cache);

ERL_NIF_TERM ep_pbw_encode(ErlNifEnv *env, ep_state_t *state, ep_pbw_cache_t *cache, pbw_ctx *ctx, ERL_NIF_TERM term);
ERL_NIF_TERM ep_pbw_decode(ErlNifEnv *env, ep_state_t *state, ep_pbw_cache_t *cache, pbw_ctx *ctx, ERL_NIF_TERM bin,
                           ep_node_t *node);

#endif
