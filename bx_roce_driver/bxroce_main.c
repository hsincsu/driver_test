/*
 *
 * This is the main file with the main funciton.All start here!
 *
 *
 *
 * 						------edited by hs in 2019/6/18
 *
 */

#include<linux/module.h>
#include <linux/idr.h>

#include <rdma/rdma_netlink.h>
#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_mad.h>

#include <linux/netdevice.h>
#include <net/addrconf.h>

#include "header/bxroce.h"
#include "header/bxroce_verbs.h"
#include "header/bxroce_ah.h"
#include "header/bxroce_hw.h"
#include "header/bxroce_abi.h"
//#include <rdma/ocrdma-abi.h>
#define HSDEBUG 1

MODULE_VERSION(BXROCEDRV_VER);
MODULE_AUTHOR("HS");
MODULE_LICENSE("Dual BSD/GPL");

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
	.query_gid = bxroce_query_gid,
	.get_netdev = bxroce_get_netdev,
	.add_gid = bxroce_add_gid,
	.del_gid = bxroce_del_gid,
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
	.modify_ah = bxroce_modify_ah,

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
		(1ull << IB_USER_VERBS_CMD_REG_MR)				|
		(1ull << IB_USER_VERBS_CMD_REREG_MR)			|
		(1ull << IB_USER_VERBS_CMD_DEREG_MR)			|
		(1ull << IB_USER_VERBS_CMD_CREATE_COMP_CHANNEL) |
		(1ull << IB_USER_VERBS_CMD_CREATE_CQ)			|
		(1ull << IB_USER_VERBS_CMD_RESIZE_CQ)			|
		(1ull << IB_USER_VERBS_CMD_DESTROY_CQ)			|
		(1ull << IB_USER_VERBS_CMD_CREATE_QP)			|
		(1ull << IB_USER_VERBS_CMD_MODIFY_QP)			|
		(1ull << IB_USER_VERBS_CMD_QUERY_QP)			|
		(1ull << IB_USER_VERBS_CMD_DESTROY_QP)			|
		(1ull << IB_USER_VERBS_CMD_ATTACH_MCAST)		|
		(1ull << IB_USER_VERBS_CMD_DETACH_MCAST)		|
		(1ull << IB_USER_VERBS_CMD_CREATE_SRQ)			|
		(1ull << IB_USER_VERBS_CMD_MODIFY_SRQ)			|
		(1ull << IB_USER_VERBS_CMD_QUERY_SRQ)			|
		(1ull << IB_USER_VERBS_CMD_DESTROY_SRQ)			|
		(1ull << IB_USER_VERBS_CMD_CREATE_XSRQ)			|
		(1ull << IB_USER_VERBS_CMD_OPEN_QP);

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
	dev->used_cqs = find_next_zero_bit(dev->allocated_cps,dev->attr.max_cq,0);

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

#if HSDEBUG //added by hs for debug
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

#endif
	BXROCE_PR("bxroce:bx_add succeed end\n");//added by hs for printing info

//	/*test ibdev*/
//	status = bxroce_alloc_hw_resources(dev);
//	if (status)
//		goto alloc_hwres;

	return dev;//turn back the ib dev
alloc_hwres:
	printk("alloc hw res failed\n");//added by hs for info
err_alloc:
	printk("alloc failed\n");//added by hs for info
alloc_err:
	ib_dealloc_device(&dev->ibdev);
	printk("bxroce:error!alloc_err as bxroce_register_ibdev failed\n");//added by hs for info
err_getinfo:
	printk("read hw failed\n");//added by hs;
err_inithw:
	printk("init hw failed\n");//added by hs for info
	return NULL;
}

static void bx_remove(struct bxroce_dev *dev)
{
	BXROCE_PR("bxrove:bx_remove start\n");//added by hs for printing bx_remove info
	ib_unregister_device(&dev->ibdev);
	ib_dealloc_device(&dev->ibdev);
	BXROCE_PR("bxroce:bx_remove succeed end \n");//added by hs for printing bx_remove info
}

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
	bxroce_cache_exit();
	BXROCE_PR("bxroce:exit module succeed!\n");//added by hs for print exit info
}

module_init(bx_init_module);
module_exit(bx_exit_module);
