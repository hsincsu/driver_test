/*
 *
 * This is the main file with the main funciton.All start here!
 *
 *
 *
 * 						------edited by hs in 2019/6/18
 *
 */

#include <linux/module.h>
#include <linux/idr.h>
#include <linux/inetdevice.h>
#include <linux/if_addr.h>
#include <linux/notifier.h>


#include <rdma/rdma_netlink.h>
#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_mad.h>

#include <linux/netdevice.h>
#include <net/addrconf.h>

#include "header/bxroce.h"
//#include "header/bxroce_verbs.h"
//#include "header/bxroce_ah.h"
//#include "header/bxroce_hw.h"
//#include "header/bxroce_abi.h"
//#include <rdma/ocrdma-abi.h>
#define HSDEBUG 1

MODULE_VERSION(BXROCEDRV_VER);
MODULE_AUTHOR("HS");
MODULE_LICENSE("Dual BSD/GPL");

static DEFINE_IDR(bx_dev_id);

//for cm test 
int  len_array_0 [1024];
int  wptr_0;
int  rptr_0;
int  send_cnt_0;
int  send_len;
int  recv_cnt_0;

int  len_array_1 [1024];
int  wptr_1;
int  rptr_1;
int  send_cnt_1;
int  recv_cnt_1;


int get_len(int port_id)
{
	int len;
    if(port_id == 0)
    {
        len = len_array_0[rptr_0];
        rptr_0 ++;
        recv_cnt_0 ++;  
        if(rptr_0 == 1024)
        {
            rptr_0 = 0;
            printk("Port 0 recv %d msg\n",recv_cnt_0);
        }
    }
    else
    {
        len = len_array_1[rptr_1];
        rptr_1 ++;
        recv_cnt_1 ++;  
        if(rptr_1 == 1024)
        {
            rptr_1 = 0;
            printk("Port 1 recv %d msg\n",recv_cnt_1);
        }
    }
    return len;
}

void set_len(int port_id, int len)
{
	if(port_id == 0)
    {
        len_array_0[wptr_0] = len;
        wptr_0 ++;
        send_cnt_0 ++;
        send_len = send_len + (len >> 2) + (len%4 != 0);
        
        if(wptr_0 == 1024)
        {
            wptr_0 = 0;
            printk("Port 0 send %d msg\n",send_cnt_0);
        }
        //printf("Port 0 send_len is %d\n",send_len);   
    }
    else
    {
        len_array_1[wptr_1] = len;
        wptr_1 ++;
        send_cnt_1 ++;
        if(wptr_1 == 1024)
        {
            wptr_1 = 0;
            printk("Port 1 send %d msg\n",send_cnt_1);
        }
    }
   
}




int bxroce_cm_test_msg_send(struct bxroce_dev *dev)
{
	int addr;    	
	int rdata;
	int wdata;
	int cm_msg_4byte_len;
	int cm_msg_flit_len;
	int remain_flit;     
	int i;
	int header_flit;
	int port_id;
	unsigned long randnumber;

	void __iomem *base_addr;
	int status = 0;
	struct bx_dev_info *devinfo = & dev->devinfo;
	u32 regval = 0;
	printk("------------CM MSG SEND START----------- \n");

	base_addr = dev->devinfo.base_addr;

	header_flit = 0;
	get_random_bytes(&randnumber,sizeof(unsigned long));
	cm_msg_4byte_len = randnumber % MAX_CM_MSG_4BYTE_LEN + 5;

	if(cm_msg_4byte_len % 4 == 0)
		cm_msg_flit_len = cm_msg_4byte_len/4;
	else
		cm_msg_flit_len = cm_msg_4byte_len/4 +1;

	rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_SRAM_STATE);

	printk("rdata is 0x%x \n",rdata);
	remain_flit = rdata & 0xffff;

	printk("cm_msg_4byte_len is %d, cm_msg_flit_len is %d, remain_flit is %d\n",cm_msg_4byte_len,cm_msg_flit_len, remain_flit);

	if (remain_flit >= cm_msg_flit_len)
	{
		rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_5);
		
		port_id = 0;

		rdata = rdma_set_bits(rdata,17,17,port_id);

		printk("In send, rdata is 0x%x \n",rdata);

		bxroce_mpb_reg_write(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_5,rdata);

		set_len(port_id, cm_msg_4byte_len);

		bxroce_mpb_reg_write(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_MSG_4BYTE_LEN, cm_msg_4byte_len);

		addr = 0;

		for (i = 0; i < 4; i++)
		{
			bxroce_mpb_reg_write(base_addr,CM_BASE,addr, 0);
			addr = addr + 1;
		}

		for (i = 0; i < cm_msg_4byte_len - 4; i++)
		{
			bxroce_mpb_reg_write(base_addr, CM_BASE, addr,((cm_msg_4byte_len & 0xffff) << 16) + (i & 0xffff));
			addr = addr + 1;
		}

		addr = CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH;
		wdata = 0;
		
		wdata = rdma_set_bits(wdata,CM_MSG_SEND_MSG_SRAM_WR_FINISH_RANGE,1);
		bxroce_mpb_reg_write(base_addr,CM_CFG,addr,wdata);

		printk("INFO: port_%0d cm msg send:\tcm_msg_4byte_len=%08X.\n",port_id,cm_msg_4byte_len);
		#if 0
		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_DSR0));
				 BXROCE_PR("SEND DMA_DSRO: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_DSR1));
		BXROCE_PR("SEND DMA_DSR1: 0x%x \n",regval);
		#endif
		

	}

	printk("------------CM MSG SEND END----------- \n");
	printk("\n");

	return 0;
}


