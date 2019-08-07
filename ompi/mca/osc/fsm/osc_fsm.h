/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012      Sandia National Laboratories.  All rights reserved.
 * Copyright (c) 2014-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2015-2017 Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016-2017 IBM Corporation. All rights reserved.
 * Copyright (c) 2019      Hewlett Packard Enterprise. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef OSC_FSM_FSM_H
#define OSC_FSM_FSM_H

#include "opal/mca/shmem/base/base.h"
#include "ompi/communicator/communicator.h"
#include <pthread.h>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_ext_zhpe.h>

#define CACHELINE_SZ 64

#ifdef __aarch64__
static inline void smp_wmb(void)
{
    asm volatile("dsb st":::"memory");
}
#endif

#ifdef __x86_64__
static inline void smp_wmb(void)
{
    asm volatile("sfence":::"memory");
}
#endif

typedef uint64_t __attribute__((aligned(CACHELINE_SZ))) aligned_uint64_t;
typedef uint32_t __attribute__((aligned(CACHELINE_SZ))) aligned_uint32_t;

#if OPAL_HAVE_ATOMIC_MATH_64

typedef uint64_t __attribute__((aligned(CACHELINE_SZ))) osc_aligned_fsm_post_type_t;
typedef uint64_t osc_fsm_post_type_t;
typedef opal_atomic_int64_t osc_fsm_atomic_type_t;
typedef opal_atomic_int64_t __attribute__((aligned(CACHELINE_SZ))) osc_fsm_aligned_atomic_type_t;
#define OSC_FSM_POST_BITS 6
#define OSC_FSM_POST_MASK 0x3f

#else

typedef uint32_t __attribute__((aligned(CACHELINE_SZ))) osc_aligned_fsm_post_type_t;
typedef uint32_t osc_fsm_post_type_t;
typedef opal_atomic_uint32_t osc_fsm_atomic_type_t;
typedef opal_atomic_uint32_t __attribute__((aligned(CACHELINE_SZ))) osc_aligned_fsm_atomic_type_t;
#define OSC_FSM_POST_BITS 5
#define OSC_FSM_POST_MASK 0x1f

#endif

#define OSC_FSM_VERBOSE(x, s) do { \
    OPAL_OUTPUT_VERBOSE((x, ompi_osc_base_framework.framework_output, "%s:%d: " s,__FILE__, __LINE__)); \
} while (0)

