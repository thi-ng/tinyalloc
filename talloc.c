#include "talloc.h"

extern void print_s(char *);
extern void print_i(size_t);
extern void print_f(float);

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
    size_t limit;  // heap limit
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

static void compress() {
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

bool talloc_init() {
    heap->free   = NULL;
    heap->used   = NULL;
    heap->avail  = heap->blocks;
    heap->top    = TA_BASE + sizeof(Heap);
    heap->limit  = TA_HEAP_LIMIT;
    Block *block = heap->blocks;
    for (size_t i = TA_HEAP_BLOCKS - 1; i > 0; i--) {
        block->next = block + 1;
        block++;
    }
    return true;
}

bool talloc_free(void *free) {
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
            compress();
            return true;
        }
        prev  = block;
        block = block->next;
    }
    return false;
}

void *talloc(size_t num) {
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
                if (excess >= TA_ALIGN) {
                    ptr->size    = num;
                    Block *split = heap->avail;
                    heap->avail  = split->next;
                    split->addr  = ptr->addr + num;
                    print_s("split");
                    print_i((size_t)split->addr);
                    split->size = excess;
                    insert_block(split);
                    compress();
                }
            }
            return ptr->addr;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    // no matching free blocks
    // see if any other blocks available
    size_t new_top = top + num;
    if (heap->avail != NULL && new_top <= heap->limit) {
        ptr         = heap->avail;
        heap->avail = ptr->next;
        ptr->addr   = (void *)top;
        ptr->next   = heap->used;
        ptr->size   = num;
        heap->used  = ptr;
        heap->top   = new_top;
        return ptr->addr;
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

size_t talloc_num_free() {
    return count_blocks(heap->free);
}

size_t talloc_num_used() {
    return count_blocks(heap->used);
}

size_t talloc_num_avail() {
    return count_blocks(heap->avail);
}

bool talloc_check() {
    return TA_HEAP_BLOCKS ==
           talloc_num_free() + talloc_num_used() + talloc_num_avail();
}