int bxroce_cm_test_msg_recv(struct bxroce_dev *dev)
{
	char *tst;
 	int rdata;
	int wdata;
	int golden_cm_msg_4byte_len;
	int i;
	
	int addr;
	int port_id;
	int header_flit;

	void __iomem *base_addr;
	int status = 0;
	struct bx_dev_info *devinfo = & dev->devinfo;
	u32 regval = 0;
	printk("------------CM MSG RECV START----------- \n");

	base_addr = dev->devinfo.base_addr;
	header_flit = 0;

	rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CMERRINTSTA);
	printk("cm_test_msg_recv1: 0x%x \n",rdata);
//	rdata = rdata | 0x1;

	printk("cm_test_msg_recv2: 0x%x \n",rdata);
	if (rdma_get_bits(rdata, 0, 0) == 1)
	{
		BXROCE_PR("have intr\n");
		while (1)
		{
			rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CMMSGRECEIVESRAMSTATE);
			if (rdma_get_bits(rdata, 0, 0) == 1)
			{
				printk("clear intr\n");
				bxroce_mpb_reg_write(base_addr,CM_CFG,CM_REG_ADDR_ERR_INT_STA_CLR,0x3); // clear intr
				break;
			}
			else
			{
				rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_5);
				port_id = rdma_get_bits(rdata,17,17);
				printk("cm_random_test_msg_recv get_len start \n");
				 golden_cm_msg_4byte_len = get_len(port_id);

				 rdata = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_RECEIVE_MSG_4BYTE_LEN);
				 if (rdata != golden_cm_msg_4byte_len)
				 {
					 printk("SIMERR: port_%d receive_msg_4byte_len (%08X) is not equal with golden_msg_len(%08X).\n",port_id,rdata,golden_cm_msg_4byte_len);
					 status = -1;
					 break;
				 }

				 addr = 0;
				 for (i = 0; i < 4; i++)
				 {
					 rdata = bxroce_mpb_reg_read(base_addr,CM_BASE,addr);
#ifndef NO_CHECK_CM_MSG
						 if (rdata != header_flit)
						 {
								printk("SIMERR: port_%d rdata (%08X) is not equal with header_flit(%08X).\n",port_id,rdata,header_flit);
								status = -1;
								break;
						 }
					 addr = addr + 1;
#endif
				 }

				  for(i = 0; i < golden_cm_msg_4byte_len - 4; i++) 
                    {
                        rdata = bxroce_mpb_reg_read(base_addr,CM_BASE,addr);
                    #ifndef NO_CHECK_CM_MSG
                        if(rdata != (golden_cm_msg_4byte_len << 16) + (i & 0xffff))
                        {
                            printk("SIMERR: port_%d rdata (%08X) is not equal with golden_cm_rdata(%08X).\n",port_id,rdata,(golden_cm_msg_4byte_len << 16) + (i & 0xffff)); 
                            status = -1;
							break;
                        }    
                    #endif
                        addr = addr + 1;
                    }



				 wdata = 0;
				 wdata = rdma_set_bits(wdata,CM_MSG_RECEIVE_MSG_SRAM_RD_FINISH_RANGE,1);
				 
				 bxroce_mpb_reg_write(base_addr,CM_CFG,CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH,wdata);
				 
				 printk("INFO: port_%0d cm msg recv:\tcm_msg_4byte_len=%08X.\n",port_id,golden_cm_msg_4byte_len);
				#if 0
				 regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_DSR0));
				 BXROCE_PR("SEND DMA_DSRO: 0x%x \n",regval);

				 regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_DSR1));
				 BXROCE_PR("SEND DMA_DSR1: 0x%x \n",regval);
				#endif
			}

		}
	}
	else
		printk("cm_random_test_msg_recv with get_bit(rdata,0)=%d \n",rdma_get_bits(rdata,0,0));

	printk("------------CM MSG RECV END----------- \n");
	printk("\n");
	return status;
}



