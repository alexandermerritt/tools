#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "ib.h"

unsigned int psn = 0xbeef, lid, qpn;

uintptr_t remote_addr;
unsigned int remote_key;

int ib_open(verbs_t *verbs)
{
    int num;

    if (!verbs)
        return -1;

    struct ibv_device **devs = ibv_get_device_list(&num);
    if (!devs)
        return -1;

    if (num < 1)
        return -1;

    verbs->context = ibv_open_device(devs[0]);
    if (!verbs->context)
        return -1;
    ibv_free_device_list(devs);

    verbs->pd = ibv_alloc_pd(verbs->context);
    if (!verbs->pd)
        return -1;

    return 0;
}

int ib_mr(verbs_t *verbs, size_t len)
{
    if (!verbs)
        return -1;

    void *area = calloc(1, len);
    if (!area)
        return -1;
    if (mlock(area, len))
        return -1;

    verbs->mr = ibv_reg_mr(verbs->pd, area, len,
            IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE |
            IBV_ACCESS_REMOTE_READ );

    if (!verbs->mr)
        return -1;

    return 0;
}

int ib_close(verbs_t *verbs)
{
    return 0;
}

int ib_qp(verbs_t *verbs)
{
    int err;

    if (!verbs)
        return -1;

    verbs->ch = ibv_create_comp_channel(verbs->context);
    if (!verbs->ch)
        return -1;

    // can be shared with multiple queues (send/recv)
    verbs->cq = ibv_create_cq(verbs->context, 1, NULL, verbs->ch, 0);
    if (!verbs->cq)
        return -1;

    struct ibv_qp_init_attr init_attr;
    memset(&init_attr, 0, sizeof(init_attr));
    init_attr.send_cq = verbs->cq;
    init_attr.recv_cq = verbs->cq;
    init_attr.cap.max_send_wr  = 8;
    init_attr.cap.max_recv_wr  = 8;
    init_attr.cap.max_send_sge = 8;
    init_attr.cap.max_recv_sge = 8;
    init_attr.sq_sig_all = 1;
    init_attr.qp_type = IBV_QPT_RC;

    verbs->qp = ibv_create_qp(verbs->pd, &init_attr);
    if (!verbs->qp)
        return -1;

    struct ibv_qp_attr attr;
    int mask = IBV_QP_STATE;
    memset(&attr, 0, sizeof(attr));
    memset(&init_attr, 0, sizeof(init_attr));
    err = ibv_query_qp(verbs->qp, &attr, mask, &init_attr);
    if (err) {
        perror("query qp");
        return -1;
    }

    struct ibv_port_attr pattr;
    if (ibv_query_port(verbs->context, 1, &pattr)) {
        perror("query port");
        return -1;
    }

    printf("lid qpn va rkey:\n");
    printf("0x%x 0x%x",
            pattr.lid, verbs->qp->qp_num);
    printf(" 0x%lx 0x%x\n",
            (unsigned long)verbs->mr->addr, verbs->mr->rkey);

    return 0;
}

int ib_toinit(verbs_t *verbs)
{
    struct ibv_qp_attr attr;
    const int port = 1;

    memset(&attr, 0, sizeof(attr));
    attr.qp_state        = IBV_QPS_INIT;
    attr.pkey_index      = 0; // must be zero
    attr.port_num        = port;
    attr.qp_access_flags =
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE |
        IBV_ACCESS_REMOTE_READ;
    int mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX |
                IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    if (ibv_modify_qp(verbs->qp, &attr, mask)) {
        perror("modify qp to init");
        return -1;
    }
    return 0;
}

int ib_tortr(verbs_t *verbs)
{
    char line[256];
    printf("> please enter lid qpn rva rkey: ");
    fflush(stdout);
    if (!fgets(line, 256, stdin))
        return -1;
    if (4 != sscanf(line, "0x%x 0x%x 0x%lx 0x%x",
                &lid, &qpn, &remote_addr, &remote_key))
        return -1;

    /*  Value                   Error if incorrect
     *  path_mtu                EINVAL
     *  port_num                101 network unreachable
     *  qpn, lid                unknown
     */

    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state              = IBV_QPS_RTR;
    attr.path_mtu              = IBV_MTU_1024;
    attr.dest_qp_num           = qpn;
    attr.rq_psn                = psn;
    attr.max_dest_rd_atomic    = 1;
    attr.min_rnr_timer         = 12; /* seconds to wait */
    attr.ah_attr.is_global     = 0;
    attr.ah_attr.dlid          = (uint16_t)lid;
    attr.ah_attr.sl            = 0; /* service level */
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num      = 1;
    int mask = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
               IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
               IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    if (ibv_modify_qp(verbs->qp, &attr, mask)) {
        perror("modify qp to RTR");
        return -1;
    }

    return 0;
}

int ib_torts(verbs_t *verbs)
{
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state        = IBV_QPS_RTS;
    attr.sq_psn          = psn;
    attr.max_rd_atomic   = 1;
    attr.retry_cnt       = 7;
    attr.rnr_retry       = 7;
    attr.timeout         = 14;
    int mask = IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC |
        IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_TIMEOUT;
    if (ibv_modify_qp(verbs->qp, &attr, mask)) {
        perror("modify qp to RTR");
        return -1;
    }

    return 0;
}

int ib_post_recv(verbs_t *verbs, size_t offset, unsigned int len)
{
    struct ibv_sge sge;
    struct ibv_recv_wr wr, *bad_wr;
    int err;

    sge.addr = mraddrat(verbs, uint64_t, offset);
    sge.length = len;
    sge.lkey = mrlkey(verbs);

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = 1;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    err = ibv_post_recv(verbs->qp, &wr, &bad_wr);
    if (err) {
        fprintf(stderr, "post_recv: %s\n", strerror(err));
        return -1;
    }

    return 0;
}

int ib_send(verbs_t *verbs, size_t offset, unsigned int len)
{
    struct ibv_sge sge;
    struct ibv_send_wr wr, *bad_wr;
    int err;

    if (!verbs)
        return -1;

    sge.addr = mraddrat(verbs, uint64_t, offset);
    sge.length = len;
    sge.lkey = mrlkey(verbs);

    memset(&wr, 0, sizeof(wr));
    wr.wr_id        = 1;
    wr.opcode       = IBV_WR_RDMA_WRITE;
    wr.send_flags   = IBV_SEND_SIGNALED;
    wr.sg_list      = &sge; // just one
    wr.num_sge      = 1;
    wr.wr.rdma.rkey = remote_key;
    wr.wr.rdma.remote_addr =
        (uintptr_t)remote_addr + offset;

    err = ibv_post_send(verbs->qp, &wr, &bad_wr);
    if (err) {
        fprintf(stderr, "send: %s\n", strerror(err));
        return -1;
    }

    return 0;
}

int ib_poll(verbs_t *verbs)
{
    struct ibv_wc wc;
    struct ibv_cq *evt_cq;
    void *cq_ctxt;
    int ne;

    if (!verbs)
        return -1;

    if (ibv_req_notify_cq(verbs->cq, 0))
        return -1;

    if (ibv_get_cq_event(verbs->ch, &evt_cq, &cq_ctxt))
        return -1;

    ibv_ack_cq_events(evt_cq, 1);

    if (ibv_req_notify_cq(evt_cq, 0))
        return -1;

    do {
        ne = ibv_poll_cq(verbs->cq, 1, &wc);
        if (!ne)
            continue;
        else if (ne < 0)
            return -1;
        if (wc.status != IBV_WC_SUCCESS)
            return -1;
    } while (ne);

    return 0;
}

