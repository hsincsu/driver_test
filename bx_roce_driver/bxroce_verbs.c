/*
 *
 *
 *	this file is for verbs.operation function is almost here!
 *						-------edited by hs 2019/6/22
 *
 *
 */
#include <linux/dma-mapping.h>
#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/iw_cm.h>
#include <rdma/ib_umem.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_cache.h>
#include <linux/netdevice.h>

#include "header/bxroce.h"
//#include "header/bxroce_verbs.h"
//#include "header/bxroce_hw.h"

//for mmap operation. we need to add some address that will be mapped to user space.
static int bxroce_add_mmap(struct bxroce_ucontext *uctx, u64 phy_addr, unsigned long len)
{
	struct bxroce_mm *mm;

	mm = kzalloc(sizeof(*mm), GFP_KERNEL);
	if(mm == NULL)
			return -ENOMEM;
	mm->key.phy_addr = phy_addr;
	mm->key.len = len;
	INIT_LIST_HEAD(&mm->entry);

	mutex_lock(&uctx->mm_list_lock);
	list_add_tail(&mm->entry, &uctx->mm_head); // add to uctx's list.
	mutex_unlock(&uctx->mm_list_lock);
	return 0;
}

static void bxroce_del_mmap(struct bxroce_ucontext *uctx, u64 phy_addr, unsigned long len)
{
	struct bxroce_mm *mm, *tmp;

	mutex_lock(&uctx->mm_list_lock);
	list_for_each_entry_safe(mm, tmp, &uctx->mm_head, entry) {
		if(len != mm->key.len && phy_addr != mm->key.phy_addr)
			continue;
		list_del(&mm->entry);
		kfree(mm);
		break;
	}
	mutex_unlock(&uctx->mm_list_lock);
}


static void bxroce_set_wqe_dmac(struct bxroce_qp *qp, struct bxroce_wqe *wqe)
{
	struct bxroce_wqe tmpwqe;
	u8 tmpvalue;
	int i =0;
	BXROCE_PR("bxroce:mac addr ");//added by hs
	for(i = 0;i<6;i++)
	BXROCE_PR("0x%x,",qp->mac_addr[i]);//addedby hs
//	BXROCE_PR("\n");//added by hs
	memset(&tmpwqe,0,sizeof(struct bxroce_wqe));
//	BXROCE_PR("bxroce:tmpwqe.destqp:0x%x\n",tmpwqe.destqp);//added by hs
	tmpwqe.destqp = qp->mac_addr[1];
//	BXROCE_PR("bxroce:tmpwqe.destqp1:0x%x\n",tmpwqe.destqp);//added by hs
	tmpwqe.destqp = tmpwqe.destqp << 8;
//	BXROCE_PR("bxroce:tmpwqe.destqp2:0x%x\n",tmpwqe.destqp);
	tmpwqe.destqp = tmpwqe.destqp + qp->mac_addr[0];
//	BXROCE_PR("bxroce:tmpwqe.destqp3:0x%x\n",tmpwqe.destqp);
	tmpwqe.destqp = tmpwqe.destqp <<4;
//	BXROCE_PR("bxroce:tmpwqe.destqp4:0x%x\n",tmpwqe.destqp);
	tmpwqe.destsocket1 = qp->mac_addr[5];
	tmpwqe.destsocket1 = tmpwqe.destsocket1 << 8;
	tmpwqe.destsocket1 += qp->mac_addr[4];
	tmpwqe.destsocket1 = tmpwqe.destsocket1 << 8;
	tmpwqe.destsocket1 += qp->mac_addr[3];
	tmpwqe.destsocket1 = tmpwqe.destsocket1 <<8;
	tmpwqe.destsocket1 +=qp->mac_addr[2];
	tmpwqe.destsocket1 = tmpwqe.destsocket1 <<4;
	tmpvalue = qp->mac_addr[1];
	tmpvalue = tmpvalue >> 4;
	tmpwqe.destsocket1 += tmpvalue;
	tmpvalue = qp->mac_addr[5];
	tmpvalue = tmpvalue >> 4;
	tmpwqe.destsocket2 += tmpvalue;

	wqe->destqp = wqe->destqp & 0x000f;
	BXROCE_PR("bxroce:wqe->destqp1:0x%x\n",wqe->destqp);
	wqe->destqp += tmpwqe.destqp;
	BXROCE_PR("bxroce:wqe->destqp1:0x%x\n",wqe->destqp);
	wqe->destsocket1 = wqe->destsocket1 & 0x0;
	wqe->destsocket1 =tmpwqe.destsocket1;
	wqe->destsocket2 = wqe->destsocket2 & 0xf0;
	wqe->destsocket2 += tmpwqe.destsocket2;
	BXROCE_PR("bxroce:%s destqp:0x%x,  destsocket1: 0x%x,  destsocket2: 0x%x \n"
		,__func__,wqe->destqp,wqe->destsocket1,wqe->destsocket2);//added by hs


}

static void bxroce_set_wqe_opcode(struct bxroce_wqe *wqe,u8 qp_type,u8 opcode)
{
	u8 opcode_l = 0;
	u8 opcode_h = 0;
	opcode_l = opcode;
	opcode_l = opcode_l << 4;
	opcode_h = qp_type;
	if(wqe->destsocket2 >> 4)
		wqe->destsocket2 = wqe->destsocket2 & 0x0f;
	wqe->destsocket2 += opcode_l;
	BXROCE_PR("bxroce:%s,destsocket2:0x%x \n",__func__,wqe->destsocket2);//added by hs
	if(wqe->opcode << 4)
		wqe->opcode = wqe->destsocket2 & 0xf0;
	wqe->opcode += opcode_h;
	BXROCE_PR("bxroce:%s,opcode:0x%x \n",__func__,wqe->opcode);//added by hs
	wqe->opcode += 0x10;
	BXROCE_PR("bxroce:%s,opcode final:0x%x \n",__func__,wqe->opcode);//added by hs
}

//set rc,uc 's wqe
static void bxroce_set_rcwqe_destqp(struct bxroce_qp *qp,struct bxroce_wqe *wqe)
{
	
	u16 tempqpn;
	u16 tempqpn_l = 0;
	u16 tempqpn_h = 0;

	tempqpn =qp->destqp; // get lower 16bits ,but qpn only 10 bits.
	tempqpn_l = tempqpn & 0x003f;
	tempqpn_l = tempqpn_l << 10;

	tempqpn_h = tempqpn& 0x03c0; // get higher 4 bits;
	tempqpn_h = tempqpn_h >> 6;

	/*make sure the wqe's eecntx higher 6 bits is zero*/
	if(wqe->eecntx >> 10)
		wqe->eecntx = wqe->eecntx & 0x03ff; // mask is 0000 0011 1111 1111 to make higher 6 bits zero.
	wqe->eecntx += tempqpn_l;
	
	if(wqe->destqp << 12)
		wqe->destqp = wqe->destqp & 0xfff0;
	wqe->destqp += tempqpn_h;
	BXROCE_PR("bxroce:%s2,eecntx:0x%x , destqp:0x%x \n",__func__,wqe->eecntx,wqe->destqp);
}

//set ud 's wqe
static void bxroce_set_wqe_destqp(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ib_send_wr *wr) {
	
	u16 tempqpn;
	u16 tempqpn_l = 0;
	u16 tempqpn_h = 0;

	tempqpn =ud_wr(wr)->remote_qpn; // get lower 16bits ,but qpn only 10 bits.
	tempqpn_l = tempqpn & 0x003f;
	tempqpn_l = tempqpn_l << 10;

	tempqpn_h = tempqpn& 0x03c0; // get higher 4 bits;
	tempqpn_h = tempqpn_h >> 6;

	/*make sure the wqe's eecntx higher 6 bits is zero*/
	if(wqe->eecntx >> 10)
		wqe->eecntx = wqe->eecntx & 0x03ff; // mask is 0000 0011 1111 1111 to make higher 6 bits zero.
	wqe->eecntx += tempqpn_l;

	if(wqe->destqp << 12)
		wqe->destqp = wqe->destqp & 0xfff0;
	wqe->destqp += tempqpn_h;
	BXROCE_PR("bxroce:%s3,eecntx:0x%x, destqp:0x%x \n",__func__,wqe->eecntx,wqe->destqp);//added by hs
}

static int  bxroce_build_wqe_opcode(struct bxroce_qp *qp,struct bxroce_wqe *wqe,struct ib_send_wr *wr)
{
	int status= 0;
	u8 qp_type;
	u8 opcode;
	switch(qp->qp_type) {
		case IB_QPT_UD:
				qp_type = UD;
				break;
		case IB_QPT_UC:
				qp_type = UC;
				break;
		case IB_QPT_RESERVED2:
				qp_type = RD;
				break;
		case IB_QPT_RC:
				qp_type = RC;
				break;
		default:
				printk("bxroce: qp type default ...\n");//added by hs
				status = 0x1;
				break;
	}

	switch (wr->opcode) {
	case IB_WR_SEND:
				opcode = SEND;
				break;
	case IB_WR_SEND_WITH_IMM:
				opcode = SEND_WITH_IMM;
				break;
	case IB_WR_SEND_WITH_INV:
				opcode = SEND_WITH_INV;
				break;
	case IB_WR_RDMA_WRITE:
				opcode = RDMA_WRITE;
				break;
	case IB_WR_RDMA_WRITE_WITH_IMM:
				opcode = WRITE_WITH_IMM;
				break;
	case IB_WR_RDMA_READ:
				opcode = RDMA_READ;
				break;	
	default:
				printk("bxroce: wr opcode default...\n");//added by hs
				status = status | 0x2;	
				break;	
	}
	if(status & 0x1||status & 0x2)
	{
		printk("bxroce: transport or  opcode not supported \n");//added by hs 	
		return -EINVAL;
	}
	if(qp_type == UD && !(opcode & (SEND|SEND_WITH_IMM)))
		return -EINVAL;
	if(qp_type == UC && !(opcode &(SEND|SEND_WITH_IMM|RDMA_WRITE|WRITE_WITH_IMM )))
		return -EINVAL;
	if(qp_type == RD && (opcode & SEND_WITH_INV))
		return -EINVAL;
	BXROCE_PR("bxroce: %s,qp_type:0x%x , opcode:0x%x \n "
			,__func__,qp_type,opcode);//added by hs
	bxroce_set_wqe_opcode(wqe,qp_type,opcode);
	return 0;

}

static int bxroce_build_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe, int num_sge, struct ib_sge *sg_list,struct ib_send_wr *wr)
{
	int i;
	int status = 0;
	struct bxroce_wqe *tmpwqe = wqe;
	for (i = 0; i < num_sge; i++) {
		status = bxroce_build_wqe_opcode(qp,tmpwqe,wr);//added by hs 
		if(status)
			return status;
		if(qp->destqp)
			bxroce_set_rcwqe_destqp(qp,tmpwqe);
		
		bxroce_set_wqe_dmac(qp,tmpwqe);
		tmpwqe->qkey = qp->qkey;	
		//tmpwqe->rkey = sg_list[i].rkey;
		tmpwqe->lkey = sg_list[i].lkey;
		tmpwqe->localaddr = sg_list[i].addr;
		tmpwqe->dmalen = sg_list[i].length;
		tmpwqe->pkey = qp->pkey_index;
		//only ipv4 now!by hs
		tmpwqe->llpinfo_lo = 0;
		tmpwqe->llpinfo_hi = 0;
		memcpy(&tmpwqe->llpinfo_lo,&qp->dgid[0],4);
		BXROCE_PR("bxroce: ---------------check send wqe--------------\n");//added by hs
		BXROCE_PR("bxroce:immdat:0x%x \n",tmpwqe->immdt);//added by hs
		BXROCE_PR("bxroce:pkey:0x%x \n",tmpwqe->pkey);//added by hs
		BXROCE_PR("bxroce:rkey:0x%x \n",tmpwqe->rkey);//added by hs
		BXROCE_PR("bxroce:lkey:0x%x \n",tmpwqe->lkey);//added by hs
		BXROCE_PR("bxroce:qkey:0x%x \n",tmpwqe->qkey);//added by hs
		BXROCE_PR("bxroce:dmalen:0x%x \n",tmpwqe->dmalen);//added by hs
		BXROCE_PR("bxroce:destaddr:0x%lx \n",tmpwqe->destaddr);//added by hs
		BXROCE_PR("bxroce:localaddr:0x%lx \n",tmpwqe->localaddr);//added by hs
		BXROCE_PR("bxroce:eecntx:0x%x \n",tmpwqe->eecntx);//added by hs
		BXROCE_PR("bxroce:destqp:0x%x \n",tmpwqe->destqp);//added by hs
		BXROCE_PR("bxroce:destsocket1:0x%x \n",tmpwqe->destsocket1);//added by hs
		BXROCE_PR("bxroce:destsocket2:0x%x \n",tmpwqe->destsocket2);//added by hs
		BXROCE_PR("bxroce:opcode:0x%x \n",tmpwqe->opcode);//added by hs
		BXROCE_PR("bxroce:llpinfo_lo:0x%x\n",tmpwqe->llpinfo_lo);
		BXROCE_PR("bxroce:llpinfo_hi:0x%x\n",tmpwqe->llpinfo_hi);
		BXROCE_PR("bxroce:wqe's addr:%lx \n",tmpwqe);//added by hs
		BXROCE_PR("bxroce:----------------check send wqe end------------\n");//added by hs
		
		if(wr->send_flags & IB_SEND_SIGNALED || qp->signaled)
				qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].signaled = 1;
		else
				qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].signaled = 0;
		qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].wrid = wr->wr_id;

		tmpwqe += 1;
	}
	if (num_sge == 0) {
		memset(wqe,0,sizeof(*wqe));
	}
	return status;
}