static int bxroce_cm_test(struct bxroce_dev *dev)
{
	struct bx_dev_info *devinfo = &dev->devinfo;

	void __iomem *base_addr;
	unsigned long randnumber;
	unsigned long testnumber; 
	int status = 0;
	u32 regval = 0;
	struct rnic_pdata *rnic_pdata = dev->devinfo.rnic_pdata;

	rptr_0      = 0;
    rptr_1      = 0;
    wptr_0      = 0;
    wptr_1      = 0;

    send_cnt_0  = 0;
    recv_cnt_0  = 0;
    send_cnt_1  = 0;
    recv_cnt_1  = 0;
    send_len    = 0;

	base_addr = dev->devinfo.base_addr;

	printk("cm init config\n");
	regval = bxroce_mpb_reg_read(base_addr,CM_CFG,0x0);
	printk("cmcfg: offset 0x0: 0x%x \n",regval);//
	regval = bxroce_mpb_reg_read(base_addr,CM_CFG,0x1);
	printk("cmcfg: offset 0x1: 0x%x \n",regval);//
	regval = bxroce_mpb_reg_read(base_addr,CM_CFG,0x2);
	printk("cmcfg: offset 0x2: 0x%x \n",regval);//


	
	testnumber = 25;
	printk("------------------CM_RANDOME_TEST START--------------- \n");
	while (testnumber-- )
	{
		get_random_bytes(&randnumber,sizeof(unsigned long));
		printk("randnumber is %x \n",randnumber);
		switch (randnumber % 3) {
		case 0 : status = bxroce_cm_test_msg_recv(dev);
				 if(status == -1)
					 return status;

		default: status = bxroce_cm_test_msg_send(dev);
			if(status == -1)
					 return status;
		}
		msleep(1000);
	}
	//make sure recv is all over.
	status = bxroce_cm_test_msg_recv(dev);
	if(status == -1)
		return status;
	printk("------------------CM_RANDOME_TEST END--------------- \n");


	//clear the msg sram and clear the flit

	printk("--------------------DMA_CH_CA   printing info start --------------------------\n");
		regval = bxroce_mpb_reg_read(base_addr,CM_CFG,CM_REG_ADDR_MSG_SEND_SRAM_STATE);
		BXROCE_PR("CM_REG_ADDR_MSG_SEND_SRAM_STATE: 0x%x \n",regval);

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
	
		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_DSR0));
		BXROCE_PR("DMA_DSRO: 0x%x \n",regval);

		regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_DSR1));
		BXROCE_PR("DMA_DSR1: 0x%x \n",regval);

		printk("--------------------DMA_CH_CA  printing info end --------------------------\n");


		mac_print_all_regs(rnic_pdata,0);

	return status;
}

//end of cm test


/*
 *rdma link_layer
 */
static enum rdma_link_layer bxroce_get_link_layer(struct ib_device *device,
                                              u8 port_num)
{
        BXROCE_PR("bxroce:bxroce_link_layer start!\n");//added by hs for printing stat info
        BXROCE_PR("bxroce:bxroce_link_layer end!\n");//added by hs for printing end info
        return IB_LINK_LAYER_ETHERNET;
}

/*
 * bxroce_port_immutable
 */
static int bxroce_port_immutable(struct ib_device *ibdev, u8 port_num, 
			struct ib_port_immutable *immutable)
{
	BXROCE_PR("bxroce:bxroce_port_immutable start\n");//added by hs for start info
	/*wait to add*/
	struct ib_port_attr attr;
	struct bxroce_dev *dev;
	int err;

	dev = get_bxroce_dev(ibdev);
	immutable->core_cap_flags = RDMA_CORE_PORT_IBA_ROCE;
	/*support udp encap ?*/
	immutable->core_cap_flags |= RDMA_CORE_CAP_PROT_ROCE_UDP_ENCAP;
	err = ib_query_port(ibdev,port_num, &attr);
	if(err)
			return err;
	/*not sure , need verified*/
	immutable->pkey_tbl_len = attr.pkey_tbl_len;
	immutable->gid_tbl_len = attr.gid_tbl_len;
	immutable->max_mad_size = IB_MGMT_MAD_SIZE;

	/*wait to add*/
	BXROCE_PR("bxroce:bxroce_port_immutable end\n");//added by hs for end info
	return 0;
}

/*dev_attr_hw_rev 's show function*/
static ssize_t hw_rev_show(struct device *device, struct device_attribute *attr, char *buf)
{
	BXROCE_PR("bxroce: hw_rev_show start\n");//added by hs
	struct bxroce_dev *dev;
	struct mac_pdata *pdata;
	dev =  container_of(device, struct bxroce_dev, ibdev.dev);
	pdata = dev->devinfo.pdata;
	if(pdata)
	BXROCE_PR("bxroce: hw_rev is 0x\n");//added by hs
	BXROCE_PR("bxroce: hw_rev_show end\n");//added by hs	
	return scnprintf(buf,PAGE_SIZE,"0x%x\n",dev->devinfo.pcidev->vendor);
	return 0;
}
static DEVICE_ATTR_RO(hw_rev);

