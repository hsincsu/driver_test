/*
 *			For userspace header file
 *				
 *
 * */
#ifndef __BXROCE_MAIN_H__
#define __BXROCE_MAIN_H__


#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>

#include <inttypes.h>
#include <stddef.h>
#include <endian.h>

#include <infiniband/driver.h>
//#include <infiniband/verbs.h>
#include <util/udma_barrier.h>
#include <ccan/bitmap.h>
#include <ccan/list.h>

#include <rdma/ib_user_ioctl_cmds.h>
#include <infiniband/verbs.h>
//#include <infiniband/verbs_api.h>
//#include <infiniband/cmd_ioctl.h>
//#include <infiniband/cmd_write.h>
#include <linux/types.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "list.h"
#include "bxroce_abi.h"
#include "bxroce_reg.h"

#define bxroce_err(format,arg...) printf(format, ##arg)

#define BXROCE_DPP_PAGE_SIZE (4096)
#define VERBS_OPS_NUM (sizeof(struct verbs_context_ops) / sizeof(void *))

struct verbs_ex1_private {
        BITMAP_DECLARE(unsupported_ioctls, VERBS_OPS_NUM);
        uint32_t driver_id;
        bool use_ioctl_write;
        struct verbs_context_ops ops;
};    

struct bxroce_qp;

struct sg_phy_info {
	uint64_t	phyaddr;
	uint64_t	size;
};
//added by hs to store mr sg info
#define MAX_SG_NUM 8;
struct bxroce_mr_sginfo {
	 struct userlist_head sg_list;
	 struct sg_phy_info *sginfo;
	 uint32_t offset;
	 uint64_t iova;
	 uint32_t num_sge;
};


struct bxroce_dev {
	struct verbs_device ibv_dev;
	struct bxroce_qp **qp_tbl;

	uint32_t id;
	pthread_mutex_t dev_lock;
	pthread_spinlock_t flush_q_lock;
	uint32_t wqe_size;
	uint32_t rqe_size;
	uint8_t fw_ver[32];

	//add a list struct to store mr's phyaddr
	struct userlist_head mr_list;
};

struct bxroce_devctx {
	struct verbs_context ibv_ctx;
	uint32_t *ah_tbl;
	uint32_t ah_tbl_len;
	pthread_mutex_t tbl_lock;
};


struct bxroce_txcqe { // cqe need 16 byte memory.
	uint16_t pkey;
	uint8_t  opcode;
	uint16_t destqp;
	uint32_t reserved1;
	uint16_t reserved2; //56bits to zero
	uint8_t  reserved3;
	uint32_t immdt;
}__attribute__ ((packed));

struct bxroce_rxcqe {
	uint16_t bth_pkey;
	uint8_t bth_24_31;
	uint16_t bth_destqp;
	uint16_t rcvdestqpeecofremoterqt;
	uint16_t bth_64_87_lo;
	uint8_t  bth_64_87_hi;
	uint8_t  aeth;
	uint32_t immdt;
	uint8_t  hff; //8'ff
}__attribute__ ((packed));

struct bxroce_xmitcqe {
	uint16_t bth_pkey;
	uint8_t  bth_24_31;
	uint16_t bth_destqp;
	uint16_t destqpeecofremoterqt;
	uint16_t bth_64_87_lo;
	uint8_t  bth_64_87_hi;
	uint8_t  aeth;
	uint32_t immdt;
	uint8_t  hff; //hff
}__attribute__ ((packed));

enum bxroce_qp_foe {
        BXROCE_Q_FULL = 0,
        BXROCE_Q_EMPTY = 1,
        BXROCE_Q_FREE = 2,
};

struct bxroce_pd {
	struct ibv_pd ibv_pd;
	struct bxroce_dev *dev;
	struct bxroce_devctx *uctx;
	uint32_t id;

};
struct bxroce_cq {
	struct ibv_cq ibv_cq;
	struct bxroce_dev *dev;	

	struct bxroce_txcqe *txva;
	struct bxroce_rxcqe *rxva;
	struct bxroce_xmitcqe *xmitva;
	
	uint32_t txwp;
	uint32_t txrp;
	uint32_t rxwp;
	uint32_t rxrp;
	uint32_t xmitwp;
	uint32_t xmitrp;

	uint64_t txpa;
	uint64_t rxpa;
	uint64_t xmitpa;

	uint32_t phase;
	uint32_t getp;
	uint32_t max_hw_cqe;
	uint32_t qp_id;

	uint32_t id;
	uint32_t cqe_cnt;
	uint32_t cqe_size;
	uint32_t len; // cq size
	pthread_spinlock_t lock;

