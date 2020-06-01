/*
* some user resp struct definition
*/

#ifndef BXROCE_ABI_USER_H
#define BXROCE_ABI_USER_H

#include <linux/types.h>

#define BXROCE_MIN_ABI_VERSION 1
#define BXROCE_MAX_ABI_VERSION 2

struct bxroce_alloc_ucontext_resp {
	__u32 dev_id;
	__u32 wqe_size;
	__u32 max_inline_data;
	__u32 rqe_size;
	__u8  fw_ver[32];

	__aligned_u64 ah_tbl_page;
	__u32 ah_tbl_len;

	__aligned_u64 rsvd1;
	__aligned_u64 rsvd2;
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

	   __aligned_u64 qp_info_addr;
	    __u32 qp_info_len;
};

struct bxroce_reg_mr_ureq {
		__aligned_u64 user_addr;
};

#define MAX_SG_NUM		8
struct bxroce_reg_mr_uresp{
		__aligned_u64 sg_phy_addr[MAX_SG_NUM];
		__aligned_u64 sg_phy_size[MAX_SG_NUM];
		__u32		  sg_phy_num;
		__u32		  offset;
};


#endif
