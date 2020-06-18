/*
 *
 *
 *
 * 	this file is a header file for verbs.
 * 					--------edited by hs  2019/6/22
 *
 *
 */
#ifndef __BXROCE_VERBS_H__
#define __BXROCE_VERBS_H__


struct bxroce_pbe {
	u32 pa_hi;
	u32 pa_lo;
};

struct bxroce_eth_basic {
		u8 dmac[6];
        u8 smac[6];
        __be16 eth_type;
};
struct bxroce_eth_vlan {
		u8 dmac[6];
        u8 smac[6];
        __be16 eth_type;
        __be16 vlan_tag;
        __be16 roce_eth_type;
};


struct bxroce_grh {
		__be32  tclass_flow;
        __be32  pdid_hoplimit;
        u8      sgid[16];
        u8      dgid[16];      
        u16     rsvd;
};

struct bxroce_av {
	 struct bxroce_eth_vlan eth_hdr;
	 struct bxroce_grh grh;
	 u32 valid;
};

struct bxroce_ah {
	struct ib_ah	ibah;
	struct bxroce_av *av;
	u16 sgid_index;
	u32 id;
	u8 hdr_type;
};

enum bxroce_qp_foe {
        BXROCE_Q_FULL = 0,
        BXROCE_Q_EMPTY = 1,
        BXROCE_Q_FREE = 2,
};

struct bxroce_pd {
	struct bxroce_pool_entry pelem;
	struct ib_pd ibpd;
	u32 id;
	struct bxroce_ucontext *uctx;
};
struct bxroce_txcqe { // cqe need 16 byte memory.
	u16 pkey;
	u8  opcode;
	u16 destqp;
	u32 reserved1;
	u16 reserved2; //56bits to zero
	u8  reserved3;
	u32 immdt;
}__attribute__ ((packed));

struct bxroce_rxcqe {
	u16 bth_pkey;
	u8 bth_24_31;
	u16 bth_destqp;
	u16 rcvdestqpeecofremoterqt;
	u16 bth_64_87_lo;
	u8  bth_64_87_hi;
	u8  aeth;
	u32 immdt;
	u8  hff; //8'ff
}__attribute__ ((packed));

struct bxroce_xmitcqe {
	u16 bth_pkey;
	u8  bth_24_31;
	u16 bth_destqp;
	u16 destqpeecofremoterqt;
	u16 bth_64_87_lo;
	u8  bth_64_87_hi;
	u8  aeth;
	u32 immdt;
	u8  hff; //hff
}__attribute__ ((packed));

struct bxroce_cq {
	struct ib_cq ibcq;
	u32 qp_id;
	/*three types of cq,tx,rx,xmit*/
	struct bxroce_txcqe *txva;
	struct bxroce_rxcqe *rxva;
	struct bxroce_xmitcqe *xmitva;
	/*wp ,rp for three types of cq*/
	u32 txwp;
	u32 txrp;
	u32 rxwp;
	u32 rxrp;
	u32 xmitwp;
	u32 xmitrp;

	dma_addr_t txpa;
	dma_addr_t rxpa;
	dma_addr_t xmitpa;

	u32 cqe_size;	
	u32 phase;
    u32 getp;
	u32 max_hw_cqe;

	u32 id; // allocate a unique id for cq.
    u32 cqe_cnt;//cqe count
    u32 len; // cq's len
    spinlock_t lock; //for serialize accessing to the CQ
    struct list_head sq_head, rq_head, xmit_head;
	struct bxroce_ucontext *uctx;
};


struct bxroce_mr {
	struct bxroce_pool_entry pelem;
	union{
		struct ib_mr ibmr;
		struct ib_mw ibmw;
	};
	struct bxroce_pd *pd;
	struct ib_umem	  *umem;

	u32				 lkey;
	u32				 rkey;

	enum bxroce_mem_state	state;
	enum bxroce_mem_type	type;
	u64				 va;
	u64				 iova;
	size_t			 length;
	u32				 offset;
	int				 access;

