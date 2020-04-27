/*
*	ABI HEADER FILE IN KERNEL-ABI
*
*/
#ifndef BXROCE_ABI_USER_H
#define BXROCE_ABI_USER_H

#include <linux/types.h>
#include <kernel-abi/bxroce-abi.h>
#include <rdma/bxroce-abi.h>

/*user kernel communication data structures.*/

struct bxroce_alloc_ucontext_resp {
	__u32 dev_id;
	__u32 wqe_size;
	__u32 max_inline_data;
	__u32 rqe_size;
	__u8  fw_ver[32];

	__aligned_u64 rsvd1;
	__aligned_u64 rsvd2;
};

struct bxroce_alloc_pd_uresp {
	__u32 id;
	__u32 rsvd[2];
};

struct bxroce_create_cq_ureq {
	__u32 dpp_cq;
	__u32 rsvd;
};

#define MAX_CQ_PAGES 8
struct bxroce_create_cq_uresp {
	__u32 cq_id;
	__u32 page_size;
	__u32 num_pages;
	__u32 max_hw_cqe;
	__u32 phase_change;
	__aligned_u64 txpage_addr[MAX_CQ_PAGES];
	__aligned_u64 rxpage_addr[MAX_CQ_PAGES];
	__aligned_u64 xmitpage_addr[MAX_CQ_PAGES];	

	//__aligned_u64 rsvd1;
	//__aligned_u64 rsvd2;
};

struct bxroce_create_qp_ureq {
	__u8 enable_dpp_cq;
	__u8 rsvd;
	__u16 dpp_cq_id;
	__u32 rsvd1;
};

#define MAX_QP_PAGES 8	
struct bxroce_create_qp_uresp {
		 __u16 qp_id;
        __u16 resv0;    /* pad */
        __u32 sq_page_size;
        __u32 rq_page_size;
        __u32 num_sq_pages;
        __u32 num_rq_pages;
        __aligned_u64 sq_page_addr[MAX_QP_PAGES];
        __aligned_u64 rq_page_addr[MAX_QP_PAGES];
        __u32 num_wqe_allocated;
        __u32 num_rqe_allocated;
		/*for user access hw reg*/
        __aligned_u64 ioaddr;
	   __u32 reg_len;
};

#endif