static int bxroce_buildwrite_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe,int num_sge, struct ib_sge *sg_list, struct ib_send_wr *wr)
{
	int i;
	int status = 0;
	struct bxroce_wqe *tmpwqe = wqe;
	char *testbl = NULL;

	for (i = 0; i < num_sge; i++) {
		status = bxroce_build_wqe_opcode(qp,tmpwqe,wr);//added by hs 
		if(status)
			return -EINVAL;
		if(qp->destqp)
			bxroce_set_rcwqe_destqp(qp,tmpwqe);
		bxroce_set_wqe_dmac(qp,tmpwqe);
		tmpwqe->rkey = rdma_wr(wr)->rkey;
		tmpwqe->lkey = sg_list[i].lkey;
		tmpwqe->localaddr = sg_list[i].addr;
		tmpwqe->dmalen = sg_list[i].length;
		tmpwqe->destaddr = rdma_wr(wr)->remote_addr;
		testbl = (char *)&tmpwqe->destaddr; 
		tmpwqe->qkey = qp->qkey;
		tmpwqe->pkey = qp->pkey_index;
		//only ipv4 now!by hs
		tmpwqe->llpinfo_lo = 0;
		tmpwqe->llpinfo_hi = 0;
		memcpy(&tmpwqe->llpinfo_lo,&qp->dgid[0],4);
		BXROCE_PR("bxroce: ---------------check write wqe--------------\n");//added by hs
		BXROCE_PR("bxroce:immdat:0x%x \n       addr:%lx\n",tmpwqe->immdt,&tmpwqe->immdt);//added by hs
		BXROCE_PR("bxroce:pkey:0x%x \n		   addr:%lx\n",tmpwqe->pkey,&tmpwqe->pkey);//added by hs
		BXROCE_PR("bxroce:rkey:0x%x \n         addr:%lx\n",tmpwqe->rkey,&tmpwqe->rkey);//added by hs
		BXROCE_PR("bxroce:lkey:0x%x \n         addr:%lx\n",tmpwqe->lkey,&tmpwqe->lkey);//added by hs
		BXROCE_PR("bxroce:qkey:0x%x \n         addr:%lx\n",tmpwqe->qkey,&tmpwqe->qkey);//added by hs
		BXROCE_PR("bxroce:dmalen:0x%x \n       addr:%lx\n",tmpwqe->dmalen,&tmpwqe->dmalen);//added by hs
		BXROCE_PR("bxroce:destaddr:0x%lx \n    addr:%lx\n",tmpwqe->destaddr,&tmpwqe->destaddr);//added by hs
		BXROCE_PR("bxroce:localaddr:0x%lx \n   addr:%lx\n",tmpwqe->localaddr,&tmpwqe->localaddr);//added by hs
		BXROCE_PR("bxroce:eecntx:0x%x \n       addr:%lx\n",tmpwqe->eecntx,&tmpwqe->eecntx);//added by hs
		BXROCE_PR("bxroce:destqp:0x%x \n       addr:%lx\n",tmpwqe->destqp,&tmpwqe->destqp);//added by hs
		BXROCE_PR("bxroce:destsocket1:0x%x \n  addr:%lx\n",tmpwqe->destsocket1,&tmpwqe->destsocket1);//added by hs
		BXROCE_PR("bxroce:destsocket2:0x%x \n  addr:%lx\n",tmpwqe->destsocket2,&tmpwqe->destsocket2);//added by hs
		BXROCE_PR("bxroce:opcode:0x%x \n       addr:%lx\n",tmpwqe->opcode,&tmpwqe->opcode);//added by hs
		BXROCE_PR("bxroce:llpinfo_lo:0x%x\n    addr:%lx\n",tmpwqe->llpinfo_lo,&tmpwqe->llpinfo_lo);
		BXROCE_PR("bxroce:llpinfo_hi:0x%x\n    addr:%lx\n",tmpwqe->llpinfo_hi,&tmpwqe->llpinfo_hi);
		BXROCE_PR("bxroce:wqe's addr:%lx \n    addr:%lx\n",tmpwqe);//added by hs
		BXROCE_PR("bxroce:bltest: destaddr: 0x%x",*testbl);
		BXROCE_PR("bxroce:----------------check write wqe end------------\n");//added by hs
		if(wr->send_flags & IB_SEND_SIGNALED || qp->signaled)
				qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].signaled = 1;
		else
				qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].signaled = 0;
		qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].wrid = wr->wr_id;

		tmpwqe += 1;
	}
	if (num_sge == 0) {
		memset(wqe,0,sizeof(*wqe));
	}
	return status;
}

static inline uint32_t bxroce_sglist_len(struct ib_sge *sg_list, int num_sge)
{
	uint32_t total_len =0, i;
	for(i=0;i < num_sge; i++)
		total_len += sg_list[i].length;
	return total_len;
}

static int bxroce_build_inline_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ib_send_wr *wr, u32 wqe_size)
{
	int i;

	if (wr->send_flags & IB_SEND_INLINE && qp->qp_type != IB_QPT_UD) {//
		wqe->dmalen = bxroce_sglist_len(wr->sg_list,wr->num_sge);
			printk("%s() supported_len = 0x%x,\n"
				   "unsupported len req =0x%x,\n,the funtion is not supported now\n",__func__,qp->max_inline_data,wqe->dmalen);//added by hs 
			return -EINVAL;
	}
	else {
		bxroce_build_sges(qp,wqe,wr->num_sge,wr->sg_list,wr);
		if(wr->num_sge){
			wqe_size +=((wr->num_sge-1) * sizeof(struct bxroce_wqe));
			qp->sq.head = (qp->sq.head + wr->num_sge) % qp->sq.max_cnt; // update the head ptr,and check if the queue if full.
			if(qp->sq.head == qp->sq.tail){
				qp->sq.qp_foe = BXROCE_Q_FULL;
			}
		}
		else {
			qp->sq.head = (qp->sq.head + 1) % qp->sq.max_cnt; // update the head ptr, and check if the queue if full.
			if(qp->sq.head == qp->sq.tail){
				qp->sq.qp_foe = BXROCE_Q_FULL;
			}
		}
	}
	BXROCE_PR("bxroce: post send, sq.head is %d, sq.tail is %d \n",qp->sq.head,qp->sq.tail);//added by hs
	return 0;


}


static int bxroce_buildwrite_inline_sges(struct bxroce_qp *qp,struct bxroce_wqe *wqe,const struct ib_send_wr *wr, u32 wqe_size)
{
	int i;
	int status = 0;
	if (wr->send_flags & IB_SEND_INLINE && qp->qp_type != IB_QPT_UD) {//
		wqe->dmalen = bxroce_sglist_len(wr->sg_list,wr->num_sge);
			printk("%s() supported_len = 0x%x,\n"
				   "unsupported len req =0x%x,\n,the funtion is not supported now\n",__func__,qp->max_inline_data,wqe->dmalen);//added by hs 
			return -EINVAL;
	}
	else {
		status = bxroce_buildwrite_sges(qp,wqe,wr->num_sge,wr->sg_list,wr);
		if(wr->num_sge){
			wqe_size +=((wr->num_sge-1)*sizeof(struct bxroce_wqe));
			qp->sq.head = (qp->sq.head + wr->num_sge) % qp->sq.max_cnt; // update the head ptr,and check if the queue if full.
			if(qp->sq.head == qp->sq.tail){
				qp->sq.qp_foe = BXROCE_Q_FULL;
			}
		}
		else {
			qp->sq.head = (qp->sq.head + 1) % qp->sq.max_cnt; // update the head ptr, and check if the queue if full.
			if(qp->sq.head == qp->sq.tail){
				qp->sq.qp_foe = BXROCE_Q_FULL;
			}
		}
	}
	BXROCE_PR("bxroce: post send, sq.head is %d, sq.tail is %d\n",qp->sq.head,qp->sq.tail);//added by hs
	return status;
}


static int bxroce_build_send(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ib_send_wr *wr)
{
	int status;
	struct bxroce_sge *sge;
	u32 wqe_size = sizeof(*wqe);

	if (qp->qp_type == IB_QPT_UD || qp->qp_type == IB_QPT_GSI) {
			bxroce_set_wqe_destqp(qp,wqe,wr);
			if(qp->qp_type == IB_QPT_GSI)
				wqe->qkey = qp->qkey;
			else
				wqe->qkey = ud_wr(wr)->remote_qkey;
	}
	status = bxroce_build_inline_sges(qp,wqe,wr,wqe_size);
	return status;
}

static int bxroce_build_write(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ib_send_wr *wr) 
{
	int status;
	u32 wqe_size = sizeof(*wqe);

	status = bxroce_buildwrite_inline_sges(qp,wqe,wr,wqe_size);
	if(status)
		return status;
	return 0;
}

static void bxroce_build_read(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ib_send_wr *wr)
{
	u32 wqe_size = sizeof(*wqe);
	bxroce_buildwrite_inline_sges(qp,wqe,wr,wqe_size);
}

static int bxroce_build_reg(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ib_reg_wr *wr)
{
	u32 wqe_size = sizeof(*wqe);
	printk("don't support reg mr now\n");//added by hs 
	return 0;
}

static void bxroce_ring_sq_hw(struct bxroce_qp *qp) {
	struct bxroce_dev *dev;
	u32 qpn;
	dev = get_bxroce_dev(qp->ibqp.device);
	/*from head to get dma address*/
	u32 phyaddr,tmpvalue;
	phyaddr =qp->sq.head * sizeof(struct bxroce_wqe); //head * sizeof(wqe)
	BXROCE_PR("bxroce: post send wp's phyaddr is %x \n",phyaddr);//added by hs	
	/*access hw ,write wp to notify hw*/
	void __iomem* base_addr;
	base_addr = dev->devinfo.base_addr;
	qpn = qp->id;

	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTREADQPN,qpn);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEORREADQPLIST,0x1);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEQPLISTMASK,0x7);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTWRITEQPN,0x0);

	tmpvalue = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA);
	BXROCE_PR("bxroce:wp:0x%x ,",tmpvalue);//added by hs
	#if 0 //added by hs
	tmpvalue = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA2);
	BXROCE_PR("rp:0x%x,phyaddr: 0x%x\n",tmpvalue,phyaddr);//added by hs
	tmpvalue = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA3);
	BXROCE_PR("bxorce:readqplistdata3:%x \n",tmpvalue);//added by hs
	tmpvalue = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA4);
	BXROCE_PR("bxroce:readqplistdata4:%x \n",tmpvalue);//added by hs
	#endif
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WPFORQPLIST,phyaddr);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEQPLISTMASK,0x1);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTWRITEQPN,0x1);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEORREADQPLIST,0x0);
	// end


}

static int bxroce_check_foe(struct bxroce_qp_hwq_info *q, struct ib_send_wr *wr, u32 free_cnt)
{
	if (wr->num_sge > free_cnt)
		return -ENOMEM;
	else return 0;
}

static int bxroce_hwq_free_cnt(struct bxroce_qp_hwq_info *q)
{
	if(q->head > q->tail)
		return ((q->max_wqe_idx - q->head) + q->tail)% q->max_cnt;
	if (q->head == q->tail) {
		if(q->qp_foe == BXROCE_Q_EMPTY)
			return q->max_cnt;
		else
			return 0;	
	}
	if(q->head < q->tail)
		return q->tail - q->head;
}

static void *bxroce_hwq_head(struct bxroce_qp_hwq_info *q) {
	return q->va + (q->head * q->entry_size);
}


