#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    void *base;
    void *limit;
    size_t max_blocks;
    size_t split_thresh;
    size_t alignment;
} ta_cfg_t;

void ta_init(const ta_cfg_t *cfg);
void *ta_alloc(const ta_cfg_t *cfg, size_t num);
void *ta_calloc(const ta_cfg_t *cfg, size_t num, size_t size);
size_t ta_getsize(const ta_cfg_t *cfg, void *ptr);
void *ta_realloc(const ta_cfg_t *cfg, void *ptr, size_t num);
bool ta_free(const ta_cfg_t *cfg, void *ptr);

size_t ta_num_free(const ta_cfg_t *cfg);
size_t ta_num_used(const ta_cfg_t *cfg);
size_t ta_num_fresh(const ta_cfg_t *cfg);
bool ta_check(const ta_cfg_t *cfg);

#ifdef __cplusplus
}
#endif
