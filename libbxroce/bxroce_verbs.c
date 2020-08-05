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
//#include <ccan/list.h>
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

int bxroce_reg_mrinfo(struct qp_vaddr *vaddr)
{
	int port_num = 11988;
	uint32_t ipaddr;
	struct sg_phy_info *sginfo = NULL;
	int i = 0;
	int len;
	uint64_t phyaddr = 0;

	sginfo = malloc(sizeof(struct sg_phy_info));
	memset(sginfo,0,sizeof(*sginfo));

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	while(1){
		printf("reg mr\n");	
		printf("cmd:0x%x,vaddr:0x%lx,phyaddr:0x%lx,len:0x%x,rkey:0x%x\n",vaddr->cmd,vaddr->vaddr,vaddr->phyaddr,vaddr->len,vaddr->rkey);
		len = sizeof(*vaddr);
		write(client_fd,vaddr,len);

		len = sizeof(*sginfo);
		int readret =read(client_fd,sginfo,len);
		if(readret == -1)
		{
			printf("error\n");
			return -1;
		}else if(readret == 0){
			printf("server closed socket\n");
			break;
		}
		else{
					printf("sginfo->phyaddr:0x%lx \n",sginfo->phyaddr);
					printf("sginfo->size: 0x%x\n",sginfo->size);
					printf("\n");
				break;
			}
		}

	free(sginfo);
	close(client_fd);
	return 0;
}



/*
*bxroce_reg_mr
*/
struct ibv_mr *bxroce_reg_mr(struct ibv_pd *pd, void *addr, size_t length,uint64_t hca_va,
									 int access)
{
	struct verbs_mr *vmr;
	struct ubxroce_reg_mr cmd;
	//struct ib_uverbs_reg_mr_resp resp;
	struct ubxroce_reg_mr_resp resp;
	struct bxroce_dev *dev;
	struct bxroce_pd *bxpd;
	struct bxroce_mr_sginfo *mr_sginfo;
	struct sg_phy_info *sg_phy_info;
	struct qp_vaddr *vaddr;
	int ret;
	int num_sg = 0;
	int i = 0;
	int stride = 0;

	vmr = malloc(sizeof(*vmr));
	if(!vmr)
			return NULL;
	bzero(vmr, sizeof *vmr);

	cmd.user_addr = hca_va;
	ret = ibv_cmd_reg_mr(pd, addr, length, hca_va, access, vmr,
						 &cmd.ibv_cmd, sizeof cmd, &resp.ibv_resp, sizeof resp);
	if (ret) {
		free(vmr);
		return NULL;
	}

	mr_sginfo = malloc(sizeof(*mr_sginfo));
	bzero(mr_sginfo, sizeof(*mr_sginfo));
	num_sg = resp.sg_phy_num;
	
	sg_phy_info = (struct sg_phy_info *)malloc(num_sg * sizeof(*sg_phy_info));
	vaddr = (struct qp_vaddr *)malloc(sizeof(struct qp_vaddr));
	memset(sg_phy_info,0,num_sg * sizeof(*sg_phy_info));
	memset(vaddr,0,sizeof(struct qp_vaddr));

	mr_sginfo->sginfo = sg_phy_info;
	mr_sginfo->iova = hca_va;
	mr_sginfo->num_sge = num_sg;
	mr_sginfo->offset = resp.offset;
	mr_sginfo->vmr = vmr;
	mr_sginfo->length = length;

	bxpd = get_bxroce_pd(pd);
	dev = bxpd->dev;
	stride = sizeof(*sg_phy_info);

		BXPRMR("------------check reg mr 's sg info --------------\n");
		BXPRMR("ibv_resp size : 0x%x \n",sizeof(resp.ibv_resp));
		BXPRMR("resp's size: 0x%x \n",sizeof(resp));
		BXPRMR("stride:0x%x \n", stride);
		BXPRMR("num_sge:0x%x \n", resp.sg_phy_num);
		BXPRMR("resp.offset:0x%x\n",mr_sginfo->offset);
		BXPRMR("resp[0]'s addr is: 0x%x \n",resp.sg_phy_addr[0]);
		BXPRMR("resp[0]'s size is: 0x%x  \n",resp.sg_phy_size[0]);
		BXPRMR("mr's va: 0x%x \n",hca_va);
	for(i = 0 ; i< num_sg; i++)
	{ 
		(mr_sginfo->sginfo + i*stride)->phyaddr = resp.sg_phy_addr[i];
		(mr_sginfo->sginfo + i*stride)->size	= resp.sg_phy_size[i];
		//mr_sginfo->sginfo[i].phyaddr = resp.sg_phy_addr[i];
		//mr_sginfo->sginfo[i].size	 = resp.sg_phy_size[i];
		BXPRMR("resp[%d]'s size is: 0x%lx \n",i,resp.sg_phy_addr[i]);
		BXPRMR("resp[%d]'s size is: 0x%x  \n",i,resp.sg_phy_size[i]);
		BXPRMR("sg[%d]'s phyaddr is:0x%lx \n",i,(mr_sginfo->sginfo + i*stride)->phyaddr);
		BXPRMR("sg[%d]'s size is:0x%x  \n",i,(mr_sginfo->sginfo + i*stride)->size);
		BXPRMR("\n");
		
	}
	
	if(num_sg <= 256)
	{ 
		pthread_mutex_lock(&dev->dev_lock);
		userlist_add_tail(&mr_sginfo->sg_list,&dev->mr_list); // add this info to dev so  i can access it;
		pthread_mutex_unlock(&dev->dev_lock);
	}

	printf("reg mr to exchange server \n");
	vaddr->phyaddr = resp.sg_phy_addr[0];
	vaddr->vaddr   = mr_sginfo->iova;
	vaddr->len	 = length;
	vaddr->rkey	 = vmr->ibv_mr.rkey;
	vaddr->cmd	 = CMD_WRITE;
	ret = bxroce_reg_mrinfo(vaddr);
	if(ret)
	{
		bxroce_dereg_mr(vmr);
		free(vaddr);
		printf("reg mr error\n");
		return NULL;
	}


	return &vmr->ibv_mr;
}


static int bxroce_dereg_mrinfo(struct qp_vaddr *vaddr)
{
	int port_num = 11988;
	uint32_t ipaddr;
	struct sg_phy_info *sginfo = NULL;
	int i = 0;
	int len =0;
	uint64_t phyaddr = 0;

	sginfo = malloc(sizeof(struct sg_phy_info));
	memset(sginfo,0,sizeof(*sginfo));

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	while(1){
		printf("dereg mr\n");	
		printf("cmd:0x%x,vaddr:0x%lx,phyaddr:0x%lx,len:0x%x,rkey:0x%x\n",vaddr->cmd,vaddr->vaddr,vaddr->phyaddr,vaddr->len,vaddr->rkey);
		len = sizeof(*vaddr);
		write(client_fd,vaddr,len);

		len = sizeof(*sginfo);
		int readret =read(client_fd,sginfo,len);
		if(readret == -1)
		{
			printf("error\n");
			return -1;
		}else if(readret == 0){
			printf("server closed socket\n");
			break;
		}
		else{
					printf("sginfo->phyaddr:0x%lx \n",sginfo->phyaddr);
					printf("sginfo->size: 0x%x\n",sginfo->size);
					printf("\n");
				break;
			}
		}

	free(sginfo);
	close(client_fd);
	return 0;

}