	void *iova;
	uint32_t reg_len;
};


struct bxroce_qp_hwq_info {
	uint8_t *va;
	uint32_t max_sges;
	uint32_t head,tail;
	uint32_t entry_size;
	uint32_t max_cnt;
	uint32_t max_wqe_idx;
	//uint16_t dbid;
	uint32_t len;
	uint64_t pa;
	enum bxroce_qp_foe qp_foe;
	
};

enum bxroce_qp_state {
	BXROCE_QPS_RST				=0,
	BXROCE_QPS_INIT			    =1,
	BXROCE_QPS_RTR				=2,
	BXROCE_QPS_RTS				=3,
	BXROCE_QPS_SQE				=4,
	BXROCE_QPS_SQ_DRAINING	    =5,
	BXROCE_QPS_ERR				=6,
	BXROCE_QPS_SQD				=7,

};

struct bxroce_rqe {
	uint64_t  descbaseaddr;
	uint32_t  dmalen;
	uint32_t  opcode;
}__attribute__((packed));

struct bxroce_wqe {//defaultly,we use 48 byte WQE.a queue may have 256 wqes. 48 bytes long,but length is 64 bytes.
	uint32_t immdt;
	uint16_t pkey;
	uint32_t rkey;
	uint32_t lkey;
	uint32_t qkey;
	uint32_t dmalen;
	uint64_t destaddr;
	uint64_t localaddr;
	uint16_t eecntx; // just the first 10 bits is for eecntx,the later 6bits is for destqp;
	uint16_t destqp; //just the first 4 bits is for destqp.the later 12 bits is for destsocket1.
	uint32_t destsocket1;
	uint8_t destsocket2;//just the first 4 bits is for destsocket2,the later 4 bits is for opcode.
	uint8_t  opcode; // just the first 4 bits is for opcode .the later 4 bits is useless.
	uint64_t llpinfo_lo;
	uint64_t llpinfo_hi; // 
}__attribute__((packed));

	struct qp_change_info
	{
	uint32_t qkey;
	int signaled;
	uint32_t destqp;
	uint32_t pkey_index;
	int sgid_idx;
	uint8_t mac_addr[6];
	uint8_t sgid[16];
	uint8_t dgid[16];
	};

struct bxroce_qp {
	struct ibv_qp ibv_qp;
	struct bxroce_dev *dev;
	pthread_spinlock_t q_lock;

	uint32_t id;
	uint32_t len;
	uint32_t max_inline_data;

	struct {
		uint64_t wrid;
		uint16_t dpp_wqe_idx;
		uint16_t dpp_wqe;
		uint8_t  signaled;
		uint8_t  rsvd[3];
	}*wqe_wr_id_tbl;
	
	uint64_t *rqe_wr_id_tbl;
	

	struct bxroce_cq *sq_cq;
	struct bxroce_cq *rq_cq;
	struct list_node sq_entry;
	struct list_node rq_entry;

	struct bxroce_pd *pd;

	struct bxroce_qp_hwq_info rq;
	struct bxroce_qp_hwq_info sq;

	enum ibv_qp_type qp_type;
	enum bxroce_qp_state qp_state;

	uint32_t qkey;
	int  signaled;
	uint32_t destqp;
	uint32_t pkey_index;
	int sgid_idx;
	uint8_t mac_addr[6];
	uint8_t sgid[16];
	uint8_t dgid[16];

	struct qp_change_info *qp_change_info;
	uint32_t qp_info_len;

	void *iova;
	uint32_t reg_len;
	

};


struct bxroce_ah {
	struct ibv_ah ibv_ah;
	struct bxroce_pd *pd;
	uint16_t id;
	uint8_t isvlan;
	uint8_t type;
};