/*dev_attr_hca_type 's show function*/
static ssize_t hca_type_show(struct device *device, struct device_attribute *attr, char *buf)
{
	BXROCE_PR("bxroce: hca_type_show start \n");//added by hs
	struct  bxroce_dev *dev; 
	struct 	mac_pdata *pdata;
	dev = container_of(device, struct bxroce_dev, ibdev.dev);
 	pdata = dev->devinfo.pdata;
	if(pdata)
	BXROCE_PR("bxroce: hca_type is BX\n");//added by hs
	BXROCE_PR("bxroce: hca_type_show end \n");//added by hs
	return sprintf(buf,"BX%d\n",dev->devinfo.pcidev->device);
	return 0;
}
static DEVICE_ATTR_RO(hca_type);

/*attribute file*/
static struct attribute *bxroce_attributes[] = {
	&dev_attr_hw_rev.attr,
	&dev_attr_hca_type.attr,
	NULL

};

/*attribute group*/
static const struct attribute_group bxroce_attr_group = {
	.attrs = bxroce_attributes,
};

/*ib_device_ops definition for roce*/
const struct ib_device_ops bxroce_dev_ops = {
	.query_device = bxroce_query_device,
	.query_port = bxroce_query_port,
	.modify_port = bxroce_modify_port,
	//.query_gid = bxroce_query_gid, //del by hs@20200429
	.get_netdev = bxroce_get_netdev,
	//.add_gid = bxroce_add_gid, //del by hs@20200429
	//.del_gid = bxroce_del_gid, //del by hs@20200429
	.get_link_layer = bxroce_get_link_layer,
	.alloc_pd = bxroce_alloc_pd,
	.dealloc_pd = bxroce_dealloc_pd,
	
	.create_cq = bxroce_create_cq,
	.destroy_cq = bxroce_destroy_cq,
	.resize_cq = bxroce_resize_cq,

	.create_qp = bxroce_create_qp,
	.modify_qp = bxroce_modify_qp,
	.query_qp = bxroce_query_qp,
	.destroy_qp = bxroce_destroy_qp,

	.query_pkey = bxroce_query_pkey,
	.create_ah = bxroce_create_ah,
	.destroy_ah = bxroce_destroy_ah,
	.query_ah = bxroce_query_ah,
	//.modify_ah = bxroce_modify_ah, //del by hs@20200429

	.poll_cq = bxroce_poll_cq,
	.post_send = bxroce_post_send,
	.post_recv = bxroce_post_recv,
	.req_notify_cq = bxroce_arm_cq,

	.get_dma_mr = bxroce_get_dma_mr,
	.dereg_mr = bxroce_dereg_mr,
	.reg_user_mr = bxroce_reg_user_mr,

	.alloc_mr = bxroce_alloc_mr,
	.map_mr_sg = bxroce_map_mr_sg,

	.alloc_ucontext = bxroce_alloc_ucontext,
	.dealloc_ucontext = bxroce_dealloc_ucontext,
	.mmap = bxroce_mmap,

	.process_mad = bxroce_process_mad,
	.get_port_immutable = bxroce_port_immutable,
};

/*
 *bxroce_register_ibdev.To register the ibdev to kernel.must exec it before unregister_ibdev.
 *@struct bxroce_dev  *dev. the struct describe the ibdev attribute.
 */
