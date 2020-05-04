/*
 *For userspace driver.
 *
 *
 * */
#include <config.h>

#include <assert.h>
#include <endian.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <sys/mman.h>
#include <unistd.h>
#include <endian.h>

#include "bxroce_main.h"
#include "bxroce_abi.h"
#include <ccan/list.h>
#include <util/compiler.h>
#include <util/mmio.h>

/*
* bxroce_query_device
*/
int bxroce_query_device(struct ibv_context *context,
						 struct ibv_device_attr *attr)
{
	struct ibv_query_device cmd;
	uint64_t raw_fw_ver;
	struct bxroce_dev *dev = get_bxroce_dev(context->device);
	unsigned major, minor, sub_minor;
	int status;
	bzero(attr, sizeof *attr);
	status = ibv_cmd_query_device(context, attr, &raw_fw_ver, &cmd, sizeof cmd);
	if(status)
			return status;
	major = (raw_fw_ver >> 32) & 0xffff;
	minor = (raw_fw_ver >> 16) & 0xffff;
	sub_minor = raw_fw_ver & 0xffff;

	snprintf(attr->fw_ver, sizeof attr->fw_ver,
                 "%d.%d.%d", major, minor, sub_minor);
	return status;

}

/*
*bxroce_query_port
*/
int bxroce_query_port(struct ibv_context *context, uint8_t port,
					   struct ibv_port_attr *attr)
{
	struct ibv_query_port cmd;
	int status;
	status = ibv_cmd_query_port(context, port, attr, &cmd, sizeof cmd);
	return status;
}

/*bxroce_init_ahid_tbl*/
#define BXROCE_INVALID_AH_IDX 0xffffffff
void bxroce_init_ahid_tbl(struct bxroce_devctx *ctx)
{
	 int i;
	 pthread_mutex_init(&ctx->tbl_lock,NULL);

	 for (i = 0; i < (ctx->ah_tbl_len / sizeof(uint32_t)); i++) 
                ctx->ah_tbl[i] = BXROCE_INVALID_AH_IDX;

}


/*
*bxroce_alloc_pd
*/
struct ibv_pd *bxroce_alloc_pd(struct ibv_context *context)
{
	struct ibv_alloc_pd cmd;
	struct ib_uverbs_alloc_pd_resp resp;
	struct bxroce_pd *pd;

	pd = malloc(sizeof *pd);
	if(!pd)
		return NULL;
	
	bzero(pd,sizeof *pd);
	memset(&cmd,0,sizeof(cmd));

	if (ibv_cmd_alloc_pd(context, &pd->ibv_pd, &cmd, sizeof cmd, &resp, sizeof resp)) {
		free(pd);
		return NULL;
	}

	pd->dev = get_bxroce_dev(context->device);
	pd->uctx = get_bxroce_ctx(context);
	
	return &pd->ibv_pd;
}

/*
*bxroce free pd
*/
int bxroce_free_pd(struct ibv_pd *pd)
{
	int ret;
	struct bxroce_pd *bxpd = get_bxroce_pd(pd);
	
	ret = ibv_cmd_dealloc_pd(pd);
	if(!ret)
		free(bxpd);

	return ret;
}

/*
*bxroce_reg_mr
*/
struct ibv_mr *bxroce_reg_mr(struct ibv_pd *pd, void *addr, size_t length,uint64_t hca_va,
									 int access)
{
	struct verbs_mr *vmr;
	struct ibv_reg_mr cmd;
	struct ib_uverbs_reg_mr_resp resp;
	int ret;

	vmr = malloc(sizeof(*vmr));
	if(!vmr)
			return NULL;
	bzero(vmr, sizeof *vmr);

	ret = ibv_cmd_reg_mr(pd, addr, length, hca_va, access, vmr,
						 &cmd, sizeof cmd, &resp, sizeof resp);
	if (ret) {
			free(vmr);
			return NULL;
	}
	
	return &vmr->ibv_mr;
}


/*
*bxroce_dereg_mr
*/
int bxroce_dereg_mr(struct verbs_mr *vmr)
{
	int ret;

	ret = ibv_cmd_dereg_mr(vmr);
	if(ret)
			return ret;

	free(vmr);
	return 0;
}

/*
*bxroce_create_cq
*/
struct ibv_cq *bxroce_create_cq(struct ibv_context *context, int cqe,
										struct ibv_comp_channel *channel,
										int comp_vector)
{
	int status;
	struct bxroce_cq *cq;
	struct ubxroce_create_cq cmd;
	struct ubxroce_create_cq_resp resp;
	struct bxroce_dev *dev = get_bxroce_dev(context->device);

	cq = malloc(sizeof *cq);
	if(!cq)
			return NULL;
	bzero(cq, sizeof *cq);
	cmd.dpp_cq = 0;
	status = ibv_cmd_create_cq(context, cqe, channel, comp_vector,
							   &cq->ibv_cq, &cmd.ibv_cmd, sizeof cmd,
							   &resp.ibv_resp, sizeof resp);
	if(status)
			goto cq_err1;
	pthread_spin_init(&cq->lock, PTHREAD_PROCESS_PRIVATE);

	cq->dev=dev;
	cq->id= resp.cq_id;
	cq->len = resp.page_size;
	cq->max_hw_cqe = resp.max_hw_cqe;

	cq->txva = mmap(NULL,resp.page_size, PROT_READ|PROT_WRITE,MAP_SHARED, context->cmd_fd, resp.txpage_addr[0]);
	if(cq->txva == MAP_FAILED)
			goto cq_err2;

	cq->rxva = mmap(NULL,resp.page_size, PROT_READ|PROT_WRITE,MAP_SHARED, context->cmd_fd, resp.rxpage_addr[0]);
	if(cq->rxva == MAP_FAILED)
			goto cq_err2;

	cq->xmitva = mmap(NULL,resp.page_size, PROT_READ|PROT_WRITE,MAP_SHARED, context->cmd_fd, resp.xmitpage_addr[0]);
	if(cq->xmitva == MAP_FAILED)
			goto cq_err2;
	
	cq->ibv_cq.cqe = cqe;

	return &cq->ibv_cq;
cq_err2:
	(void)ibv_cmd_destroy_cq(&cq->ibv_cq);
cq_err1:
	bxroce_err("%s:ibv_cmd_creae_cq failed \n",__func__);
	free(cq);
	return NULL;

}