#define get_bxroce_xxx(xxx, type)				\
 	container_of(ib##xxx, struct bxroce_##type, ibv_##xxx)

static inline struct bxroce_dev *get_bxroce_dev(struct ibv_device *ibdev)
{
	return container_of(ibdev,struct bxroce_dev,ibv_dev.device);
}

static inline struct bxroce_devctx *get_bxroce_ctx(struct ibv_context *ibctx)
{
	return container_of(ibctx, struct bxroce_devctx, ibv_ctx.context);
}

static inline struct bxroce_qp *get_bxroce_qp(struct ibv_qp *ibqp)
{
	return get_bxroce_xxx(qp,qp);
}

static inline struct bxroce_pd *get_bxroce_pd(struct ibv_pd *ibpd)
{
	return get_bxroce_xxx(pd,pd);
}

static inline struct bxroce_cq *get_bxroce_cq(struct ibv_cq *ibcq)
{
	return get_bxroce_xxx(cq,cq);
}

static inline struct bxroce_ah* get_bxroce_ah(struct ibv_ah* ibah)
{
	return get_bxroce_xxx(ah,ah);
}



int bxroce_query_device(struct ibv_context *, struct ibv_device_attr *);
int bxroce_query_port(struct ibv_context *, uint8_t, struct ibv_port_attr *);
struct ibv_pd *bxroce_alloc_pd(struct ibv_context *);
int bxroce_free_pd(struct ibv_pd *);
struct ibv_mr *bxroce_reg_mr(struct ibv_pd *, void *, size_t,uint64_t,
                             int ibv_access_flags);
int bxroce_dereg_mr(struct verbs_mr *vmr);

struct ibv_cq *bxroce_create_cq(struct ibv_context *, int,
                                struct ibv_comp_channel *, int);
int bxroce_resize_cq(struct ibv_cq *, int);
int bxroce_destroy_cq(struct ibv_cq *);
int bxroce_poll_cq(struct ibv_cq *, int, struct ibv_wc *);
int bxroce_arm_cq(struct ibv_cq *, int);

struct ibv_qp *bxroce_create_qp(struct ibv_pd *, struct ibv_qp_init_attr *);
int bxroce_modify_qp(struct ibv_qp *, struct ibv_qp_attr *,
                     int ibv_qp_attr_mask);
int bxroce_query_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr, int attr_mask,
                    struct ibv_qp_init_attr *init_attr);
int bxroce_destroy_qp(struct ibv_qp *);
int bxroce_post_send(struct ibv_qp *, struct ibv_send_wr *,
                     struct ibv_send_wr **);
int bxroce_post_recv(struct ibv_qp *, struct ibv_recv_wr *,
                     struct ibv_recv_wr **);

struct ibv_ah *bxroce_create_ah(struct ibv_pd *, struct ibv_ah_attr *);
int bxroce_destroy_ah(struct ibv_ah *);

void bxroce_init_ahid_tbl(struct bxroce_devctx *ctx);

void bxroce_mpb_reg_write(void *iova, uint32_t module_addr, uint32_t regaddr, uint32_t value);
uint32_t bxroce_mpb_reg_read(void *iova, uint32_t module_addr, uint32_t regaddr);

//DEBUGINFO DEFINITION 

//#define BXROCE_MRINFO
//#define BXROCE_QPINFO
//#define BXROCE_CQINFO
//#define BXROCE_PDINFO

//#define BXROCE_OTHINFO
//#define BXROCE_HWINFO
//#define BXROCE_RECVINFO
//#define BXROCE_SENDINFO


#ifdef BXROCE_RECVINFO
#define BXPRREC(fmt, args...)\
		printf("[%s,%d]:" fmt, __func__, __LINE__, ## args);
#else
#define BXPRREC(x...) do {} while(0)
#endif

#ifdef BXROCE_SENDINFO
#define BXPRSEN(fmt, args...)\
		printf("[%s,%d]:" fmt, __func__, __LINE__, ## args);
#else
#define BXPRSEN(x...) do {} while(0)
#endif

#ifdef BXROCE_MRINFO
#define BXPRMR(fmt, args...)\
		printf("[%s,%d]:" fmt, __func__, __LINE__, ## args);
#else
#define BXPRMR(x...) do {} while(0)
#endif

#ifdef BXROCE_QPINFO
#define BXPRQP(fmt, args...)\
		printf("[%s,%d]:" fmt, __func__, __LINE__, ## args);
#else
#define BXPRQP(x...) do {} while(0)
#endif

#ifdef BXROCE_CQINFO
#define BXPRCQ(fmt, args...)\
		printf("[%s,%d]:" fmt, __func__, __LINE__, ## args);
#else
#define BXPRCQ(x...) do {} while(0)
#endif

#ifdef BXROCE_PDINFO
#define BXPRPD(fmt, args...)\
		printf("[%s,%d]:" fmt, __func__, __LINE__, ## args);
#else
#define BXPRPD(x...) do {} while(0)
#endif

#ifdef BXROCE_HWINFO
#define BXPRHW(fmt, args...)\
		printf("[%s,%d]:" fmt, __func__, __LINE__, ## args);
#else
#define BXPRHW(x...) do {} while(0)
#endif

#ifdef BXROCE_OTHINFO
#define BXPROTH(fmt, args...)\
		printf("[%s,%d]:" fmt, __func__, __LINE__, ## args);
#else
#define BXPROTH(x...) do {} while(0)
#endif

#endif