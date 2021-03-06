/*
 *For userspace drvier
 *
 *
 *
 */


#include "bxroce_main.h"

static void bxroce_free_context(struct ibv_context *ibctx);

static const struct verbs_match_ent bx_table[] = {
	VERBS_DRIVER_ID(RDMA_DRIVER_UNKNOWN),
	VERBS_PCI_MATCH(0x1ea9, 0x7312,NULL),
	VERBS_PCI_MATCH(0x16ca, 0x7312,NULL),
	VERBS_PCI_MATCH(0x17cd, 0x7312,NULL),
	VERBS_PCI_MATCH(0x1ea9, 0x1001,NULL),
	{}
};

static const struct verbs_context_ops bxroce_ctx_ops = {
	.query_device = bxroce_query_device,
    .query_port = bxroce_query_port,
    .alloc_pd = bxroce_alloc_pd,
    .dealloc_pd = bxroce_free_pd,
    .reg_mr = bxroce_reg_mr,
    .dereg_mr = bxroce_dereg_mr,
    .create_cq = bxroce_create_cq,
    .poll_cq = bxroce_poll_cq,
    .req_notify_cq = bxroce_arm_cq,
    .resize_cq = bxroce_resize_cq,
    .destroy_cq = bxroce_destroy_cq,

    .create_qp = bxroce_create_qp,
    .query_qp = bxroce_query_qp,
    .modify_qp = bxroce_modify_qp,
    .destroy_qp = bxroce_destroy_qp,
    .post_send = bxroce_post_send,
    .post_recv = bxroce_post_recv,
    .create_ah = bxroce_create_ah,
    .destroy_ah = bxroce_destroy_ah,

  //  .create_srq = bxroce_create_srq,
  //  .modify_srq = bxroce_modify_srq,
  //  .query_srq = bxroce_query_srq,
  //  .destroy_srq = bxroce_destroy_srq,
  //  .post_srq_recv = bxroce_post_srq_recv,
  //  .attach_mcast = bxroce_attach_mcast,
  //  .detach_mcast = bxroce_detach_mcast

	.free_context = bxroce_free_context
};

/*free verbs device*/
static void bxroce_uninit_device(struct verbs_device *verbs_device)
{
	BXPROTH("libbxroce:%s, start \n",__func__);//added by hs
	struct bxroce_dev *dev = get_bxroce_dev(&verbs_device->device);
	if(shmdt((void *)dev->hw_lock) == -1)
    {
        printf("failed to shmdt\n");
	}

	free(dev);
	BXPROTH("libbxroce:%s, end \n",__func__);//added by hs
}

#if 0
static void *server_fun(void *arg){
	Serverinfo *info = (Serverinfo *)arg;
	struct bxroce_dev *dev = info->dev;
	struct sg_phy_info *sginfo = NULL;
	struct sg_phy_info *tmpsginfo =NULL;
	struct qp_vaddr *vaddr = NULL;
	struct qp_vaddr *tmpvaddr = NULL;
	int len = 0;
	struct bxroce_mr_sginfo *mr_sginfo;
	int i =0;
	int shm;
	int buflen;
	void *shmstart = NULL;

	vaddr = malloc(sizeof(*vaddr));
	sginfo = malloc(sizeof(struct sg_phy_info) );
	tmpsginfo = sginfo;

	memset(vaddr,0,sizeof(*vaddr));
	memset(sginfo,0,sizeof(struct sg_phy_info));
	
	tmpvaddr = (struct qp_vaddr *)info->addr;
	vaddr->vaddr = tmpvaddr->vaddr;
	printf("vaddr:0x%lx\n",vaddr->vaddr);
	userlist_for_each_entry(mr_sginfo, &dev->mr_list, sg_list)
	{
		printf("addr:0x%lx \n",mr_sginfo->iova);
		if (vaddr->vaddr == mr_sginfo->iova)
		{		
			printf("build send :  find it \n");
			tmpvaddr->vaddr = mr_sginfo->sginfo->phyaddr + mr_sginfo->offset;
			tmpvaddr->qpid 	= 0;
			break;
		}
	}
	free(info);
	info = NULL;
	printf("pthread exit\n");
	pthread_exit(NULL);

}
#endif

static void bxroce_start_listening_server(struct bxroce_dev *dev)
{
	int shm;
	int buflen;
	void *shmstart = NULL;
	int status;
	struct qp_vaddr *tmpvaddr = NULL;
	uint8_t *shmoffset = NULL;

	buflen = sizeof(pthread_mutex_t);
	shm = shmget(IPC_KEY,buflen,IPC_CREAT|0666);
	if(shm == -1 )
	{
		printf("shmget failed \n");
		return -1;
	}

	shmstart = shmat(shm,NULL,0);
	if(shmstart == (void *) -1)
	{
		printf("shmat fialed");
		return -1;
	}
	dev->hw_lock  = (pthread_mutex_t *)(shmstart);
}