static void bxroce_pgu_info_before_wqe(struct bxroce_dev *dev,struct bxroce_qp *qp)
{
	void __iomem* base_addr;
	base_addr = dev->devinfo.base_addr;
	u32 regval = 0;
	u32 txop = 0;
	u32 rxop = 0;
	u32 xmitop = 0;

	printk("----------------------PGU INFO BEFORE WQE START ----------------\n");//added by hs
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_INF,qp->id);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_WRRD,0x8);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RCVQ_DI);
	BXROCE_PR("\t PGUINFO: RQ BASE_ADDR_L: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RCVQ_DI +0x4);
	BXROCE_PR("\t PGUINFO: RQ BASE_ADDR_H: 0x%x\n",regval);

    bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_INF,qp->id);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_WRRD,0x10);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RCVQ_DI);
	BXROCE_PR("\t PGUINFO: RQ WP_L: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RCVQ_DI +0x4);
	BXROCE_PR("\t PGUINFO: RQ WP_H: 0x%x\n",regval);

	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_INF,qp->id);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_WRRD,0x20);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RCVQ_DI);
	BXROCE_PR("\t PGUINFO: RQ RP_L: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RCVQ_DI +0x4);
	BXROCE_PR("\t PGUINFO: RQ RP_H: 0x%x\n",regval);

	txop = qp->id;
	txop = txop << 2;//left move 2 bits
	txop = txop + 0x01;
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,CQESIZE,txop);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,CQWRITEPTR);
	BXROCE_PR("\t PGUINFO: TXCQ WP_LO: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,CQWRITEPTR +0x4);
	BXROCE_PR("\t PGUINFO: TXCQ WP_HI: 0x%x\n",regval);

	rxop = qp->id;
	rxop = rxop << 2;//left move 2 bits
	rxop = rxop + 0x01;
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RxCQEOp,rxop);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RxCQWPT);
	BXROCE_PR("\t PGUINFO: RXCQ WP_LO: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RxCQWPT +0x4);
	BXROCE_PR("\t PGUINFO: RXCQ WP_HI: 0x%x\n",regval);

	xmitop = qp->id;
	xmitop = xmitop << 2;//left move 2 bits
	xmitop = xmitop + 0x01;
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,XmitCQEOp,xmitop);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,XmitCQWPT);
	BXROCE_PR("\t PGUINFO: XMITCQ WP_LO: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,XmitCQWPT +0x4);
	BXROCE_PR("\t PGUINFO: XMITCQ WP_HI: 0x%x\n",regval);


	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTREADQPN,qp->id);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEORREADQPLIST,0x1);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEQPLISTMASK,0x7);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTWRITEQPN,0x0);

	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA);
	BXROCE_PR("\t PGUINFO: SQ WP:0x%x \n",regval);//added by hs
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA2);
	BXROCE_PR("\t PGUINFO: SQ RP:0x%x \n",regval);//added by hs
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA3);
	BXROCE_PR("\t PGUINFO: SQ BASE_ADDR_LO:0x%x \n",regval);//added by hs
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA4);
	BXROCE_PR("\t PGUINFO: SQ BASE_ADDR_HI:0x%x \n",regval);//added by hs

	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEQPLISTMASK,0x1);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTWRITEQPN,0x1);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEORREADQPLIST,0x0);

	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,GENRSP);
	BXROCE_PR("\t PGUINFO: GENRSP(0x2000): 0x%x \n",regval);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,CFGRNR);
	BXROCE_PR("\t PGUINFO: CFGRNR(0x2004): 0x%x \n",regval);
	regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,INTRMASK);
	BXROCE_PR("\t PGUINFO: INTRMASK(0x2020):0x%x \n",regval);

	printk("------------------------PGU INFO BEFROE WQE END-------------------------------\n");


}



int bxroce_post_send(struct ib_qp *ibqp,const struct ib_send_wr *wr,const struct ib_send_wr **bad_wr)
{
	BXROCE_PR("bxroce:bxroce_post_send start!\n");//added by hs for printing start info
	/*wait to add 2019/6/24*/
	struct bxroce_qp *qp;
	struct bxroce_dev *dev;
	struct bxroce_wqe *hdwqe;
	unsigned long flags;
	int status = 0;
	u32 free_cnt;
	qp = get_bxroce_qp(ibqp);
	dev = get_bxroce_dev(ibqp->device);

	//added by hs for printing all pgu info
    //bxroce_pgu_info_before_wqe(dev,qp);

	spin_lock_irqsave(&qp->q_lock,flags);
	if (qp->qp_state != BXROCE_QPS_RTS) {
		spin_unlock_irqrestore(&qp->q_lock,flags);
		*bad_wr = wr;
		return -EINVAL;
	}

	BXROCE_PR("bxroce: post_send, process sge.. \n");//added by hs
	while(wr){ //if UD, should support SEND OR SEND WITH IMM,or it can't do anything.
		if(qp->qp_type == IB_QPT_UD &&
		  (wr->opcode != IB_WR_SEND &&
		   wr->opcode != IB_WR_SEND_WITH_IMM)){
			*bad_wr = wr;
			status = -EINVAL;
			break;
		}
		free_cnt = bxroce_hwq_free_cnt(&qp->sq);

		if(free_cnt == 0 || wr->num_sge > qp->sq.max_sges){
			*bad_wr = wr;
			status = -ENOMEM;
			break;
		}
		status = bxroce_check_foe(&qp->sq,wr,free_cnt);// check if the wr can be processed with enough memory.
		if(status) break;

		hdwqe = bxroce_hwq_head(&qp->sq); // To get the head ptr.
		BXROCE_PR("bxroce:wp's va:%x \n",hdwqe);//added by hs
		switch(wr->opcode){
		case IB_WR_SEND_WITH_IMM:
			hdwqe->immdt = ntohl(wr->ex.imm_data);
		case IB_WR_SEND:
			status = bxroce_build_send(qp,hdwqe,wr);
			break;
		case IB_WR_SEND_WITH_INV:
			hdwqe->lkey = wr->ex.invalidate_rkey;
			status = bxroce_build_send(qp,hdwqe,wr);
			break;
		case IB_WR_RDMA_WRITE_WITH_IMM:
			hdwqe->immdt = ntohl(wr->ex.imm_data);
		case IB_WR_RDMA_WRITE:
			status = bxroce_build_write(qp,hdwqe,wr);
			break;
		case IB_WR_RDMA_READ:
			bxroce_build_read(qp,hdwqe,wr);
			break;
		case IB_WR_LOCAL_INV:
			hdwqe->lkey = wr->ex.invalidate_rkey;
		case IB_WR_REG_MR:
			status = bxroce_build_reg(qp,hdwqe,reg_wr(wr));
			break;
		default:
			status = -EINVAL;
			break;
		}
		if (status) {
			*bad_wr = wr;
			break;
		}
		/* //wqe_wr_id_tbl is to store wqe->wrid.
		if(wr->send_flags & IB_SEND_SIGNALED || qp->signaled)
				qp->wqe_wr_id_tbl[qp->sq.head].signaled = 1;
		else
				qp->wqe_wr_id_tbl[qp->sq.head].signaled = 0;
		qp->wqe_wr_id_tbl[qp->sq.head].wrid = wr->wr_id;
		*/
		/*make sure wqe is written befor adapter can access it*/
		BXROCE_PR("bxroce:wmb... \n");//added by hs
		wmb();
		BXROCE_PR("bxroce:access hw.. \n");//added by hs
		bxroce_ring_sq_hw(qp); // notify hw to send wqe.
		wr = wr->next;
	}

	/*wait to add end!*/
	spin_unlock_irqrestore(&qp->q_lock,flags);
	BXROCE_PR("bxroce:bxroce_post_send succeed end!\n");//added by hs for printing end info
	return status;
}

static void bxroce_ring_rq_hw(struct bxroce_qp *qp)
{
	 struct bxroce_dev *dev;
	 dev = get_bxroce_dev(qp->ibqp.device);
	 /*from head to get dma address*/
	u32 phyaddr;
	phyaddr =qp->rq.head * sizeof(struct bxroce_rqe); //head * sizeof(wqe)
	//BXROCE_PR("rq wp's phyaddr is %x\n",phyaddr);//added by hs
	/*access hw ,write wp to notify hw*/
	void __iomem* base_addr;
	u32 qpn;
	base_addr = dev->devinfo.base_addr;
	qpn = qp->id;
	//phyaddr = phyaddr << 10; // because wp 's postition is 10 bytes from revq_inf.
	//BXROCE_PR("rq wp's phyadr is %x \n",phyaddr);//added by hs
	//qpn = qpn + phyaddr;
	//BXROCE_PR("rq wp+qpn is %x \n",qpn);//added by hs

	//update rq's wp ,so hw can judge that there is still some wqes not processed.
#if 1
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_INF,qpn);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_DI,phyaddr);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_DI + 0x4,0);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RCVQ_WRRD,0x2);
#endif 
	//end


}

static void bxroce_build_rqsges(struct bxroce_rqe *rqe, struct ib_recv_wr *wr,struct bxroce_qp *qp)
{
	int i;
	int num_sge = wr->num_sge;
	struct bxroce_rqe *tmprqe = rqe;
	struct ib_sge *sg_list;
	sg_list = wr->sg_list;
	for (i = 0; i < num_sge; i++) {
		tmprqe->descbaseaddr = sg_list[i].addr;
		tmprqe->dmalen = sg_list[i].length;
		tmprqe->opcode = 0x80000000;
		tmprqe += 1;
	
		qp->rqe_wr_id_tbl[(qp->rq.head + i) % qp->rq.max_cnt] = wr->wr_id; // to store the wr ,so CQ can verify which one is for this wr.
		//BXROCE_PR("bxroce: in rq,num_sge = %d, tmprqe 's addr is %x\n",num_sge,tmprqe);//added by hs
	}
	if(num_sge == 0)
		memset(tmprqe,0,sizeof(*tmprqe));
}



static void bxroce_build_rqe(struct bxroce_qp *qp,struct bxroce_rqe *rqe, const struct ib_recv_wr *wr) 
{
	u32 wqe_size = 0;

	bxroce_build_rqsges(rqe,wr,qp);
	//BXROCE_PR("bxroce:-----------------------check rq wqe--------------------\n");//added by hs
	//BXROCE_PR("bxroce:descbaseaddr: %x \n",rqe->descbaseaddr);//added by hs
	//BXROCE_PR("bxroce:dmalen:		  %x \n",rqe->dmalen);//added by hs
	//BXROCE_PR("bxroce:opcode:		  %x \n",rqe->opcode);//added by hs
	if(wr->num_sge){
			wqe_size +=((wr->num_sge-1) * sizeof(struct bxroce_rqe));
			qp->rq.head = (qp->rq.head + wr->num_sge) % qp->rq.max_cnt; // update the head ptr,and check if the queue if full.
			if(qp->rq.head == qp->rq.tail){
				qp->rq.qp_foe = BXROCE_Q_FULL;
			}
			
	}
		else {
			qp->rq.head = (qp->rq.head + 1) % qp->rq.max_cnt; // update the head ptr, and check if the queue if full.
			if(qp->rq.head == qp->rq.tail){
				qp->rq.qp_foe = BXROCE_Q_FULL;
			}
	}
	//BXROCE_PR("bxroce: in rq,qp->rq.head is %d, qp->rq.tail is %d \n",qp->rq.head,qp->rq.tail);//added by hs
	
}

int bxroce_post_recv(struct ib_qp *ibqp,const struct ib_recv_wr *wr,const struct ib_recv_wr **bad_wr)
{
		BXROCE_PR("bxroce:bxroce_post_recv start!\n");//added by hs for printing start info
		int status = 0;
		unsigned long flags;
		struct bxroce_qp *qp = get_bxroce_qp(ibqp);
		struct bxroce_rqe *rqe;
		struct bxroce_dev *dev = get_bxroce_dev(ibqp->device);
		u32 free_cnt;
		/*wait to add 2019/6/24*/
		//BXROCE_PR("bxroce: in rq qpn is %d \n",qp->id);//added by hs
		spin_lock_irqsave(&qp->q_lock,flags);
		if (qp->qp_state == BXROCE_QPS_RST || qp->qp_state == BXROCE_QPS_ERR) {
			spin_unlock_irqrestore(&qp->q_lock,flags);
			*bad_wr = wr;
			return -EINVAL;
		}
		while (wr) {
			free_cnt = bxroce_hwq_free_cnt(&qp->rq);
			if (free_cnt == 0 || wr->num_sge > qp->rq.max_sges) {
				*bad_wr = wr;
				status = -ENOMEM;
				break;
			}
			status = bxroce_check_foe(&qp->rq,wr,free_cnt);// check if the wr can be processed with enough memory.
			if(status) break;

			rqe = bxroce_hwq_head(&qp->rq);
			//BXROCE_PR("bxroce: in rq, free_cnt=%d, rqe is %x \n",free_cnt,rqe);//added by hs
			bxroce_build_rqe(qp,rqe,wr); // update rq->head & set rqe 's value
	
			/*make sure rqe is written before hw access it*/
			wmb();
			/*notify hw to process the rq*/
			bxroce_ring_rq_hw(qp);

			wr = wr->next;
		}
	
		/*wait to add end!*/	
		spin_unlock_irqrestore(&qp->q_lock,flags);
		BXROCE_PR("bxroce:bxroce_post_recv succeed end!\n");//added by hs for printing end info
		return status;
}

/*get txcq head*/
static void *bxroce_txcq_head(struct bxroce_cq *cq)
{
	return cq->txva + (cq->cqe_size * cq->txrp);
}

/*get rxcq head*/
static void *bxroce_rxcq_head(struct bxroce_cq *cq)
{
	return cq->rxva +(cq->cqe_size * cq->txrp);
}

static void *bxroce_xmitcq_head(struct bxroce_cq *cq)
{
	return cq->xmitva +(cq->cqe_size * cq->xmitrp);
}

/*read hw to get hw wp*/
static void *bxroce_txcq_hwwp(struct bxroce_cq *cq ,struct bxroce_dev *dev,struct bxroce_qp *qp)
{
	void __iomem* base_addr;
	u32 cqwp_lo = 0;
	u32 cqwp_hi = 0;
	u64 cqwp = 0;
	u32 txop = 0;

	txop = 0;
	txop = qp->id;
	txop = txop << 2;//left move 2 bits
	txop = txop + 0x01;
	base_addr = dev->devinfo.base_addr;
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,CQESIZE,txop);
	cqwp_lo = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,CQWRITEPTR);
	cqwp_hi = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,CQWRITEPTR +0x4);
	cqwp = cqwp_hi;
	cqwp = cqwp << 32;//hi left move to higher bits
	cqwp = cqwp + cqwp_lo;
	cq->txwp = (cqwp - cq->txpa)/cq->cqe_size;
	BXROCE_PR("cq->txwp:0x%x , cqe_size:0x%x \n",cq->txwp,cq->cqe_size);
	
	return cq->txva +(cq->txwp * cq->cqe_size);

}

