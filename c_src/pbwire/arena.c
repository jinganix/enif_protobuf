/* Bump allocator backing pbw_ctx: allocation is a pointer add, releasing is a
 * pointer reset. Chunks double in size so a steady decode workload stops
 * calling malloc entirely after the first few messages. */

#include "internal.h"

#define PBW_ARENA_MIN_CHUNK 4096u

static size_t
align_up(size_t size)
{
    /* 8 bytes keeps every struct in this library naturally aligned. */
    return (size + 7u) & ~(size_t)7u;
}

static int
chunk_add(pbw_arena *arena, size_t need)
{
    size_t cap = arena->chunk_size;
    pbw_arena_chunk *chunk;

    if (cap < need) {
        cap = need;
    }
    chunk = (pbw_arena_chunk *)malloc(sizeof(*chunk) + cap);
    if (chunk == NULL) {
        return 0;
    }
    chunk->next = arena->head;
    chunk->used = 0;
    chunk->cap = cap;
    arena->head = chunk;
    /* Grow the next chunk so a workload that needs more keeps its balance: the
     * chunk count stays small while per-call allocation stays O(1). */
    arena->chunk_size = cap < (size_t)1 << 20 ? cap * 2 : cap;
    return 1;
}

void *
pbw_arena_alloc_slow(pbw_arena *arena, size_t size)
{
    pbw_arena_chunk *chunk;
    void *ptr;

    size = align_up(size == 0 ? 1 : size);
    chunk = arena->head;
    if (chunk == NULL || chunk->cap - chunk->used < size) {
        if (!chunk_add(arena, size)) {
            return NULL;
        }
        chunk = arena->head;
    }
    ptr = (char *)(chunk + 1) + chunk->used;
    chunk->used += size;
    return ptr;
}

void *
pbw_arena_calloc(pbw_arena *arena, size_t size)
{
    void *ptr = pbw_arena_alloc(arena, size);

    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void
pbw_arena_reset(pbw_arena *arena)
{
    /* Chunks are pushed at the head and grow, so the head is the largest one:
     * keep it and drop the rest. A workload that needs the same amount of space
     * every time then stops calling malloc entirely. */
    pbw_arena_chunk *head = arena->head;
    pbw_arena_chunk *chunk;

    if (head == NULL) {
        return;
    }
    for (chunk = head->next; chunk != NULL;) {
        pbw_arena_chunk *next = chunk->next;
        free(chunk);
        chunk = next;
    }
    head->next = NULL;
    head->used = 0;
}

void
pbw_arena_free(pbw_arena *arena)
{
    pbw_arena_chunk *chunk = arena->head;

    while (chunk != NULL) {
        pbw_arena_chunk *next = chunk->next;
        free(chunk);
        chunk = next;
    }
    arena->head = NULL;
    arena->chunk_size = PBW_ARENA_MIN_CHUNK;
}

size_t
pbw_arena_used(const pbw_arena *arena)
{
    const pbw_arena_chunk *chunk;
    size_t used = 0;

    for (chunk = arena->head; chunk != NULL; chunk = chunk->next) {
        used += chunk->used;
    }
    return used;
}
