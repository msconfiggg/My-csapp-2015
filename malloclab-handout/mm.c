/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4                         /* 头部/脚部的大小 */
#define DSIZE 8                         /* 双字 */
#define CHUNKSIZE (1 << 9)              /* 扩展堆时的默认大小 */

#define ALLOCATED 1                     /* 分配位 */
#define FREE 0                          /* 未分配位 */

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* 设置头部和脚部的值, 块大小+分配位 */
#define PACK(size, alloc) ((size) | (alloc))

/* 读写指针p的位置 */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) ((*(unsigned int *)(p)) = (val))

/* 获得block的Size或Alloc信息 */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* 获取block的header或footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 注意区别前一个prev/前驱pred， 后一个next/后继succ*/
/* 获得给定block块的后一个块或前一个块 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* 存储块链表后继前驱地址的地址 */
#define SUCC(bp) ((char*) bp)
#define PRED(bp) ((char*)(bp) + WSIZE)

/* 获取块链表后继前驱的地址 */
#define SUCC_BLKP(bp) (GET(SUCC(bp)))
#define PRED_BLKP(bp) (GET(PRED(bp)))

/* 大小类总类数 */
#define SEG_SIZE 15

/* block结构 */
/*                    bp                   Data(Variable Size)                                      */
/* | Header (4 bytes) | Succ(4 bytes) Pred(4 bytes) FormalData (Data Size - 8) | Footer (4 bytes) | */
/* 函数参数size表示原始大小，asize表示对齐后大小 */


char *heap_list;                             /* 指向空闲块的起始位置 */
char *heap_head;                             /* 指向整个堆的起始位置 */


int get_seg(size_t size);                    /* 根据size获取对应的大小类 */
void *extend_heap(size_t asize);             /* 扩展堆 */
void *coalesce(void *bp);                    /* 合并空闲块 */
void insert_block(void *bp);                 /* 插入空闲块 */
void delete_block(void *bp);                 /* 删除空闲块 */
size_t align_size(size_t size);              /* 对齐size */
void *find_fit(size_t asize, int seg_index); /* 查找合适的空闲块 */
void *place(void *bp, size_t asize);         /* 分配空闲块 */


/* 
 * get_seg - 由大小获得大小类序号
 */