/*
*bxroce_resize_cq
*/
int bxroce_resize_cq(struct ibv_cq *ibcq, int new_entries)
{
	int status;
	struct ibv_resize_cq cmd;
	struct ib_uverbs_resize_cq_resp resp;
	
	status = ibv_cmd_resize_cq(ibcq, new_entries,
							   &cmd, sizeof cmd, &resp, sizeof resp);
	if(status == 0)
			ibcq->cqe = new_entries;
	return status;
}
/*
 * bxroce_destroy_cq
 */
int bxroce_destroy_cq(struct ibv_cq *ibv_cq)
{
        struct bxroce_cq *cq = get_bxroce_cq(ibv_cq);
        int status;
        
        status = ibv_cmd_destroy_cq(ibv_cq);
        if (status)
                return status;
		if(cq->txva)
			munmap((void *)cq->txva, cq->len);
		if(cq->rxva)
			munmap((void *)cq->rxva, cq->len);
		if(cq->xmitva)
			munmap((void *)cq->xmitva, cq->len);
		free(cq);
        return 0;
}

static void bxroce_add_qpn_map(struct bxroce_dev *dev, struct bxroce_qp *qp)
{
	pthread_mutex_lock(&dev->dev_lock);
	dev->qp_tbl[qp->id] = qp;
	pthread_mutex_unlock(&dev->dev_lock);
}

static void bxroce_del_qpn_map(struct bxroce_dev *dev, struct bxroce_qp *qp)
{
	dev->qp_tbl[qp->id] = NULL;
}


/*
 * ocrdma_create_qp
 */
struct ibv_qp *bxroce_create_qp(struct ibv_pd *pd,
								 struct ibv_qp_init_attr *attrs)
{
	int status = 0;
	struct ubxroce_create_qp cmd;
	struct ubxroce_create_qp_resp resp;
	struct bxroce_qp *qp;
	
	qp = calloc(1, sizeof *qp);
	if(!qp)
			return NULL;
	memset(&cmd, 0, sizeof(cmd));

	qp->qp_type = attrs->qp_type;
	pthread_spin_init(&qp->q_lock, PTHREAD_PROCESS_PRIVATE);

	if (attrs->cap.max_inline_data) {
		bxroce_err("%s:NIC do not support send inline data now \n",__func__);
		goto err_inline_data;
	}
	if (attrs->srq) {
		bxroce_err("%s:NIC do not support srq now \n",__func__);
		goto err_inline_data;
	}

	status = ibv_cmd_create_qp(pd, &qp->ibv_qp, attrs, &cmd.ibv_cmd,
							   sizeof cmd, &resp.ibv_resp, sizeof resp);
	if(status)
		goto kernel_err;

	qp->dev = get_bxroce_dev(pd->context->device);
	qp->signaled = attrs->sq_sig_all;
	qp->id = resp.qp_id;
	bxroce_add_qpn_map(qp->dev, qp);

	qp->sq.max_cnt = resp.num_wqe_allocated;
	qp->rq.max_cnt = resp.num_rqe_allocated;

	qp->sq_cq = get_bxroce_cq(attrs->send_cq);
	qp->rq_cq = get_bxroce_cq(attrs->recv_cq);

	qp->wqe_wr_id_tbl = calloc(qp->sq.max_cnt, sizeof(*qp->wqe_wr_id_tbl));
	if(qp->wqe_wr_id_tbl == NULL)
		goto map_err;
	
	qp->rqe_wr_id_tbl = calloc(qp->rq.max_cnt, sizeof(uint64_t));
	if(qp->rqe_wr_id_tbl == NULL)
		goto map_err;

	qp->sq.va  = mmap(NULL, resp.sq_page_size, PROT_READ | PROT_WRITE, MAP_SHARED, pd->context->cmd_fd, resp.sq_page_addr[0]);
	if(qp->sq.va == MAP_FAILED)
		 goto kernel_err;
	
	qp->sq.len = resp.sq_page_size;
	qp->sq.max_wqe_idx = resp.num_wqe_allocated - 1;
	qp->sq.entry_size  = qp->dev->wqe_size;
	qp->sq.max_sges = attrs->cap.max_send_sge;
	qp->max_inline_data = attrs->cap.max_inline_data;
	
	qp->rq.va  = mmap(NULL, resp.rq_page_size, PROT_READ | PROT_WRITE, MAP_SHARED, pd->context->cmd_fd, resp.rq_page_addr[0]);
	if(qp->rq.va == MAP_FAILED)
		goto map_err;
	qp->reg_len = resp.reg_len;
	qp->rq.len = resp.rq_page_size;
	qp->rq.max_wqe_idx = resp.num_rqe_allocated - 1;
	qp->rq.entry_size = qp->dev->rqe_size;
	qp->rq.max_sges = attrs->cap.max_recv_sge;

	qp->iova = mmap(NULL, resp.reg_len, PROT_READ | PROT_WRITE, MAP_SHARED, pd->context->cmd_fd, resp.ioaddr);
	if(qp->iova == MAP_FAILED)
		goto map_err1;

