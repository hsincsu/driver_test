/*
 *			For userspace header file
 *				
 *
 * */
#ifndef __BXROCE_MAIN_H__
#define __BXROCE_MAIN_H__

#include <inttypes.h>
#include <stddef.h>
#include <endian.h>

#include <infiniband/driver.h>
//#include <infiniband/verbs.h>
#include <util/udma_barrier.h>
#include <ccan/bitmap.h>
#include <ccan/list.h>
#include "list.h"

#define bxroce_err(format,arg...) printf(format, ##arg)

#define BXROCE_DPP_PAGE_SIZE (4096)
#define VERBS_OPS_NUM (sizeof(struct verbs_context_ops) / sizeof(void *))

/*define some reg offset for qp,cq*/
#define MPB_WRITE_ADDR      0x00
#define MPB_RW_DATA			0x100
#define PGU_BASE 			0x00 /*IN MPB, PGU IS 0x00*/

#define GENRSP 				0x2000	/*whether CQ interrup*/
#define CFGRNR 				0x2004	/*QP WQE, NIC WORK, RNR,CREDIT TIMER*/
#define CFGSIZEOFWRENTRY 		0x2008	/*size of workrequest, 16byte times*/
#define UPLINKDOWNLINK 			0x2010	/*MTU with uplink & downlink*/
#define WQERETRYCOUNT			0x2014	/*WQE retry count*/
#define WQERETRYTIMER			0x2018	/*WQE timer*/
#define INTRMASK			0x2020	/*INTR MASK*/
#define WRONGVECTOR			0x2024	/*err vector*/
#define WRONGFIELD			0x2028	/*err field*/
#define TLBINIT				0x202c  /*TLB INIT*/
#define SOCKETID			0x2030  /*SOCKET ID TO IP*/
#define SRCQP				0x2034	/*QP TO QP*/
#define DESTQP				0x2038  /*DEST QP*/
#define RC_QPMAPPING		0x203c	/*RC MAPPING*/
#define CQQUEUEUP			0x0000	/*the upper border of cq*/
#define CQQUEUEDOWN			0x0008	/*the lower border of cq*/
#define CQREADPTR			0x0010	/*cq read ptr*/
#define CQESIZE				0x0018	/*cqe size*/
#define CQWRITEPTR			0x001c 	/*cq write ptr*/
#define RxUpAddrCQE			0x0028
#define RxBaseAddrCQE		0x0030
#define RxCQEWP				0x0038
#define RxCQEOp             0x0040
#define XmitUpAddrCQE		0x0050
#define XmitBaseAddrCQE		0x0058
#define XmitCQEWP			0x0060
#define XmitCQEOp			0x0068
#define RCVQ_INF			0x2040  /*RECVQ_INF REGISTER*/
#define RCVQ_DI				0x2044  /*REVQ_DI REGISTER*/
#define RCVQ_WRRD			0x2050  /*REVQ_WRRD*/ 
#define QPLISTREADQPN			0x4000  /*READ QPLIST FOR QPN*/
#define WRITEORREADQPLIST   		0x4004  /*READ OR WIRTE QPLIST*/
#define WPFORQPLIST			0x4008  /*WRITE QPLIST DATA: WP*/
#define WPFORQPLIST2		0x400c
#define RPFORQPLIST 			0x4010	/*WRITE QPLIST DATA: RP*/
#define RPFORQPLIST2			0x4014  /*WRITE QPLIST DATA*/
#define QPNANDVALID			0x4018  /*WRITE QPLIST DATA: QPN AND QPVALID*/
#define QPNANDVALID2		0x401c	/*WRITE QPLIST DATA*/
#define QPLISTWRITEQPN			0x4020  /*WRITE QPLIST DATA: WRITE QPN*/
#define READQPLISTDATA			0x4024	/*READ QPLIST DATA*/
#define READQPLISTDATA2			0x4028
#define	READQPLISTDATA3			0x402c
#define READQPLISTDATA4			0x4030
#define WRITEQPLISTMASK			0x403c  /*MASK FOR PAGE,RP,WP*/
/*end*/

/*Transport mode definition*/
#define UD				0x6
#define UC				0x2
#define RC				0x0
#define RD				0x4
#define SEND			0x4
#define SEND_WITH_IMM	0x5
#define SEND_WITH_INV   0x6
#define RDMA_WRITE			0x8
#define WRITE_WITH_IMM	0x9
#define RDMA_READ			0x0
/*END*/


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
#define MAX_SG_NUM 256;
struct bxroce_mr_sginfo {
	 struct userlist_head sg_list;
	 struct sg_phy_info *sginfo;
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

	struct bxroce_cqe *txva;
	struct bxroce_cqe *rxva;
	struct bxroce_cqe *xmitva;
	uint32_t txhead;
	uint32_t txtail;
	uint32_t rxhead;
	uint32_t rxtail;
	uint32_t xmithead;
	uint32_t xmittail;


	uint32_t phase;
	uint32_t getp;
	uint32_t max_hw_cqe;
	uint32_t qp_id;

	uint32_t id;
	uint32_t cqe_cnt;
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

//DEBUGINFO DEFINITION 

#define BXROCE_MRINFO
//#define BXROCE_QPINFO
//#define BXROCE_CQINFO
//#define BXROCE_PDINFO

#define BXROCE_OTHINFO
//#define BXROCE_HWINFO
//#define BXROCE_RECVINFO
#define BXROCE_SENDINFO


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