	int				 page_shift;
	int				 page_mask;
	int				 map_shift;
	int				 map_mask;

	u32				 num_buf;
	u32				 nbuf;

	u32				 max_buf;
	u32				 num_map;

	struct bxroce_map  **map;
};

struct bxroce_qp_hwq_info {
	u8 *va;
	u32 max_sges;
	u32 head,tail;
	u32 entry_size;
	u32 max_cnt;
	u32 max_wqe_idx;
	//u16 dbid;
	u32 len;
	dma_addr_t pa;
	enum bxroce_qp_foe qp_foe;
	
};

struct bxroce_sge {
	u32 addr_hi;
	u32 addr_lo;
	u32 lrkey;
	u32 len;
};

struct bxroce_rqe {
	u64  descbaseaddr;
	u32  dmalen;
	u32  opcode;
}__attribute__ ((packed));


//we don't need the default align,we need to make sure the wqe is 48 bytes.
struct bxroce_wqe {//defaultly,we use 48 byte WQE.a queue may have 256 wqes. 48 bytes long,but length is 64 bytes.
	u32 immdt;
	u16 pkey;
	u32 rkey;
	u32 lkey;
	u32 qkey;
	u32 dmalen;
	u64 destaddr;
	u64 localaddr;
	u16 eecntx; // just the first 10 bits is for eecntx,the later 6bits is for destqp;
	u16 destqp; //just the first 4 bits is for destqp.the later 12 bits is for destsocket1.
	u32 destsocket1;
	u8 destsocket2;//just the first 4 bits is for destsocket2,the later 4 bits is for opcode.
	u8  opcode; // just the first 4 bits is for opcode .the later 4 bits is useless.
	u64 llpinfo_lo;
	u64 llpinfo_hi; // 
}__attribute__ ((packed));


enum bxroce_qp_state {
	BXROCE_QPS_RST				=0,
	BXROCE_QPS_INIT			=1,
	BXROCE_QPS_RTR				=2,
	BXROCE_QPS_RTS				=3,
	BXROCE_QPS_SQE				=4,
	BXROCE_QPS_SQ_DRAINING			=5,
	BXROCE_QPS_ERR				=6,
	BXROCE_QPS_SQD				=7,

};

struct qp_change_info
{
	u32 qkey;
	int signaled;
	u32 destqp;
	u32 pkey_index;
	int sgid_idx;
	u8 mac_addr[6];
	u8 sgid[16];
	u8 dgid[16];
};

struct bxroce_qp {
	struct ib_qp ibqp;
	
	u32 id; // qp unique id.
	u32 len; // qp len.send queue is same to recv queue.
	u32 max_inline_data;
	struct {
		uint64_t wrid;
		uint16_t dpp_wqe_idx;
		uint16_t dpp_wqe;
		uint8_t  signaled;
		uint8_t  rsvd[3];
	} *wqe_wr_id_tbl;
	u64 *rqe_wr_id_tbl;

	struct bxroce_cq *sq_cq;
	struct bxroce_cq *rq_cq;
	struct bxroce_pd *pd;

	spinlock_t q_lock ____cacheline_aligned;
	struct bxroce_qp_hwq_info rq;
	struct bxroce_qp_hwq_info sq;
	struct list_head rq_entry;
	struct list_head sq_entry;

	enum ib_qp_type qp_type;
	enum bxroce_qp_state qp_state;
	
	u32 qkey;
	u32 init_sqpsn;
	u32 init_rqpsn;
	int signaled;
	u32 destqp;
	u32 pkey_index;
	int sgid_idx;
	u8 mac_addr[6]; // dest mac addr
	u8 dgid[16]; // to resotre dest ip;
	u8 sgid[16];

	struct qp_change_info *qp_change_info;

	struct bxroce_ucontext *uctx;

};


struct bxroce_pbl {
	void *va;
	dma_addr_t pa;
	u32 size;
};

struct bxroce_dev{
	struct ib_device ibdev;
	u32 				id;
	struct list_head  devlist;