static int bxroce_register_ibdev(struct bxroce_dev *dev)
{
	BXROCE_PR("bxroce:bxroce_register_ibdev start \n");//added by hs for info
#if HSDEBUG //added by hs for debug
	//strlcpy(dev->ibdev.name, "bxroce%d", IB_DEVICE_NAME_MAX);
//	BXROCE_PR("bxroce:ibdev.name = %s",dev->ibdev.name);//added by hs for name info
	bxroce_get_guid(dev,(u8 *)&dev->ibdev.node_guid);
	BXROCE_PR("bxroce: node_guid is %0lx\n",dev->ibdev.node_guid);//added by hs
	dev->ibdev.owner = THIS_MODULE;	
	dev->ibdev.uverbs_abi_ver = 2;//temple value.added by hs
	//added later.now have no this information.
	dev->ibdev.node_type = RDMA_NODE_IB_CA;
	dev->ibdev.phys_port_cnt = 1;//not finished ,add later! hs 2019/6/22
	dev->ibdev.num_comp_vectors = 3;
	dev->ibdev.uverbs_cmd_mask = 
		(1ull << IB_USER_VERBS_CMD_GET_CONTEXT)			|
		(1ull << IB_USER_VERBS_CMD_QUERY_DEVICE)		|
		(1ull << IB_USER_VERBS_CMD_QUERY_PORT)			|
		(1ull << IB_USER_VERBS_CMD_ALLOC_PD)			|
		(1ull << IB_USER_VERBS_CMD_DEALLOC_PD)			|
		(1ull << IB_USER_VERBS_CMD_CREATE_AH)			|
		(1ull << IB_USER_VERBS_CMD_DESTROY_AH)			|
		(1ull << IB_USER_VERBS_CMD_MODIFY_AH)			|
		(1ull << IB_USER_VERBS_CMD_QUERY_AH)			|
		(1ull << IB_USER_VERBS_CMD_REG_MR)				|
		(1ull << IB_USER_VERBS_CMD_REREG_MR)			|
		(1ull << IB_USER_VERBS_CMD_DEREG_MR)			|
		(1ull << IB_USER_VERBS_CMD_CREATE_COMP_CHANNEL) |
		(1ull << IB_USER_VERBS_CMD_CREATE_CQ)			|
		(1ull << IB_USER_VERBS_CMD_RESIZE_CQ)			|
		(1ull << IB_USER_VERBS_CMD_DESTROY_CQ)			|
		(1ull << IB_USER_VERBS_CMD_REQ_NOTIFY_CQ)		|
		(1ull << IB_USER_VERBS_CMD_POLL_CQ)				|
		(1ull << IB_USER_VERBS_CMD_CREATE_QP)			|
		(1ull << IB_USER_VERBS_CMD_MODIFY_QP)			|
		(1ull << IB_USER_VERBS_CMD_QUERY_QP)			|
		(1ull << IB_USER_VERBS_CMD_DESTROY_QP)			|
		(1ull << IB_USER_VERBS_CMD_POST_SEND)			|
		(1ull << IB_USER_VERBS_CMD_POST_RECV);
	//	(1ull << IB_USER_VERBS_CMD_ATTACH_MCAST)		|
	//	(1ull << IB_USER_VERBS_CMD_DETACH_MCAST)		|
	//	(1ull << IB_USER_VERBS_CMD_CREATE_SRQ)			|
	//	(1ull << IB_USER_VERBS_CMD_MODIFY_SRQ)			|
	//	(1ull << IB_USER_VERBS_CMD_QUERY_SRQ)			|
	//	(1ull << IB_USER_VERBS_CMD_DESTROY_SRQ)			|
	//	(1ull << IB_USER_VERBS_CMD_CREATE_XSRQ)			|
	//	(1ull << IB_USER_VERBS_CMD_OPEN_QP);

	/*mandatory verbs. */
	dev->ibdev.dev.parent =&dev->devinfo.pcidev->dev;
	ib_set_device_ops(&dev->ibdev, &bxroce_dev_ops);
	/*create device attr file*/
	rdma_set_device_sysfs_group(&dev->ibdev,&bxroce_attr_group);
	/*end*/
	dev->ibdev.driver_id = RDMA_DRIVER_UNKNOWN;
	dev->Is_qp1_allocated = false;
#endif
	BXROCE_PR("bxroce:bxroce_register_ibdev succeed end\n");//added by hs for info
	return ib_register_device(&dev->ibdev,"bxroce%d", NULL);//wait a moment
//	return 0;
}
static int bxroce_init_pools(struct bxroce_dev *dev)
{
	BXROCE_PR("bxroce: bxroce_init_pools start\n");//added by hs 
	int err;
	err = bxroce_pool_init(dev,&dev->pd_pool,BXROCE_TYPE_PD,
							dev->attr.max_pd);
	
	if (err)
		goto err1;
	err = bxroce_pool_init(dev,&dev->mr_pool,BXROCE_TYPE_MR,
							dev->attr.max_mr);
	if (err)
		goto err2;
	BXROCE_PR("bxroce: bxroce_init_pools end\n");//added by hs 
	return 0;
err2:
	bxroce_pool_cleanup(&dev->mr_pool);
err1:
	bxroce_pool_cleanup(&dev->pd_pool);
	return err;
}

static int bxroce_init_cqqp(struct bxroce_dev *dev)//allocate id for cq &qp
{
	BXROCE_PR("bxroce: bxroce_init_cqqp start \n");//added by hs 
	u32 max_qp;
	u32 max_cq;
	u32 resources_size;
	void *resource_ptr;
	
	max_qp = dev->attr.max_qp;
	max_cq = dev->attr.max_cq;

	resources_size = sizeof(unsigned long) * BITS_TO_LONGS(max_qp);
	resources_size += sizeof(unsigned long) * BITS_TO_LONGS(max_cq);
	resources_size += sizeof(struct bxroce_qp **) * max_qp;
	resources_size += sizeof(struct bxroce_cq **) * max_cq;

	dev->mem_resources = kzalloc(resources_size,GFP_KERNEL);

	if(!dev->mem_resources)
		return -ENOMEM;
	resource_ptr = dev->mem_resources;

	dev->allocated_qps = resource_ptr;
	dev->allocated_cqs = &dev->allocated_qps[BITS_TO_LONGS(max_qp)];
	dev->qp_table =(struct bxroce_qp **)(&dev->allocated_cqs[BITS_TO_LONGS(max_cq)]);
	dev->cq_table =(struct bxroce_cq **)(&dev->allocated_cqs[BITS_TO_LONGS(max_cq)+sizeof(struct bxroce_qp **) * max_qp]);

	set_bit(0,dev->allocated_qps); // qp0 is not used in Roce
	set_bit(0,dev->allocated_cqs);
	
	set_bit(1,dev->allocated_qps); // qp1 is for GSI/ CM.
	set_bit(1,dev->allocated_cqs);
	set_bit(2,dev->allocated_cqs);

	spin_lock_init(&dev->resource_lock);
	spin_lock_init(&dev->qptable_lock);
	BXROCE_PR("bxroce: bxroce_init_cqqp end \n");//added by hs 
	return 0;
}