/*alloc verbs context*/
static struct verbs_context* bxroce_alloc_context(struct ibv_device *ibdev,
												   int cmd_fd,
												   void *private_data)
{
	struct bxroce_devctx *ctx;
	struct bxroce_dev *dev;
	struct ubxroce_get_context cmd;
	struct ubxroce_get_context_resp resp;
	BXPROTH("libbxroce:%s, start \n",__func__);//added by hs
	ctx = verbs_init_and_alloc_context(ibdev,cmd_fd,ctx,ibv_ctx,RDMA_DRIVER_UNKNOWN);
	if(!ctx)
			return NULL;

	if(ibv_cmd_get_context(&ctx->ibv_ctx,(struct ibv_get_context *)&cmd, sizeof cmd,&resp.ibv_resp, sizeof(resp)))
		goto cmd_err;
	
//	BXPROTH("libbxroce:%s, cmd get ctx  end\n ",__func__);//added by hs
	verbs_set_ops(&ctx->ibv_ctx, &bxroce_ctx_ops);

	dev = get_bxroce_dev(ibdev);
	dev->id = resp.dev_id;
	dev->wqe_size = resp.wqe_size;
	dev->rqe_size = resp.rqe_size;
	memcpy(dev->fw_ver, resp.fw_ver, sizeof(resp.fw_ver));

	ctx->ah_tbl = mmap(NULL,resp.ah_tbl_len,PROT_READ | PROT_WRITE, MAP_SHARED, cmd_fd, resp.ah_tbl_page);
	if(ctx->ah_tbl == MAP_FAILED)
		goto cmd_err;

	ctx->ah_tbl_len = resp.ah_tbl_len;
	bxroce_init_ahid_tbl(ctx);

	BXPROTH("-------------------check dev param-------------------\n");
	BXPROTH("id:%x \n",dev->id);
	BXPROTH("wqe_size:%x \n",dev->wqe_size);
	BXPROTH("rqe_size:%x \n",dev->rqe_size);
	BXPROTH("-------------------check dev param end---------------\n");
//	get_bxroce_dev(ibdev)->id = resp.dev_id;
	BXPROTH("libbxroce:%s, end \n",__func__);//added by hs

	printf("start listening server to accept data to exchange...\n");

	#if 0 // del by hs
	pid_t pid = 0;
	pid = fork(); // usr child process to start this server.
	if(pid == 0)
	{
		bxroce_start_listening_server(dev,ctx);
	}
	#endif
	// create thread not process.
	bxroce_start_listening_server(dev);
	

	return &ctx->ibv_ctx;
cmd_err:
	bxroce_err("%s:Failed to allocate context for device .\n",__func__);
	verbs_uninit_context(&ctx->ibv_ctx);
	free(ctx);
	BXPROTH("libbxroce:%s, err end \n",__func__);//added by hs
	return NULL;
}

/*
*bxroce_free context
*/
static void bxroce_free_context(struct ibv_context *ibctx)
{
		struct bxroce_devctx *ctx = get_bxroce_ctx(ibctx);
		BXPROTH("libbxroce:%s, start \n",__func__);//added by hs

		if(ctx->ah_tbl)
			munmap((void *)ctx->ah_tbl,ctx->ah_tbl_len);

		verbs_uninit_context(&ctx->ibv_ctx);
		free(ctx);
		BXPROTH("libbxroce:%s, end \n",__func__);//added by hs
}


/*alloc verbs_device,these will be accessed by libibverbs*/
static struct verbs_device *
bxroce_device_alloc(struct verbs_sysfs_dev *sysfs_dev)
{
	struct bxroce_dev *dev;
	BXPROTH("libbxroce:%s, start \n",__func__);//added by hs
	dev = calloc(1,sizeof(*dev));
	if(!dev)
		return NULL;
	dev->qp_tbl = malloc(BXROCE_MAX_QP * sizeof(struct bxroce_qp *));
	if(!dev->qp_tbl)
		goto qp_err;
	bzero(dev->qp_tbl,BXROCE_MAX_QP * sizeof(struct bxroce_qp *));
	INIT_USERLIST_HEAD(&dev->mr_list); // init mr list;
	pthread_mutex_init(&dev->dev_lock,NULL);
	dev->hw_lock = NULL;
	pthread_spin_init(&dev->flush_q_lock,PTHREAD_PROCESS_PRIVATE);

	//start a listen server to exchange data.

	BXPROTH("libbxroce:%s, end \n",__func__);//added by hs
	return &dev->ibv_dev;

qp_err:
	BXPROTH("libbxroce:%s, err end \n",__func__);//added by hs
	free(dev);
	return NULL;
}


/*bxorce_dev_ops - for device ops in userspace,
*A interface provided by libibverbs
*/
static const struct verbs_device_ops bxroce_dev_ops = {
	.name = "bxroce",
	.match_min_abi_version = BXROCE_ABI_MIN_VERSION,
	.match_max_abi_version = BXROCE_ABI_MAX_VERSION,
	.match_table = bx_table,
	.alloc_device = bxroce_device_alloc,
	.uninit_device = bxroce_uninit_device,
	.alloc_context = bxroce_alloc_context,
};
PROVIDER_DRIVER(bxroce,bxroce_dev_ops);