int get_seg(size_t size) {
    size_t temp, shift;
    temp = (size > 0xFFFF) << 4; size >>= temp;
    shift = (size > 0xFF)  << 3; size >>= shift; temp |= shift;
    shift = (size > 0xF)   << 2; size >>= shift; temp |= shift;
    shift = (size > 0x3)   << 1; size >>= shift; temp |= shift;
                                                 temp |= (size >> 1);

    int x = (int) temp - 4;                  /* 从2^4开始 */
    if (x < 0) {
        x = 0;
    }

    if (x >= SEG_SIZE) {
        x = SEG_SIZE - 1;
    }

    return x;
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_list = mem_sbrk((SEG_SIZE + 3) * WSIZE)) == (void *) -1) {    /* 初始化堆，+3是因为除了空闲块以外还需要序言块、结尾块，空闲块头指针块*/
        return -1;
    }

    for (int i = 0; i < SEG_SIZE; i++) {
        PUT(heap_list + i * WSIZE, NULL);                       /* 初始化空闲块头指针 */
    }

    /* 分配块 */
    PUT(heap_list + (SEG_SIZE + 0) * WSIZE, PACK(DSIZE, ALLOCATED));        /* 序言块头部 */
    PUT(heap_list + (SEG_SIZE + 1) * WSIZE, PACK(DSIZE, ALLOCATED));        /* 序言块尾部 */
    PUT(heap_list + (SEG_SIZE + 2) * WSIZE, PACK(0, ALLOCATED));            /* 结尾块头部 */

    /* 对齐 */
    heap_head = heap_list;                                                  /* 堆头部 */
    heap_list += (SEG_SIZE + 2) * WSIZE;                                    /* 指向结尾块头部即空闲块的起始位置 */

    if (extend_heap(CHUNKSIZE) == NULL) {
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }
    
    size_t asize = align_size(size);
    size_t extendsize;
    char *bp;

    if ((bp = find_fit(asize, get_seg(asize))) != NULL) {
        return place(bp, asize);
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize)) == NULL) {
        return NULL;
    }

    return place(bp, asize);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    char *bp = ptr;
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, FREE));
    PUT(FTRP(bp), PACK(size, FREE));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return mm_malloc(size);
    } else if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t asize = align_size(size);
    size_t old_size = GET_SIZE(HDRP(ptr));
    char *newptr;

    if (asize == old_size) {
        return ptr;
    }

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    size_t total_size = old_size;
    char *next_bp = NEXT_BLKP(ptr);

    if (prev_alloc && !next_alloc && ((old_size + next_size) >= asize)) {       /* 后空闲 */
        total_size += next_size;
        delete_block(next_bp);
        PUT(HDRP(ptr), PACK(total_size, ALLOCATED));
        PUT(FTRP(ptr), PACK(total_size, ALLOCATED));
        place(ptr, total_size);
    } else if (!next_size && asize >= old_size) {                               /* 没有后继块，且空间不够 */
        size_t extend_size = asize - old_size;
        if ((long) (mem_sbrk(extend_size)) == -1) {
            return NULL;
        }

        PUT(HDRP(ptr), PACK(total_size + extend_size, ALLOCATED));
        PUT(FTRP(ptr), PACK(total_size + extend_size, ALLOCATED));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, ALLOCATED));
        place(ptr, asize);
    } else {
        newptr = mm_malloc(asize);
        if (newptr == NULL) {
            return NULL;
        }

        memcpy(newptr, ptr, MIN(old_size, size));
        mm_free(ptr);
        return newptr;
    }

    return ptr;
}

/* 
 * extend_heap - 扩展堆，对齐size，并执行合并，返回bp指针
 */
void *extend_heap(size_t asize) {
    char *bp;
    if ((long) (bp = mem_sbrk(asize)) == -1) {
        return NULL;
    }

    /* 新建空闲块，同时更新结尾块 */
    PUT(HDRP(bp), PACK(asize, FREE));                /* 新建空闲块头部 */
    PUT(FTRP(bp), PACK(asize, FREE));                /* 新建空闲块尾部 */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, ALLOCATED));    /* 更新结尾块头部 */

    return coalesce(bp);
}

/* 
 * coalesce - 对bp指向块进行前后合并，返回bp指针
 */
