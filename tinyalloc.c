#include "tinyalloc.h"

#ifdef NDEBUG
#define print_s(X)
#define print_i(X)
#else
extern void print_s(char *);
extern void print_i(size_t);
#endif

typedef struct Block Block;

struct Block {
    void *addr;
    Block *next;
    size_t size;
};

typedef struct {
    Block *free;   // first free block
    Block *used;   // first used block
    Block *avail;  // first available blank block
    size_t top;    // top free addr
    Block blocks[TA_HEAP_BLOCKS];
} Heap;

static Heap *heap = (Heap *)TA_BASE;

/**
 * Insert block into free list, sorted by addr.
 */
static void insert_block(Block *block) {
    Block *ptr  = heap->free;
    Block *prev = NULL;
    while (ptr != NULL) {
        if ((size_t)block->addr <= (size_t)ptr->addr) {
            print_s("insert");
            print_i((size_t)ptr);
            break;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    if (prev != NULL) {
        if (ptr == NULL) {
            print_s("new tail");
        }
        prev->next = block;
    } else {
        print_s("new head");
        heap->free = block;
    }
    block->next = ptr;
}

static void release_blocks(Block *scan, Block *to) {
    Block *scan_next;
    while (scan != to) {
        print_s("release");
        print_i((size_t)scan);
        scan_next   = scan->next;
        scan->next  = heap->avail;
        scan->addr  = 0;
        scan->size  = 0;
        heap->avail = scan;
        scan        = scan_next;
    }
}

static void compact() {
    Block *ptr  = heap->free;
    Block *prev = NULL;
    Block *scan;
    while (ptr != NULL) {
        prev        = ptr;
        scan        = ptr->next;
        size_t base = (size_t)ptr->addr;
        while (scan != NULL &&
               (size_t)prev->addr + prev->size == (size_t)scan->addr) {
            print_s("merge");
            print_i((size_t)scan);
            prev = scan;
            scan = scan->next;
        }
        if (prev != ptr) {
            size_t new_size = prev->addr + prev->size - ptr->addr;
            print_s("new size");
            print_i(new_size);
            ptr->size   = new_size;
            Block *next = prev->next;
            // make merged blocks available
            release_blocks(ptr->next, prev->next);
            // relink
            ptr->next = next;
            ptr       = next;
        } else {
            ptr = ptr->next;
        }
    }
}

bool ta_init() {
    heap->free   = NULL;
    heap->used   = NULL;
    heap->avail  = heap->blocks;
    heap->top    = TA_HEAP_START;
    Block *block = heap->blocks;
    size_t i = TA_HEAP_BLOCKS - 1;
    while(i--) {
        block->next = block + 1;
        block++;
    }
    return true;
}

bool ta_free(void *free) {
    Block *block = heap->used;
    Block *prev  = NULL;
    while (block != NULL) {
        if (free == block->addr) {
            if (prev) {
                prev->next = block->next;
            } else {
                heap->used = block->next;
            }
            insert_block(block);
            compact();
            return true;
        }
        prev  = block;
        block = block->next;
    }
    return false;
}

static Block *alloc_block(size_t num) {
    Block *ptr  = heap->free;
    Block *prev = NULL;
    size_t top  = heap->top;
    num         = (num + TA_ALIGN - 1) & -TA_ALIGN;
    while (ptr != NULL) {
        const int is_top = (size_t)ptr->addr + ptr->size >= top;
        if (is_top || ptr->size >= num) {
            if (prev != NULL) {
                prev->next = ptr->next;
            } else {
                heap->free = ptr->next;
            }
            ptr->next  = heap->used;
            heap->used = ptr;
            if (is_top) {
                print_s("resize top block");
                ptr->size = num;
                heap->top = (size_t)ptr->addr + num;
            } else if (heap->avail != NULL) {
                size_t excess = ptr->size - num;
                if (excess >= TA_SIZE_THRESHOLD) {
                    ptr->size    = num;
                    Block *split = heap->avail;
                    heap->avail  = split->next;
                    split->addr  = ptr->addr + num;
                    print_s("split");
                    print_i((size_t)split->addr);
                    split->size = excess;
                    insert_block(split);
                    compact();
                }
            }
            return ptr;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    // no matching free blocks
    // see if any other blocks available
    size_t new_top = top + num;
    if (heap->avail != NULL && new_top <= TA_HEAP_LIMIT) {
        ptr         = heap->avail;
        heap->avail = ptr->next;
        ptr->addr   = (void *)top;
        ptr->next   = heap->used;
        ptr->size   = num;
        heap->used  = ptr;
        heap->top   = new_top;
        return ptr;
    }
    return NULL;
}

void *ta_alloc(size_t num) {
    Block *block = alloc_block(num);
    if (block != NULL) {
        return block->addr;
    }
    return NULL;
}

static void memset(void *ptr, uint8_t c, size_t num) {
    size_t *ptrw = (size_t *)ptr;
    size_t numw  = (num & -sizeof(size_t)) / sizeof(size_t);
    size_t cw    = c;
    cw           = (cw << 24) | (cw << 16) | (cw << 8) | cw;
    #ifdef __LP64__
    cw |= (cw << 32);
    #endif
    while (numw--) {
        *ptrw++ = cw;
    }
    num &= (sizeof(size_t) - 1);
    uint8_t *ptrb = (uint8_t *)ptrw;
    while (num--) {
        *ptrb++ = c;
    }
}

void *ta_calloc(size_t num, size_t size) {
    num *= size;
    Block *block = alloc_block(num);
    if (block != NULL) {
        memset(block->addr, 0, num);
        return block->addr;
    }
    return NULL;
}

static size_t count_blocks(Block *ptr) {
    size_t num = 0;
    while (ptr != NULL) {
        num++;
        ptr = ptr->next;
    }
    return num;
}

size_t ta_num_free() {
    return count_blocks(heap->free);
}

size_t ta_num_used() {
    return count_blocks(heap->used);
}

size_t ta_num_avail() {
    return count_blocks(heap->avail);
}

bool ta_check() {
    return TA_HEAP_BLOCKS == ta_num_free() + ta_num_used() + ta_num_avail();
}
