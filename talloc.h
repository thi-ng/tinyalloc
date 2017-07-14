#include <stdbool.h>
#include <stddef.h>

#ifndef TA_ALIGN
#define TA_ALIGN 8
#endif

#ifndef TA_BASE
#define TA_BASE 0x400
#endif

#ifndef TA_HEAP_START
#define TA_HEAP_START 0x444
#endif

#ifndef TA_HEAP_LIMIT
#define TA_HEAP_LIMIT (1 << 24)
#endif

#ifndef TA_HEAP_BLOCKS
#define TA_HEAP_BLOCKS 0x4
#endif

bool talloc_init();
void *talloc(size_t num);
bool talloc_free(void *ptr);

size_t talloc_num_free();
size_t talloc_num_used();
size_t talloc_num_avail();
bool talloc_check();