static int bxroce_get_used_rsrc(struct bxroce_dev *dev)
{
	dev->used_qps = find_next_zero_bit(dev->allocated_qps,dev->attr.max_qp,0);
	dev->used_cqs = find_next_zero_bit(dev->allocated_cqs,dev->attr.max_cq,0);

	return 0;

}

static int bxroce_create_ah_tbl(struct bxroce_dev *dev)
{
	int i;
	int status = -ENOMEM;
	int max_ah;
	struct pci_dev *pdev = dev->devinfo.pcidev;
	dma_addr_t pa;
	struct bxroce_pbe *pbes;
	
	max_ah = 256; // not sure
	dev->av_tbl.size = sizeof(struct bxroce_av) * max_ah;

	dev->av_tbl.pbl.va = dma_alloc_coherent(&pdev->dev,PAGE_SIZE,
											&dev->av_tbl.pbl.pa,
											GFP_KERNEL);
	if(dev->av_tbl.pbl.va == NULL)
		goto mem_err;

	dev->av_tbl.va = dma_alloc_coherent(&pdev->dev,dev->av_tbl.size,
										&pa,GFP_KERNEL);
	if(dev->av_tbl.va == NULL)
		goto mem_err_ah;
	dev->av_tbl.pa = pa;
	dev->av_tbl.num_ah = max_ah;
	memset(dev->av_tbl.va,0,dev->av_tbl.size);

	pbes = (struct bxroce_pbe *)dev->av_tbl.pbl.va;
	for (i = 0; i < dev->av_tbl.size / 4096; i++) {
		pbes[i].pa_lo = (u32)cpu_to_le32(pa & 0xffffffff);
		pbes[i].pa_hi = (u32)cpu_to_le32(upper_32_bits(pa));
		pa += PAGE_SIZE;
	}

	dev->av_tbl.ahid = (u32)(dev->av_tbl.pbl.pa & 0xffffffff); // need an unique id , alter this later. --added by hs.
	
	spin_lock_init(&dev->av_tbl.lock);
	return 0;
mem_err_ah:
	dma_free_coherent(&pdev->dev,PAGE_SIZE,dev->av_tbl.pbl.va,
					  dev->av_tbl.pbl.pa);
	dev->av_tbl.pbl.va = NULL;
	dev->av_tbl.size= 0;
mem_err:
	return status;

}

static int bxroce_alloc_resource(struct bxroce_dev *dev)
{
	BXROCE_PR("bxroce: bxroce_alloc_resource start\n");//added by hs
	mutex_init(&dev->dev_lock);
	int status;
	status = bxroce_init_pools(dev);
	if(status)
		goto err1;
	status = bxroce_init_cqqp(dev);
	if(status)
		goto err1;
	status = bxroce_get_used_rsrc(dev);
	if(status)
		goto err1;
	status = bxroce_create_ah_tbl(dev);
	if(status)
		goto err1;
	return 0;
err1:
	BXROCE_PR("init pool failed\n");//added by hs
	return status;
}

static int bxroce_alloc_hw_resources(struct bxroce_dev *dev)
{
	BXROCE_PR("bxroce:-----------------------test start------------------------\n");//added by hs 
	BXROCE_PR("bxroce:test ib_alloc_pd:\n");//added by hs 
	struct ib_cq_init_attr cq_attr = {.cqe = 1};
	struct ib_pd *ibpd = ib_alloc_pd(&dev->ibdev,NULL);
	struct ib_port_attr attr;
	if(!ibpd)
		goto err_pd;
	BXROCE_PR("bxroce:ib_alloc_pd succeed!\n");//added by hs
	struct ib_cq *ibcq = ib_create_cq(&dev->ibdev,NULL,NULL,NULL,&cq_attr);
	if(!ibcq)
		goto err_cq;
	BXROCE_PR("bxroce: ib_alloc_pd succeed! \n");//added by hs
	BXROCE_PR("bxroce:------------------------test end --------------------\n");//added by hs 

	ib_query_port(&dev->ibdev, 1, &attr);	
	ib_dealloc_pd(ibpd);
	ib_destroy_cq(ibcq);
	return 0;
err_cq:
	printk("create_cq");//added by hs 
err_pd:
	printk("alloc_pd failed\n");//added by hs 
	return ERR_PTR(-ENOMEM);	
}


static struct bxroce_dev *bx_add(struct bx_dev_info *dev_info)
{
	BXROCE_PR("bxroce:bx_add start\n");//added by hs for printing info
	int status = 0;
	u8 lstate = 0;
	struct bxroce_dev *dev;