/*read hw to get hw wp*/
static void *bxroce_rxcq_hwwp(struct bxroce_cq *cq ,struct bxroce_dev *dev,struct bxroce_qp *qp)
{
	void __iomem* base_addr;
	u32 cqwp_lo = 0;
	u32 cqwp_hi = 0;
	u64 cqwp = 0;
	u32 rxop = 0;

	rxop = 0;
	rxop = qp->id;
	rxop = rxop << 2;//left move 2 bits
	rxop = rxop + 0x01;
	base_addr = dev->devinfo.base_addr;
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RxCQEOp,rxop);
	cqwp_lo = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RxCQWPT);
	cqwp_hi = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RxCQWPT +0x4);
	cqwp = cqwp_hi;
	cqwp = cqwp << 32;//hi left move to higher bits
	cqwp = cqwp + cqwp_lo;
	cq->rxwp = (cqwp - cq->rxpa)/cq->cqe_size;
	BXROCE_PR("cq->rxwp:0x%x , cqe_size:0x%x \n",cq->rxwp,cq->cqe_size);
	
	return cq->rxva +(cq->rxwp * cq->cqe_size);

}

/*read hw to get hw wp*/
static void *bxroce_xmitcq_hwwp(struct bxroce_cq *cq ,struct bxroce_dev *dev,struct bxroce_qp *qp)
{
	void __iomem* base_addr;	
	u32 cqwp_lo = 0;
	u32 cqwp_hi = 0;
	u64 cqwp = 0;
	u32 xmitop = 0;

	
	xmitop = 0;
	xmitop = qp->id;
	xmitop = xmitop << 2;//left move 2 bits
	xmitop = xmitop + 0x01;
	base_addr = dev->devinfo.base_addr;
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,XmitCQEOp,xmitop);
	cqwp_lo = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,XmitCQWPT);
	cqwp_hi = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,XmitCQWPT +0x4);
	cqwp = cqwp_hi;
	cqwp = cqwp << 32;//hi left move to higher bits
	cqwp = cqwp + cqwp_lo;
	cq->xmitwp = (cqwp - cq->xmitpa)/cq->cqe_size;
	BXROCE_PR("cq->xmitwp:0x%x , cqe_size:0x%x \n",cq->xmitwp,cq->cqe_size);
	
	return cq->xmitva +(cq->xmitwp * cq->cqe_size);

}

static void *bxroce_get_hwq_by_idx(struct bxroce_qp_hwq_info *q, u32 idx)
{
	return q->va + (idx * q->entry_size);
}

static void bxroce_hwq_inc_tail(struct bxroce_qp_hwq_info *q)
{
	q->tail = (q->tail +1) & q->max_wqe_idx;
}

static void bxroce_update_wc(struct bxroce_qp *qp, struct ib_wc *ibwc, u32 wqe_idx,struct bxroce_txcqe *cqe)
{
	struct bxroce_wqe *wqe;
	int opcode;

	wqe = bxroce_get_hwq_by_idx(&qp->sq, wqe_idx);

	ibwc->wr_id = qp->wqe_wr_id_tbl[wqe_idx].wrid;
	opcode = cqe->opcode;
	switch (opcode & 0x0f) {
	case RDMA_WRITE:
		ibwc->opcode = IB_WC_RDMA_WRITE;
		break;
	case RDMA_READ:
		ibwc->opcode = IB_WC_RDMA_READ;
		ibwc->byte_len = wqe->dmalen; // if wqe is rdma_read, dmalen means how long to get
		break;
	case SEND:
	case SEND_WITH_IMM:
	case SEND_WITH_INV:
		ibwc->opcode = IB_WC_SEND;
		break;
	default:
		ibwc->status = IB_WC_GENERAL_ERR;
		printk("invalid opcode received\n");
		break;
	}


}

static void bxroce_poll_success_scqe(struct bxroce_qp *qp, struct bxroce_txcqe *cqe,struct ib_wc *ibwc, bool *polled)
{
	int tail = qp->sq.tail;
	u32 wqe_idx;

	if (qp->wqe_wr_id_tbl[tail].signaled) {
		*polled = false;
	}
	else {
		ibwc->status = IB_WC_SUCCESS;
		ibwc->wc_flags = 0;
		ibwc->qp = &qp->ibqp;
		bxroce_update_wc(qp,ibwc,tail,cqe);
		*polled = true;
	}
	
	bxroce_hwq_inc_tail(&qp->sq);
	return 0;

}


static int bxroce_update_ud_rcqe(struct ib_wc *ibwc, struct bxroce_rxcqe *cqe)
{
	 int status =0;
	 ibwc->src_qp = cqe->bth_destqp & 0x3ff;
	 ibwc->pkey_index = 0;
	 ibwc->wc_flags = IB_WC_GRH;

	 //not proceesd
	 //ibwc->byte_len?
	 //ibwc->network_hdr_type?


	 return status;
}


static void bxroce_poll_success_rcqe(struct bxroce_qp *qp, struct bxroce_rxcqe *cqe,struct ib_wc *ibwc, bool *polled)
{
		int status;
		
		ibwc->wc_flags = 0;

		*polled = true;
		ibwc->opcode = IB_WC_RECV;
		ibwc->qp = &qp->ibqp;
		ibwc->status = IB_WC_SUCCESS;
		
		if (qp->qp_type == IB_QPT_UD || qp->qp_type == IB_QPT_GSI)
				bxroce_update_ud_rcqe(ibwc,cqe);	
		//not processed
		//ibwc->byte_len?

		if (cqe->immdt) { // may error
			ibwc->ex.imm_data =htonl(le32_to_cpu(cqe->immdt));
			ibwc->wc_flags |= IB_WC_WITH_IMM;
		}

		ibwc->wr_id = qp->rqe_wr_id_tbl[qp->rq.tail];
		bxroce_hwq_inc_tail(&qp->rq);
}





static void bxroce_update_hw_txcq_rp(struct bxroce_qp *qp,struct bxroce_cq *cq,struct bxroce_dev *dev)
{
	void __iomem* base_addr;
	u32 newrp_lo;
	u32 newrp_hi;
	u64 newrp;
	base_addr = dev->devinfo.base_addr;
	u32 txop = 0;

	txop = 0;
	txop = qp->id;
	txop = txop << 2;//left move 2 bits
	txop = txop + 0x3;

	newrp = cq->txpa + (cq->txrp * cq->cqe_size);
	newrp_lo = newrp;
	newrp_hi = newrp >>32;

	//update hw's rp ,rp need software to update it.
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,CQREADPTR,newrp_lo);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,CQREADPTR + 0x4,newrp_hi);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,CQESIZE,txop);


}

static void bxroce_update_hw_rxcq_rp(struct bxroce_qp *qp,struct bxroce_cq *cq,struct bxroce_dev *dev)
{
	void __iomem* base_addr;
	u32 newrp_lo;
	u32 newrp_hi;
	u64 newrp;
	base_addr = dev->devinfo.base_addr;
	u32 rxop = 0;

	rxop = 0;
	rxop = qp->id;
	rxop = rxop << 2;//left move 2 bits
	rxop = rxop + 0x3;

	newrp = cq->rxpa + (cq->rxrp * cq->cqe_size);
	newrp_lo = newrp;
	newrp_hi = newrp >>32;

	//update hw's rp ,rp need software to update it.
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RxCQEWP,newrp_lo);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RxCQEWP + 0x4,newrp_hi);
	bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RxCQEOp,rxop);


}

/*access hw for cqe*/
static int bxroce_poll_hwcq(struct bxroce_cq *cq, int num_entries, struct ib_wc *ibwc)
{
		u16 qpn = 0;
        int i = 0;
        bool expand = false;
        int polled_hw_cqes = 0;
        struct bxroce_qp *qp = NULL;
        struct bxroce_dev *dev = get_bxroce_dev(cq->ibcq.device);
        struct bxroce_txcqe *txrpcqe;
		struct bxroce_rxcqe *rxrpcqe;
		struct bxroce_xmit_cqe *xmitrpcqe;
		struct bxroce_txcqe *txwpcqe;
		struct bxroce_rxcqe *rxwpcqe;
		struct bxroce_xmit_cqe *xmitwpcqe;

        void __iomem *base_addr;
	    u16 cur_getp; bool txpolled = false;bool rxpolled = false; bool stop = false; 
		u32 phyaddr = 0;

		if(dev->qp_table[cq->qp_id]) //different from other rdma driver, cq only mapped to one qp.
			qp = dev->qp_table[cq->qp_id];
		BUG_ON(qp == NULL);

		//get hw wp,hw update it;
		txwpcqe = bxroce_txcq_hwwp(cq,dev,qp);
		rxwpcqe = bxroce_rxcq_hwwp(cq,dev,qp);
		xmitwpcqe = bxroce_xmitcq_hwwp(cq,dev,qp);

		//get rp,software need update it;
		txrpcqe = bxroce_txcq_head(cq);
		rxrpcqe = bxroce_rxcq_head(cq);
		xmitrpcqe = bxroce_xmitcq_head(cq);

		u32 tmpvalue = 100; //just to make poll exit success if no cqe in cq.

		while (num_entries && tmpvalue) {//process wqe one by one.i think 
			
			if(cq->txrp != cq->txwp) //means txcq have cqe not processed.
			{
				bxroce_poll_success_scqe(qp,txrpcqe,ibwc,&txpolled);
				txwpcqe = bxroce_txcq_hwwp(cq,dev,qp); //update hw's wp;
				cq->txrp = (cq->txrp +1) % (cq->max_hw_cqe);
				txrpcqe = bxroce_txcq_head(cq);
				bxroce_update_hw_txcq_rp(qp,cq,dev);


			}

			if (cq->rxrp != cq->rxwp)//mean rxcq have cqe not processed.
			{
				bxroce_poll_success_rcqe(qp,rxrpcqe,ibwc,&rxpolled);
				rxwpcqe = bxroce_rxcq_hwwp(cq,dev,qp); // update hw's wp;
				cq->rxrp = (cq->rxrp + 1) % (cq->max_hw_cqe -1);
				rxrpcqe = bxroce_rxcq_head(cq);
				bxroce_update_hw_rxcq_rp(qp,cq,dev);
	
			}


		// following codes is just for checking hw's qp 's rp whether updated or not.
		//BXROCE_PR("bxroce:qp->id is %d \n",qp->id);//added by hs
		base_addr = dev->devinfo.base_addr;
		bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTREADQPN,qp->id);
		bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEORREADQPLIST,0x1);
		bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEQPLISTMASK,0x7);
		bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTWRITEQPN,0x0);
		phyaddr = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA);
		BXROCE_PR("bxroce:wp is phyaddr:0x%x \n",phyaddr);//added by hs
		phyaddr = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,READQPLISTDATA2);
		BXROCE_PR("bxroce:rp is phyaddr:0x%x , sq.tail:%d \n",phyaddr,qp->sq.tail);//added by hs
		bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEQPLISTMASK,0x1);
		bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,QPLISTWRITEQPN,0x1);
		bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,WRITEORREADQPLIST,0x0);

		polled_hw_cqes += 1;
		if(txpolled)
		{
			num_entries -=1;
			i += 1;
			txpolled = false;
		}
		if(rxpolled)
		{
			num_entries -=1;
			i +=1;
			rxpolled = false;
		}

		ibwc = ibwc + 1;
		tmpvalue -= 1;
			
		}

		

		return i;
}

/*poll cqe from cq.*/
int bxroce_poll_cq(struct ib_cq *ibcq, int num_entries, struct ib_wc *wc)
{
		BXROCE_PR("bxroce:bxroce_poll_cq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		int cqes_to_poll = num_entries;
		struct bxroce_cq *cq = get_bxroce_cq(ibcq);
		struct bxroce_dev *dev = get_bxroce_dev(ibcq->device);
		int num_os_cqe = 0, err_cqes = 0;
		struct bxroce_qp *qp;
		unsigned long flags;

		struct bx_dev_info *devinfo = &dev->devinfo; //added by hs for printing info
		struct rnic_pdata *rnic_pdata = dev->devinfo.rnic_pdata;
	  	void __iomem *base_addr;
		base_addr = dev->devinfo.base_addr;

		  unsigned int rdma_channel = RDMA_CHANNEL;
	  	u32 regval = 0;

		/*poll cq from hw*/
		spin_lock_irqsave(&cq->lock,flags);
		num_os_cqe = bxroce_poll_hwcq(cq, cqes_to_poll,wc);//To get cq from hw,Please note that there is 3 types cq queues for one cq.	
		spin_unlock_irqrestore(&cq->lock,flags);

		cqes_to_poll -= num_os_cqe; //if cqes_to_poll ==0,means all cqs needed have been received.
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_poll_cq succeed end!\n");//added by hs for printing end info	
		
		if (cqes_to_poll) {
			BXROCE_PR("bxroce:process cq, but may some err happened;");
		}


		mac_print_all_regs(rnic_pdata,0);//


		/*added by hs for printing some hw info*/
		printk("--------------------poll cq  printing info start --------------------------\n");
		regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,0x2024); //err vector
		BXROCE_PR("PGU err vector: 0x%x \n",regval);
		
		regval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,0x2028);
		BXROCE_PR("PGU err pos   : 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_TDLR));
		BXROCE_PR("DMA_CH_CA_TDLR: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_TDHR));
		BXROCE_PR("DMA_CH_CA_TDHR: 0x%x \n",regval);
		
		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_RDLR));
		BXROCE_PR("DMA_CH_CA_RDLR: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_RDHR));
		BXROCE_PR("DMA_CH_CA_RDHR: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_TBLR));
		BXROCE_PR("DMA_CH_CA_TBLR: 0x%x \n",regval);
	
		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_TBHR));
		BXROCE_PR("DMA_CH_CA_TBHR: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_RBLR));
		BXROCE_PR("DMA_CH_CA_RBLR: 0x%x \n",regval);
	
		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_CA_RBHR));
		BXROCE_PR("DMA_CH_CA_RBHR: 0x%x \n",regval);
	
		printk("--------------------poll cq  printing info end --------------------------\n");
		return num_os_cqe;

}