void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {                 /* 前后都不空闲 */
        insert_block(bp);
        return bp;
    } else if (prev_alloc && !next_alloc) {         /* 后空闲 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, FREE));
        PUT(FTRP(bp), PACK(size, FREE));            /* 注意这里的处理，FTRP是根据bp位置和size计算的，而size则取的是HDRP的记录 */
        PUT(PRED(bp), NULL);
        PUT(SUCC(bp), NULL);
    } else if (!prev_alloc && next_alloc) {         /* 前空闲 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        delete_block(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, FREE));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, FREE)); /* 注意这里的处理，FTRP是根据bp位置和size计算的，而size则取的是HDRP的记录 */
        bp = PREV_BLKP(bp);
        PUT(PRED(bp), NULL);
        PUT(SUCC(bp), NULL);
    } else {                                        /* 前后都空闲 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_block(PREV_BLKP(bp));
        delete_block(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, FREE));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, FREE));
        bp = PREV_BLKP(bp);
        PUT(PRED(bp), NULL);
        PUT(SUCC(bp), NULL);
    }

    insert_block(bp);
    return bp;
}

/* 
 * insert_block - 插入块
 */
void insert_block(void *bp) {
    int seg_index = get_seg(GET_SIZE(HDRP(bp)));
    char *head = heap_head + seg_index * WSIZE;             /* 该类的头指针 */
    void *succ = head;

    while (SUCC_BLKP(succ) != NULL) {
        succ = (char *) SUCC_BLKP(succ);
        if ((unsigned int) succ >= (unsigned int) bp) {     /* 按照地址顺序插入pred<->bp<->succ */
            char *pred = PRED_BLKP(succ);
            PUT(SUCC(bp), succ);
            PUT(PRED(bp), pred);
            PUT(SUCC(pred), bp);
            PUT(PRED(succ), bp);
            return;
        }
    }

    PUT(SUCC(bp), NULL);                                    /* 该类无其他块或新的块按地址顺序应该被插入到最后 */
    PUT(PRED(bp), succ);
    PUT(SUCC(succ), bp);
}

/* 
 * delete_block - 删除块
 */
void delete_block(void *bp) {
    int seg_index = get_seg(GET_SIZE(HDRP(bp)));
    char *head = heap_head + seg_index * WSIZE;

    if (SUCC_BLKP(bp) && PRED_BLKP(bp)) {
        PUT(SUCC(PRED_BLKP(bp)), SUCC_BLKP(bp));
        PUT(PRED(SUCC_BLKP(bp)), PRED_BLKP(bp));
    } else if (PRED_BLKP(bp)) {
        PUT(SUCC(PRED_BLKP(bp)), NULL);
    }

    PUT(PRED(bp), NULL);
    PUT(SUCC(bp), NULL);
}

/* 
 * align_size - 对块大小进行对齐，留出首尾空间，返回真实分配大小
 */
size_t align_size(size_t size) {
    if (size <= DSIZE) {
        return 2 * DSIZE;
    } else {
        return DSIZE * ((size + DSIZE * 2 - 1) / DSIZE);
    }
}

/* 
 * find_fit - 寻找适配，返回适配到的空闲块指针,使用首次适配
 */
void *find_fit(size_t asize, int seg_index) {
    while (seg_index < SEG_SIZE) {
        char *head = heap_head + seg_index * WSIZE;
        char *bp = (char *) SUCC_BLKP(head);
        while (bp) {
            if ((size_t) (GET_SIZE(HDRP(bp))) >= asize) {
                return bp;
            }

            bp = (char *) SUCC_BLKP(bp);
        }

        seg_index++;
    }

    return NULL;
}

/* 
 * place - 放置块，若剩余后部大于一个最小块大小(16 bytes)就进行分割
 */
void *place(void *bp, size_t asize) {
    size_t blk_size = GET_SIZE(HDRP(bp));
    if (!GET_ALLOC(HDRP(bp))) {
        delete_block(bp);
    }

    if ((blk_size - asize) >= 2 * DSIZE) {               /* 判断分割后剩余部分能不能成为一个块 */
        if (asize > 64) {                                /* 如果用户申请大于64bytes，则把后面部分给用户，前面部分作为新空闲块*/
            PUT(HDRP(bp), PACK(blk_size - asize, FREE));
            PUT(FTRP(bp), PACK(blk_size - asize, FREE));
            PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, ALLOCATED));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, ALLOCATED));
            insert_block(bp);
            return NEXT_BLKP(bp);
        } else {                                        /* 如果小于64bytes，则把前面部分给用户，后面部分作为新空闲块 */
            PUT(HDRP(bp), PACK(asize, ALLOCATED));
            PUT(FTRP(bp), PACK(asize, ALLOCATED));
            PUT(HDRP(NEXT_BLKP(bp)), PACK(blk_size - asize, FREE));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(blk_size - asize, FREE));
            coalesce(NEXT_BLKP(bp));                    /* 将新产生的空闲块与后面的块合并 */
        }
    } else {                                            /* 如果剩余部分不足以成为一个块，则直接分配给用户 */
        PUT(HDRP(bp), PACK(blk_size, ALLOCATED));
        PUT(FTRP(bp), PACK(blk_size, ALLOCATED));
    }

    return bp;
}