	qp->qp_change_info =(struct qp_change_info *)mmap(NULL,resp.qp_info_len, PROT_READ | PROT_WRITE, MAP_SHARED, pd->context->cmd_fd, resp.qp_info_addr);
	if(qp->qp_change_info == MAP_FAILED)
		goto map_err2;
	qp->qp_info_len = resp.qp_info_len;

	qp->reg_len = resp.reg_len;
	qp->sq_cq->iova = qp->iova;
	qp->sq_cq->reg_len = qp->reg_len;
	qp->rq_cq->iova = qp->iova;
	qp->rq_cq->reg_len = qp->reg_len;

	qp->qp_state = BXROCE_QPS_RST;
	printf("------------------------check user qp param--------------------\n");
	printf("id:%d \n",qp->id);
	printf("sq.max_cnt:0x%x \n",qp->sq.max_cnt);
	printf("rq.max_cnt:0x%x \n",qp->rq.max_cnt);
	printf("sq_page_addr:0x%lx \n",resp.sq_page_addr[0]);
	printf("rq_page_addr:0x%lx \n",resp.rq_page_addr[0]);
	printf("sq len:0x%x  \n",qp->sq.len);
	printf("rq len:0x%x  \n",qp->rq.len);
	printf("ioaddr:0x%lx \n",resp.ioaddr);
	printf("ioreglen:0x%x \n",resp.reg_len);
	printf("rq max_wqe_idx: 0x%x \n",qp->sq.max_wqe_idx);
	printf("sq max_rqe_idx: 0x%x \n",qp->rq.max_wqe_idx);
	printf("-----------------------check user qp param end-----------------\n");
	
	list_node_init(&qp->sq_entry);
	list_node_init(&qp->rq_entry);
	return &qp->ibv_qp;
map_err2:
	bxroce_err("%s,qp_change_info map failed \n",__func__);
	munmap(qp->iova,qp->reg_len);
map_err1:
	bxroce_destroy_qp(&qp->ibv_qp);
	return NULL;
map_err:
	munmap(qp->sq.va, qp->sq.len);
kernel_err:
	bxroce_err("%s:Kernel cmd create user qp failed \n",__func__);

err_inline_data:
	pthread_spin_destroy(&qp->q_lock);
	free(qp);
	return NULL;
}

enum bxroce_qp_state get_bxroce_qp_state(enum ibv_qp_state qps) {
	switch (qps) {
        case IBV_QPS_RESET:
                return BXROCE_QPS_RST;
        case IBV_QPS_INIT:
                return BXROCE_QPS_INIT;
        case IBV_QPS_RTR:
                return BXROCE_QPS_RTR; 
        case IBV_QPS_RTS:
                return BXROCE_QPS_RTS;
        case IBV_QPS_SQD:
                return BXROCE_QPS_SQD;
        case IBV_QPS_SQE:
                return BXROCE_QPS_SQE;
        case IBV_QPS_ERR:
                return BXROCE_QPS_ERR;
		default:
				break;
        }
        return BXROCE_QPS_ERR;

}

static int bxroce_qp_state_machine(struct bxroce_qp *qp,
									enum ibv_qp_state new_ib_state)
{
	int status = 0;
	enum bxroce_qp_state new_state;
	new_state = get_bxroce_qp_state(new_ib_state);

	pthread_spin_lock(&qp->q_lock);

	if (new_state == qp->qp_state) {
			pthread_spin_unlock(&qp->q_lock);
			return 1;
	}

	switch (qp->qp_state) {
	case BXROCE_QPS_RST:
		switch (new_state) {
                case BXROCE_QPS_RST:
                        break;
                case BXROCE_QPS_INIT:
                        /* init pointers to place wqe/rqe at start of hw q */
                        printf("init pointers\n");
						qp->sq.head= 0;
						qp->sq.tail= 0;
						qp->rq.head= 0;
						qp->rq.tail= 0;
						qp->sq.qp_foe = BXROCE_Q_EMPTY;
						qp->rq.qp_foe = BXROCE_Q_EMPTY;
                        /* detach qp from the CQ flush list */
                       
                        break;
                default:
                        status = EINVAL;
                        break;
        };
		break;
	case BXROCE_QPS_INIT:
		switch (new_state) {
                case BXROCE_QPS_INIT:
                        break;
                case BXROCE_QPS_RTR:
                        
						break;
                case BXROCE_QPS_ERR:
                        //ocrdma_flush_qp(qp);
                        //free(qp);
						printf("%s:qp err\n",__func__);//added by hs
						break;
                default:
					status = EINVAL;
					break;
		};
		break;
	case BXROCE_QPS_RTR:
		switch (new_state) {
                case BXROCE_QPS_RTS:
                        break;
                case BXROCE_QPS_ERR:
                       // free(qp);
                        printf("%s:qp err\n",__func__);
						break;
                default:
                        /* invalid state change. */
                        status = EINVAL;
                        break;
        };
        break;
	case BXROCE_QPS_RTS:
		switch (new_state) {
               
                case BXROCE_QPS_ERR:
                       // free(qp);
                        printf("%s:qp err \n",__func__);//added by hs
						break;
                default:
                        /* invalid state change. */
                        status = EINVAL;
                        break;
        };
        break;
	case BXROCE_QPS_ERR:
		switch (new_state) {
		case BXROCE_QPS_RST:
			break;
		default:
			status = EINVAL;
			break;
		};
		break;
	default:
		status = EINVAL;
		break;
	};

	if(!status)
		qp->qp_state = new_state;

	pthread_spin_unlock(&qp->q_lock);
	return status;
}


