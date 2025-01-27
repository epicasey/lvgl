/**
 * @file lv_malloc_builtin.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_mem.h"
#if LV_USE_BUILTIN_MALLOC
#include "lv_malloc_builtin.h"
#include "lv_tlsf.h"
#include "lv_assert.h"
#include "lv_log.h"
#include "lv_ll.h"
#include "lv_math.h"

#ifdef LV_MEM_POOL_INCLUDE
    #include LV_MEM_POOL_INCLUDE
#endif

/*********************
 *      DEFINES
 *********************/
/*memset the allocated memories to 0xaa and freed memories to 0xbb (just for testing purposes)*/
#ifndef LV_MEM_ADD_JUNK
    #define LV_MEM_ADD_JUNK  0
#endif

#ifdef LV_ARCH_64
    #define MEM_UNIT         uint64_t
    #define ALIGN_MASK       0x7
#else
    #define MEM_UNIT         uint32_t
    #define ALIGN_MASK       0x3
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_mem_walker(void * ptr, size_t size, int used, void * user);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_tlsf_t tlsf;
static uint32_t cur_used;
static uint32_t max_used;
static lv_ll_t pool_ll;

/**********************
 *      MACROS
 **********************/
#if LV_LOG_TRACE_MEM
    #define MEM_TRACE(...) LV_LOG_TRACE(__VA_ARGS__)
#else
    #define MEM_TRACE(...)
#endif

#define _COPY(d, s) *d = *s; d++; s++;
#define _SET(d, v) *d = v; d++;
#define _REPEAT8(expr) expr expr expr expr expr expr expr expr

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the dyn_mem module (work memory and other variables)
 */
void lv_mem_init_builtin(void)
{
#if LV_MEM_ADR == 0
#ifdef LV_MEM_POOL_ALLOC
    tlsf = lv_tlsf_create_with_pool((void *)LV_MEM_POOL_ALLOC(LV_MEM_SIZE), LV_MEM_SIZE);
#else
    /*Allocate a large array to store the dynamically allocated data*/
    static LV_ATTRIBUTE_LARGE_RAM_ARRAY MEM_UNIT work_mem_int[LV_MEM_SIZE / sizeof(MEM_UNIT)];
    tlsf = lv_tlsf_create_with_pool((void *)work_mem_int, LV_MEM_SIZE);
#endif
#else
    tlsf = lv_tlsf_create_with_pool((void *)LV_MEM_ADR, LV_MEM_SIZE);
#endif
    _lv_ll_init(&pool_ll, sizeof(lv_pool_t));

    /*Record the first pool*/
    lv_pool_t * pool_p = _lv_ll_ins_tail(&pool_ll);
    LV_ASSERT_MALLOC(pool_p);
    *pool_p = lv_tlsf_get_pool(tlsf);

#if LV_MEM_ADD_JUNK
    LV_LOG_WARN("LV_MEM_ADD_JUNK is enabled which makes LVGL much slower");
#endif
}

void lv_mem_deinit_builtin(void)
{
    _lv_ll_clear(&pool_ll);
    lv_tlsf_destroy(tlsf);
    lv_mem_init_builtin();
}

lv_mem_builtin_pool_t lv_mem_builtin_add_pool(void * mem, size_t bytes)
{
    lv_mem_builtin_pool_t new_pool = lv_tlsf_add_pool(tlsf, mem, bytes);
    if(!new_pool) {
        LV_LOG_WARN("failed to add memory pool, address: %p, size: %zu", mem, bytes);
        return NULL;
    }

    lv_pool_t * pool_p = _lv_ll_ins_tail(&pool_ll);
    LV_ASSERT_MALLOC(pool_p);
    *pool_p = new_pool;

    return new_pool;
}

void lv_mem_builtin_remove_pool(lv_mem_builtin_pool_t pool)
{
    lv_pool_t * pool_p;
    _LV_LL_READ(&pool_ll, pool_p) {
        if(*pool_p == pool) {
            _lv_ll_remove(&pool_ll, pool_p);
            lv_free(pool_p);
            lv_tlsf_remove_pool(tlsf, pool);
            return;
        }
    }
    LV_LOG_WARN("invalid pool: %p", pool);
}

void lv_mem_monitor_builtin(lv_mem_monitor_t * mon_p)
{
    /*Init the data*/
    lv_memset(mon_p, 0, sizeof(lv_mem_monitor_t));
    MEM_TRACE("begin");

    lv_pool_t * pool_p;
    _LV_LL_READ(&pool_ll, pool_p) {
        lv_tlsf_walk_pool(*pool_p, lv_mem_walker, mon_p);
    }

    mon_p->used_pct = 100 - (100U * mon_p->free_size) / mon_p->total_size;
    if(mon_p->free_size > 0) {
        mon_p->frag_pct = mon_p->free_biggest_size * 100U / mon_p->free_size;
        mon_p->frag_pct = 100 - mon_p->frag_pct;
    }
    else {
        mon_p->frag_pct = 0; /*no fragmentation if all the RAM is used*/
    }

    mon_p->max_used = max_used;

    MEM_TRACE("finished");
}
#include <stdio.h>
void * lv_malloc_builtin(size_t size)
{
    cur_used += size;
    max_used = LV_MAX(cur_used, max_used);
    return lv_tlsf_malloc(tlsf, size);
}

void * lv_realloc_builtin(void * p, size_t new_size)
{
    return lv_tlsf_realloc(tlsf, p, new_size);
}

void lv_free_builtin(void * p)
{
#if LV_MEM_ADD_JUNK
    lv_memset(p, 0xbb, lv_tlsf_block_size(data));
#endif
    size_t size = lv_tlsf_free(tlsf, p);
    if(cur_used > size) cur_used -= size;
    else cur_used = 0;
}

lv_res_t lv_mem_test_builtin(void)
{
    if(lv_tlsf_check(tlsf)) {
        LV_LOG_WARN("failed");
        return LV_RES_INV;
    }

    lv_pool_t * pool_p;
    _LV_LL_READ(&pool_ll, pool_p) {
        if(lv_tlsf_check_pool(*pool_p)) {
            LV_LOG_WARN("pool failed");
            return LV_RES_INV;
        }
    }

    MEM_TRACE("passed");
    return LV_RES_OK;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_mem_walker(void * ptr, size_t size, int used, void * user)
{
    LV_UNUSED(ptr);

    lv_mem_monitor_t * mon_p = user;
    mon_p->total_size += size;
    if(used) {
        mon_p->used_cnt++;
    }
    else {
        mon_p->free_cnt++;
        mon_p->free_size += size;
        if(size > mon_p->free_biggest_size)
            mon_p->free_biggest_size = size;
    }
}
#endif /*LV_USE_BUILTIN_MALLOC*/
