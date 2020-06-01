/*
*ABI HEADER FILE
*
*/

#ifndef	 __BXROCE_ABI_H__
#define  __BXROCE_ABI_H__

#include <stdint.h>
#include <infiniband/kern-abi.h>
#include <rdma/bxroce-abi.h>
#include <kernel-abi/bxroce-abi.h>

#define BXROCE_ABI_MIN_VERSION 1
#define BXROCE_ABI_MAX_VERSION 2
#define BXROCE_MAX_QP 1024

DECLARE_DRV_CMD(ubxroce_get_context, IB_USER_VERBS_CMD_GET_CONTEXT,
				empty, bxroce_alloc_ucontext_resp);

DECLARE_DRV_CMD(ubxroce_alloc_pd, IB_USER_VERBS_CMD_ALLOC_PD,
				empty, empty);
DECLARE_DRV_CMD(ubxroce_create_cq, IB_USER_VERBS_CMD_CREATE_CQ,
				bxroce_create_cq_ureq, bxroce_create_cq_uresp);

DECLARE_DRV_CMD(ubxroce_create_qp, IB_USER_VERBS_CMD_CREATE_QP,
				bxroce_create_qp_ureq, bxroce_create_qp_uresp);

DECLARE_DRV_CMD(ubxroce_reg_mr, IB_USER_VERBS_CMD_REG_MR,
				bxroce_reg_mr_ureq,bxroce_reg_mr_uresp);
#endif