/*
*bxroce_dereg_mr
*/
int bxroce_dereg_mr(struct verbs_mr *vmr)
{
	int ret;
	struct bxroce_mr_sginfo *mr_sginfo;
	struct bxroce_dev *dev = get_bxroce_dev(vmr->ibv_mr.context->device);
	struct qp_vaddr *vaddr = NULL;

	vaddr = malloc(sizeof(struct qp_vaddr));
	memset(vaddr,0,sizeof(struct qp_vaddr));

	userlist_for_each_entry(mr_sginfo, &dev->mr_list, sg_list)
		{
			if (vmr == mr_sginfo->vmr)
			{		
				printf("build mr : vmr find it \n");
				printf("addr:0x%lx \n",mr_sginfo->iova);
				break;
			}
		}

	vaddr->vaddr 	= mr_sginfo->iova;
	vaddr->phyaddr  = mr_sginfo->sginfo->phyaddr;
	vaddr->rkey     = vmr->ibv_mr.rkey;
	vaddr->cmd		= CMD_REMOVE;

	userlist_del(&mr_sginfo->sg_list);
	free(mr_sginfo->sginfo);
	free(mr_sginfo);

	ret = ibv_cmd_dereg_mr(vmr);
	if(ret)
			return ret;

	ret = bxroce_dereg_mrinfo(vaddr);
	if(ret)
	{
		free(vaddr);
		return ret;
	}
	free(vaddr);
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

	BXPRCQ("ibv_resp.size : 0x%x , resp.size: 0x%x \n", sizeof(resp.ibv_resp),sizeof(resp));
	cq->dev=dev;
	cq->id= resp.cq_id;
	cq->len = resp.page_size;
	cq->max_hw_cqe = resp.max_hw_cqe;

	cq->txva = mmap(NULL,resp.page_size, PROT_READ|PROT_WRITE,MAP_SHARED, context->cmd_fd, resp.txpage_addr[0]);
	if(cq->txva == MAP_FAILED)
			goto cq_err2;
	cq->txwp = 0;
	cq->txrp = 0;
	cq->txpa = resp.txpage_addr[0];

	cq->rxva = mmap(NULL,resp.page_size, PROT_READ|PROT_WRITE,MAP_SHARED, context->cmd_fd, resp.rxpage_addr[0]);
	if(cq->rxva == MAP_FAILED)
			goto cq_err2;
	cq->rxwp = 0;
	cq->rxrp = 0;
	cq->rxpa = resp.rxpage_addr[0];

	cq->xmitva = mmap(NULL,resp.page_size, PROT_READ|PROT_WRITE,MAP_SHARED, context->cmd_fd, resp.xmitpage_addr[0]);
	if(cq->xmitva == MAP_FAILED)
			goto cq_err2;
	cq->xmitwp = 0;
	cq->xmitrp = 0;
	cq->xmitpa = resp.xmitpage_addr[0];

	cq->cqe_size = sizeof(struct bxroce_txcqe);

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
	struct bxroce_dev *dev;
	
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
	qp->sq_cq->qp_id = qp->id;
	qp->rq_cq->qp_id = qp->id;
	qp->addrtbl = malloc(sizeof(struct qp_addrtbl) * 32);

	qp->qp_state = BXROCE_QPS_RST;
	BXPRQP("------------------------check user qp param--------------------\n");
	BXPRQP("id:%d \n",qp->id);
	BXPRQP("sq.max_cnt:0x%x \n",qp->sq.max_cnt);
	BXPRQP("rq.max_cnt:0x%x \n",qp->rq.max_cnt);
	BXPRQP("sq_page_addr:0x%lx \n",resp.sq_page_addr[0]);
	BXPRQP("rq_page_addr:0x%lx \n",resp.rq_page_addr[0]);
	BXPRQP("sq len:0x%x  \n",qp->sq.len);
	BXPRQP("rq len:0x%x  \n",qp->rq.len);
	BXPRQP("ioaddr:0x%lx \n",resp.ioaddr);
	BXPRQP("ioreglen:0x%x \n",resp.reg_len);
	BXPRQP("rq max_wqe_idx: 0x%x \n",qp->sq.max_wqe_idx);
	BXPRQP("sq max_rqe_idx: 0x%x \n",qp->rq.max_wqe_idx);
	BXPRQP("-----------------------check user qp param end-----------------\n");


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
                        BXPRQP("init pointers\n");
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
						BXPRQP("%s:qp err\n",__func__);//added by hs
						break;
                default:
					status = EINVAL;
					break;
		};
		break;
	case BXROCE_QPS_RTR:
		switch (new_state) {
                case BXROCE_QPS_RTS:
				//before RTS state to user, we need to exchange some phy info with remote user  
                        break;
                case BXROCE_QPS_ERR:
                       // free(qp);
                        BXPRQP("%s:qp err\n",__func__);
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
                        BXPRQP("%s:qp err \n",__func__);//added by hs
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
			BXPRQP("modify_qp qp_change_info \n");
			qp->qkey = qp->qp_change_info->qkey;
			qp->pkey_index = qp->qp_change_info->pkey_index;
			qp->signaled = qp->qp_change_info->signaled;
			qp->sgid_idx = qp->qp_change_info->sgid_idx;	
			qp->destqp = qp->qp_change_info->destqp;
			memcpy(qp->mac_addr,qp->qp_change_info->mac_addr,6);
			memcpy(qp->dgid,qp->qp_change_info->dgid,16);
			memcpy(qp->sgid,qp->qp_change_info->sgid,16);

			int i =0;
			BXPRQP("check...\n");
			BXPRQP("qp->destqp:0x%x \n",qp->destqp);
			BXPRQP("qp->qkey:0x%x \n",qp->qkey);
			BXPRQP("qp->pkey_index:0x%x\n",qp->pkey_index);
			BXPRQP("qp->signaled:0x%x\n",qp->pkey_index);
			BXPRQP("qp->sgid_idx:0x%x\n",qp->sgid_idx);
			BXPRQP("qp->macaddr:");
			for(i =0;i<6;i++)
				BXPRQP("%x",qp->mac_addr[i]);

			BXPRQP("\n");
			BXPRQP("qp->dgid:");
			for(i=0;i<16;i++)
				BXPRQP("%x",qp->dgid[i]);
			BXPRQP("\n");
			BXPRQP("qp->sgid:");
			for(i=0;i<16;i++)
				BXPRQP("%x",qp->sgid[i]);
			BXPRQP("\n");
		
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
	free(qp->addrtbl);

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
	BXPRSEN("libbxroce:%s2,eecntx:0x%x , destqp:0x%x \n",__func__,wqe->eecntx,wqe->destqp);
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
	BXPRSEN("libbxroce:%s3,eecntx:0x%x, destqp:0x%x \n",__func__,wqe->eecntx,wqe->destqp);//added by hs
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
	BXPRSEN("libbxroce:%s,destsocket2:0x%x \n",__func__,wqe->destsocket2);//added by hs
	if(wqe->opcode << 4)
		wqe->opcode = wqe->destsocket2 & 0xf0;
	wqe->opcode += opcode_h;
	BXPRSEN("libbxroce:%s,opcode:0x%x \n",__func__,wqe->opcode);//added by hs
	wqe->opcode += 0x10;
	BXPRSEN("libbxroce:%s,opcode final:0x%x \n",__func__,wqe->opcode);//added by hs
}

static void bxroce_set_wqe_dmac(struct bxroce_qp *qp, struct bxroce_wqe *wqe)
{
	struct bxroce_wqe tmpwqe;
	uint8_t tmpvalue;
	int i =0;
	BXPRSEN("libbxroce:mac addr ");//added by hs
	for(i = 0;i<6;i++)
	BXPRSEN("0x%x,",qp->mac_addr[i]);//addedby hs
//	BXPRSEN("\n");//added by hs
	memset(&tmpwqe,0,sizeof(struct bxroce_wqe));
//	BXPRSEN("libbxroce:tmpwqe.destqp:0x%x\n",tmpwqe.destqp);//added by hs
	tmpwqe.destqp = qp->mac_addr[1];
//	BXPRSEN("libbxroce:tmpwqe.destqp1:0x%x\n",tmpwqe.destqp);//added by hs
	tmpwqe.destqp = tmpwqe.destqp << 8;
//	BXPRSEN("libbxroce:tmpwqe.destqp2:0x%x\n",tmpwqe.destqp);
	tmpwqe.destqp = tmpwqe.destqp + qp->mac_addr[0];
//	BXPRSEN("libbxroce:tmpwqe.destqp3:0x%x\n",tmpwqe.destqp);
	tmpwqe.destqp = tmpwqe.destqp <<4;
//	BXPRSEN("libbxroce:tmpwqe.destqp4:0x%x\n",tmpwqe.destqp);
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
	BXPRSEN("libbxroce:wqe->destqp1:0x%x\n",wqe->destqp);
	wqe->destqp += tmpwqe.destqp;
	BXPRSEN("libbxroce:wqe->destqp1:0x%x\n",wqe->destqp);
	wqe->destsocket1 = wqe->destsocket1 & 0x0;
	wqe->destsocket1 =tmpwqe.destsocket1;
	wqe->destsocket2 = wqe->destsocket2 & 0xf0;
	wqe->destsocket2 += tmpwqe.destsocket2;
	BXPRSEN("libbxroce:%s destqp:0x%x,  destsocket1: 0x%x,  destsocket2: 0x%x \n"
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
				BXPRSEN("libbxroce: qp type default ...\n");//added by hs
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
				BXPRSEN("libbxroce: wr opcode default...\n");//added by hs
				status = status | 0x2;	
				break;	
	}
	if(status & 0x1||status & 0x2)
	{
		BXPRSEN("libbxroce: transport or  opcode not supported \n");//added by hs 	
		return EINVAL;
	}
	if(qp_type == UD && !(opcode & (SEND|SEND_WITH_IMM)))
		return EINVAL;
	if(qp_type == UC && !(opcode &(SEND|SEND_WITH_IMM|RDMA_WRITE|WRITE_WITH_IMM )))
		return EINVAL;
	if(qp_type == RD && (opcode & SEND_WITH_INV))
		return EINVAL;
	BXPRSEN("libbxroce: %s,qp_type:0x%x , opcode:0x%x \n "
			,__func__,qp_type,opcode);//added by hs
	bxroce_set_wqe_opcode(wqe,qp_type,opcode);
	return 0;

}