/*
*	bxroce_modify_qp
*/
int bxroce_modify_qp(struct ibv_qp *ibqp, struct ibv_qp_attr *attr,
					  int attr_mask)
{
	
	struct ibv_modify_qp cmd ={};
	struct bxroce_qp *qp = get_bxroce_qp(ibqp);
	int status;

	status = ibv_cmd_modify_qp(ibqp, attr, attr_mask, &cmd, sizeof cmd);
	if((!status) && (attr_mask & IBV_QP_STATE))
			 bxroce_qp_state_machine(qp,attr->qp_state);
	if(qp->qp_change_info)
		{
			printf("modify_qp qp_change_info \n");
			qp->qkey = qp->qp_change_info->qkey;
			qp->pkey_index = qp->qp_change_info->pkey_index;
			qp->signaled = qp->qp_change_info->signaled;
			qp->sgid_idx = qp->qp_change_info->sgid_idx;		
			memcpy(qp->mac_addr,qp->qp_change_info->mac_addr,6);
		}
	
	return status;
}

/*
*bxroce_query_qp
*/
int bxroce_query_qp(struct ibv_qp *ibqp, struct ibv_qp_attr *attr,
					 int attr_mask, struct ibv_qp_init_attr *init_attr)
{
	struct ibv_query_qp cmd;
    struct bxroce_qp *qp = get_bxroce_qp(ibqp);
    int status;

    status = ibv_cmd_query_qp(ibqp, attr, attr_mask,
                                init_attr, &cmd, sizeof(cmd));

    if (!status)
            bxroce_qp_state_machine(qp, attr->qp_state);

    return status;
}

/*
*bxroce_destroy_qp
*/
int bxroce_destroy_qp(struct ibv_qp *ibqp)
{
	int status = 0;
	struct bxroce_qp *qp;
	struct bxroce_dev *dev;

	qp = get_bxroce_qp(ibqp);
	dev = qp->dev;

	if(qp->iova && (qp->iova != MAP_FAILED))
		munmap(qp->iova, qp->reg_len);
	if(qp->sq.va)
		munmap(qp->sq.va, qp->sq.len);
	if(qp->rq.va)
		munmap(qp->rq.va, qp->rq.len);
	if(qp->qp_change_info)
		munmap(qp->qp_change_info,qp->qp_info_len);

	pthread_spin_lock(&qp->sq_cq->lock);

        if (qp->rq_cq && (qp->rq_cq != qp->sq_cq))
                pthread_spin_lock(&qp->rq_cq->lock);
   
	bxroce_del_qpn_map(qp->dev, qp);

        if (qp->rq_cq && (qp->rq_cq != qp->sq_cq))
                pthread_spin_unlock(&qp->rq_cq->lock);

        pthread_spin_unlock(&qp->sq_cq->lock);



	pthread_mutex_lock(&dev->dev_lock);
	status = ibv_cmd_destroy_qp(ibqp);
	pthread_mutex_unlock(&dev->dev_lock);


	pthread_spin_destroy(&qp->q_lock);

	if(qp->rqe_wr_id_tbl)
		free(qp->rqe_wr_id_tbl);
	if(qp->wqe_wr_id_tbl)
		free(qp->wqe_wr_id_tbl);

	free(qp);
	return status;
}


