#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef TA_ALIGN
#define TA_ALIGN 8
#endif

#ifndef TA_BASE
#define TA_BASE 0x400
#endif

#ifndef TA_HEAP_START
#define TA_HEAP_START (TA_BASE + sizeof(Heap))
#endif

#ifndef TA_HEAP_LIMIT
#define TA_HEAP_LIMIT (1 << 24)
#endif

#ifndef TA_HEAP_BLOCKS
#define TA_HEAP_BLOCKS 256
#endif

#ifndef TA_SIZE_THRESHOLD
#define TA_SIZE_THRESHOLD 16
#endif

bool ta_init();
void *ta_alloc(size_t num);
void *ta_calloc(size_t num, size_t size);
bool ta_free(void *ptr);

size_t ta_num_free();
size_t ta_num_used();
size_t ta_num_avail();
bool ta_check();