static int bxroce_prepare_send_wqe(struct bxroce_qp *qp, struct bxroce_wqe *tmpwqe, struct ibv_send_wr *wr, int i)
{
		//FOR UD
		int status = 0;
		if (qp->qp_type == IBV_QPT_UD) {
				bxroce_set_wqe_destqp(qp,tmpwqe,wr);
				tmpwqe->qkey = wr->wr.ud.remote_qkey;
		}
		else{
			tmpwqe->qkey = qp->qkey;
		}
	
		status = bxroce_build_wqe_opcode(qp,tmpwqe,wr);//added by hs 
		if(status)
			return status;
		//FOR RC
		if(qp->destqp)
			bxroce_set_rcwqe_destqp(qp,tmpwqe);
		
		//FOR RC
		bxroce_set_wqe_dmac(qp,tmpwqe);	
		//tmpwqe->rkey = sg_list[i].rkey;

		tmpwqe->pkey = qp->pkey_index;
		//only ipv4 now!by hs
		tmpwqe->llpinfo_lo = 0;
		tmpwqe->llpinfo_hi = 0;
		memcpy(&tmpwqe->llpinfo_lo,&qp->dgid[0],4);
	
		//add to wqe_tbl, so poll cq will use it.
		if(wr->send_flags & IBV_SEND_SIGNALED || qp->signaled)
				qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].signaled = 1;
		else
				qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].signaled = 0;
		qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].wrid = wr->wr_id;

		return status;

}

static int bxroce_prepare_write_wqe(struct bxroce_qp *qp, struct bxroce_wqe *tmpwqe, struct ibv_send_wr *wr, int i)
{
		int status = 0;
		status = bxroce_build_wqe_opcode(qp,tmpwqe,wr);//added by hs 
		if(status)
			return EINVAL;
		if(qp->destqp)
			bxroce_set_rcwqe_destqp(qp,tmpwqe);
		bxroce_set_wqe_dmac(qp,tmpwqe);
		tmpwqe->rkey = wr->wr.rdma.rkey;
		tmpwqe->destaddr = qp->rdma_addr;//wr->wr.rdma.remote_addr; // a problem, if the other side is passing it's virtual addr, how to resolve it.?
		tmpwqe->qkey = qp->qkey;
		tmpwqe->pkey = qp->pkey_index;
		//only ipv4 now!by hs
		tmpwqe->llpinfo_lo = 0;
		tmpwqe->llpinfo_hi = 0;
		memcpy(&tmpwqe->llpinfo_lo,&qp->dgid[0],4);

		if(wr->send_flags & IBV_SEND_SIGNALED || qp->signaled)
				qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].signaled = 1;
		else
				qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].signaled = 0;
		qp->wqe_wr_id_tbl[(qp->sq.head + i)%qp->sq.max_cnt].wrid = wr->wr_id;

		return status;
}


static void bxroce_printf_wqe(struct bxroce_wqe *tmpwqe)
{
	BXPRSEN("libbxroce: ---------------check wqe--------------\n");//added by hs
	BXPRSEN("libbxroce:immdat:0x%x \n",tmpwqe->immdt);//added by hs
	BXPRSEN("libbxroce:pkey:0x%x \n",tmpwqe->pkey);//added by hs
	BXPRSEN("libbxroce:rkey:0x%x \n",tmpwqe->rkey);//added by hs
	BXPRSEN("libbxroce:lkey:0x%x \n",tmpwqe->lkey);//added by hs
	BXPRSEN("libbxroce:qkey:0x%x \n",tmpwqe->qkey);//added by hs
	BXPRSEN("libbxroce:dmalen:0x%x \n",tmpwqe->dmalen);//added by hs
	BXPRSEN("libbxroce:destaddr:0x%lx \n",tmpwqe->destaddr);//added by hs
	BXPRSEN("libbxroce:localaddr:0x%lx \n",tmpwqe->localaddr);//added by hs
	BXPRSEN("libbxroce:eecntx:0x%x \n",tmpwqe->eecntx);//added by hs
	BXPRSEN("libbxroce:destqp:0x%x \n",tmpwqe->destqp);//added by hs
	BXPRSEN("libbxroce:destsocket1:0x%x \n",tmpwqe->destsocket1);//added by hs
	BXPRSEN("libbxroce:destsocket2:0x%x \n",tmpwqe->destsocket2);//added by hs
	BXPRSEN("libbxroce:opcode:0x%x \n",tmpwqe->opcode);//added by hs
	BXPRSEN("bxroce:llpinfo_lo:0x%x\n",tmpwqe->llpinfo_lo);
	BXPRSEN("bxroce:llpinfo_hi:0x%x\n",tmpwqe->llpinfo_hi);
	BXPRSEN("libbxroce:wqe's addr:%lx \n",tmpwqe);//added by hs
	BXPRSEN("libbxroce:----------------check wqe end------------\n");//added by hs

}


static int bxroce_build_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe, int num_sge, struct ibv_sge *sg_list,struct ibv_send_wr *wr)
{ 
	int i;
	int status = 0;
	struct bxroce_wqe *tmpwqe = wqe;
	struct bxroce_mr_sginfo *mr_sginfo;
	struct sg_phy_info *sg_phy_info;
	struct bxroce_dev *dev;
	int stride = sizeof(*sg_phy_info);
	int j = 0;
	int free_cnt = 0;
	uint64_t offset;
	uint64_t length;
	uint32_t sglength;

	free_cnt = bxroce_hwq_free_cnt(&qp->sq); // need to check again that if wqe's num is enough again
	dev = qp->dev;
	

	BXPRSEN("post send stride: %d \n",stride);

	for (i = 0; i < num_sge; i++) {
		memset(tmpwqe,0,sizeof(*tmpwqe));
		// test every mr.
		userlist_for_each_entry(mr_sginfo, &dev->mr_list, sg_list)
		{
			if ((sg_list[i].addr >= mr_sginfo->iova) && (sg_list[i].addr <= (mr_sginfo->iova + mr_sginfo->length)))
			{		
				printf("build send : find it \n");
				offset = sg_list[i].addr - mr_sginfo->iova;
				tmpwqe->localaddr = mr_sginfo->sginfo->phyaddr + offset + mr_sginfo->offset;
				printf("tmpwqe->localaddr:0x%lx \n",tmpwqe->localaddr);
				break;
			}
		}
		sglength = sg_list[i].length;
		#if 0
		/*if no left space , then return*/
	    if(free_cnt <= 0)
			return ENOMEM;
		#endif

		/*prepare wqe 's pkey,qkey,wqe,dmac info*/
		status = bxroce_prepare_send_wqe(qp,tmpwqe,wr,i);
		if(status)
				return status;

		/*add dma addr that will be access*/
		tmpwqe->lkey = sg_list[i].lkey;

		#if 0
		length = (mr_sginfo->sginfo + j*stride)->size - offset;
		if(sglength >= length)
		tmpwqe->dmalen  = length;//(mr_sginfo->sginfo + j*stride)->size;
		#endif
		tmpwqe->dmalen 	= sglength;

		printf("localaddr: 0x%lx \n",tmpwqe->localaddr);
		printf("dmalen	 : %d	 \n",tmpwqe->dmalen);
		//tmpwqe->localaddr = sg_list[i].addr;
		//tmpwqe->dmalen = sg_list[i].length;
		bxroce_printf_wqe(tmpwqe);	
		//free_cnt -=1;

		if((qp->sq.head + 1) == qp->sq.max_cnt)
			qp->overhead = 1; //means rear is full.
		qp->sq.head = (qp->sq.head + 1) % qp->sq.max_cnt;
		tmpwqe = qp->sq.va + qp->sq.head * qp->sq.entry_size;
		
	}

	if( (num_sge == 0) && (wr->opcode == IBV_WR_SEND_WITH_IMM)) {
		status = bxroce_prepare_send_wqe(qp,tmpwqe,wr,0);
		if((qp->sq.head + 1) == qp->sq.max_cnt)
			qp->overhead = 1; //means rear is full.
		qp->sq.head = (qp->sq.head + 1) % qp->sq.max_cnt;
	}

	return status;
}