static int bxroce_check_foe(struct bxroce_qp_hwq_info *q, struct ibv_send_wr *wr, uint32_t free_cnt)
{
	if (wr->num_sge > free_cnt)
		return ENOMEM;
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

//set rc,uc 's wqe
static void bxroce_set_rcwqe_destqp(struct bxroce_qp *qp,struct bxroce_wqe *wqe)
{
	
	uint16_t tempqpn;
	uint16_t tempqpn_l = 0;
	uint16_t tempqpn_h = 0;

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
	printf("libbxroce:%s2,eecntx:0x%x , destqp:0x%x \n",__func__,wqe->eecntx,wqe->destqp);
}

static void bxroce_set_wqe_destqp(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr) {
	
	uint16_t tempqpn;
	uint16_t tempqpn_l = 0;
	uint16_t tempqpn_h = 0;

	tempqpn =wr->wr.ud.remote_qpn; // get lower 16bits ,but qpn only 10 bits.
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
	printf("libbxroce:%s3,eecntx:0x%x, destqp:0x%x \n",__func__,wqe->eecntx,wqe->destqp);//added by hs
}

static inline uint32_t bxroce_sglist_len(struct ibv_sge *sg_list, int num_sge)
{
	uint32_t total_len =0, i;
	for(i=0;i < num_sge; i++)
		total_len += sg_list[i].length;
	return total_len;
}


static void bxroce_set_wqe_opcode(struct bxroce_wqe *wqe,uint8_t qp_type,uint8_t opcode)
{
	uint8_t opcode_l = 0;
	uint8_t opcode_h = 0;
	opcode_l = opcode;
	opcode_l = opcode_l << 4;
	opcode_h = qp_type;
	if(wqe->destsocket2 >> 4)
		wqe->destsocket2 = wqe->destsocket2 & 0x0f;
	wqe->destsocket2 += opcode_l;
	printf("libbxroce:%s,destsocket2:0x%x \n",__func__,wqe->destsocket2);//added by hs
	if(wqe->opcode << 4)
		wqe->opcode = wqe->destsocket2 & 0xf0;
	wqe->opcode += opcode_h;
	printf("libbxroce:%s,opcode:0x%x \n",__func__,wqe->opcode);//added by hs
	wqe->opcode += 0x10;
	printf("libbxroce:%s,opcode final:0x%x \n",__func__,wqe->opcode);//added by hs
}

static void bxroce_set_wqe_dmac(struct bxroce_qp *qp, struct bxroce_wqe *wqe)
{
	struct bxroce_wqe tmpwqe;
	uint8_t tmpvalue;
	int i =0;
	printf("libbxroce:mac addr ");//added by hs
	for(i = 0;i<6;i++)
	printf("0x%x,",qp->mac_addr[i]);//addedby hs
//	printf("\n");//added by hs
	memset(&tmpwqe,0,sizeof(struct bxroce_wqe));
//	printf("libbxroce:tmpwqe.destqp:0x%x\n",tmpwqe.destqp);//added by hs
	tmpwqe.destqp = qp->mac_addr[4];
//	printf("libbxroce:tmpwqe.destqp1:0x%x\n",tmpwqe.destqp);//added by hs
	tmpwqe.destqp = tmpwqe.destqp << 8;
//	printf("libbxroce:tmpwqe.destqp2:0x%x\n",tmpwqe.destqp);
	tmpwqe.destqp = tmpwqe.destqp + qp->mac_addr[5];
//	printf("libbxroce:tmpwqe.destqp3:0x%x\n",tmpwqe.destqp);
	tmpwqe.destqp = tmpwqe.destqp <<4;
//	printf("libbxroce:tmpwqe.destqp4:0x%x\n",tmpwqe.destqp);
	tmpwqe.destsocket1 = qp->mac_addr[0];
	tmpwqe.destsocket1 = tmpwqe.destsocket1 << 8;
	tmpwqe.destsocket1 += qp->mac_addr[1];
	tmpwqe.destsocket1 = tmpwqe.destsocket1 << 8;
	tmpwqe.destsocket1 += qp->mac_addr[2];
	tmpwqe.destsocket1 = tmpwqe.destsocket1 <<8;
	tmpwqe.destsocket1 +=qp->mac_addr[3];
	tmpwqe.destsocket1 = tmpwqe.destsocket1 <<4;
	tmpvalue = qp->mac_addr[4];
	tmpvalue = tmpvalue >> 4;
	tmpwqe.destsocket1 += tmpvalue;
	tmpvalue = qp->mac_addr[0];
	tmpvalue = tmpvalue >> 4;
	tmpwqe.destsocket2 += tmpvalue;

	wqe->destqp = wqe->destqp & 0x000f;
	printf("libbxroce:wqe->destqp1:0x%x\n",wqe->destqp);
	wqe->destqp += tmpwqe.destqp;
	printf("libbxroce:wqe->destqp1:0x%x\n",wqe->destqp);
	wqe->destsocket1 = wqe->destsocket1 & 0x0;
	wqe->destsocket1 =tmpwqe.destsocket1;
	wqe->destsocket2 = wqe->destsocket2 & 0xf0;
	wqe->destsocket2 += tmpwqe.destsocket2;
	printf("libbxroce:%s destqp:0x%x,  destsocket1: 0x%x,  destsocket2: 0x%x \n"
		,__func__,wqe->destqp,wqe->destsocket1,wqe->destsocket2);//added by hs


}

static int  bxroce_build_wqe_opcode(struct bxroce_qp *qp,struct bxroce_wqe *wqe,struct ibv_send_wr *wr)
{
	int status= 0;
	uint8_t qp_type;
	uint8_t opcode;
	switch(qp->qp_type) {
		case IBV_QPT_UD:
				qp_type = UD;
				break;
		case IBV_QPT_UC:
				qp_type = UC;
				break;
		case IBV_QPT_DRIVER: //for RD.
				qp_type = RD;
				break;
		case IBV_QPT_RC:
				qp_type = RC;
				break;
		default:
				printf("libbxroce: qp type default ...\n");//added by hs
				status = 0x1;
				break;
	}

	switch (wr->opcode) {
	case IBV_WR_SEND:
				opcode = SEND;
				break;
	case IBV_WR_SEND_WITH_IMM:
				opcode = SEND_WITH_IMM;
				break;
	case IBV_WR_SEND_WITH_INV:
				opcode = SEND_WITH_INV;
				break;
	case IBV_WR_RDMA_WRITE:
				opcode = RDMA_WRITE;
				break;
	case IBV_WR_RDMA_WRITE_WITH_IMM:
				opcode = WRITE_WITH_IMM;
				break;
	case IBV_WR_RDMA_READ:
				opcode = RDMA_READ;
				break;	
	default:
				printf("libbxroce: wr opcode default...\n");//added by hs
				status = status | 0x2;	
				break;	
	}
	if(status & 0x1||status & 0x2)
	{
		printf("libbxroce: transport or  opcode not supported \n");//added by hs 	
		return EINVAL;
	}
	if(qp_type == UD && !(opcode & (SEND|SEND_WITH_IMM)))
		return EINVAL;
	if(qp_type == UC && !(opcode &(SEND|SEND_WITH_IMM|RDMA_WRITE|WRITE_WITH_IMM )))
		return EINVAL;
	if(qp_type == RD && (opcode & SEND_WITH_INV))
		return EINVAL;
	printf("libbxroce: %s,qp_type:0x%x , opcode:0x%x \n "
			,__func__,qp_type,opcode);//added by hs
	bxroce_set_wqe_opcode(wqe,qp_type,opcode);
	return 0;

}

static int bxroce_build_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe, int num_sge, struct ibv_sge *sg_list,struct ibv_send_wr *wr)
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
		printf("libbxroce: ---------------check send wqe--------------\n");//added by hs
		printf("libbxroce:immdat:0x%x \n",tmpwqe->immdt);//added by hs
		printf("libbxroce:pkey:0x%x \n",tmpwqe->pkey);//added by hs
		printf("libbxroce:rkey:0x%x \n",tmpwqe->rkey);//added by hs
		printf("libbxroce:lkey:0x%x \n",tmpwqe->lkey);//added by hs
		printf("libbxroce:qkey:0x%x \n",tmpwqe->qkey);//added by hs
		printf("libbxroce:dmalen:0x%x \n",tmpwqe->dmalen);//added by hs
		printf("libbxroce:destaddr:0x%lx \n",tmpwqe->destaddr);//added by hs
		printf("libbxroce:localaddr:0x%lx \n",tmpwqe->localaddr);//added by hs
		printf("libbxroce:eecntx:0x%x \n",tmpwqe->eecntx);//added by hs
		printf("libbxroce:destqp:0x%x \n",tmpwqe->destqp);//added by hs
		printf("libbxroce:destsocket1:0x%x \n",tmpwqe->destsocket1);//added by hs
		printf("libbxroce:destsocket2:0x%x \n",tmpwqe->destsocket2);//added by hs
		printf("libbxroce:opcode:0x%x \n",tmpwqe->opcode);//added by hs
		printf("libbxroce:wqe's addr:%lx \n",tmpwqe);//added by hs
		printf("libbxroce:----------------check send wqe end------------\n");//added by hs
		tmpwqe += 1;
	}
	if (num_sge == 0) {
		memset(wqe,0,sizeof(*wqe));
	}
	return status;
}

