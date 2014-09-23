#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ib.h"

bool is_server;
verbs_t verbs;

const int val = 0xbeef;
const int offset = 64; // byte offset

int do_server(void)
{
    volatile int *addr = mraddr(&verbs, int*);
    unsigned long i;
    printf("> waiting for write from client\n");
    //if (ib_post_recv(&verbs, 0, 1024)) return -1;
    //if (ib_poll(&verbs)) return -1;

    // scan area
    while (1)
        for (i = 0; i < 1024 / sizeof(int); i++)
            if (addr[i] != 0)
                goto out;

out:
    printf("> got val 0x%x from client at offset %lu\n",
            addr[i], i * sizeof(int));
    return 0;
}

int do_client(void)
{
    int err;
    *mraddrat(&verbs, int*, offset) = val;
    err = ib_send(&verbs, 0, 1024);
    if (err)
        return -1;
    printf("> sent val to server\n");
    sleep(4);
    return 0;
}

int main(int argc, char *argv[])
{
    int err;

    memset(&verbs, 0, sizeof(verbs));

    const char *idx = strchr(*argv, 's');
    if (!idx) {
        idx = strchr(*argv, 'c');
        if (!idx)
            return 1;
    }
    is_server = (0 == strncmp(idx, "server", strlen("server")));
    printf("> we are %s\n", (is_server ? "server" : "client"));

    err = ib_open(&verbs);
    if (err)
        return 1;

    err = ib_mr(&verbs, MR_LEN);
    if (err)
        return 1;

    err = ib_qp(&verbs);
    if (err)
        return 1;

    // FOR RC TYPES
    // Init IBV_QP_STATE, IBV_QP_PKEY_INDEX, IBV_QP_PORT, IBV_QP_ACCESS_FLAGS
    // RTR  IBV_QP_STATE, IBV_QP_AV, IBV_QP_PATH_MTU, IBV_QP_DEST_QPN,
    //      IBV_QP_RQ_PSN, IBV_QP_MAX_DEST_RD_ATOMIC, IBV_QP_MIN_RNR_TIMER
    // RTS  IBV_QP_STATE, IBV_QP_SQ_PSN, IBV_QP_MAX_QP_RD_ATOMIC,
    //      IBV_QP_RETRY_CNT, IBV_QP_RNR_RETRY, IBV_QP_TIMEOUT

    err = ib_toinit(&verbs);
    if (err)
        return 1;

    err = ib_tortr(&verbs);
    if (err)
        return 1;

    err = ib_torts(&verbs);
    if (err)
        return 1;

    if (is_server && do_server())
            return 1;
    else if (!is_server && do_client())
        return 1;

    //ibv_destroy_qp(qp);
    //ibv_destroy_cq(cq);
    //ibv_destroy_comp_channel(ch); // once all CQs are destroyed
    //ibv_dereg_mr(mr);
    //ibv_dealloc_pd(pd);
    //ibv_close_device(context);
    return 0;
}