	dev = (struct bxroce_dev *)ib_alloc_device(sizeof(struct bxroce_dev));
	if(!dev) {
		printk("bxroce:Unable to allocate ib device\n");//to show the err information.
		return NULL;
	}	

	memcpy(&dev->devinfo, dev_info, sizeof(*dev_info));
	BXROCE_PR("bxroce:get the mac address is:%x,base addr is %x\n", dev->devinfo.mac_base,dev_info->base_addr);
	BXROCE_PR("bxroce: get vendor is 0x%x,pci device is BX%d\n",dev->devinfo.pcidev->vendor,dev->devinfo.pcidev->device);//added by hs
	
	/*get io addr*/
	int i =0;
	for (i = 0; i <= PCI_STD_RESOURCE_END; i++) {
		if(pci_resource_len(dev->devinfo.pcidev,i) == 0)
			continue;
		dev->ioaddr = pci_resource_start(dev->devinfo.pcidev,i);
		if(!dev->ioaddr)
			printk("bxorce: cannot get ioaddr\n");
		break;
	}
	BXROCE_PR("bxroce: get ioaddr:0x%lx \n",dev->ioaddr);
	/*get io addr end*/

	mutex_init(&dev->pd_mutex);
	dev->id = idr_alloc(&bx_dev_id, NULL, 0, 0, GFP_KERNEL);
	if(dev->id < 0)
		goto idr_err;

	status = bxroce_init_hw(dev);// init hw
	if (status)
		goto err_inithw;
	status = bxroce_get_hwinfo(dev);//read hw
	if (status)
		goto err_getinfo;
	status = bxroce_alloc_resource(dev);//alloc some resources
	if (status)
		goto err_alloc;
	status = bxroce_register_ibdev(dev);//register ib_device
	if (status)
		goto alloc_err;

	#if 0 //added by hs
	status = bxroce_cm_test(dev);
	if(status)
		goto err_cm_test;
	#endif

	BXROCE_PR("bxroce:bx_add succeed end\n");//added by hs for printing info

//	/*test ibdev*/
//	status = bxroce_alloc_hw_resources(dev);
//	if (status)
//		goto alloc_hwres;

	return dev;//turn back the ib dev
err_cm_test:
	BXROCE_PR("err cm test \n");
alloc_hwres:
	printk("alloc hw res failed\n");//added by hs for info
err_alloc:
	printk("alloc failed\n");//added by hs for info
alloc_err:
	//ib_dealloc_device(&dev->ibdev);
	printk("bxroce:error!alloc_err as bxroce_register_ibdev failed\n");//added by hs for info
err_getinfo:
	printk("read hw failed\n");//added by hs;
err_inithw:
	printk("init hw failed\n");//added by hs for info
idr_err:
	BXROCE_PR("err idr\n");
	idr_remove(&bx_dev_id, dev->id);
	ib_dealloc_device(&dev->ibdev);

	return NULL;
}

static void bx_remove(struct bxroce_dev *dev)
{
	BXROCE_PR("bxrove:bx_remove start\n");//added by hs for printing bx_remove info
	u32 regval = 0;
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	struct bx_dev_info *devinfo = &dev->devinfo;
	struct pci_dev *pdev = dev->devinfo.pcidev;	
	
	ib_unregister_device(&dev->ibdev);
	/*disable some hw function*/
#if 0
	//disable pgu
	regval = bxroce_mpb_reg_read(base_addr,PGU_BASE,CFGRNR);
	regval = rdma_set_bits(regval,0,1,0);
	BXROCE_PR("disable pgu: 0x%x",regval);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,CFGRNR,regval);

	//disable phd
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHD_REG_ADDR_PHD_START,0);

	//wait for tx to stop --add later

	//disable tx queue
	regval = readl(MAC_RDMA_MTL_REG(devinfo,RDMA_CHANNEL,MTL_Q_TQOMR));
	regval = MAC_SET_REG_BITS(regval,MTL_Q_TQOMR_TXQEN_POS,MTL_Q_TQOMR_TXQEN_LEN,0);
	BXROCE_PR("disable rxqueue: 0x%x \n",regval);
	writel(regval,MAC_RDMA_MTL_REG(devinfo,RDMA_CHANNEL,MTL_Q_TQOMR));

	//disable tx dma channel
	regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_TCR));
	regval = MAC_SET_REG_BITS(regval,DMA_CH_TCR_ST_POS,DMA_CH_TCR_ST_LEN,0);
	BXROCE_PR("disable txdma channel: 0x%x \n",regval);
	writel(regval,MAC_RDMA_DMA_REG(devinfo,DMA_CH_TCR));

	//wait for rx to stop --add later

	//disable rx DMA CHANNEL
	regval = readl(MAC_RDMA_DMA_REG(devinfo,DMA_CH_RCR));
	regval = MAC_SET_REG_BITS(regval,DMA_CH_RCR_SR_POS,DMA_CH_RCR_SR_LEN,0);
	BXROCE_PR("disable rxdma channel: 0x%x \n",regval);
	writel(regval,MAC_RDMA_DMA_REG(devinfo,DMA_CH_RCR));