static int bxroce_build_inline_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr, uint32_t wqe_size)
{
	int status = 0;

	if (wr->send_flags & IBV_SEND_INLINE && qp->qp_type != IBV_QPT_UD) {//
		wqe->dmalen = bxroce_sglist_len(wr->sg_list,wr->num_sge);
			BXPRSEN("%s() supported_len = 0x%x,\n"
				   "unsupported len req =0x%x,\n,the funtion is not supported now\n",__func__,qp->max_inline_data,wqe->dmalen);//added by hs 
			return EINVAL;
	}
	else {
		status = bxroce_build_sges(qp,wqe,wr->num_sge,wr->sg_list,wr);
		wqe_size +=((wr->num_sge-1)*sizeof(struct bxroce_wqe));
	}
	BXPRSEN("libbxroce: post send, sq.head is %d, sq.tail is %d \n",qp->sq.head,qp->sq.tail);//added by hs
	return status;


}

static int bxroce_build_send(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr)
{
	int status;
	uint32_t wqe_size = sizeof(*wqe);
	status = bxroce_build_inline_sges(qp,wqe,wr,wqe_size);
	return status;
}

static int bxroce_buildwrite_sges(struct bxroce_qp *qp, struct bxroce_wqe *wqe,int num_sge, struct ibv_sge *sg_list, struct ibv_send_wr *wr)
{
	int i;
	int status = 0;
	struct bxroce_wqe *tmpwqe = wqe;
	struct bxroce_mr_sginfo *mr_sginfo =NULL;
	int stride = sizeof(struct sg_phy_info *);
	int j = 0;
	int free_cnt = 0;
	struct bxroce_dev *dev;
	uint64_t offset;
	uint64_t length;
	uint32_t sglength;

	dev= qp->dev;
	free_cnt = bxroce_hwq_free_cnt(&qp->sq); // need to check again that if wqe's num is enough again?
	BXPRSEN("post send stride: %d \n",stride);

	for (i = 0; i < num_sge; i++) {

		memset(tmpwqe,0,sizeof(*tmpwqe));
		// test every mr.
		userlist_for_each_entry(mr_sginfo, &dev->mr_list, sg_list)
		{
			if ((sg_list[i].addr >= mr_sginfo->iova) && (sg_list[i].addr <= (mr_sginfo->iova + mr_sginfo->length)))
			{		
				printf("build send : find it \n");
				printf("sginfo va:0x%lx \n",mr_sginfo->iova);
				printf("sglist addr: 0x%lx\n",sg_list[i].addr);
				printf("sginfo phyaddr:0x%lx\n",mr_sginfo->sginfo->phyaddr);
				printf("sginfo  length: 0x%x \n",mr_sginfo->length);

				offset = sg_list[i].addr - mr_sginfo->iova;
				printf("offset:0x%lx\n",offset);
				tmpwqe->localaddr = mr_sginfo->sginfo->phyaddr + offset + mr_sginfo->offset;
				printf("tmpwqe->localaddr:0x%lx \n",tmpwqe->localaddr);
				break;
			}
		}
		sglength = sg_list[i].length;
		/*prepare wqe qkey,pkey,dmac, info*/
		status = bxroce_prepare_write_wqe(qp,tmpwqe,wr,i);
		if(status)
				return status;

		/*add wqe dma addr to access*/
		tmpwqe->lkey = sg_list[i].lkey;
		#if 0
		length = (mr_sginfo->sginfo + j*stride)->size - offset;
		if(sglength >= length)
		tmpwqe->dmalen  = length;//(mr_sginfo->sginfo + j*stride)->size;
		#endif
		tmpwqe->dmalen 	= sglength;
		printf("localaddr: 0x%lx \n",tmpwqe->localaddr);
		
		//tmpwqe->localaddr = sg_list[i].addr;
		//tmpwqe->dmalen = sg_list[i].length;
		bxroce_printf_wqe(tmpwqe);
		if((qp->sq.head + 1) == qp->sq.max_cnt)
			qp->overhead = 1; //means rear is full.
		qp->sq.head = (qp->sq.head + 1) % qp->sq.max_cnt;
		tmpwqe = qp->sq.va + qp->sq.head * qp->sq.entry_size;
	}

	if((num_sge == 0) && (wr->opcode == IBV_WR_RDMA_WRITE_WITH_IMM)) {
		status = bxroce_prepare_write_wqe(qp,tmpwqe,wr,0);
		if((qp->sq.head + 1) == qp->sq.max_cnt)
			qp->overhead = 1; //means rear is full.
		qp->sq.head = (qp->sq.head + 1) % qp->sq.max_cnt;
	}

	return status;
}

static int bxroce_buildwrite_inline_sges(struct bxroce_qp *qp,struct bxroce_wqe *wqe,const struct ibv_send_wr *wr, uint32_t wqe_size)
{
	int i;
	int status = 0;
	if (wr->send_flags & IBV_SEND_INLINE && qp->qp_type != IBV_QPT_UD) {//
		wqe->dmalen = bxroce_sglist_len(wr->sg_list,wr->num_sge);
			BXPRSEN("%s() supported_len = 0x%x,\n"
				   "unsupported len req =0x%x,\n,the funtion is not supported now\n",__func__,qp->max_inline_data,wqe->dmalen);//added by hs 
			return -EINVAL;
	}
	else {
		status = bxroce_buildwrite_sges(qp,wqe,wr->num_sge,wr->sg_list,wr);
		wqe_size +=((wr->num_sge-1)*sizeof(struct bxroce_wqe));
	}
	BXPRSEN("libbxroce: post send, sq.head is %d, sq.tail is %d\n",qp->sq.head,qp->sq.tail);//added by hs
	return status;
}

static uint64_t bxroce_exchange_dmaaddrinfo(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr)
{
	int port_num = 11988;
	uint32_t ipaddr;
	struct sg_phy_info *sginfo = NULL;
	struct qp_vaddr *vaddr = NULL;
	struct qp_vaddr *tmpvaddr = NULL;
	int i = 0;
	struct ibv_sge *sg_list = NULL;
	int num_sge;
	int len;
	uint64_t phyaddr = 0;

	num_sge = wr->num_sge;
	sg_list = wr->sg_list;
	vaddr  = malloc(sizeof(*vaddr));
	tmpvaddr = vaddr;
	sginfo = malloc(sizeof(struct sg_phy_info));
	memset(vaddr,0,sizeof(*vaddr));
	memset(sginfo,0,sizeof(struct sg_phy_info));

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	ipaddr = (qp->dgid[3] << 24) | (qp->dgid[2] << 16) | (qp->dgid[1] << 8) | (qp->dgid[0]);

	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(ipaddr);

	connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	while(1){
		printf("send data\n");	
		tmpvaddr->vaddr = wr->wr.rdma.remote_addr;
		tmpvaddr->rkey 	= wr->wr.rdma.rkey;
		tmpvaddr->cmd 	= CMD_READ;
		printf("vaddr:0x%lx , addr:0x%lx ,rkey:0x%x\n",tmpvaddr->vaddr, wr->wr.rdma.remote_addr,tmpvaddr->rkey);
		len = sizeof(*vaddr);
		write(client_fd,vaddr,len);

		len = sizeof(struct sg_phy_info);
		int readret =read(client_fd,sginfo,len);
		if(readret == -1)
		{
			printf("error\n");
			return EINVAL;
		}else if(readret == 0){
			printf("server closed socket\n");
			return EINVAL;
		}
		else{
					printf("sginfo->phyaddr:0x%lx \n",sginfo->phyaddr);
					printf("sginfo->size: 0x%x\n",sginfo->size);
					printf("\n");
				break;
		}
	}
	if(sginfo->phyaddr == tmpvaddr->vaddr)
	{	
		printf("cannot find remote's dmaaddr\n");
		return EINVAL;
	}

	qp->addrtbl->vaddr   = wr->wr.rdma.remote_addr;
	qp->addrtbl->dmaaddr = sginfo->phyaddr;
	qp->addrtbl->len	 = sginfo->size;

	phyaddr = sginfo->phyaddr;
	free(vaddr);
	free(sginfo);
	close(client_fd);
	return 0;
	
}


