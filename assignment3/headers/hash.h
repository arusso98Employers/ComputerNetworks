#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>

struct sha1sum_ctx;

struct sha1sum_ctx * sha1sum_create(const uint8_t *salt, size_t len);
int sha1sum_update(struct sha1sum_ctx*, const uint8_t *payload, size_t len);
int sha1sum_finish(struct sha1sum_ctx*, const uint8_t *payload, size_t len, uint8_t *out);
int sha1sum_reset(struct sha1sum_ctx*);
int sha1sum_destroy(struct sha1sum_ctx*);

#endif