static int bxroce_build_inline_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr, uint32_t wqe_size)
{

	if (wr->send_flags & IBV_SEND_INLINE && qp->qp_type != IBV_QPT_UD) {//
		wqe->dmalen = bxroce_sglist_len(wr->sg_list,wr->num_sge);
			printf("%s() supported_len = 0x%x,\n"
				   "unsupported len req =0x%x,\n,the funtion is not supported now\n",__func__,qp->max_inline_data,wqe->dmalen);//added by hs 
			return EINVAL;
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
	printf("libbxroce: post send, sq.head is %d, sq.tail is %d \n",qp->sq.head,qp->sq.tail);//added by hs
	return 0;


}

static int bxroce_build_send(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr)
{
	int status;
	uint32_t wqe_size = sizeof(*wqe);

	if (qp->qp_type == IBV_QPT_UD) {
			bxroce_set_wqe_destqp(qp,wqe,wr);
			wqe->qkey = wr->wr.ud.remote_qkey;
	}
	status = bxroce_build_inline_sges(qp,wqe,wr,wqe_size);
	return status;
}

static int bxroce_buildwrite_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe,int num_sge, struct ibv_sge *sg_list, struct ibv_send_wr *wr)
{
	int i;
	int status = 0;
	struct bxroce_wqe *tmpwqe = wqe;
	for (i = 0; i < num_sge; i++) {
		status = bxroce_build_wqe_opcode(qp,tmpwqe,wr);//added by hs 
		if(status)
			return -EINVAL;
		if(qp->destqp)
			bxroce_set_rcwqe_destqp(qp,tmpwqe);
		bxroce_set_wqe_dmac(qp,tmpwqe);
		tmpwqe->rkey = wr->wr.rdma.rkey;
		tmpwqe->lkey = sg_list[i].lkey;
		tmpwqe->localaddr = sg_list[i].addr;
		tmpwqe->dmalen = sg_list[i].length;
		tmpwqe->destaddr = wr->wr.rdma.remote_addr;
		tmpwqe->qkey = qp->qkey;
		tmpwqe->pkey = qp->pkey_index;
		printf("libbxroce: ---------------check write wqe--------------\n");//added by hs
		printf("libbxroce:immdat:0x%x \n",tmpwqe->immdt);//added by hs
		printf("libbxroce:pkey:0x%x \n",tmpwqe->pkey);//added by hs
		printf("libbxroce:rkey:0x%x \n",tmpwqe->rkey);//added by hs
		printf("libbxroce:lkey:0x%x \n",tmpwqe->lkey);//added by hs
		printf("libbxroce:qkey:0x%x \n",tmpwqe->qkey);//added by hs
		printf("libbxroce:dmalen:0x%x \n",tmpwqe->dmalen);//added by hs
		printf("libbxroce:destaddr:0x%lx \n",tmpwqe->destaddr);//added by hs
		printf("libbxroce:localaddr:0x%lx \n",tmpwqe->localaddr);//added by hs
		printf("libbxroce:eecntx:0x%x \n",tmpwqe->eecntx);//added by hs
		printf("libbxroce:destqp:0x%x \n",tmpwqe->destqp);//added by hs
		printf("libbxroce:destsocket1:0x%x \n",tmpwqe->destsocket1);//added by hs
		printf("libbxroce:destsocket2:0x%x \n",tmpwqe->destsocket2);//added by hs
		printf("libbxroce:opcode:0x%x \n",tmpwqe->opcode);//added by hs
		printf("libbxroce:wqe's addr:%lx \n",tmpwqe);//added by hs
		printf("libbxroce:----------------check write wqe end------------\n");//added by hs
		tmpwqe += 1;
	}
	if (num_sge == 0) {
		memset(wqe,0,sizeof(*wqe));
	}
	return status;
}

static int bxroce_buildwrite_inline_sges(struct bxroce_qp *qp,struct bxroce_wqe *wqe,const struct ibv_send_wr *wr, uint32_t wqe_size)
{
	int i;
	int status = 0;
	if (wr->send_flags & IBV_SEND_INLINE && qp->qp_type != IBV_QPT_UD) {//
		wqe->dmalen = bxroce_sglist_len(wr->sg_list,wr->num_sge);
			printf("%s() supported_len = 0x%x,\n"
				   "unsupported len req =0x%x,\n,the funtion is not supported now\n",__func__,qp->max_inline_data,wqe->dmalen);//added by hs 
			return -EINVAL;
	}
	else {
		status = bxroce_buildwrite_sges(qp,wqe,wr->num_sge,wr->sg_list,wr);
		if(wr->num_sge){
			wqe_size +=((wr->num_sge-1)*sizeof(struct bxroce_wqe));
			qp->sq.head = (qp->sq.head + wr->num_sge) % qp->sq.max_cnt; // update the head ptr,and check if the queue if full.
			if(qp->sq.head == qp->sq.tail){
				qp->sq.qp_foe == BXROCE_Q_FULL;
			}
		}
		else {
			qp->sq.head = (qp->sq.head + 1) % qp->sq.max_cnt; // update the head ptr, and check if the queue if full.
			if(qp->sq.head == qp->sq.tail){
				qp->sq.qp_foe == BXROCE_Q_FULL;
			}
		}
	}
	printf("libbxroce: post send, sq.head is %d, sq.tail is %d\n",qp->sq.head,qp->sq.tail);//added by hs
	return status;
}