static int bxroce_build_write(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr) 
{
	int status = 0;
	uint64_t dmaaddr = 0;
	uint32_t wqe_size = sizeof(*wqe);
	int offset = 0;
	if((wr->wr.rdma.remote_addr >= qp->addrtbl->vaddr) && (wr->wr.rdma.remote_addr <= (qp->addrtbl->vaddr + qp->addrtbl->len)))
	{
		offset = wr->wr.rdma.remote_addr - qp->addrtbl->vaddr;
		qp->rdma_addr = qp->addrtbl->dmaaddr + offset;
	}
	else{
	status = bxroce_exchange_dmaaddrinfo(qp,wqe,wr);
	if(status)
			return status;
	qp->rdma_addr = qp->addrtbl->dmaaddr;
	printf("dmaaddr:0x%lx \n",dmaaddr);
	}

	status = bxroce_buildwrite_inline_sges(qp,wqe,wr,wqe_size);
	if(status)
		return status;
	return status;
}

static void bxroce_build_read(struct bxroce_qp *qp, struct bxroce_wqe *wqe, const struct ibv_send_wr *wr)
{
	uint32_t wqe_size = sizeof(*wqe);
	uint64_t dmaaddr = 0;
	int status = 0;
	int offset = 0;
	if((wr->wr.rdma.remote_addr >= qp->addrtbl->vaddr) && (wr->wr.rdma.remote_addr <= (qp->addrtbl->vaddr + qp->addrtbl->len)))
	{
		offset = wr->wr.rdma.remote_addr - qp->addrtbl->vaddr;
		qp->rdma_addr = qp->addrtbl->dmaaddr + offset;
	}
	else{
	dmaaddr = bxroce_exchange_dmaaddrinfo(qp,wqe,wr);
	qp->rdma_addr = qp->addrtbl->dmaaddr;
	printf("dmaaddr:0x%lx \n",dmaaddr);
	}
	status = bxroce_buildwrite_inline_sges(qp,wqe,wr,wqe_size);
	if(status)
			return status;
	return status;
}

static void bxroce_ring_sq_hw(struct bxroce_qp *qp, const struct ibv_send_wr *wr,struct bxroce_dev *dev) {
	uint32_t qpn;
	uint32_t phyaddr,tmpvalue;
	uint32_t num_sge;
	uint32_t head;
	uint32_t sq_head;
	int i = 0;
	int num;

	phyaddr = qp->sq.head * qp->sq.entry_size;
	qpn  = qp->id;
	num_sge = wr->num_sge;
	num = qp->overhead?2:1;
	if((num_sge == 0) && (wr->opcode != IBV_WR_RDMA_WRITE_WITH_IMM) && (wr->opcode != IBV_WR_SEND_WITH_IMM))
	{
		printf("send err!, nothing to send\n");
		return;
	}
	
	pthread_mutex_lock(dev->hw_lock);
	for(i = 0;i < num; i++)
	{
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTREADQPN,qpn);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEORREADQPLIST,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x7);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTWRITEQPN,0x0);
	tmpvalue = bxroce_mpb_reg_read(qp->iova,PGU_BASE,READQPLISTDATA);
	BXPRSEN("bxroce:wp:0x%x ,",tmpvalue);//added by hs
	sq_head = tmpvalue / qp->sq.entry_size;
	BXPRSEN("bxroce:sq_head:0x%x ,",sq_head);//added by hs
	if(qp->overhead)
	{phyaddr = qp->sq.max_cnt * qp->sq.entry_size;}
	else
	phyaddr = qp->sq.head * qp->sq.entry_size;
	BXPRSEN("bxroce:qp->sq.head:0x%x \n",qp->sq.head);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WPFORQPLIST,phyaddr);

	if(qp->overhead)
	{bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x3);qp->overhead=0;}
	else
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x1);

	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTWRITEQPN,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEORREADQPLIST,0x0);
	}
	pthread_mutex_unlock(dev->hw_lock);
}


static void bxroce_pgu_info_before_wqe(struct bxroce_qp *qp)
{
	
	
	uint32_t regval = 0;
	uint32_t txop = 0;
	uint32_t rxop = 0;
	uint32_t xmitop = 0;

	BXPRSEN("----------------------PGU INFO BEFORE WQE START ----------------\n");//added by hs
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_INF,qp->id);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_WRRD,0x8);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RCVQ_DI);
	BXPRSEN("\t PGUINFO: RQ qp->iova_L: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RCVQ_DI +0x4);
	BXPRSEN("\t PGUINFO: RQ qp->iova_H: 0x%x\n",regval);

    bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_INF,qp->id);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_WRRD,0x10);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RCVQ_DI);
	BXPRSEN("\t PGUINFO: RQ WP_L: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RCVQ_DI +0x4);
	BXPRSEN("\t PGUINFO: RQ WP_H: 0x%x\n",regval);

	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_INF,qp->id);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_WRRD,0x20);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RCVQ_DI);
	BXPRSEN("\t PGUINFO: RQ RP_L: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RCVQ_DI +0x4);
	BXPRSEN("\t PGUINFO: RQ RP_H: 0x%x\n",regval);

	txop = qp->id;
	txop = txop << 2;//left move 2 bits
	txop = txop + 0x01;
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,CQESIZE,txop);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,CQWRITEPTR);
	BXPRSEN("\t PGUINFO: TXCQ WP_LO: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,CQWRITEPTR +0x4);
	BXPRSEN("\t PGUINFO: TXCQ WP_HI: 0x%x\n",regval);

	rxop = qp->id;
	rxop = rxop << 2;//left move 2 bits
	rxop = rxop + 0x01;
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RxCQEOp,rxop);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RxCQWPT);
	BXPRSEN("\t PGUINFO: RXCQ WP_LO: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RxCQWPT +0x4);
	BXPRSEN("\t PGUINFO: RXCQ WP_HI: 0x%x\n",regval);

	xmitop = qp->id;
	xmitop = xmitop << 2;//left move 2 bits
	xmitop = xmitop + 0x01;
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,XmitCQEOp,xmitop);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,XmitCQWPT);
	BXPRSEN("\t PGUINFO: XMITCQ WP_LO: 0x%x\n",regval);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,XmitCQWPT +0x4);
	BXPRSEN("\t PGUINFO: XMITCQ WP_HI: 0x%x\n",regval);


	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTREADQPN,qp->id);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEORREADQPLIST,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x7);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTWRITEQPN,0x0);

	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,READQPLISTDATA);
	BXPRSEN("\t PGUINFO: SQ WP:0x%x \n",regval);//added by hs
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,READQPLISTDATA2);
	BXPRSEN("\t PGUINFO: SQ RP:0x%x \n",regval);//added by hs
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,READQPLISTDATA3);
	BXPRSEN("\t PGUINFO: SQ qp->iova_LO:0x%x \n",regval);//added by hs
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,READQPLISTDATA4);
	BXPRSEN("\t PGUINFO: SQ qp->iova_HI:0x%x \n",regval);//added by hs

	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTWRITEQPN,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEORREADQPLIST,0x0);

	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,GENRSP);
	BXPRSEN("\t PGUINFO: GENRSP(0x2000): 0x%x \n",regval);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,CFGRNR);
	BXPRSEN("\t PGUINFO: CFGRNR(0x2004): 0x%x \n",regval);
	regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,INTRMASK);
	BXPRSEN("\t PGUINFO: INTRMASK(0x2020):0x%x \n",regval);

	BXPRSEN("------------------------PGU INFO BEFROE WQE END-------------------------------\n");


}


