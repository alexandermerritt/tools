
#pragma once

#include <infiniband/verbs.h>

#define MR_LEN (1UL<<20)

struct ibv_t
{
    unsigned long long buf_va, buf_len;
    unsigned int buf_rkey, lid, qpn, psn;
};

struct verbs
{
    struct ibv_context *context;
    struct ibv_pd       *pd;
    struct ibv_comp_channel  *ch;
    struct ibv_cq *cq;
    struct ibv_mr *mr;
    struct ibv_qp *qp;
};

typedef struct verbs verbs_t;

int ib_open(verbs_t *verbs);
int ib_close(verbs_t *verbs);
int ib_mr(verbs_t *verbs, size_t len);
int ib_qp(verbs_t *verbs);
int ib_toinit(verbs_t *verbs);
int ib_tortr(verbs_t *verbs);
int ib_torts(verbs_t *verbs);
int ib_send(verbs_t *verbs, size_t offset, unsigned int len);
int ib_post_recv(verbs_t *verbs, size_t offset, unsigned int len);
int ib_poll(verbs_t *verbs);

#define mraddr(verbs, type) \
    ((type)(verbs)->mr->addr)
#define mraddrat(verbs, type, offset) \
    ((type)((uintptr_t)((verbs)->mr->addr) + (offset)))
#define mrlkey(verbs) \
    ((verbs)->mr->lkey)

