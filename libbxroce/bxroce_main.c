/*
 *For userspace drvier
 *
 *
 *
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>

#include "bxroce_main.h"
#include "bxroce_abi.h"
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

static void bxroce_free_context(struct ibv_context *ibctx);

static const struct verbs_match_ent bx_table[] = {
	VERBS_DRIVER_ID(RDMA_DRIVER_UNKNOWN),
	VERBS_PCI_MATCH(0x16ca, 0x7312,NULL),
	VERBS_PCI_MATCH(0x17cd, 0x7312,NULL),
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
	printf("libbxroce:%s, start \n",__func__);//added by hs
	struct bxroce_dev *dev = get_bxroce_dev(&verbs_device->device);
	free(dev);
	printf("libbxroce:%s, end \n",__func__);//added by hs
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
	printf("libbxroce:%s, start \n",__func__);//added by hs
	ctx = verbs_init_and_alloc_context(ibdev,cmd_fd,ctx,ibv_ctx,RDMA_DRIVER_UNKNOWN);
	if(!ctx)
			return NULL;

	if(ibv_cmd_get_context(&ctx->ibv_ctx,(struct ibv_get_context *)&cmd, sizeof cmd,&resp.ibv_resp, sizeof(resp)))
		goto cmd_err;
	
//	printf("libbxroce:%s, cmd get ctx  end\n ",__func__);//added by hs
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

	printf("-------------------check dev param-------------------\n");
	printf("id:%x \n",dev->id);
	printf("wqe_size:%x \n",dev->wqe_size);
	printf("rqe_size:%x \n",dev->rqe_size);
	printf("-------------------check dev param end---------------\n");
//	get_bxroce_dev(ibdev)->id = resp.dev_id;
	printf("libbxroce:%s, end \n",__func__);//added by hs

	

	return &ctx->ibv_ctx;
cmd_err:
	bxroce_err("%s:Failed to allocate context for device .\n",__func__);
	verbs_uninit_context(&ctx->ibv_ctx);
	free(ctx);
	printf("libbxroce:%s, err end \n",__func__);//added by hs
	return NULL;
}

/*
*bxroce_free context
*/
static void bxroce_free_context(struct ibv_context *ibctx)
{
		struct bxroce_devctx *ctx = get_bxroce_ctx(ibctx);
		printf("libbxroce:%s, start \n",__func__);//added by hs

		if(ctx->ah_tbl)
			munmap((void *)ctx->ah_tbl,ctx->ah_tbl_len);

		verbs_uninit_context(&ctx->ibv_ctx);
		free(ctx);
		printf("libbxroce:%s, end \n",__func__);//added by hs
}


/*alloc verbs_device,these will be accessed by libibverbs*/
static struct verbs_device *
bxroce_device_alloc(struct verbs_sysfs_dev *sysfs_dev)
{
	struct bxroce_dev *dev;
	printf("libbxroce:%s, start \n",__func__);//added by hs
	dev = calloc(1,sizeof(*dev));
	if(!dev)
		return NULL;
	dev->qp_tbl = malloc(BXROCE_MAX_QP * sizeof(struct bxroce_qp *));
	if(!dev->qp_tbl)
		goto qp_err;
	bzero(dev->qp_tbl,BXROCE_MAX_QP * sizeof(struct bxroce_qp *));
	INIT_USERLIST_HEAD(&dev->mr_list); // init mr list;
	pthread_mutex_init(&dev->dev_lock,NULL);
	pthread_spin_init(&dev->flush_q_lock,PTHREAD_PROCESS_PRIVATE);

	printf("libbxroce:%s, end \n",__func__);//added by hs
	return &dev->ibv_dev;

qp_err:
	printf("libbxroce:%s, err end \n",__func__);//added by hs
	free(dev);
	return NULL;
}


/*bxorce_dev_ops - for device ops in userspace,
*A interface provided by libibverbs
*/
static const struct verbs_device_ops bxroce_dev_ops = {
	.name = "bxorce",
	.match_min_abi_version = BXROCE_ABI_MIN_VERSION,
	.match_max_abi_version = BXROCE_ABI_MAX_VERSION,
	.match_table = bx_table,
	.alloc_device = bxroce_device_alloc,
	.uninit_device = bxroce_uninit_device,
	.alloc_context = bxroce_alloc_context,
};
PROVIDER_DRIVER(bxroce,bxroce_dev_ops);