#define OSC_FSM_VERBOSE_F(x, s, ...) do { \
    OPAL_OUTPUT_VERBOSE((x, ompi_osc_base_framework.framework_output, "%s:%d: " s,__FILE__, __LINE__ , ##__VA_ARGS__)); \
} while (0)

/* data shared across all peers */
struct ompi_osc_fsm_global_state_t {
    int use_barrier_for_fence;

    pthread_mutex_t mtx;
    pthread_cond_t cond;

    int sense;
    int32_t count;
} __attribute__((aligned(CACHELINE_SZ)));
typedef struct ompi_osc_fsm_global_state_t ompi_osc_fsm_global_state_t;

/* this is data exposed to remote nodes */
struct ompi_osc_fsm_lock_t {
    osc_fsm_aligned_atomic_type_t counter;
    osc_fsm_aligned_atomic_type_t write;
    osc_fsm_aligned_atomic_type_t read;
} __attribute__((aligned(CACHELINE_SZ)));
typedef struct ompi_osc_fsm_lock_t ompi_osc_fsm_lock_t;

struct ompi_osc_fsm_node_state_t {
    osc_fsm_aligned_atomic_type_t complete_count;
    ompi_osc_fsm_lock_t lock;
    osc_fsm_aligned_atomic_type_t accumulate_lock;
} __attribute__((aligned(CACHELINE_SZ)));
typedef struct ompi_osc_fsm_node_state_t ompi_osc_fsm_node_state_t __attribute__((aligned(CACHELINE_SZ)));

struct ompi_osc_fsm_component_t {
    ompi_osc_base_component_t super;

    char *backing_directory;
};
typedef struct ompi_osc_fsm_component_t ompi_osc_fsm_component_t;
OMPI_DECLSPEC extern ompi_osc_fsm_component_t mca_osc_fsm_component;

enum ompi_osc_fsm_locktype_t {
    lock_none = 0,
    lock_nocheck,
    lock_exclusive,
    lock_shared
};

struct ompi_osc_fsm_module_t {
    ompi_osc_base_module_t super;
    struct ompi_communicator_t *comm;
    int flavor;
    void *my_segment_base;
    struct fid_mr *mr;
    uint64_t mr_key;
    bool noncontig;

    /* FIXME: ZHPE related */
    struct fi_zhpe_mmap_desc **mdesc;
    struct fi_zhpe_ext_ops_v1 *ext_ops;

    size_t *sizes;
    void **bases;
    int *disp_units;

    ompi_group_t *start_group;
    ompi_group_t *post_group;

    int my_sense;

    enum ompi_osc_fsm_locktype_t *outstanding_locks;

    /* exposed data */
    ompi_osc_fsm_global_state_t *global_state;
    ompi_osc_fsm_node_state_t *my_node_state;
    ompi_osc_fsm_node_state_t **node_states;

    osc_aligned_fsm_post_type_t **posts;

    opal_mutex_t lock;
};
typedef struct ompi_osc_fsm_module_t ompi_osc_fsm_module_t;

static inline void osc_fsm_flush(ompi_osc_fsm_module_t * module, int target, void * addr, size_t len, bool fence) {
    if(ompi_comm_rank(module->comm) == target){
        //TODO
    } else {
        module->ext_ops->commit(module->mdesc[target], addr, len, fence);
    }
}

static inline void osc_fsm_flush_window(ompi_osc_fsm_module_t * module, int target, bool fence) {
    if(ompi_comm_rank(module->comm) == target){
        //TODO
    } else {
        osc_fsm_flush(module, target, module->bases[target], module->sizes[target], fence);
    }
}

static inline void osc_fsm_invalidate(ompi_osc_fsm_module_t * module, int target, void * addr, size_t len, bool fence) {
    if(ompi_comm_rank(module->comm) == target){
        //TODO
    } else {
        //TODO
    }
}

static inline void osc_fsm_invalidate_window(ompi_osc_fsm_module_t * module, int target, bool fence) {
    if(ompi_comm_rank(module->comm) == target){
        //TODO
    } else {
        //TODO
    }
}

int ompi_osc_fsm_shared_query(struct ompi_win_t *win, int rank, size_t *size, int *disp_unit, void *baseptr);

int ompi_osc_fsm_attach(struct ompi_win_t *win, void *base, size_t len);
int ompi_osc_fsm_detach(struct ompi_win_t *win, const void *base);

int ompi_osc_fsm_free(struct ompi_win_t *win);

int ompi_osc_fsm_put(const void *origin_addr,
                          int origin_count,
                          struct ompi_datatype_t *origin_dt,
                          int target,
                          ptrdiff_t target_disp,
                          int target_count,
                          struct ompi_datatype_t *target_dt,
                          struct ompi_win_t *win);

int ompi_osc_fsm_get(void *origin_addr,
                          int origin_count,
                          struct ompi_datatype_t *origin_dt,
                          int target,
                          ptrdiff_t target_disp,
                          int target_count,
                          struct ompi_datatype_t *target_dt,
                          struct ompi_win_t *win);

int ompi_osc_fsm_accumulate(const void *origin_addr,
                                 int origin_count,
                                 struct ompi_datatype_t *origin_dt,
                                 int target,
                                 ptrdiff_t target_disp,
                                 int target_count,
                                 struct ompi_datatype_t *target_dt,
                                 struct ompi_op_t *op,
                                 struct ompi_win_t *win);

int ompi_osc_fsm_compare_and_swap(const void *origin_addr,
                                       const void *compare_addr,
                                       void *result_addr,
                                       struct ompi_datatype_t *dt,
                                       int target,
                                       ptrdiff_t target_disp,
                                       struct ompi_win_t *win);

int ompi_osc_fsm_fetch_and_op(const void *origin_addr,
                                   void *result_addr,
                                   struct ompi_datatype_t *dt,
                                   int target,
                                   ptrdiff_t target_disp,
                                   struct ompi_op_t *op,
                                   struct ompi_win_t *win);

int ompi_osc_fsm_get_accumulate(const void *origin_addr,
                                     int origin_count,
                                     struct ompi_datatype_t *origin_datatype,
                                     void *result_addr,
                                     int result_count,
                                     struct ompi_datatype_t *result_datatype,
                                     int target_rank,
                                     MPI_Aint target_disp,
                                     int target_count,
                                     struct ompi_datatype_t *target_datatype,
                                     struct ompi_op_t *op,
                                     struct ompi_win_t *win);

int ompi_osc_fsm_rput(const void *origin_addr,
                           int origin_count,
                           struct ompi_datatype_t *origin_dt,
                           int target,
                           ptrdiff_t target_disp,
                           int target_count,
                           struct ompi_datatype_t *target_dt,
                           struct ompi_win_t *win,
                           struct ompi_request_t **request);

int ompi_osc_fsm_rget(void *origin_addr,
                           int origin_count,
                           struct ompi_datatype_t *origin_dt,
                           int target,
                           ptrdiff_t target_disp,
                           int target_count,
                           struct ompi_datatype_t *target_dt,
                           struct ompi_win_t *win,
                           struct ompi_request_t **request);

int ompi_osc_fsm_raccumulate(const void *origin_addr,
                                  int origin_count,
                                  struct ompi_datatype_t *origin_dt,
                                  int target,
                                  ptrdiff_t target_disp,
                                  int target_count,
                                  struct ompi_datatype_t *target_dt,
                                  struct ompi_op_t *op,
                                  struct ompi_win_t *win,
                                  struct ompi_request_t **request);

int ompi_osc_fsm_rget_accumulate(const void *origin_addr,
                                      int origin_count,
                                      struct ompi_datatype_t *origin_datatype,
                                      void *result_addr,
                                      int result_count,
                                      struct ompi_datatype_t *result_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      int target_count,
                                      struct ompi_datatype_t *target_datatype,
                                      struct ompi_op_t *op,
                                      struct ompi_win_t *win,
                                      struct ompi_request_t **request);

int ompi_osc_fsm_fence(int assert, struct ompi_win_t *win);

int ompi_osc_fsm_start(struct ompi_group_t *group,
                            int assert,
                            struct ompi_win_t *win);

int ompi_osc_fsm_complete(struct ompi_win_t *win);

int ompi_osc_fsm_post(struct ompi_group_t *group,
                           int assert,
                           struct ompi_win_t *win);

int ompi_osc_fsm_wait(struct ompi_win_t *win);

int ompi_osc_fsm_test(struct ompi_win_t *win,
                           int *flag);

int ompi_osc_fsm_lock(int lock_type,
                           int target,
                           int assert,
                           struct ompi_win_t *win);

int ompi_osc_fsm_unlock(int target,
                             struct ompi_win_t *win);


int ompi_osc_fsm_lock_all(int assert,
                               struct ompi_win_t *win);

int ompi_osc_fsm_unlock_all(struct ompi_win_t *win);

int ompi_osc_fsm_sync(struct ompi_win_t *win);

int ompi_osc_fsm_flush(int target,
                            struct ompi_win_t *win);
int ompi_osc_fsm_flush_all(struct ompi_win_t *win);
int ompi_osc_fsm_flush_local(int target,
                                  struct ompi_win_t *win);
int ompi_osc_fsm_flush_local_all(struct ompi_win_t *win);

int ompi_osc_fsm_set_info(struct ompi_win_t *win, struct opal_info_t *info);
int ompi_osc_fsm_get_info(struct ompi_win_t *win, struct opal_info_t **info_used);

#endif