int bxroce_arm_cq(struct ib_cq *ibcq,enum ib_cq_notify_flags flags)
{
		BXROCE_PR("bxroce:bxroce_arm_cq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_arm_cq succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_query_device(struct ib_device *ibdev, struct ib_device_attr *props,struct ib_udata *uhw)
{
		BXROCE_PR("bxroce:bxroce_query_device start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_dev *dev;
		dev = get_bxroce_dev(ibdev);
	/*a little mistake is that props should be attrs, may fix later*/
	//	if(uhw->inlen || uhw->outlen)
	//		return -EINVAL;
		memset(props,0,sizeof *props);
		bxroce_get_guid(dev,(u8 *)&props->sys_image_guid);
		props->vendor_id = dev->devinfo.pcidev->vendor;
		props->vendor_part_id = dev->devinfo.pcidev->device;
		props->page_size_cap = 0xffff000;
		props->fw_ver = BXROCE_FW_VER;	
		props->device_cap_flags = IB_DEVICE_CURR_QP_STATE_MOD |
											IB_DEVICE_RC_RNR_NAK_GEN |
											IB_DEVICE_SHUTDOWN_PORT |
											IB_DEVICE_SYS_IMAGE_GUID |
											IB_DEVICE_LOCAL_DMA_LKEY |
											IB_DEVICE_MEM_MGT_EXTENSIONS;	
		props->max_ah = 512;
		props->max_pd = 1024;
		props->max_mr = 256*1024;
		props->max_cq = 1024;
       	props->max_qp = 1024;
		props->max_cqe = 256;
		props->max_qp_wr = 1024;//1024;1 wr,256 wqe to process most 256 sge.
		props->max_send_sge = 2;
		props->max_recv_sge = 2;

		props->atomic_cap = 0;
		props->max_fmr = 0;
		props->max_map_per_fmr = 0;
		props->max_pkeys = 1;


		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_query_device succeed end!\n");//added by hs for printing end info	
		return 0;
}

int bxroce_query_port(struct ib_device *ibdev, u8 port, struct ib_port_attr *props)
{
		BXROCE_PR("bxroce:bxroce_query_port start!\n");//added by hs for printing start info
		enum ib_port_state port_state;
		struct bxroce_dev *dev;
		struct net_device *netdev;
		/*wait to add 2019/6/24*/
		dev = get_bxroce_dev(ibdev);
		netdev = dev->devinfo.netdev;
		BXROCE_PR("bxrpce:query_port next is netif_running\n");//added by hs
		if(netif_running(netdev) && netif_oper_up(netdev)){
			BXROCE_PR("bxroce:query_port in active\n");//added by hs 
				port_state = IB_PORT_ACTIVE;
				props->phys_state = 5;
		}
		else {
			BXROCE_PR("bxroce:query_port in down\n");//added by hs 
			port_state = IB_PORT_DOWN;
			props->phys_state = 3;
		}
		BXROCE_PR("bxroce:query_port next is lid .. \n");//added by hs 
		props->max_mtu = IB_MTU_4096;
		props->active_mtu = iboe_get_mtu(netdev->mtu);
		props->lid = 0;
		props->lmc = 0;
		props->sm_lid = 0;
		props->sm_sl = 0;
		props->state = port_state;
		props->port_cap_flags = IB_PORT_CM_SUP | IB_PORT_REINIT_SUP |
								IB_PORT_DEVICE_MGMT_SUP |
								IB_PORT_VENDOR_CLASS_SUP;
		/* not sure,need verified!*/
		props->ip_gids = true;
		props->gid_tbl_len = 16;
		props->pkey_tbl_len = 1;
		props->bad_pkey_cntr = 0;
		props->qkey_viol_cntr = 0;
		
		//not sure,change later by hs@20200429
		props->active_speed = IB_SPEED_DDR;
		props->active_width = IB_WIDTH_4X;
		props->max_msg_sz = 1 << 16;
		props->max_vl_num = 0;

		/*end */
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_query_port succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_modify_port(struct ib_device *ibdev, u8 port, int mask,
		       struct ib_port_modify *props)
{
		BXROCE_PR("bxroce:bxroce_modify_port start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_modify_port succeed end!\n");//added by hs for printing end info
		return 0;
}

#if 0	/*this funtion is not need  right now*/
enum rdma_protocol_type
bxroce_query_protocol(struct ib_device *device, u8 port_num)
{
		BXROCE_PR("bxroce:bxroce_query_protocol start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_query_protocol succeed end!\n");//added by hs for printing end info
}
#endif

void bxroce_get_guid(struct bxroce_dev *dev, u8 *guid)
{
		BXROCE_PR("bxroce:bxroce_get_guid start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		   // u8 *addr;
		   u8 mac[ETH_ALEN];
		// addr = dev->devinfo.netdev->dev_addr;
			memcpy(mac, dev->devinfo.mac_addr, ETH_ALEN);
		BXROCE_PR("bxroce:mac address is %s \n",dev->devinfo.netdev->dev_addr);//added by hs
			guid[0] = mac[0] ^ 2;
			guid[1] = mac[1];
			guid[2] = mac[2];
			guid[3] = 0xff;
			guid[4] = 0xfe;
			guid[5] = mac[3];
			guid[6] = mac[4];
			guid[7] = mac[5];
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_get_guid succeed end!\n");//added by hs for printing end info
}

int bxroce_query_gid(struct ib_device *ibdev, u8 port,
		     int index, union ib_gid *gid)
{
		/*variable declaration*/
		int ret;
		/*variable declaration*/
		BXROCE_PR("bxroce:bxroce_query_gid start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
	

		ret = 0;//alter this later, detail function not added 2019/6/24
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_query_gid succeed end!\n");//added by hs for printing end info
		return ret;
}

struct net_device *bxroce_get_netdev(struct ib_device *device, u8 port_num)
{
		BXROCE_PR("bxroce:bxroce_get_netdev start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_dev *dev;
		struct net_device * ndev;
		/*rcu make sure that the shared file is safe*/
		rcu_read_lock();
		dev = get_bxroce_dev(device);
		if(dev)
		ndev = dev->devinfo.netdev;
		if(ndev)
			dev_hold(ndev);
		rcu_read_unlock();
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_get_netdev succeed end!\n");//added by hs for printing end info
		return ndev;
}

int bxroce_add_gid(const struct ib_gid_attr *attr,
		   void **context)
{
		BXROCE_PR("bxroce:bxroce_add_gid start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_add_gid succeed end!\n");//added by hs for printing end info
		return 0;
}

int  bxroce_del_gid(const struct ib_gid_attr *attr,
		    void **context)
{
		BXROCE_PR("bxroce:bxroce_del_gid start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_del_gid succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_query_pkey(struct ib_device *ibdev, u8 port, u16 index, u16 *pkey)
{
		BXROCE_PR("bxroce:bxroce_query_pkey start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		if (index > 1)
					return -EINVAL;

			*pkey = 0xffff;
			return 0;

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_query_pkey succeed end!\n");//added by hs for printing end info
		return 0;
}

static int _bxroce_dealloc_pd(struct bxroce_dev *dev, struct bxroce_pd *pd)
{
	int status = 0;
	bxroce_drop_ref(pd);
	return status;
	
}


static struct bxroce_pd *_bxroce_alloc_pd(struct bxroce_dev *dev, struct bxroce_ucontext *uctx, struct ib_udata *udata)
{
	struct bxroce_pd *pd = NULL;

	pd = bxroce_alloc(&dev->pd_pool);

	return pd;
}


static int bxroce_alloc_ucontext_pd(struct bxroce_dev *dev, struct bxroce_ucontext *uctx, struct ib_udata *udata)
{
	int status = 0;

	uctx->ctx_pd = _bxroce_alloc_pd(dev,uctx,udata);
	if (IS_ERR(uctx->ctx_pd)) {
		status = PTR_ERR(uctx->ctx_pd);
		uctx->ctx_pd = NULL;
		goto err;
	}
	uctx->ctx_pd->uctx = uctx;
	uctx->ctx_pd->ibpd.device = &dev->ibdev;
err:
	return status;
}

static int bxroce_dealloc_ucontext_pd(struct bxroce_ucontext *uctx)
{
	struct bxroce_pd *pd = uctx->ctx_pd;
	struct bxroce_dev *dev = get_bxroce_dev(pd->ibpd.device);

	if (uctx->pd_in_use) {
		printk("pd is in use \n");
	}
	uctx->ctx_pd = NULL;
	(void)_bxroce_dealloc_pd(dev,pd);
	return 0;
}

struct ib_ucontext *bxroce_alloc_ucontext(struct ib_device *ibdev,
					  struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_alloc_ucontext start!\n");//added by hs for printing start info
		
		int status;
		struct bxroce_ucontext *ctx;
		struct bxroce_alloc_ucontext_resp resp;		
		struct bxroce_dev *dev = get_bxroce_dev(ibdev);		
		struct pci_dev *pdev = dev->devinfo.pcidev;
		u32 map_len = roundup(sizeof(u32) * 2048, PAGE_SIZE);

		if(!udata)
				return ERR_PTR(-EFAULT);
		
		ctx = kzalloc(sizeof(*ctx),GFP_KERNEL);
		if(!ctx)
				return ERR_PTR(-ENOMEM);
	
		INIT_LIST_HEAD(&ctx->mm_head);
		mutex_init(&ctx->mm_list_lock);

		ctx->ah_tbl.va = dma_alloc_coherent(&pdev->dev, map_len,&ctx->ah_tbl.pa,GFP_KERNEL);

		if (!ctx->ah_tbl.va) {
			kfree(ctx);
			return ERR_PTR(-ENOMEM);
		}
		ctx->ah_tbl.len  = map_len;

		memset(&resp, 0, sizeof(resp));
		resp.ah_tbl_len = ctx->ah_tbl.len;
		resp.ah_tbl_page = virt_to_phys(ctx->ah_tbl.va);

		status = bxroce_add_mmap(ctx,resp.ah_tbl_page, resp.ah_tbl_len);
		if(status)
			goto map_err;

		status = bxroce_alloc_ucontext_pd(dev,ctx,udata);
		if(status)
			goto pd_err;

		resp.dev_id = 0;
		resp.wqe_size = sizeof(struct bxroce_wqe);
		resp.rqe_size = sizeof(struct bxroce_rqe);
		memcpy(resp.fw_ver, &dev->attr.fw_ver, sizeof(resp.fw_ver));	

		status = ib_copy_to_udata(udata, &resp, sizeof(resp));		
		if(status)
			goto cpy_err;
		BXROCE_PR("bxroce:bxroce_alloc_ucontext succeed end!\n");//added by hs for printing end info
		return &ctx->ibucontext;

	cpy_err:
		
	pd_err:

		bxroce_del_mmap(ctx,ctx->ah_tbl.pa,ctx->ah_tbl.len);
	map_err:		
		dma_free_coherent(&pdev->dev,ctx->ah_tbl.len,ctx->ah_tbl.va,ctx->ah_tbl.pa);
		kfree(ctx);
		return ERR_PTR(status);


}

int bxroce_dealloc_ucontext(struct ib_ucontext *ibctx)
{
		/*variable declaration*/
		int status;
		/*variable declaration*/
		BXROCE_PR("bxroce:bxroce_dealloc_ucontext start!\n");//added by hs for printing start info
		struct bxroce_mm *mm, *tmp;
		struct bxroce_ucontext *uctx = get_bxroce_ucontext(ibctx);
		struct bxroce_dev *dev = get_bxroce_dev(ibctx->device);
		struct pci_dev *pdev = dev->devinfo.pcidev;

		status = bxroce_dealloc_ucontext_pd(uctx);
		bxroce_del_mmap(uctx,uctx->ah_tbl.pa,uctx->ah_tbl.len);
		dma_free_coherent(&pdev->dev,uctx->ah_tbl.len,uctx->ah_tbl.va,uctx->ah_tbl.pa);

		list_for_each_entry_safe(mm, tmp, &uctx->mm_head, entry) {
			list_del(&mm->entry);
			kfree(mm);
		} // free all the resource that mmap to user space

		kfree(uctx);

		BXROCE_PR("bxroce:bxroce_dealloc_ucontext succeed end!\n");//added by hs for printing end info
		return status;
}

/*find if a specific mmap added to list*/
static bool bxroce_find_mmap(struct bxroce_ucontext *uctx, u64 phy_addr, unsigned long len)
{
		bool found = false;
		struct bxroce_mm *mm;
		
		mutex_lock(&uctx->mm_list_lock);
		list_for_each_entry(mm, &uctx->mm_head, entry) {
			if(len != mm->key.len && phy_addr != mm->key.phy_addr)
				continue;

			found = true;
			break;
		}
		mutex_unlock(&uctx->mm_list_lock);
		return found;
}



/*bxroce_mmap*/
int bxroce_mmap(struct ib_ucontext *ibctx, struct vm_area_struct *vma)
{
		/*variable declaration*/
		int status;
		/*variable declaration*/
		BXROCE_PR("bxroce:bxroce_mmap start!\n");//added by hs for printing start info
		struct bxorce_ucontext *ucontext = get_bxroce_ucontext(ibctx);
		struct bxroce_dev *dev = get_bxroce_dev(ibctx->device);
		unsigned long vm_page = vma->vm_pgoff << PAGE_SHIFT;
		u64 regaddr = dev->ioaddr;
		unsigned long len =(vma->vm_end - vma->vm_start);
		bool found;

		if(vma->vm_start & (PAGE_SIZE - 1))
		{ 
			printk("bxroce: vm start not & PAGE_SIZE");
			return -EINVAL;
		}
		found = bxroce_find_mmap(ucontext, vma->vm_pgoff << PAGE_SHIFT, len);
		if(!found)
				return -EINVAL;
		/*if it is io memory, need io_remap_pfn_range*/
		if ((vm_page >= regaddr) && ((vm_page <= (regaddr + SQ_REG_LEN))) && (len <= SQ_REG_LEN))
		{
			BXROCE_PR("bxroce: mmap io space\n");
			/*write combine,and bypass cache system, may help performance.*/
			 vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
			 status = io_remap_pfn_range( vma, vma->vm_start, vma->vm_pgoff, len, vma->vm_page_prot);

		}
		else{
		/*if not rempa_pfn_range*/
			BXROCE_PR("bxroce: mmap queue\n");
			status = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, len, vma->vm_page_prot);
		}
		BXROCE_PR("bxroce:bxroce_mmap succeed end!\n");//added by hs for printing end info
		return status;
}


static struct bxroce_pd *bxroce_get_ucontext_pd(struct bxroce_ucontext *uctx)
{
	struct bxroce_pd *pd = NULL;

	mutex_lock(&uctx->mm_list_lock);
	if (!uctx->pd_in_use) {
		uctx->pd_in_use = true;
		pd = uctx->ctx_pd;
	}
	mutex_unlock(&uctx->mm_list_lock);
	
	return pd;
}

static inline int is_ucontext_pd(struct bxroce_ucontext *uctx, struct bxroce_pd *pd)
{
	return (uctx->ctx_pd == pd);
}

static void bxroce_release_ucontext_pd(struct bxroce_ucontext *uctx)
{
	mutex_lock(&uctx->mm_list_lock);
	uctx->pd_in_use = false;
	mutex_unlock(&uctx->mm_list_lock);
}


struct ib_pd *bxroce_alloc_pd(struct ib_device *ibdev,
			  struct ib_ucontext *ibctx, struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_alloc_pd start!\n");//added by hs for printing start info
		struct bxroce_pd *pd;
		struct bxroce_dev *dev = get_bxroce_dev(ibdev);
		/*wait to add 2019/6/24*/
		struct bxroce_ucontext *uctx = NULL;
		u8 is_uctx_pd = false;

		if(ibctx){
		BXROCE_PR("bxroce: alloc user pd\n");
		uctx = get_bxroce_ucontext(ibctx);
		pd = bxroce_get_ucontext_pd(uctx);
			if (pd) {
				 is_uctx_pd = true;
				 goto pd_mapping;
			}

		}

		pd = _bxroce_alloc_pd(dev,uctx,udata);

pd_mapping:
		if(pd)
		BXROCE_PR("pd is exist\n");//added by hs	
		/*wait to add end!*/	
		if(ibctx){
			BXROCE_PR("bxroce: get uctx \n");
			pd->uctx = uctx;
		}
		BXROCE_PR("bxroce:bxroce_alloc_pd succeed end!\n");//added by hs for printing end info
		return pd ? &pd->ibpd:ERR_PTR(-ENOMEM);
}



int bxroce_dealloc_pd(struct ib_pd *pd)
{
		BXROCE_PR("bxroce:bxroce_dealloc_pd start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_pd *bxpd = get_bxroce_pd(pd);
		struct bxroce_dev *dev = get_bxroce_dev(pd->device);
		struct bxroce_ucontext *uctx = NULL;
		int status = 0;

		uctx = bxpd->uctx;
		if (uctx) {
			if (is_ucontext_pd(uctx, bxpd)) {
				bxroce_release_ucontext_pd(uctx);
				return status;
			}

		}

		status = _bxroce_dealloc_pd(dev,bxpd);
		BXROCE_PR("bxroce:bxroce_dealloc_pd succeed end!\n");//added by hs for printing end info
		return status;
	
}

static int bxroce_copy_cq_uresp(struct bxroce_dev *dev, struct bxroce_cq *cq, struct ib_udata *udata, struct ib_ucontext *ib_ctx)
{
		int status;
		struct bxroce_create_cq_uresp uresp;
		BXROCE_PR("bxroce:%s start1 \n",__func__);
		struct bxroce_ucontext *uctx = get_bxroce_ucontext(ib_ctx);
		BXROCE_PR("bxroce:%s start2 \n",__func__);
		memset(&uresp, 0 ,sizeof(uresp));
		uresp.cq_id = cq->id;
		uresp.page_size = PAGE_ALIGN(cq->len);
		uresp.num_pages = 1;
		uresp.max_hw_cqe= dev->attr.max_cqe;
		uresp.txpage_addr[0] = virt_to_phys(cq->txva);
		uresp.rxpage_addr[0] = virt_to_phys(cq->rxva);
		uresp.xmitpage_addr[0] = virt_to_phys(cq->xmitva);
		uresp.phase_change = 0;
		BXROCE_PR("bxroce:%s copy to user space \n",__func__);
		BXROCE_PR("bxroce:uresp's size: 0x%x , udata'outlen: 0x%x \n",sizeof(uresp),udata->outlen);
		status = ib_copy_to_udata(udata, &uresp, sizeof(uresp));
		if (status) {
			BXROCE_PR("%s copy error cqid = 0x%x \n",__func__,cq->id);
			goto err;
		}
		BXROCE_PR("bxroce: %s add mmap to txcq\n",__func__);
		status = bxroce_add_mmap(uctx,uresp.txpage_addr[0],uresp.page_size);
		if(status)
			goto err;

		BXROCE_PR("bxroce: %s add mmap to xmitcq \n",__func__);
		status = bxroce_add_mmap(uctx,uresp.rxpage_addr[0],uresp.page_size);
		if(status){
			bxroce_del_mmap(uctx,uresp.txpage_addr[0],uresp.page_size);
			goto err;
		}
		status = bxroce_add_mmap(uctx,uresp.xmitpage_addr[0],uresp.page_size);
		if (status) {
			bxroce_del_mmap(uctx,uresp.txpage_addr[0],uresp.page_size);
			bxroce_del_mmap(uctx,uresp.rxpage_addr[0],uresp.page_size);
			goto err;
		}
		cq->uctx = uctx;
	err:
		return status;

}


/*free resources*/
static void bxroce_free_cqresource(struct bxroce_dev *dev, struct bxroce_cq *cq)
{
		struct pci_dev *pdev = dev->devinfo.pcidev;
		unsigned long flags;
		/*free kernel dma memory*/
		dma_free_coherent(&pdev->dev,cq->len,cq->txva,(dma_addr_t)cq->txpa);
		dma_free_coherent(&pdev->dev,cq->len,cq->rxva,(dma_addr_t)cq->rxpa);
		dma_free_coherent(&pdev->dev,cq->len,cq->xmitva,(dma_addr_t)cq->xmitpa);

		/*free va*/
		cq->txva = NULL;
		cq->rxva= NULL;
		cq->xmitva = NULL;

		bxroce_free_cqqpresource(dev,dev->allocated_cqs, cq->id);
		
}


/*bxroce_create_cq*/
struct ib_cq *bxroce_create_cq(struct ib_device *ibdev,
			       const struct ib_cq_init_attr *attr,
			       struct ib_ucontext *ib_ctx,
			       struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_create_cq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		int entries = attr->cqe;
		int vector = attr->comp_vector;
		struct bxroce_cq *cq;
		struct bxroce_dev *dev;
		struct bxroce_ucontext *uctx = NULL;
		u16 pd_id = 0;
		int status;
		u32 cq_num = 0;
		struct bxroce_create_cq_ureq ureq;

		dev = get_bxroce_dev(ibdev);

		BXROCE_PR("bxroce: entries is %d, flags is %d\n",entries,attr->flags);//added by hs
		if (udata) {
			BXROCE_PR("bxroce: user create cq, copy udata ..\n");
			if(ib_copy_from_udata(&ureq, udata, sizeof(ureq)))
				return ERR_PTR(-EFAULT);
		}else
			BXROCE_PR("bxroce:create_cq in kernel\n");//added by hs 

		cq = kzalloc(sizeof(*cq),GFP_KERNEL);
		if(!cq)
			return ERR_PTR(-ENOMEM);

		spin_lock_init(&cq->lock);
		
		//add ib_ctx process by hs@20200429
		if (ib_ctx) {
			 BXROCE_PR("create cq in user\n");
			 uctx = get_bxroce_ucontext(ib_ctx);
			 //pd_id = uctx->ctx_pd->id;
		}
		
		status = bxroce_alloc_cqqpresource(dev,dev->allocated_cqs,dev->attr.max_cq,&cq_num,&dev->next_cq);
		if (status)
		{
			printk("bxroce_alloc_resource failed\n");//added by hs 
			return ERR_PTR(status);
		}
		cq->id = cq_num;
		BXROCE_PR("bxroce: create_cq for cq_num is %d \n",cq_num);//added by hs 
		/*create cq -- access hw for these*/
		status = bxroce_hw_create_cq(dev,cq,entries,pd_id);
		if (status) {
			kfree(cq);
			return ERR_PTR(status); 
		}

		if (ib_ctx) {
			status = bxroce_copy_cq_uresp(dev,cq,udata,ib_ctx);
			if(status)
				goto err1;
		}

		dev->cq_table[cq->id] = cq;//for storing cq.
		BXROCE_PR("bxroce:bxroce_create_cq succeed end!\n");//added by hs for printing end info
		return &cq->ibcq;
	err1:
		bxroce_free_cqresource(dev,cq);
		kfree(cq);
		return ERR_PTR(status);
}

int bxroce_resize_cq(struct ib_cq *ibcq, int cqe, struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_resize_cq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		int status = 0;
		struct bxroce_cq *cq = get_bxroce_cq(ibcq);

		if (cqe < 1 || cqe > cq->max_hw_cqe) {
			status  = -EINVAL;
			return status;
		}
		ibcq->cqe = cqe;
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_resize_cq succeed end!\n");//added by hs for printing end info
		return status;
}


/*destroy cq*/
int bxroce_destroy_cq(struct ib_cq *ibcq)
{
		BXROCE_PR("bxroce:bxroce_destroy_cq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_cq *cq;
		struct bxroce_dev *dev;
		cq = get_bxroce_cq(ibcq);
		dev = get_bxroce_dev(ibcq->device);

		dev->cq_table[cq->id] = NULL;
		//need add: add flush cq fucntion to flush the cq(tx,rx,xmit).

		bxroce_free_cqresource(dev,cq);
		if (cq->uctx) {
			bxroce_del_mmap(cq->uctx,(u64)cq->txpa,PAGE_ALIGN(cq->len));
			bxroce_del_mmap(cq->uctx,(u64)cq->rxpa,PAGE_ALIGN(cq->len));
			bxroce_del_mmap(cq->uctx,(u64)cq->xmitpa,PAGE_ALIGN(cq->len));
		}

		kfree(cq);
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_destroy_cq succeed end!\n");//added by hs for printing end info
		return 0;
}

static int bxroce_check_qp_params(struct ib_pd *ibpd, struct bxroce_dev *dev,
								   struct ib_qp_init_attr *attrs, struct ib_udata *udata)
{
	   
		BXROCE_PR("--------------check qp1's ib_qp_init_attr start---------------\n");//added by hs
//		BXROCE_PR("cap.max_send_wr is %d \n",attrs->cap.max_send_wr);
//		BXROCE_PR("cap.max_recv_wr is %d \n",attrs->cap.max_recv_wr);
//		BXROCE_PR("cap.max_send_sge is %d \n",attrs->cap.max_send_sge);
//		BXROCE_PR("cap.max_recv_sge is %d \n",attrs->cap.max_recv_sge);
//		BXROCE_PR("cap.max_inline_data is %d \n",attrs->cap.max_inline_data);
//		BXROCE_PR("cap.max_rdma_ctxs is %d \n",attrs->cap.max_rdma_ctxs);
//		BXROCE_PR("qptype is %d \n",attrs->qp_type);
//		BXROCE_PR("port_num is %d \n",attrs->port_num);
//		BXROCE_PR("source_qpn is %d \n",attrs->source_qpn);
//		BXROCE_PR("--------------check qp1's ib_qp_init_attr  end---------------\n");//added by hs

		if ((attrs->qp_type != IB_QPT_GSI) &&
				(attrs->qp_type != IB_QPT_RC) &&
				(attrs->qp_type != IB_QPT_UC) &&
				(attrs->qp_type != IB_QPT_UD)) {
				printk("%s unsupported qp type = 0x%x requested \n",__func__,attrs->qp_type);
				return -EINVAL;
		}
		BXROCE_PR("bxroce: gsi ,max_send_wr \n");//added by hs
	    if ((attrs->qp_type != IB_QPT_GSI) &&
				(attrs->cap.max_send_wr > dev->attr.max_qp_wr)) {
					printk("bxroce: %s unsupported send_wr =0x%x requested\n",__func__,attrs->cap.max_send_wr);//added by hs
					printk("bxroce: %s unsupported send_wr = 0x%x\n",__func__,dev->attr.max_qp_wr);//added by hs 
					return -EINVAL;
		}
//	    if (!attrs->srq && (attrs->cap.max_recv_wr > dev->attr.max_qp_wr)) {
//               pr_err("%s unsupported recv_wr=0x%x requested\n",
//                       __func__,attrs->cap.max_recv_wr);
//                pr_err("%s(%d) supported recv_wr=0x%x\n",
//                       __func__,dev->attr.max_qp_wr);
//               return -EINVAL;
//        }
		BXROCE_PR("bxroce: cap.max_inline-data \n");//added by hs
        if (attrs->cap.max_inline_data > 0) {
                pr_err("%s unsupported inline data size=0x%x requested\n",
                       __func__,attrs->cap.max_inline_data);
                pr_err("%s supported inline data size=0\n",
                       __func__);
                return -EINVAL;
        }
		if (attrs->cap.max_send_sge > dev->attr.max_send_sge) {
			pr_err("%s unsupported send_sge=0x%x requested \n",__func__,attrs->cap.max_send_sge);
			pr_err("%s supported send_sge = 0x%x \n",__func__,dev->attr.max_send_sge);
			return -EINVAL;
		}
		if (attrs->cap.max_recv_sge > dev->attr.max_recv_sge) {
			pr_err("%s unsupported send_sge=0x%x requested \n",__func__,attrs->cap.max_recv_sge);
			pr_err("%s supported send_sge = 0x%x \n",__func__,dev->attr.max_recv_sge);
			return -EINVAL;
		}
		if (attrs->qp_type == IB_QPT_GSI && udata) {
			pr_err("Userpace can't create special QPs of type = 0x %x \n",__func__,attrs->qp_type);
		}

	BXROCE_PR("bxcroce:check qp param end \n");//added by hs

		

		 return 0;

}

/*bxroce_set_qp_init_params. To get init params to private qp bxroce_qp*/
static void bxroce_set_qp_init_params(struct bxroce_qp *qp, struct bxroce_pd *pd, struct ib_qp_init_attr *attrs)
{
		BXROCE_PR("bxroce: set_qp_init params \n");//added by hs
		qp->pd = pd;
		spin_lock_init(&qp->q_lock);
		INIT_LIST_HEAD(&qp->sq_entry);
		INIT_LIST_HEAD(&qp->rq_entry);


		qp->qp_type = attrs->qp_type;
		qp->max_inline_data = attrs->cap.max_inline_data;
		qp->sq.max_sges = attrs->cap.max_send_sge;
		qp->rq.max_sges = attrs->cap.max_recv_sge;
		qp->qp_state = BXROCE_QPS_RST;
		qp->signaled = (attrs->sq_sig_type == IB_SIGNAL_ALL_WR) ? true:false;
		qp->qp_change_info->signaled = qp->signaled;
		BXROCE_PR("bxroce: set_qp_init params end .. \n");//added by hs
}

static int bxroce_alloc_wr_id_tbl(struct bxroce_qp *qp)
{
		qp->wqe_wr_id_tbl =
				kcalloc(qp->sq.max_cnt, sizeof(*(qp->wqe_wr_id_tbl)),
						GFP_KERNEL);
			if (qp->wqe_wr_id_tbl == NULL)
					return -ENOMEM;
			qp->rqe_wr_id_tbl =
				kcalloc(qp->rq.max_cnt, sizeof(u64), GFP_KERNEL);
			if (qp->rqe_wr_id_tbl == NULL)
					return -ENOMEM;

			return 0;

}

static int bxroce_copy_qp_uresp(struct bxroce_qp *qp, struct ib_udata *udata)
{
	int status;
	u64 ioaddr;
	u32 reg_len;
	struct bxroce_create_qp_uresp uresp;
	struct bxroce_pd *pd = qp->pd;
	struct bxroce_dev *dev = get_bxroce_dev(pd->ibpd.device);
	struct qp_change_info *qp_change_info;

	qp_change_info = qp->qp_change_info;
	BXROCE_PR("qp_change_info: %lx \n",qp_change_info);

	memset(&uresp, 0, sizeof(uresp));
	ioaddr = dev->ioaddr + PGU_BASE;//just need 0x10 0x100 to get access hw reg.
	reg_len = SQ_REG_LEN;

	
	uresp.qp_id = qp->id;
	uresp.sq_page_size = PAGE_ALIGN(qp->sq.len);
	uresp.sq_page_addr[0] = virt_to_phys(qp->sq.va);
	uresp.rq_page_size = PAGE_ALIGN(qp->rq.len);
	uresp.rq_page_addr[0] = virt_to_phys(qp->rq.va);
	uresp.num_wqe_allocated = qp->sq.max_cnt;
	uresp.num_rqe_allocated = qp->rq.max_cnt;
	uresp.num_sq_pages = 1;
	uresp.num_rq_pages = 1;
	uresp.ioaddr = ioaddr;
	uresp.reg_len = reg_len;
	uresp.qp_info_addr = virt_to_phys(qp_change_info);
	uresp.qp_info_len =PAGE_ALIGN(sizeof(*qp_change_info));
	BXROCE_PR("uresp.qp_info_addr:%x, uresp.qp_info_len:%x \n",uresp.qp_info_addr, uresp.qp_info_len);

	status = ib_copy_to_udata(udata, &uresp, sizeof(uresp));
	if (status) {
			printk("bxroce:%s,copy to udata failed \n",__func__);//added by hs
			goto err;
	}
	status = bxroce_add_mmap(pd->uctx, uresp.sq_page_addr[0], uresp.sq_page_size);
	if (status) {
			printk("bxroce:%s, add mmap failed \n",__func__);
			goto err;
	}
	status = bxroce_add_mmap(pd->uctx, uresp.rq_page_addr[0], uresp.rq_page_size);
	if (status) {
			printk("bxroce:%s, add mmap failed \n",__func__);
			goto err1;
	}
	
	status = bxroce_add_mmap(pd->uctx, uresp.ioaddr, uresp.reg_len);
	if (status) {
			printk("bxroce:%s, ioaddr add mmap failed \n",__func__);
			goto err2;
	}
	status = bxroce_add_mmap(pd->uctx,uresp.qp_info_addr,uresp.qp_info_len);
	if (status) {
			printk("bxroce:%s, qpinfo add mmap failed \n",__func__);
			goto err3;
	}

	return status;

err3:
	bxroce_del_mmap(pd->uctx, uresp.ioaddr, uresp.reg_len);
err2:
	bxroce_del_mmap(pd->uctx, uresp.rq_page_addr[0], uresp.rq_page_size);
err1:
	bxroce_del_mmap(pd->uctx, uresp.sq_page_addr[0], uresp.sq_page_size);
err:
	return status;

}

static void bxroce_store_qp1_resource(struct bxroce_dev *dev, struct ib_qp_init_attr *attrs)
{
	if (attrs->qp_type == IB_QPT_GSI) {
		dev->Is_qp1_allocated = 1;
		dev->gsi_sqcq = get_bxroce_cq(attrs->send_cq);
		dev->gsi_rqcq = get_bxroce_cq(attrs->recv_cq);
	}
}


struct ib_qp *bxroce_create_qp(struct ib_pd *ibpd,
			       struct ib_qp_init_attr *attrs,
			       struct ib_udata *udata)
{
		BXROCE_PR("bxroce: bxroce_create_qp start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_dev *dev =NULL;
		struct bxroce_qp *qp;
		struct bxroce_pd *pd = NULL;
		struct bxroce_cq *cq = NULL;
		struct bxroce_cq *rqcq = NULL;
		struct bxroce_create_qp_ureq ureq;
		struct qp_change_info *qp_change_info;
		int status;
		u32 qp_num = 0;
		int sq_size;
		int rq_size;
		/*get some kernel private data*/

		sq_size = attrs->cap.max_send_wr;
		rq_size = attrs->cap.max_recv_wr;
		pd = get_bxroce_pd(ibpd);
		dev = get_bxroce_dev(ibpd->device);
		cq = get_bxroce_cq(attrs->send_cq); // To get cq? but Most important that is send_cq && recv_cq  the same one.
		rqcq = get_bxroce_cq(attrs->recv_cq);

		if(!cq){
			printk("bxroce: cq is null \n");//added by hs 
			return -ENOMEM;
		}

		memset(&ureq, 0 ,sizeof(ureq));

		/*check attrs is valid or not*/
		status = bxroce_check_qp_params(ibpd,dev,attrs,udata);
		if (status) {
			printk("bxroce: check qp error \n");//added by hs 
			return status;
		}

		memset(&ureq, 0 ,sizeof(ureq));
		if (udata) {
			BXROCE_PR("bxroce: user copy data from user space\n");
			if(ib_copy_from_udata(&ureq, udata, sizeof(ureq)))
					return ERR_PTR(-EFAULT);
		}

		/*allocate memory for private qp*/
		qp = kzalloc(sizeof(*qp),GFP_KERNEL);
		if (!qp) {
			printk("bxroce: qp is null \n");//added by hs 
			return -ENOMEM;
		}
		qp_change_info = kzalloc(PAGE_ALIGN(sizeof(*qp_change_info)),GFP_KERNEL);
		if (!qp_change_info) {
			printk("bxroce: qp_change_info is null \n");
			return -ENOMEM;
		}

		qp->pd = pd;
		qp->qp_change_info = qp_change_info;
		/*get attrs to private qp */
		bxroce_set_qp_init_params(qp,pd,attrs);

		/*alloate id for qp,should consider servral situation*/
		if(attrs->qp_type == IB_QPT_SMI)/*In Roce, SM is not supportted*/
			return -EINVAL;
		else if(attrs->qp_type == IB_QPT_GSI)
			{	
			if(!dev->Is_qp1_allocated) 
					{qp_num = 1; dev->Is_qp1_allocated = true;}
			else
				return -EINVAL;
			}		
		else 
			status = bxroce_alloc_cqqpresource(dev,dev->allocated_qps,dev->attr.max_qp,&qp_num,&dev->next_qp);
		qp->id = qp_num;
		qp->ibqp.qp_num = qp_num;
		BXROCE_PR("bxroce: create_qp for qp_num is %d\n",qp_num);//added by hs ;
		
		/*kenrel create qp*/
		mutex_lock(&dev->dev_lock); 
		status = bxroce_hw_create_qp(dev,qp,pd,attrs);
			if (status) {
				kfree(qp);
				return ERR_PTR(status);
		}
				
		/*alloc wr_id table*/
		if (udata == NULL) {
			status = bxroce_alloc_wr_id_tbl(qp);
			if(status)
				goto map_err;
		}

		/*storage the qp id,check if it is valid */
		if(qp->id < 1024 && dev->qp_table[qp->id] == NULL)
		dev->qp_table[qp->id] = qp; // store the qp in dev struc.
		else{
			status = -EINVAL;
			goto map_err;
		}

		if (udata) {
			BXROCE_PR("bxroce:%s, copy to user data \n",__func__);//added by hs
			status = bxroce_copy_qp_uresp(qp,udata);
			if(status)
				goto map_err;

		}

		bxroce_store_qp1_resource(dev,attrs);

		mutex_unlock(&dev->dev_lock);
		BXROCE_PR("bxroce: bxroce_create_qp succeed end!\n");//added by hs for printing end info
		return &qp->ibqp;
	map_err:
		dev->qp_table[qp->id] = NULL;
	map_err1:

		printk("bxroce: bxroce_create_qp map err\n");//added by hs
		(void)_bxroce_destroy_qp(dev,qp);
		mutex_unlock(&dev->dev_lock);
		kfree(qp->wqe_wr_id_tbl);
		kfree(qp->rqe_wr_id_tbl);
		kfree(qp);

		return ERR_PTR(status);
}

int _bxroce_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		      int attr_mask)
{
		BXROCE_PR("bxroce:bxroce_modify_qp start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		int status = 0;
		int tmpval = 0;
		struct bxroce_qp *qp;
		struct bxroce_dev *dev;
		enum ib_qp_state cur_state,new_state;
		void __iomem* base_addr;
		u32 destqp;
		u32 wqepagesize = 0;
		u32 cfgenable =0;
		//u64 pa= 0;
		//u32 pa_l = 0;
		//u32 pa_h = 0;
		int service_type;
		struct rnic_pdata *rnic_pdata;

		qp = get_bxroce_qp(ibqp);
		dev = get_bxroce_dev(ibqp->device);
		u32 lqp = qp->id;
		base_addr = dev->devinfo.base_addr;
		BXROCE_PR("bxroce:bxroce_modify_qp qp_state_change\n");//added by hs	

		if(attr_mask & IB_QP_STATE)
			status = bxroce_qp_state_change(qp,attr->qp_state,&cur_state);
		if(status < 0)
			return status;

		/*wait to add end!*/	
		status = bxroce_set_qp_params(qp,attr,attr_mask);
		if(status)
			return status;

		if(qp->qp_state == BXROCE_QPS_RTR)//now we may have got dest qp.we should map destqp and srcqp.
		{
			/*access hw for map destqp and srcqp*/
			BXROCE_PR("bxroce: modify_qp in RTR,map destqp and srcqp\n");//added by hs 
			destqp = qp->destqp;
			tmpval = 1;

			//mapping src and dest qp. 
			//for test, local test may only mapping once a time.
			if(1 /*lqp < destqp*/ ){
			bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,SRCQP,lqp);
			bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,DESTQP,destqp);
			bxroce_mpb_reg_write(dev,base_addr,PGU_BASE,RC_QPMAPPING,0x1);
			
			while (tmpval != 0)
			{
				tmpval = bxroce_mpb_reg_read(dev,base_addr,PGU_BASE,RC_QPMAPPING);
			}
			BXROCE_PR("bxroce:rc mapping success lqp:%d rqp:%d\n",lqp,destqp);//added by hs
			
			
			rnic_pdata = dev->devinfo.rnic_pdata;
			switch (qp->qp_type) {
			case IB_QPT_RC:
				service_type = RC_TYPE;break;
			case IB_QPT_RESERVED2:
				service_type = RD_TYPE;break; //ERR, change later by hs
			case IB_QPT_UD:
				service_type = UD_TYPE;break;
			default:
				service_type = -1;
			}
			
			if(service_type >= 0){
			pbu_init_for_recv_req(rnic_pdata,service_type,qp->destqp,0x000,0x0,qp->pkey_index,qp->qkey);
			pbu_init_for_recv_rsp(rnic_pdata,service_type,qp->id,0x0,qp->pkey_index);
			}
			
			//bxroce_mpb_reg_write(devbase_addr,PGU_BASE,INTRMASK,0x7fff);//open all mask
			}

		}
		if (qp->qp_state == BXROCE_QPS_RTS)
		{
			BXROCE_PR("TO RTS\n");
			
		}

		BXROCE_PR("bxroce:bxroce_modify_qp succeed end!\n");//added by hs for printing end info
		return status;
}

int bxroce_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		     int attr_mask, struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_modify_qp start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		unsigned long flags;
		struct bxroce_qp *qp;
		struct bxroce_dev *dev;
		enum ib_qp_type qp_type;
		enum ib_qp_state cur_state, new_state;
		int status = -EINVAL;

		qp = get_bxroce_qp(ibqp);
		dev = get_bxroce_dev(ibqp->device);
	
		mutex_lock(&dev->dev_lock);
		spin_lock_irqsave(&qp->q_lock, flags);
		cur_state = get_ibqp_state(qp->qp_state);
		if(attr_mask & IB_QP_STATE)
			new_state = attr->qp_state;
		else
			new_state = cur_state;
		spin_unlock_irqrestore(&qp->q_lock,flags);

		if (!ib_modify_qp_is_ok(cur_state, new_state, ibqp->qp_type, attr_mask)) {
			printk("%s invalid attribute mask=0x%x specified for\n"
				   "qpn=0x%x of type=0x%x old_qps=0x%x, new_qps=0x%x\n",
				   __func__,attr_mask,qp->id,ibqp->qp_type,cur_state,new_state);//added by hs 
		}

		status = _bxroce_modify_qp(ibqp,attr,attr_mask);
		if(status > 0)
			status = 0;
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_modify_qp succeed end!\n");//added by hs for printing end info
	
		mutex_unlock(&dev->dev_lock);
		return status;
}

int bxroce_query_qp(struct ib_qp *ibqp,
		    struct ib_qp_attr *qp_attr,
		    int qp_attr_mask, struct ib_qp_init_attr *init_attr)
{
		BXROCE_PR("bxroce:bxroce_query_qp start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_query_qp succeed end!\n");//added by hs for printing end info
		return 0;
}

int _bxroce_destroy_qp(struct bxroce_dev *dev,struct bxroce_qp *qp)
{
	struct pci_dev *pdev;
	
	pdev = dev->devinfo.pcidev;	
	if(qp->sq.va)
			dma_free_coherent(&pdev->dev,qp->sq.len,qp->sq.va,qp->sq.pa);
		if(qp->rq.va)
			dma_free_coherent(&pdev->dev,qp->rq.len,qp->rq.va,qp->rq.pa);
	
		bxroce_free_cqqpresource(dev,&dev->allocated_qps,qp->id);
		return 0;
}


int bxroce_destroy_qp(struct ib_qp *ibqp)
{
		BXROCE_PR("bxroce:bxroce_destroy_qp start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_qp *qp;
		struct bxroce_pd *pd;
		struct bxroce_dev *dev;
		struct pci_dev *pdev;
		unsigned long flags;
		u64 ioaddr;

		dev = get_bxroce_dev(ibqp->device);
		qp = get_bxroce_qp(ibqp);
		pd = qp->pd;
		pdev = dev->devinfo.pcidev;

		mutex_lock(&dev->dev_lock);
		(void)_bxroce_destroy_qp(dev,qp);

		spin_lock_irqsave(&qp->sq_cq->lock, flags);
		if (qp->rq_cq && (qp->rq_cq != qp->sq_cq)){
			 spin_lock(&qp->rq_cq->lock);
			 dev->qp_table[qp->id] = NULL;
			 spin_unlock(&qp->rq_cq->lock);
		}
		else {
			dev->qp_table[qp->id] = NULL;
		}
		spin_unlock_irqrestore(&qp->sq_cq->lock, flags);

		if (!pd->uctx) {
			/*access hw to clear the cqes that not processed*///by hs@20200501
		}
		mutex_unlock(&dev->dev_lock);

		ioaddr = dev->ioaddr + PGU_BASE;
		if (pd->uctx) {
			bxroce_del_mmap(pd->uctx,(u64)qp->sq.pa,PAGE_ALIGN(qp->sq.len));
			bxroce_del_mmap(pd->uctx,(u64)qp->rq.pa,PAGE_ALIGN(qp->rq.len));
			bxroce_del_mmap(pd->uctx,(u64)ioaddr,PAGE_ALIGN(SQ_REG_LEN));
		}

		kfree(qp->wqe_wr_id_tbl);
		kfree(qp->rqe_wr_id_tbl);
		kfree(qp);
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_destroy_qp succeed end!\n");//added by hs for printing end info
		return 0;
}
//void bxroce_del_flush_qp(struct ocrdma_qp *qp);

struct ib_srq *bxroce_create_srq(struct ib_pd *ibpd, struct ib_srq_init_attr *init_attr,struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_create_srq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_create_srq succeed end!\n");//added by hs for printing end info
		return NULL;
}

int bxroce_modify_srq(struct ib_srq *ibsrq, struct ib_srq_attr *srq_attr,
		      enum ib_srq_attr_mask srq_attr_mask, struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_modify_srq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_modify_srq succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_query_srq(struct ib_srq *ibsrq, struct ib_srq_attr *srq_attr)
{
		BXROCE_PR("bxroce:bxroce_query_srq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_query_srq succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_destroy_srq(struct ib_srq *ibsrq)
{
		BXROCE_PR("bxroce:bxroce_destroy_srq start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_destroy_srq succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_post_srq_recv(struct ib_srq *ibsrq, struct ib_recv_wr *wr,
			 struct ib_recv_wr **bad_recv_wr)
{
		BXROCE_PR("bxroce:bxroce_post_srq_recv start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_post_srq_recv succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_dereg_mr(struct ib_mr *ibmr)
{
		BXROCE_PR("bxroce:bxroce_dereg_mr start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		//struct bxroce_dev *dev;
		struct bxroce_mr *mr;

		mr = get_bxroce_mr(ibmr);
		if(mr)
		BXROCE_PR("mr exist\n");//added by hs 
		else 
			return 0;
		mr->state = BXROCE_MEM_STATE_ZOMBIE;
		bxroce_drop_ref(mr->pd);
		bxroce_drop_index(mr);
		bxroce_drop_ref(mr);
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_dereg_mr succeed end!\n");//added by hs for printing end info
		return 0;
}
static int bxroce_alloc_lkey(struct bxroce_dev *dev, struct bxroce_mr *mr, u32 pdid, int acc)
{
		int status;
		BXROCE_PR("bxroce: bxroce_alloc_lkey start\n");//added by hs
	
		BXROCE_PR("bxroce: bxroce_alloc_lkey end\n");//added by hs 
		return 0;
}

struct ib_mr *bxroce_get_dma_mr(struct ib_pd *ibpd, int acc)
{
		BXROCE_PR("bxroce:bxroce_get_dma_mr start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		int status;
		struct bxroce_pd *pd;
		struct bxroce_mr *mr;
		struct bxroce_dev *dev;
		u32 pdn = 0;
		pd = get_bxroce_pd(ibpd);
		dev = get_bxroce_dev(ibpd->device);
		int err;
		#if 0 // deled by hs, may change it later
		if (acc & IB_ACCESS_REMOTE_WRITE && !(acc & IB_ACCESS_LOCAL_WRITE)){
			pr_err("%s err, invalid access rights \n",__func__);
			return ERR_PTR(-EINVAL);
		}
		#endif

		mr = bxroce_alloc(&dev->mr_pool);
		if (!mr) {
			err = -ENOMEM;
			goto err1;
		}
		bxroce_add_index(mr);
		bxroce_add_ref(pd);

		err = bxroce_mem_init_dma(pd,acc,mr);
		if(err)
			goto err2;

		/*wait to add end!*/
		BXROCE_PR("bxroce:bxroce_get_dma_mr succeed end!\n");//added by hs for printing end info
		return &mr->ibmr;
	err2:
		bxroce_drop_ref(pd);
		bxroce_drop_index(mr);
		bxroce_drop_ref(mr);
	err1:
		return ERR_PTR(err);
}

/*bxroce_reg_user_mr*/
struct ib_mr *bxroce_reg_user_mr(struct ib_pd *ibpd, u64 start, u64 length,
				 u64 iova, int acc, struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_reg_user_mr start!\n");//added by hs for printing start info
		int status;
		struct bxroce_dev *dev = get_bxroce_dev(ibpd->device);
		struct bxroce_mr *mr;
		struct bxroce_pd *pd;
		struct bxroce_reg_mr_ureq ureq;
		
		pd = get_bxroce_pd(ibpd);

		mr = bxroce_alloc(&dev->mr_pool);
		if (!mr) {
			status = -ENOMEM;
			goto err2;
		}
		
		if (udata) {
			BXROCE_PR("bxroce: user create cq, copy udata ..\n");
			if(ib_copy_from_udata(&ureq, udata, sizeof(ureq)))
				return ERR_PTR(-EFAULT);
		}
		printk("bxroce:user's addr is 0x%x \n",ureq.user_addr);

		bxroce_add_index(mr);

		bxroce_add_ref(pd);

		status = bxroce_mem_init_user(pd,start,length,iova,acc,udata,mr);
		if(status)
			goto err3;
		BXROCE_PR("bxroce:%s end\n",__func__);
		return &mr->ibmr;
err3:
		bxroce_drop_ref(pd);
		bxroce_drop_index(mr);
		bxroce_drop_ref(mr);
err2:
		return ERR_PTR(status);
}

struct ib_mr *bxroce_alloc_mr(struct ib_pd *pd,
			      enum ib_mr_type mr_type,
			      u32 max_num_sg)
{
		BXROCE_PR("bxroce:bxroce_alloc_mr start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_dev *dev = get_bxroce_dev(pd->device);
		struct bxroce_pd *bxpd = get_bxroce_pd(pd);
		struct bxroce_mr *mr;
		int err;

		if(mr_type != IB_MR_TYPE_MEM_REG)
				return	ERR_PTR(-EINVAL);
		mr = bxroce_alloc(&dev->mr_pool);
		if (!mr) {
				err = -ENOMEM;
				goto err1;
		}
		bxroce_add_index(mr);
		bxroce_add_ref(bxpd);

		err = bxroce_mem_init_fast(dev,max_num_sg,mr);
		if(err)
			goto err2;
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_alloc_mr succeed end!\n");//added by hs for printing end info
		return &mr->ibmr;
	err2:
		bxroce_drop_ref(bxpd);
		bxroce_drop_index(mr);
		bxroce_drop_ref(mr);
	err1:
		return ERR_PTR(err);
}

int bxroce_map_mr_sg(struct ib_mr *ibmr, struct scatterlist *sg, int sg_nents,unsigned int *sg_offset)
{
		BXROCE_PR("bxroce:bxroce_map_mr_sg start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_map_mr_sg succeed end!\n");//added by hs for printing end info
		return 0;
}

void bxroce_cq_cleanup(struct bxroce_pool_entry *arg)
{
		BXROCE_PR("bxroce: bxroce_cq_cleanup\n");//added by hs 
}
void bxroce_qp_cleanup(struct bxroce_pool_entry *arg)
{
		BXROCE_PR("bxroce: bxroce_qp_cleanup\n");//added by hs 
}
void bxroce_mem_cleanup(struct bxroce_pool_entry *arg)
{
		BXROCE_PR("bxroce:  bxroce_mem_cleanup start\n");//added by hs 
		struct bxroce_mr *mr = container_of(arg, struct bxroce_mr, pelem);
		int i;
		if(mr->umem)
			ib_umem_release(mr->umem);
		if (mr->map) {
			for(i=0;i<mr->num_map;i++)
				kfree(mr->map[i]);
			kfree(mr->map);
		}
}