static int bxroce_build_write(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr) 
{
	int status;
	uint32_t wqe_size = sizeof(*wqe);
	
	status = bxroce_buildwrite_inline_sges(qp,wqe,wr,wqe_size);
	if(status)
		return status;
	return 0;
}

static void bxroce_build_read(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr)
{
	uint32_t wqe_size = sizeof(*wqe);
	bxroce_buildwrite_inline_sges(qp,wqe,wr,wqe_size);
}

static void bxroce_ring_sq_hw(struct bxroce_qp *qp) {
	uint32_t qpn;
	uint32_t phyaddr,tmpvalue;
	phyaddr = qp->sq.head * qp->sq.entry_size;
	qpn  = qp->id;

	udma_to_device_barrier();

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + QPLISTREADQPN);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(qpn);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + WRITEORREADQPLIST);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(0x1);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + WRITEQPLISTMASK);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(0x7);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + QPLISTWRITEQPN);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(0x0);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + READQPLISTDATA3);
	tmpvalue = le32toh(*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA));
	printf("libbxroce: sqaddr_h:0x%lx \n",tmpvalue);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + READQPLISTDATA4);
	tmpvalue = le32toh(*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA));
	printf("libbxroce: sqaddr_l:0x%lx \n",tmpvalue);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + WPFORQPLIST);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(phyaddr);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE +WRITEQPLISTMASK);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(0x1);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + QPLISTWRITEQPN);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(0x1);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + WRITEORREADQPLIST);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(0x0);

	
}


/*
*bxroce_post_send
*/
int bxroce_post_send(struct ibv_qp *ib_qp, struct ibv_send_wr *wr,
	struct ibv_send_wr** bad_wr)
{
	int status = 0;
	struct bxroce_qp *qp;
	struct bxroce_wqe *hdwqe;
	uint32_t free_cnt;

	qp = get_bxroce_qp(ib_qp);
	pthread_spin_lock(&qp->q_lock);
	if (qp->qp_state != BXROCE_QPS_RTS) {
		pthread_spin_unlock(&qp->q_lock);
		*bad_wr = wr;
		return EINVAL;
	}

	while (wr) {
		printf("%s:process wr & write wqe _(:�١��� \n",__func__);
		if(qp->qp_type == IBV_QPT_UD &&
		  (wr->opcode != IBV_WR_SEND &&
		   wr->opcode != IBV_WR_SEND_WITH_IMM)){
			*bad_wr = wr;
			status = EINVAL;
			break;
		}
		free_cnt = bxroce_hwq_free_cnt(&qp->sq);

		if(free_cnt == 0 || wr->num_sge > qp->sq.max_sges){
			*bad_wr = wr;
			status = ENOMEM;
			break;
		}

		status = bxroce_check_foe(&qp->sq,wr,free_cnt);// check if the wr can be processed with enough memory.
		if(status) break;
		hdwqe = bxroce_hwq_head(&qp->sq); // To get the head ptr.
		printf("libbxroce:wp's va:%x \n",hdwqe);//added by hs

		qp->wqe_wr_id_tbl[qp->sq.head].wrid = wr->wr_id;
		switch(wr->opcode){
		case IBV_WR_SEND_WITH_IMM:
			hdwqe->immdt = be32toh(wr->imm_data);
			SWITCH_FALLTHROUGH;
		case IBV_WR_SEND:
			status = bxroce_build_send(qp,hdwqe,wr);
			break;
		case IBV_WR_SEND_WITH_INV:
			hdwqe->lkey = be32toh(wr->imm_data);
			status = bxroce_build_send(qp,hdwqe,wr);
			break;
		case IBV_WR_RDMA_WRITE_WITH_IMM:
			hdwqe->immdt = be32toh(wr->imm_data);
			SWITCH_FALLTHROUGH;
		case IBV_WR_RDMA_WRITE:
			status = bxroce_build_write(qp,hdwqe,wr);
			break;
		case IBV_WR_RDMA_READ:
			bxroce_build_read(qp,hdwqe,wr);
			break;
		default:
			status = EINVAL;
			break;
		}
		if (status) {
			*bad_wr = wr;
			break;
		}
		if(wr->send_flags & IBV_SEND_SIGNALED || qp->signaled)
				qp->wqe_wr_id_tbl[qp->sq.head].signaled = 1;
		else
				qp->wqe_wr_id_tbl[qp->sq.head].signaled = 0;

		/*make sure wqe is written befor adapter can access it*/
		printf("libbxroce:wmb... \n");//added by hs
		printf("libbxroce:access hw.. \n");//added by hs
		bxroce_ring_sq_hw(qp); // notify hw to send wqe.
		wr = wr->next;
	}
	pthread_spin_unlock(&qp->q_lock);
	printf("libbxroce; post send end \n");
	return status;
}