	struct ib_device_attr attr;
	struct bx_dev_info devinfo;
//	unsigned long *pd_id; // for allocate an unique id to each pd.
	struct mutex pd_mutex;
	struct mutex dev_lock; 
	struct mutex hw_lock; //for hw write/read sync
	//not finished ,added later.

	struct bxroce_pool mr_pool;
	struct bxroce_pool pd_pool;
	u8 *mem_resources; // for bitmap memory

	unsigned long *allocated_cqs; // allocate id for cqs
	unsigned long *allocated_qps;//allocated id for qps
	struct bxroce_qp **qp_table;
	struct bxroce_cq **cq_table;


	struct {
		struct bxroce_av *va;
		dma_addr_t pa;
		u32 size;
		u32 num_ah;

		spinlock_t lock;/*for synchronization*/
		u32 ahid;
		struct bxroce_pbl pbl;
	} av_tbl;

	u32 next_cq;
	u32 next_qp;
	u32 used_cqs;
	u32 used_qps;
	u64 ioaddr; // to keep io addr of pcie.

	spinlock_t resource_lock; //for cq,qp resource access
	spinlock_t qptable_lock;

	/*GSI need these*/
	int Is_qp1_allocated;
	struct bxroce_cq *gsi_sqcq;
	struct bxroce_cq *gsi_rqcq;
};

struct bxroce_ucontext {
	struct ib_ucontext ibucontext;
	struct list_head mm_head; 
	struct mutex mm_list_lock; //protect list entries of mm type

	struct bxroce_pd *ctx_pd;
	int pd_in_use;

	struct {
		u32 *va;
		dma_addr_t pa;
		u32 len;
	}ah_tbl;

};

struct bxroce_mm {
	struct {
		u64 phy_addr;
		unsigned long len;
	} key;
	struct list_head entry;
};

static inline struct bxroce_dev *get_bxroce_dev(struct ib_device *ibdev)
{
	return container_of(ibdev, struct bxroce_dev, ibdev);
}

static inline struct bxroce_pd *get_bxroce_pd(struct ib_pd *ibpd)
{
	return container_of(ibpd, struct bxroce_pd, ibpd);
}

static inline struct bxroce_cq *get_bxroce_cq(struct ib_cq *ibcq)
{
	return container_of(ibcq, struct bxroce_cq, ibcq);
}

static inline struct bxroce_mr *get_bxroce_mr(struct ib_mr *ibmr)
{
	return container_of(ibmr, struct bxroce_mr, ibmr);
}

static inline struct bxroce_qp *get_bxroce_qp(struct ib_qp *ibqp)
{
	return container_of(ibqp,struct bxroce_qp, ibqp);
}

static inline struct bxroce_ah *get_bxroce_ah(struct ib_ah* ibah)
{
	return container_of(ibah,struct bxroce_ah, ibah);
}

static inline struct bxroce_ucontext *get_bxroce_ucontext(struct ib_ucontext *ibucontext)
{
	return container_of(ibucontext, struct bxroce_ucontext, ibucontext);
}
int bxroce_mem_init_fast(struct bxroce_pd *bxpd, int max_pages, struct bxroce_mr *bxmr);

int bxroce_mem_init_dma(struct bxroce_pd *bxpd,int access, struct bxroce_mr *bxmr);

int bxroce_mem_init_user(struct bxroce_pd *bxpd,u64 start, u64 length, u64 iova, int access, struct ib_udata *udata, struct bxroce_mr *mr);

int bxroce_post_send(struct ib_qp *,const struct ib_send_wr *,const struct ib_send_wr **bad_wr);

int bxroce_post_recv(struct ib_qp *,const struct ib_recv_wr *,const struct ib_recv_wr **bad_wr);

int bxroce_poll_cq(struct ib_cq *, int num_entries, struct ib_wc *wc);

int bxroce_arm_cq(struct ib_cq *,enum ib_cq_notify_flags flags);