static void bxroce_update_sq_tail(struct bxroce_qp *qp,struct bxroce_dev *dev)
{
	uint32_t tail;
	pthread_mutex_lock(dev->hw_lock);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTREADQPN,qp->id);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEORREADQPLIST,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x7);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTWRITEQPN,0x0);
	tail = bxroce_mpb_reg_read(qp->iova,PGU_BASE,READQPLISTDATA2);
	printf("bxroce:rp is phyaddr:0x%x , sq.tail:%d \n",tail,qp->sq.tail);//added by hs
	//bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x1);
	//bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTWRITEQPN,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEORREADQPLIST,0x0);
	pthread_mutex_unlock(dev->hw_lock);

	qp->sq.tail = tail / (sizeof(struct bxroce_wqe));

	if(qp->sq.tail == qp->sq.head)
	{
		qp->sq.qp_foe = BXROCE_Q_EMPTY;
	}

	printf("bxroce:sq.tail: 0x%x \n",qp->sq.tail);
}

static void bxroce_update_sq_head(struct bxroce_qp *qp, struct ibv_send_wr *wr,struct bxroce_dev *dev)
{

	uint32_t head;
	uint32_t tmphead;

	pthread_mutex_lock(dev->hw_lock);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTREADQPN,qp->id);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEORREADQPLIST,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x7);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTWRITEQPN,0x0);
	head = bxroce_mpb_reg_read(qp->iova,PGU_BASE,READQPLISTDATA);
	printf("bxroce:wp is phyaddr:0x%x , sq.head:%d \n",head,qp->sq.head);//added by hs
	//bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEQPLISTMASK,0x1);
	//bxroce_mpb_reg_write(qp->iova,PGU_BASE,QPLISTWRITEQPN,0x1);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,WRITEORREADQPLIST,0x0);
	pthread_mutex_unlock(dev->hw_lock);

	tmphead = qp->sq.head;
	qp->sq.head = head / qp->sq.entry_size;
	if((tmphead != qp->sq.head) && (qp->sq.head == qp->sq.tail)) // only when sq'head changed ,and changed to equal to tail. that is full.
	{
		qp->sq.qp_foe = BXROCE_Q_FULL;
	}

	#if 0
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
	#endif

	printf("bxroce: post send, sq.head is %d, sq.tail is %d\n",qp->sq.head,qp->sq.tail);

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
	struct bxroce_dev *dev = NULL;
	uint32_t free_cnt;

	qp = get_bxroce_qp(ib_qp);
	dev = get_bxroce_dev(ib_qp->context->device);

	pthread_spin_lock(&qp->q_lock);
	bxroce_update_sq_tail(qp,dev);
	//bxroce_pgu_info_before_wqe(qp);
	
	if (qp->qp_state != BXROCE_QPS_RTS && qp->qp_state != BXROCE_QPS_SQD) {
		pthread_spin_unlock(&qp->q_lock);
		*bad_wr = wr;
		return EINVAL;
	}

	while (wr) {
		qp->overhead = 0;
		BXPRSEN("%s:process wr & write wqe _(:3 < \n",__func__);
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
		switch(wr->opcode){
		case IBV_WR_SEND_WITH_IMM:
			memset(hdwqe,0,sizeof(*hdwqe));
			hdwqe->immdt = be32toh(wr->imm_data);
			SWITCH_FALLTHROUGH;
		case IBV_WR_SEND:
			status = bxroce_build_send(qp,hdwqe,wr);
			break;
		case IBV_WR_SEND_WITH_INV:
			memset(hdwqe,0,sizeof(*hdwqe));
			hdwqe->lkey = be32toh(wr->imm_data);
			status = bxroce_build_send(qp,hdwqe,wr);
			break;
		case IBV_WR_RDMA_WRITE_WITH_IMM:
			memset(hdwqe,0,sizeof(*hdwqe));
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
	
		/*make sure wqe is written befor adapter can access it*/
		BXPRSEN("libbxroce:access hw.. \n");//added by hs
		bxroce_ring_sq_hw(qp,wr,dev); // notify hw to send wqe.

		//update sq's head
		bxroce_update_sq_head(qp,wr,dev);
		wr = wr->next;
	}
	pthread_spin_unlock(&qp->q_lock);
	BXPRSEN("libbxroce; post send end \n");
	return status;
}

static void bxroce_printf_rqe(struct bxroce_rqe *tmprqe)
{
	//BXROCE_PR("bxroce: in rq,num_sge = %d, tmprqe 's addr is %x\n",num_sge,tmprqe);//added by hs
	BXPRREC("libbxroce: ---------------check rqe--------------\n");//added by hs
	BXPRREC("libbxroce:descbaseaddr:0x%x \n",tmprqe->descbaseaddr);//added by hs
	BXPRREC("libbxroce:dmalen:0x%x \n",tmprqe->dmalen);//added by hs
	BXPRREC("libbxroce:opcode:0x%x \n",tmprqe->opcode);//added by hs
	BXPRREC("libbxroce:wqe's addr:%lx \n",tmprqe);//added by hs
	BXPRREC("libbxroce:----------------check rqe end------------\n");//added by hs
}

static void bxroce_build_rqsges(struct bxroce_qp *qp, struct bxroce_rqe *rqe, struct ibv_recv_wr *wr)
{
	int i;
	int num_sge = 0;
	struct bxroce_rqe *tmprqe = NULL;
	struct ibv_sge *sg_list = NULL;
	struct bxroce_mr_sginfo *mr_sginfo = NULL;
	int stride = sizeof(struct sg_phy_info *);
	int j = 0;
	int free_cnt = 0;
	struct bxroce_dev *dev = NULL;
	uint32_t offset;
	uint32_t length;
	uint32_t sglength;

	tmprqe = rqe;
	sg_list = wr->sg_list;
	num_sge = wr->num_sge;
	dev = qp->dev;
	free_cnt = bxroce_hwq_free_cnt(&qp->sq); // need to check again that if wqe's num is enough again?
	pthread_mutex_lock(&dev->dev_lock);
	for (i = 0; i < num_sge; i++) {
		j = 0;
		// test every mr.
		userlist_for_each_entry(mr_sginfo, &dev->mr_list, sg_list)
		{
			if (sg_list[i].addr == mr_sginfo->iova)
			{		
				printf("build send : find it \n");
				break;
			}
		}
		offset = mr_sginfo->offset;
		sglength = sg_list[i].length;
		
		memset(tmprqe,0,sizeof(*tmprqe)); 
	    /*sg length is 0,break;*/
		if(sglength <= 0 )
			break;
		/*no left space, return*/
		if(free_cnt <= 0)
			return ENOMEM;

		tmprqe->descbaseaddr = (mr_sginfo->sginfo + j*stride)->phyaddr + mr_sginfo->offset;
		#if 0
		length = (mr_sginfo->sginfo + j*stride)->size - offset;
		if(sglength >= length)
		tmprqe->dmalen  = length;//(mr_sginfo->sginfo + j*stride)->size;
		else
		#endif
		tmprqe->dmalen 	= sglength;
		//tmprqe->descbaseaddr = sg_list[i].addr;
		//tmprqe->dmalen = sg_list[i].length;

		tmprqe->opcode = 0x80000000;
		qp->rqe_wr_id_tbl[(qp->rq.head + i) % qp->rq.max_cnt] = wr->wr_id;

		bxroce_printf_rqe(tmprqe);
		tmprqe += 1;
	}
	pthread_mutex_unlock(&dev->dev_lock);
	if(num_sge == 0)
		memset(tmprqe,0,sizeof(*tmprqe));
}