#endif

	//free some resources allocated by kernel
	dma_free_coherent(&pdev->dev,PAGE_SIZE,dev->av_tbl.pbl.va,
					  dev->av_tbl.pbl.pa);
	dev->av_tbl.pbl.va = NULL;
	dev->av_tbl.size= 0;
	if(dev->mem_resources)
	kfree(dev->mem_resources);
	bxroce_pool_cleanup(&dev->pd_pool);
	bxroce_pool_cleanup(&dev->mr_pool);


	//free idr && dev
	idr_remove(&bx_dev_id, dev->id);
	ib_dealloc_device(&dev->ibdev);

	BXROCE_PR("bxroce:bx_remove succeed end \n");//added by hs for printing bx_remove info
}


void bxroce_add_addr(struct in_ifaddr *ifa,struct mac_pdata *pdata)
{
	struct in_device *in_dev = ifa->ifa_dev;
	struct net_device *dev = in_dev->dev;
	void __iomem *base_addr;
	base_addr = pdata->rnic_pdata.pcie_bar_addr;	

	__be32 mask = ifa->ifa_mask;
	__be32 addr = ifa->ifa_local;
	__be32 prefix = ifa->ifa_address & mask;

	u32 cpumask = __be32_to_cpu(mask);
	u32 cpuaddr = __be32_to_cpu(addr);
	u32 cpuprefix  = __be32_to_cpu(prefix);
	
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDIPV4SOURCEADDR,cpuaddr);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDIPV4SOURCEADDR,cpuaddr);

	u32 data;
	data = bxroce_mpb_reg_read(base_addr,PHD_BASE_0,PHDIPV4SOURCEADDR);
	BXROCE_PR("PHD0IPV4SOURCEADDR:0x%x\n",data);
	data = bxroce_mpb_reg_read(base_addr,PHD_BASE_1,PHDIPV4SOURCEADDR);
	BXROCE_PR("PHD1IPV4SOURCEADDR:0x%x\n",data);
	BXROCE_PR("notifier netdevopen:cpumask:0x%x,cpuaddr:0x%x,prefix:0x%x\n",cpumask,cpuaddr,cpuprefix);
}

void bxroce_del_addr(struct in_ifaddr *ifa,struct mac_pdata *pdata)
{
	struct in_device *in_dev = ifa->ifa_dev;
	struct net_device *dev = in_dev->dev;
	__be32 mask = ifa->ifa_mask;
	__be32 addr = ifa->ifa_local;
	__be32 prefix = ifa->ifa_address & mask;

	u32 cpumask = __be32_to_cpu(mask);
	u32 cpuaddr = __be32_to_cpu(addr);
	u32 cpuprefix  = __be32_to_cpu(prefix);
	BXROCE_PR("notifier netdevclose:cpumask:0x%x,cpuaddr:0x%x,prefix:0x%x\n",cpumask,cpuaddr,cpuprefix);
}

static int bxroce_inetaddr_event(struct notifier_block *this, unsigned long event, void *ptr) {
	struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;
	struct net_device *dev = ifa->ifa_dev->dev;
	struct mac_pdata *pdata = netdev_priv(dev);
	
	switch (event) {
	case NETDEV_UP:
			bxroce_add_addr(ifa,pdata);
			break;
	case NETDEV_DOWN:
			bxroce_del_addr(ifa,pdata);
			break;
	}
	return NOTIFY_DONE;
}

/*Add notifier to IPV4*/
static struct notifier_block bxroce_inetaddr_notifier = {
	.notifier_call = bxroce_inetaddr_event,
};


struct bxroce_driver bx_drv = {
	.name 		="bxroce_driver",
	.add  		=bx_add,
	.remove 	=bx_remove,
	//not finished , added later
};

static int __init bx_init_module(void)
{
	int status;
	BXROCE_PR("bxroce:init module start!\n");//added by hs for printing init info
	status = bxroce_cache_init();
	if(status){
		printk("unable to init object pools\n");//added by hs 
		return status;
	}
	//register notifier
	register_inetaddr_notifier(&bxroce_inetaddr_notifier);

	status = bx_roce_register_driver(&bx_drv);	
	if(status)
		goto err_reg;
	BXROCE_PR("bxroce:init module exit succeed!\n");//added by hs for printing init info
	return 0;
	
err_reg:
	return status;
}

static void __exit bx_exit_module(void)
{
	BXROCE_PR("bxroce:exit module start\n");//added by hs for printing info
	bx_roce_unregister_driver(&bx_drv);
	
	unregister_inetaddr_notifier(&bxroce_inetaddr_notifier);

	bxroce_cache_exit();
	BXROCE_PR("bxroce:exit module succeed!\n");//added by hs for print exit info
}

module_init(bx_init_module);
module_exit(bx_exit_module);