static void bxroce_build_rqsges(struct bxroce_rqe *rqe, struct ibv_recv_wr *wr)
{
	int i;
	int num_sge = wr->num_sge;
	struct bxroce_rqe *tmprqe = rqe;
	struct ibv_sge *sg_list;
	sg_list = wr->sg_list;

	for (i = 0; i < num_sge; i++) {
		tmprqe->descbaseaddr = sg_list[i].addr;
		tmprqe->dmalen = sg_list[i].length;
		tmprqe->opcode = 0x80000000;
		tmprqe += 1;
		//BXROCE_PR("bxroce: in rq,num_sge = %d, tmprqe 's addr is %x\n",num_sge,tmprqe);//added by hs
	}
	if(num_sge == 0)
		memset(tmprqe,0,sizeof(*tmprqe));
}

static void bxroce_build_rqe(struct bxroce_qp *qp,struct bxroce_rqe *rqe, const struct ibv_recv_wr *wr) 
{
	uint32_t wqe_size = 0;

	bxroce_build_rqsges(rqe,wr);

	if(wr->num_sge){
			wqe_size +=((wr->num_sge-1) * sizeof(struct bxroce_rqe));
			qp->rq.head = (qp->rq.head + wr->num_sge) % qp->rq.max_cnt; // update the head ptr,and check if the queue if full.
			if(qp->rq.head == qp->rq.tail){
				qp->rq.qp_foe == BXROCE_Q_FULL;
			}
			
	}
		else {
			qp->rq.head = (qp->rq.head + 1) % qp->rq.max_cnt; // update the head ptr, and check if the queue if full.
			if(qp->rq.head == qp->rq.tail){
				qp->rq.qp_foe == BXROCE_Q_FULL;
			}
	}
	
	
}

static void bxroce_ring_rq_hw(struct bxroce_qp *qp)
{
	uint32_t qpn;
	uint32_t phyaddr,tmpvalue;
	phyaddr = qp->rq.head * qp->rq.entry_size;
	qpn  = qp->id;

	phyaddr = phyaddr << 10;
	qpn = qpn + phyaddr;

	udma_to_device_barrier();

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + RCVQ_INF);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(qpn);

	*(__le32 *)((uint8_t *)(qp->iova) + MPB_WRITE_ADDR) = htole32(PGU_BASE + RCVQ_WRRD);
	*(__le32 *)((uint8_t *)(qp->iova) + MPB_RW_DATA)	= htole32(0x2);

	
}

/*
*bxroce_post_recv
*/
int bxroce_post_recv(struct ibv_qp *ib_qp, struct ibv_recv_wr *wr,
					  struct ibv_recv_wr **bad_wr)
{
	int status = 0;
	struct bxroce_qp *qp;
	struct bxroce_rqe *rqe;
	uint32_t free_cnt = 0;
	qp = get_bxroce_qp(ib_qp);

	pthread_spin_lock(&qp->q_lock);
	if (qp->qp_state == BXROCE_QPS_RST || qp->qp_state == BXROCE_QPS_ERR) {
			pthread_spin_unlock(&qp->q_lock);
			*bad_wr =wr;
			return EINVAL;
	}
	
	while (wr) {
		free_cnt = bxroce_hwq_free_cnt(&qp->rq);

		if(free_cnt == 0 || wr->num_sge > qp->rq.max_sges){
			*bad_wr = wr;
			status = ENOMEM;
			break;
		}
		status = bxroce_check_foe(&qp->rq,wr,free_cnt);
		if(status) break;

		rqe = bxroce_hwq_head(&qp->rq);
		bxroce_build_rqe(qp,rqe,wr);
		qp->rqe_wr_id_tbl[qp->rq.head] = wr->wr_id;

		bxroce_ring_rq_hw(qp);

		
		wr = wr->next;
	}
	pthread_spin_unlock(&qp->q_lock);

	return status;
}

/*
*bxroce_poll_cq
*/
int bxroce_poll_cq(struct ibv_cq* ibcq, int num_entries, struct ibv_wc* wc)
{
	struct bxroce_cq *cq;
	int cqes_to_poll = num_entries;
	int num_os_cqe = 0, err_cqes = 0;
	struct bxroce_qp *qp;
	
	printf("%s:process cqe, return num_os_cqe _(:�١��� \n",__func__);//added by hs
	return num_os_cqe;
}

/*
*bxroce_arm_cq
*/
int bxroce_arm_cq(struct ibv_cq* ibcq, int solicited)
{
	struct bxroce_cq *cq;
	
	cq = get_bxroce_cq(ibcq);

	pthread_spin_lock(&cq->lock);
	printf("%s:pretend that i am working �r(������)�q \n",__func__);//added by hs
	pthread_spin_unlock(&cq->lock);

	return 0;
}

/*
*bxroce_create_ah
*/
struct ibv_ah *bxroce_create_ah(struct ibv_pd *ibpd, struct ibv_ah_attr *attr)
{
	int status;
	struct bxroce_pd *pd;
	struct bxroce_ah *ah;
	struct ib_uverbs_create_ah_resp resp;

	pd = get_bxroce_pd(ibpd);
	ah = malloc(sizeof *ah);
	if(!ah)
		return NULL;
	bzero(ah,sizeof *ah);
	ah->pd = pd;
	
	memset(&resp, 0, sizeof(resp));
	status = ibv_cmd_create_ah(ibpd, &ah->ibv_ah, attr, &resp, sizeof(resp));
	if(status)
		goto cmd_err;	

	return &ah->ibv_ah;

cmd_err:
	free(ah);
	return NULL;
}


/*
 * bxroce_destroy_ah
 */ 
int bxroce_destroy_ah(struct ibv_ah *ibah)
{
	int status;
	struct bxroce_ah *ah;
	ah = get_bxroce_ah(ibah);

	status = ibv_cmd_destroy_ah(ibah);
	free(ah);

	return status;
}