static void bxroce_build_rqe(struct bxroce_qp *qp,struct bxroce_rqe *rqe, const struct ibv_recv_wr *wr) 
{
	uint32_t wqe_size = 0;

	bxroce_build_rqsges(qp,rqe,wr);
	wqe_size +=((wr->num_sge-1) * sizeof(struct bxroce_rqe));
	#if 0
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
	#endif
	BXPRREC("qp->rq.head:0x%x\n",qp->rq.head);
	
	
}

static void bxroce_ring_rq_hw(struct bxroce_qp *qp, const struct ibv_recv_wr *wr,struct bxroce_dev *dev)
{
	uint32_t qpn;
	uint32_t phyaddr,tmpvalue;


	phyaddr = (qp->rq.head + wr->num_sge) * qp->rq.entry_size;
	qpn  = qp->id;
	
	pthread_mutex_lock(dev->hw_lock);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_INF,qpn);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_DI,phyaddr);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_DI + 0x4,0x0);
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RCVQ_WRRD, 0x2);
	pthread_mutex_unlock(dev->hw_lock);

}


static void bxroce_update_rq_head(struct bxroce_qp *qp, const struct ibv_recv_wr *wr, struct bxroce_dev *dev)
{
	if(wr->num_sge){
			qp->rq.head = (qp->rq.head + wr->num_sge) % qp->rq.max_cnt; // update the head ptr,and check if the queue if full.
			if(qp->rq.head == qp->rq.tail){
				qp->rq.qp_foe = BXROCE_Q_FULL;
			}
			
	}
}


static void bxroce_update_rq_tail(struct bxroce_qp *qp,struct bxroce_dev *dev)
{
	BXPRREC("update rq tail code later\n");
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
	struct bxroce_dev *dev;
	uint32_t free_cnt = 0;
	qp = get_bxroce_qp(ib_qp);
	dev = get_bxroce_dev(ib_qp->context->device);

	pthread_spin_lock(&qp->q_lock);
	bxroce_update_rq_tail(qp,dev);
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

		bxroce_ring_rq_hw(qp,wr,dev);
		bxroce_update_rq_head(qp,wr,dev);
		wr = wr->next;
	}
	pthread_spin_unlock(&qp->q_lock);

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
static void *bxroce_txcq_hwwp(struct bxroce_cq *cq ,struct bxroce_qp *qp)
{

	uint32_t cqwp_lo = 0;
	uint32_t cqwp_hi = 0;
	uint64_t cqwp = 0;
	uint32_t txop = 0;

	txop = 0;
	txop = qp->id;
	txop = txop << 2;//left move 2 bits
	txop = txop + 0x1;

	bxroce_mpb_reg_write(qp->iova,PGU_BASE,CQESIZE,txop);
	cqwp_lo = bxroce_mpb_reg_read(qp->iova,PGU_BASE,CQWRITEPTR);
	cqwp_hi = bxroce_mpb_reg_read(qp->iova,PGU_BASE,CQWRITEPTR +0x4);

	cqwp = cqwp_hi;
	cqwp = cqwp << 32;//hi left move to higher bits
	cqwp = cqwp + cqwp_lo;
	cq->txwp = (cqwp - cq->txpa)/cq->cqe_size;
	BXPRCQ("cq->txwp:0x%x , cqe_size:0x%x \n",cq->txwp,cq->cqe_size);
	
	return cq->txva +(cq->txwp * cq->cqe_size);

}

/*read hw to get hw wp*/
static void *bxroce_rxcq_hwwp(struct bxroce_cq *cq ,struct bxroce_qp *qp)
{

	uint32_t cqwp_lo = 0;
	uint32_t cqwp_hi = 0;
	uint64_t cqwp = 0;
	uint32_t rxop = 0;

	rxop = 0;
	rxop = qp->id;
	rxop = rxop << 2;//left move 2 bits
	rxop = rxop + 0x1;

	bxroce_mpb_reg_write(qp->iova,PGU_BASE,RxCQEOp,rxop);
	cqwp_lo = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RxCQWPT);
	cqwp_hi = bxroce_mpb_reg_read(qp->iova,PGU_BASE,RxCQWPT +0x4);
	cqwp = cqwp_hi;
	cqwp = cqwp << 32;//hi left move to higher bits
	cqwp = cqwp + cqwp_lo;
	cq->rxwp = (cqwp - cq->rxpa)/cq->cqe_size;
	BXPRCQ("cq->rxwp:0x%x , cqe_size:0x%x \n",cq->rxwp,cq->cqe_size);
	
	return cq->rxva +(cq->rxwp * cq->cqe_size);

}

/*read hw to get hw wp*/
static void *bxroce_xmitcq_hwwp(struct bxroce_cq *cq ,struct bxroce_qp *qp)
{
		
	uint32_t cqwp_lo = 0;
	uint32_t cqwp_hi = 0;
	uint64_t cqwp = 0;
	uint32_t xmitop = 0;

	
	xmitop = 0;
	xmitop = qp->id;
	xmitop = xmitop << 2;//left move 2 bits
	xmitop = xmitop + 0x1;
	bxroce_mpb_reg_write(qp->iova,PGU_BASE,XmitCQEOp,xmitop);
	cqwp_lo = bxroce_mpb_reg_read(qp->iova,PGU_BASE,XmitCQWPT);
	cqwp_hi = bxroce_mpb_reg_read(qp->iova,PGU_BASE,XmitCQWPT +0x4);
	cqwp = cqwp_hi;
	cqwp = cqwp << 32;//hi left move to higher bits
	cqwp = cqwp + cqwp_lo;
	cq->xmitwp = (cqwp - cq->xmitpa)/cq->cqe_size;
	BXPRCQ("cq->xmitwp:0x%x , cqe_size:0x%x \n",cq->xmitwp,cq->cqe_size);
	
	return cq->xmitva +(cq->xmitwp * cq->cqe_size);

}


