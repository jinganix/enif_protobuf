/* Reusable per-caller workspace: one bump allocator for decoded messages and one
 * growable buffer for encoded output. This is the explicit, thread-safe-by-
 * construction version of enif_protobuf's per-scheduler thread data. */

#include "internal.h"

struct pbw_ctx {
    pbw_arena arena; /* decoded trees: read-only */
    pbw_arena build; /* caller-built trees for encoding: writable */
    pbw_buf out;
};

pbw_ctx *
pbw_ctx_new(void)
{
    pbw_ctx *ctx = (pbw_ctx *)calloc(1, sizeof(*ctx));

    if (ctx == NULL) {
        return NULL;
    }
    ctx->arena.chunk_size = 4096;
    pbw_buf_init(&ctx->out);
    return ctx;
}

void
pbw_ctx_free(pbw_ctx *ctx)
{
    if (ctx == NULL) {
        return;
    }
    pbw_arena_free(&ctx->arena);
    pbw_arena_free(&ctx->build);
    pbw_buf_free(&ctx->out);
    free(ctx);
}

void
pbw_ctx_reset(pbw_ctx *ctx)
{
    if (ctx == NULL) {
        return;
    }
    pbw_arena_reset(&ctx->arena);
    pbw_arena_reset(&ctx->build);
    ctx->out.len = 0;
}

size_t
pbw_ctx_used(const pbw_ctx *ctx)
{
    return ctx == NULL ? 0 : pbw_arena_used(&ctx->arena);
}

pbw_status
pbw_decode_ctx(pbw_ctx *ctx, const pbw_descriptor *desc, const void *buf,
               size_t len, pbw_message **out)
{
    if (ctx == NULL) {
        return PBW_ERR_INVALID;
    }
    return pbw_decode_arena(&ctx->arena, desc, buf, len, out);
}

pbw_status
pbw_encode_ctx(pbw_ctx *ctx, const pbw_message *msg, const uint8_t **out,
               size_t *len)
{
    pbw_status status;

    if (ctx == NULL || out == NULL || len == NULL) {
        return PBW_ERR_INVALID;
    }
    status = pbw_encode_buf(&ctx->out, msg);
    if (status != PBW_OK) {
        *out = NULL;
        *len = 0;
        return status;
    }
    *out = ctx->out.data != NULL ? ctx->out.data : (const uint8_t *)"";
    *len = ctx->out.len;
    return PBW_OK;
}

/* Arena-backed constructors for caller-built messages: every allocation is a
 * pointer bump in ctx->build, and pbw_ctx_reset() releases the whole tree. */
pbw_message *
pbw_message_new_ctx(pbw_ctx *ctx, const pbw_descriptor *desc)
{
    return ctx == NULL ? NULL : pbw_message_new_a(&ctx->build, desc);
}

pbw_value *
pbw_value_new_uint_ctx(pbw_ctx *ctx, uint64_t bits)
{
    return ctx == NULL ? NULL : pbw_value_new_uint_a(&ctx->build, bits);
}

pbw_value *
pbw_value_new_int_ctx(pbw_ctx *ctx, int64_t value)
{
    return pbw_value_new_uint_ctx(ctx, (uint64_t)value);
}

pbw_value *
pbw_value_new_bool_ctx(pbw_ctx *ctx, int flag)
{
    return pbw_value_new_uint_ctx(ctx, flag ? 1u : 0u);
}

pbw_value *
pbw_value_new_float_ctx(pbw_ctx *ctx, float number)
{
    uint32_t bits;

    memcpy(&bits, &number, sizeof(bits));
    return pbw_value_new_uint_ctx(ctx, bits);
}

pbw_value *
pbw_value_new_double_ctx(pbw_ctx *ctx, double number)
{
    uint64_t bits;

    memcpy(&bits, &number, sizeof(bits));
    return pbw_value_new_uint_ctx(ctx, bits);
}

pbw_value *
pbw_value_new_bytes_ctx(pbw_ctx *ctx, const void *data, size_t len)
{
    return ctx == NULL ? NULL : pbw_value_new_bytes_a(&ctx->build, data, len);
}

pbw_value *
pbw_value_new_message_ctx(pbw_ctx *ctx, pbw_message *msg)
{
    return ctx == NULL ? NULL : pbw_value_new_message_a(&ctx->build, msg);
}