int bxroce_query_device(struct ib_device *, struct ib_device_attr *props,
		struct ib_udata *uhw);
int bxroce_query_port(struct ib_device *, u8 port, struct ib_port_attr *props);
int bxroce_modify_port(struct ib_device *, u8 port, int mask,
		       struct ib_port_modify *props);

enum rdma_protocol_type
bxroce_query_protocol(struct ib_device *device, u8 port_num);

void bxroce_get_guid(struct bxroce_dev *, u8 *guid);
int bxroce_query_gid(struct ib_device *, u8 port,
		     int index, union ib_gid *gid);
struct net_device *bxroce_get_netdev(struct ib_device *device, u8 port_num);
int bxroce_add_gid(const struct ib_gid_attr *attr,
		   void **context);
int  bxroce_del_gid(const struct ib_gid_attr *attr,
		    void **context);
int bxroce_query_pkey(struct ib_device *, u8 port, u16 index, u16 *pkey);

struct ib_ucontext *bxroce_alloc_ucontext(struct ib_device *,
					  struct ib_udata *);
int bxroce_dealloc_ucontext(struct ib_ucontext *);

int bxroce_mmap(struct ib_ucontext *, struct vm_area_struct *vma);

struct ib_pd *bxroce_alloc_pd(struct ib_device *,
			      struct ib_ucontext *, struct ib_udata *);
int bxroce_dealloc_pd(struct ib_pd *pd);

struct ib_cq *bxroce_create_cq(struct ib_device *ibdev,
			       const struct ib_cq_init_attr *attr,
			       struct ib_ucontext *ib_ctx,
			       struct ib_udata *udata);
int bxroce_resize_cq(struct ib_cq *, int cqe, struct ib_udata *);
int bxroce_destroy_cq(struct ib_cq *);

struct ib_qp *bxroce_create_qp(struct ib_pd *,
			       struct ib_qp_init_attr *attrs,
			       struct ib_udata *);
int _bxroce_modify_qp(struct ib_qp *, struct ib_qp_attr *attr,
		      int attr_mask);
int bxroce_modify_qp(struct ib_qp *, struct ib_qp_attr *attr,
		     int attr_mask, struct ib_udata *udata);
int bxroce_query_qp(struct ib_qp *,
		    struct ib_qp_attr *qp_attr,
		    int qp_attr_mask, struct ib_qp_init_attr *);
int bxroce_destroy_qp(struct ib_qp *);
int _bxroce_destroy_qp(struct bxroce_dev *,struct bxroce_qp *);
//void bxroce_del_flush_qp(struct ocrdma_qp *qp);

struct ib_srq *bxroce_create_srq(struct ib_pd *, struct ib_srq_init_attr *,
				 struct ib_udata *);
int bxroce_modify_srq(struct ib_srq *, struct ib_srq_attr *,
		      enum ib_srq_attr_mask, struct ib_udata *);
int bxroce_query_srq(struct ib_srq *, struct ib_srq_attr *);
int bxroce_destroy_srq(struct ib_srq *);
int bxroce_post_srq_recv(struct ib_srq *, struct ib_recv_wr *,
			 struct ib_recv_wr **bad_recv_wr);

int bxroce_dereg_mr(struct ib_mr *);
struct ib_mr *bxroce_get_dma_mr(struct ib_pd *, int acc);
struct ib_mr *bxroce_reg_user_mr(struct ib_pd *, u64 start, u64 length,
				 u64 virt, int acc, struct ib_udata *);
struct ib_mr *bxroce_alloc_mr(struct ib_pd *pd,
			      enum ib_mr_type mr_type,
			      u32 max_num_sg);
int bxroce_map_mr_sg(struct ib_mr *ibmr, struct scatterlist *sg, int sg_nents,
		     unsigned int *sg_offset);

/*some function definition*/
//static int bxroce_alloc_lkey(struct bxroce_dev *dev,struct bxroce_mr *mr,u32 pdid, int acc);//alter later --2019/10/28 hs
#endif