static int bxroce_poll_hwcq(struct bxroce_cq *cq, int num_entries, struct ibv_wc *ibwc)
{

		
		struct bxroce_qp *qp =NULL;
		struct bxroce_dev *dev = cq->dev;
		struct bxroce_txcqe *txrpcqe;
		struct bxroce_rxcqe *rxrpcqe;
		struct bxroce_xmitcqe *xmitrpcqe;
		struct bxroce_txcqe *txwpcqe;
		struct bxroce_rxcqe *rxwpcqe;
		struct bxroce_xmitcqe *xmitwpcqe;
		uint64_t phyaddr;
		int i  = 0;
		uint32_t num_rx_total = 0;
		uint32_t num_xmit_total = 0;
		uint32_t sq_tail = 0;
		uint32_t sq_ptail = 0;
		 uint32_t regval = 0 ;
		BXPRCQ("get in bxroce_poll_hwcq\n");

		if(dev->qp_tbl[cq->qp_id]) //different from other rdma driver, cq only mapped to one qp.
			qp = dev->qp_tbl[cq->qp_id];

		//get hw wp,hw update it;
		//txwpcqe = bxroce_txcq_hwwp(cq,qp,dev);
		//rxwpcqe = bxroce_rxcq_hwwp(cq,qp,dev);
		//xmitwpcqe = bxroce_xmitcq_hwwp(cq,qp,dev);

		//get rp,software need update it;
		txrpcqe = bxroce_txcq_head(cq);
		rxrpcqe = bxroce_rxcq_head(cq);
		xmitrpcqe = bxroce_xmitcq_head(cq);

	#if 0
		BXPRCQ("read txcq,rxcq,xmitcq's member\n");//
		BXPRCQ("\ttxrpcqe->pkey:0x%x",txrpcqe->pkey);
		BXPRCQ("\ttxrpcqe->opcode:0x%x\n",txrpcqe->opcode);
		BXPRCQ("\ttxrpcqe->immdt:0x%x\n",txrpcqe->immdt);
		BXPRCQ("\ttxrpcqe->destqp:0x%x\n",txrpcqe->destqp);
		BXPRCQ("\ttxrpcqe->reserved1:0x%x\n",txrpcqe->reserved1);
		BXPRCQ("\ttxrpcqe->reserved2:0x%x\n",txrpcqe->reserved2);
		BXPRCQ("\ttxrpcqe->reserved3:0x%x\n",txrpcqe->reserved3);

	#endif
	
		BXPRCQ("\trxrpcqe->bth_pkey:0x%x\n",rxrpcqe->bth_pkey);
		BXPRCQ("\trxrpcqe->bth_24_31:0x%x\n",rxrpcqe->bth_24_31);
		BXPRCQ("\trxrpcqe->bth_destqp:0x%x\n",rxrpcqe->bth_destqp);
		BXPRCQ("\trxrpcqe->rcvdestqp:0x%x\n",rxrpcqe->rcvdestqpeecofremoterqt);
		BXPRCQ("\trxrpcqe->bth_64_87_lo:0x%x\n",rxrpcqe->bth_64_87_lo);
		BXPRCQ("\trxrpcqe->bth_64_87_hi:0x%x\n",rxrpcqe->bth_64_87_hi);
		BXPRCQ("\trxrpcqe->aeth:0x%x\n",rxrpcqe->aeth);
		BXPRCQ("\trxrpcqe->immdt:0x%x\n",rxrpcqe->immdt);
		BXPRCQ("\trxrpcqe->hff:0x%x\n",rxrpcqe->hff);


		BXPRCQ("\txmitrpcqe->bth_pkey:0x%x\n",xmitrpcqe->bth_pkey);
		BXPRCQ("\txmitrpcqe->bth_24_31:0x%x\n",xmitrpcqe->bth_24_31);
		BXPRCQ("\txmitrpcqe->bth_destqp:0x%x\n",xmitrpcqe->bth_destqp);
		BXPRCQ("\txmitrpcqe->destqpeecofremoterqt:0x%x\n",xmitrpcqe->destqpeecofremoterqt);
		BXPRCQ("\trxrpcqe->bth_64_87_lo:0x%x\n",xmitrpcqe->bth_64_87_lo);
		BXPRCQ("\trxrpcqe->bth_64_87_hi:0x%x\n",xmitrpcqe->bth_64_87_hi);
		BXPRCQ("\trxrpcqe->aeth:0x%x\n",xmitrpcqe->aeth);
		BXPRCQ("\txmitrpcqe->immdt:0x%x\n",xmitrpcqe->immdt);
		BXPRCQ("\txmitrpcqe->hff:0x%x\n",xmitrpcqe->hff);

		

		while(num_entries){
				if(!ibwc)
						break;
				//pretend that success.
				if(xmitrpcqe->hff)
				{
				printf("get xmitcq\n");
				num_entries -= 1;
				ibwc->status = IBV_WC_SUCCESS;
				ibwc->wc_flags = 0;
				i += 1;
				ibwc += 1;
				num_xmit_total++;

				qp->sq.tail += 1;
				printf("qp->sq.tail:%d \n",qp->sq.tail);
				printf("\n");
				memset(xmitrpcqe,0,sizeof(*xmitrpcqe));
				cq->xmitrp = (cq->xmitrp + 1) % 256;
				xmitrpcqe = bxroce_xmitcq_head(cq);
				}
				else
				{
					break; // 
				}
				
	
				if(rxrpcqe->hff)
					{num_rx_total++;
					memset(rxrpcqe,0,sizeof(*rxrpcqe));
					cq->rxrp = (cq->rxrp + 1) % 256;
					rxrpcqe = bxroce_rxcq_head(cq);
					}
		
		//txwpcqe = bxroce_txcq_hwwp(cq,qp);
		//rxwpcqe = bxroce_rxcq_hwwp(cq,qp);
		//xmitwpcqe = bxroce_xmitcq_hwwp(cq,qp);
		}
		BXPRCQ("numxmitcq:0x%x , numrxcq:0x%x\n",num_xmit_total,num_rx_total);
#if 0
		BXPRCQ("numxmitcq:0x%x , numrxcq:0x%x\n",num_xmit_total,num_rx_total);
		BXPRCQ("read txcq,rxcq,xmitcq's member\n");//
		BXPRCQ("\ttxrpcqe->pkey:0x%x",txrpcqe->pkey);
		BXPRCQ("\ttxrpcqe->opcode:0x%x\n",txrpcqe->opcode);
		BXPRCQ("\ttxrpcqe->immdt:0x%x\n",txrpcqe->immdt);
		BXPRCQ("\ttxrpcqe->destqp:0x%x\n",txrpcqe->destqp);
		BXPRCQ("\ttxrpcqe->reserved1:0x%x\n",txrpcqe->reserved1);
		BXPRCQ("\ttxrpcqe->reserved2:0x%x\n",txrpcqe->reserved2);
		BXPRCQ("\ttxrpcqe->reserved3:0x%x\n",txrpcqe->reserved3);

#endif
		BXPRCQ("\trxrpcqe->bth_pkey:0x%x\n",rxrpcqe->bth_pkey);
		BXPRCQ("\trxrpcqe->bth_24_31:0x%x\n",rxrpcqe->bth_24_31);
		BXPRCQ("\trxrpcqe->bth_destqp:0x%x\n",rxrpcqe->bth_destqp);
		BXPRCQ("\trxrpcqe->rcvdestqp:0x%x\n",rxrpcqe->rcvdestqpeecofremoterqt);
		BXPRCQ("\trxrpcqe->bth_64_87_lo:0x%x\n",rxrpcqe->bth_64_87_lo);
		BXPRCQ("\trxrpcqe->bth_64_87_hi:0x%x\n",rxrpcqe->bth_64_87_hi);
		BXPRCQ("\trxrpcqe->aeth:0x%x\n",rxrpcqe->aeth);
		BXPRCQ("\trxrpcqe->immdt:0x%x\n",rxrpcqe->immdt);
		BXPRCQ("\trxrpcqe->hff:0x%x\n",rxrpcqe->hff);


		BXPRCQ("\txmitrpcqe->bth_pkey:0x%x\n",xmitrpcqe->bth_pkey);
		BXPRCQ("\txmitrpcqe->bth_24_31:0x%x\n",xmitrpcqe->bth_24_31);
		BXPRCQ("\txmitrpcqe->bth_destqp:0x%x\n",xmitrpcqe->bth_destqp);
		BXPRCQ("\txmitrpcqe->destqpeecofremoterqt:0x%x\n",xmitrpcqe->destqpeecofremoterqt);
		BXPRCQ("\txmitrpcqe->bth_64_87_lo:0x%x\n",xmitrpcqe->bth_64_87_lo);
		BXPRCQ("\txmitrpcqe->bth_64_87_hi:0x%x\n",xmitrpcqe->bth_64_87_hi);
		BXPRCQ("\txmitrpcqe->aeth:0x%x\n",xmitrpcqe->aeth);
		BXPRCQ("\txmitrpcqe->immdt:0x%x\n",xmitrpcqe->immdt);
		BXPRCQ("\txmitrpcqe->hff:0x%x\n",xmitrpcqe->hff);

		regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,0x2000);
		printf("REG ADDR 0X2000:0x%x \n",regval);

		regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,0x205c);
		printf("REG ADDR 0X205c:0X%x \n",regval);

			regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,0x2060);
		printf("REG ADDR 0X2060:0X%x \n",regval);

			regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,0x2064);
		printf("REG ADDR 0X2064:0X%x \n",regval);

			regval = bxroce_mpb_reg_read(qp->iova,PGU_BASE,0x2068);
		printf("REG ADDR 0X2068:0X%x \n",regval);




	    return i;
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
	BXPRCQ("num_entries:%d \n",num_entries);

	cq = get_bxroce_cq(ibcq);
	struct bxroce_dev *dev = cq->dev;

	pthread_spin_lock(&cq->lock);
	num_os_cqe = bxroce_poll_hwcq(cq,num_entries,wc);
	pthread_spin_unlock(&cq->lock);
	cqes_to_poll -= num_os_cqe;

	if (cqes_to_poll) {
		BXPRCQ("some err happen in cq \n");
		if(dev->qp_tbl[cq->qp_id]) //different from other rdma driver, cq only mapped to one qp.
			qp = dev->qp_tbl[cq->qp_id];
		wc = wc + num_os_cqe;

		while(cqes_to_poll)
		{
			if(qp->sq.tail == qp->sq.head) //means cqe 's number is right
				break;
			else 
			printf("wqe err\n");
			break;
		}
	}

	BXPRCQ("%s:process cqe, return num_os_cqe  \n",__func__);//added by hs
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
	BXPRCQ("%s:pretend that i am working  \n",__func__);//added by hs
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
