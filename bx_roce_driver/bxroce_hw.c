/*
*	this file to define the detail operation to hardware
*									--edited by hs
*/

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/log2.h>
#include <linux/dma-mapping.h>

#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_cache.h>

#include "header/bxroce.h"
//#include "header/bxroce_hw.h"
//#include "header/bxroce_ah.h"
//#include "header/bxroce_verbs.h"

static int phd_start(struct bxroce_dev *dev)
{
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDSTART,0x1);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDSTART,0x1);
	return 0;
}

static int phd_udp_init(struct bxroce_dev *dev)
{
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	BXROCE_PR("bxroce: no need to init udp\n");//added by hs
	return 0;
}


static int phd_ipv6_init(struct bxroce_dev *dev)
{
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;

	/*ipv6 init phd 0*/
	writel(PHD_BASE_0 + PHDIPV6VERSION, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_0 +	PHDIPV6CLASS, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_0 + PHDIPV6FLOWLABLE, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_0 + PHDIPV6NEXTHEADER, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_0 + PHDIPV6HOPLIMIT, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_0 + PHDIPV6SOURCEADDR_0, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_0 + PHDIPV6SOURCEADDR_1, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_0 + PHDIPV6SOURCEADDR_2, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_0 + PHDIPV6SOURCEADDR_3, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);
	/*END*/
	/*IPV6 INIT PHD 1*/
	writel(PHD_BASE_1 + PHDIPV6VERSION, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_1 + PHDIPV6CLASS, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_1 + PHDIPV6FLOWLABLE, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_1 + PHDIPV6NEXTHEADER, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_1 + PHDIPV6HOPLIMIT, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_1 + PHDIPV6SOURCEADDR_0, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_1 + PHDIPV6SOURCEADDR_1, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_1 + PHDIPV6SOURCEADDR_2, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);

	writel(PHD_BASE_1 + PHDIPV6SOURCEADDR_3, base_addr + MPB_WRITE_ADDR);
	writel(0x0, base_addr + MPB_RW_DATA);
	/*END*/
}

static int phd_ipv4_init(struct bxroce_dev *dev)
{
	//修改：
	//增加判断接口是否有IP地址，用安全函数访问IP地址。
	//设计一个Notifier机制来随时准备更新ip地址。
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	struct net_device *netdev;
	netdev = dev->devinfo.netdev;
	__be32 addr;
	BXROCE_PR("bxroce:%s next is get ip \n",__func__);//added by hs
	addr = netdev->ip_ptr->ifa_list->ifa_address;	
	u32 addr_k;
	BXROCE_PR("bxroce:%s next is __be32_to_cpu(addr)",__func__);//added by hs
	addr_k =__be32_to_cpu(addr);
	
	BXROCE_PR("ipv4: %x",addr_k);//added by hs for info

	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDIPV4SOURCEADDR,addr_k);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDIPV4SOURCEADDR,addr_k);

	return 0;
}

static int phd_mac_init(struct bxroce_dev *dev)
{
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	u8 *addr;
	addr = dev->devinfo.mac_addr;	

	BXROCE_PR("mac addr is %x\n",addr[5]);//added by hs for info

	unsigned int macaddr_l =0;
	unsigned int  macaddr_h = 0;
	macaddr_h = (addr[5]<<8)|(addr[4]<<0);
	macaddr_l = (addr[3]<<24)|(addr[2]<<16)|(addr[1]<<8)|(addr[0]<<0);

	BXROCE_PR("bxroce:macaddr_h:0x%x, macaddr_l:0x%x \n",macaddr_h,macaddr_l);//added by hs	

	/*mac source addr  */
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDMACSOURCEADDR_H,macaddr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDMACSOURCEADDR_L,macaddr_l);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDMACSOURCEADDR_H,macaddr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDMACSOURCEADDR_L,macaddr_l);

	BXROCE_PR("bxroce:%s end \n",__func__);//added by hs
	/*end*/

	return 0;
}

static int phd_rxdesc_init(struct bxroce_dev *dev)
{
	void __iomem *base_addr,*base_addr_mac;
	base_addr = dev->devinfo.base_addr;
	int channel_count = dev->devinfo.channel_count;
	//struct mac_pdata *pdata = channel->pdata;

	int i = 5;//channel_count -1;
	BXROCE_PR("channel_count:%d\n",i);
	u32 addr_h = 0;
	u32 addr_l = 0;
	
	base_addr_mac = RNIC_REG_BASE_ADDR_MAC_0 + DMA_CH_BASE + (DMA_CH_INC * i);
	addr_h = 0;
	addr_l = 0;

	addr_h = base_addr_mac + DMA_CH_RDTR_HI;
    addr_l = base_addr_mac + DMA_CH_RDTR_LO;
	
	BXROCE_PR("base_addr:%lx, base_addr_mac0:%lx \n",base_addr,base_addr_mac);
	BXROCE_PR("addr_h0:%x, addr_l0:%x \n",addr_h,addr_l);

	/*rx_desc_tail_lptr_addr start*/
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDRXDESCTAILPTR_H,addr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDRXDESCTAILPTR_L,addr_l);

	/*end*/
	base_addr_mac = RNIC_REG_BASE_ADDR_MAC_1 + DMA_CH_BASE + (DMA_CH_INC * i);
	addr_h = 0;
	addr_l = 0;

	addr_h = base_addr_mac + DMA_CH_RDTR_HI;
    addr_l = base_addr_mac + DMA_CH_RDTR_LO;
	
	BXROCE_PR("base_addr:%lx, base_addr_mac1:%lx \n",base_addr,base_addr_mac);
	BXROCE_PR("addr_h1:%x, addr_l1:%x \n",addr_h,addr_l);

	/*rx_desc_tail_lptr_addr start*/
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDRXDESCTAILPTR_H,addr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDRXDESCTAILPTR_L,addr_l);


	return 0;
}
static int phd_txdesc_init(struct bxroce_dev *dev)
{
	/*对Phd的发送描述符进行初始化*/
	void __iomem *base_addr, *base_addr_mac;
	base_addr = dev->devinfo.base_addr;
	int channel_count = dev->devinfo.channel_count;
	//struct mac_pdata *pdata = channel->pdata;
	int i =6;//channel_count -1;
	BXROCE_PR("channel_count:%d\n",i);
	u32 addr_h = 0;
	u32 addr_l = 0;
	
	base_addr_mac = RNIC_REG_BASE_ADDR_MAC_0 + DMA_CH_BASE + (DMA_CH_INC * i);
	addr_h = 0;
	addr_l = 0;
	addr_h = base_addr_mac + DMA_CH_TDTR_HI;
	addr_l = base_addr_mac + DMA_CH_TDTR_LO;

	BXROCE_PR("base_addr:%lx, base_addr_mac0:%lx \n",base_addr,base_addr_mac);
	BXROCE_PR("addr_h0:%x, addr_l0:%x \n",addr_h,addr_l);
	/*tx_desc_tail_lptr_addr start*/
	
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDTXDESCTAILPTR_H,addr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDTXDESCTAILPTR_L,addr_l);

	/*end*/
	base_addr_mac = RNIC_REG_BASE_ADDR_MAC_1 + DMA_CH_BASE + (DMA_CH_INC * i);
	addr_h = 0;
	addr_l = 0;
	addr_h = base_addr_mac + DMA_CH_TDTR_HI;
	addr_l = base_addr_mac + DMA_CH_TDTR_LO;

	BXROCE_PR("base_addr:%lx, base_addr_mac1:%lx \n",base_addr,base_addr_mac);
	BXROCE_PR("addr_h1:%x, addr_l1:%x \n",addr_h,addr_l);
	/*tx_desc_tail_lptr_addr start*/
	
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDTXDESCTAILPTR_H,addr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDTXDESCTAILPTR_L,addr_l);


	return 0;

}


static int bxroce_init_phd(struct bxroce_dev *dev)
{
	int status;
	status = phd_txdesc_init(dev);
	if (status)
		goto phdtxrxdesc_err;
	status = phd_rxdesc_init(dev);
	if (status)
		goto phdtxrxdesc_err;

	status = phd_mac_init(dev);
	if (status)
		goto mac_err;
	status = phd_ipv4_init(dev);
	if (status)
		goto iperr;
#if 0 // added by hs for debugging,now there is no need to init follow function.
	status = phd_ipv6_init(dev);
	if (status)
		goto iperr;
	status = phd_udp_init(dev);
	if (status)
		goto udperr;
#endif
	status = phd_start(dev);
	if (status)
		goto udperr;
	return 0;
udperr:
	printk("err: udperr\n");
iperr:
	printk("err: iperr\n");
mac_err:
	printk("err: macerr\n");

phdtxrxdesc_err:
	printk("err: phd txrxdescerr!\n");//added by hs for info
	return status;
}

static int bxroce_init_cm(struct bxroce_dev *dev)
{
	BXROCE_PR("cm  init!\n");//added by hs 
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;

	/*write cmcfg*/
	bxroce_mpb_reg_write(base_addr,CM_CFG,CMLOGEN,0x7);
	bxroce_mpb_reg_write(base_addr,CM_CFG,CMERREN,0x7);
	bxroce_mpb_reg_write(base_addr,CM_CFG,CMINTEN,0x7);
	return 0;
}

static int bxroce_init_dev_attr(struct bxroce_dev *dev)
{
	BXROCE_PR("bxroce: bxroce_init_dev_attr \n");//added by hs 
	int err = 0;
	err = bxroce_query_device(&dev->ibdev,&dev->attr,NULL);
	if(err)
		goto err1;
	BXROCE_PR("bxroce: bxroce_init_dev_attr end\n");//added by hs 
	return 0;
err1:
	printk("query device failed\n");//added by hs 
	return err;
}

void bxroce_init_tlb(void __iomem *base_addr)
{
	u32 busy = 0x1;
	BXROCE_PR("bxroce: bxroce_init_tlb start\n");//added by hs 

	while (busy & 0x00000001)//only need the first bit in busy.
	{
		BXROCE_PR("bxroce: busy cycle \n");//added by hs 
		busy = bxroce_mpb_reg_read(base_addr,PGU_BASE,TLBINIT);
	}
	BXROCE_PR("bxroce: bxroce_init_tlb end\n");//added by hs 
}


static int bxroce_init_pgu_wqe(struct bxroce_dev *dev)
{
	BXROCE_PR("bxroce: bxroce_init_PGU_wqe  start\n");//added by hs
	/*PGU INIT*/
	int err = 0;
	int i = 0;
	u32 count = 0;
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;

	count = 1ull << QPNUM; // count = 1024.
	BXROCE_PR("bxroce:WQE INIT, count : %d \n",count);//added by hs
	/*socket id*/
	//should be MAC Address,but there is only 32bits.
	bxroce_mpb_reg_write(base_addr,PGU_BASE,SOCKETID,0x0);
	/*TLB INIT*/
	bxroce_init_tlb(base_addr); 

	/*init each WQEQueue entry*/
	for (i = 0; i < count; i = i + 1)
	{
		bxroce_mpb_reg_write(base_addr,PGU_BASE,QPLISTREADQPN,i);
		/*wirtel qp list 0 - 5 start*/
		bxroce_mpb_reg_write(base_addr,PGU_BASE,WPFORQPLIST,0x0);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,WPFORQPLIST2,0x0);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RPFORQPLIST,0x0);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RPFORQPLIST2,0x0);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,QPNANDVALID,0x0);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,QPNANDVALID2,0x0);

		/*write qp list 0 - 5 end*/
		bxroce_mpb_reg_write(base_addr,PGU_BASE,WRITEORREADQPLIST,0x1);
	     // WRITEL 3'b111
		bxroce_mpb_reg_write(base_addr,PGU_BASE,WRITEQPLISTMASK,0x7);
		 // write qp list 4020  1
		bxroce_mpb_reg_write(base_addr,PGU_BASE,QPLISTWRITEQPN,0x1);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,WRITEORREADQPLIST,0x0);
	}
	BXROCE_PR("bxroce: bxroce_init_PGU_wqe end\n");//added by hs
	return err;
}

static int bxroce_init_pgu_cq(struct bxroce_dev *dev)
{
	BXROCE_PR("bxroce: bxroce_init_pgu_cq start \n");//added by hs 
	int err =0;
	int i =0;
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	u32 txop = 0;
	u32 rxop = 0;
	u32 xmitop = 0;
	u32 count = 0;

	count = 1ull << QPNUM;
	BXROCE_PR("bxroce:CQ INIT,count: %d \n",count);
	BXROCE_PR("init tx cq start \n");//added by hs 
	for (i = 0; i < count; i = i + 1) // init tx cq
	{
		txop = 0x0;
		txop = i<<2; // txop = {{(32-QPNUM-2){'b0}},i['QPNUM-1:0],1'b1,1'b1};
//		BXROCE_PR("txop is %x\n",txop);//added by hs 
		txop = txop + 0x3;
//		BXROCE_PR("txop is %x\n",txop);//added by hs 
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQQUEUEUP,0x2000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQQUEUEUP + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQQUEUEDOWN,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQQUEUEDOWN + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQREADPTR,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQREADPTR + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQESIZE,txop);

	//	BXROCE_PR("bxroce: tx cq end \n");//added by hs 
		while (txop & 0x00000001) //QPNUM = 10,SO 32 -10 -2 = 20
		{
	//		BXROCE_PR("bxroce: txcqcycle \n");//added by hs 
			txop = bxroce_mpb_reg_read(base_addr,PGU_BASE,CQESIZE);
		}
			
	}

	BXROCE_PR("bxroce: rx cq start \n");//added by hs 
	for (i = 0; i < count; i = i + 1) // init rx cq
	{
		rxop = 0;
		rxop = i << 2; // the same to upper one
		rxop = rxop + 0x3;
	//	BXROCE_PR("rxop is %x \n",rxop);//added by hs 
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxUpAddrCQE,0x2000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxUpAddrCQE + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxBaseAddrCQE,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxBaseAddrCQE + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxCQEWP,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxCQEWP + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxCQEOp,0x0000);

	//	BXROCE_PR("bxroce: rx cq end \n");//added by hs 
		while(rxop & 0x00000001)
		{
			//rxop = 0;
	//		BXROCE_PR("bxroce: rx cq cycle \n");//added by hs 
			rxop = bxroce_mpb_reg_read(base_addr,PGU_BASE,RxCQEOp);
		//	if(rxop)
		//		BXROCE_PR("rxop cycle is %x\n",rxop);//added by hs 
		}

	}

	BXROCE_PR("bxroce: xmit cq start \n");//added by hs 
	for (i = 0; i < count; i = i + 1)
	{
		xmitop =0;
		xmitop = i<<2; // the same to uppper one
		xmitop = xmitop + 0x3;
	//	BXROCE_PR("bxroce: xmitop is %x\n",xmitop);//added by hs 
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitBaseAddrCQE,0x2000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitUpAddrCQE + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitBaseAddrCQE,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitBaseAddrCQE + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitCQEWP,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitCQEWP + 0x4,0x0000);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitCQEOp,xmitop);

		while (xmitop & 0x00000001)
		{
		//	BXROCE_PR("bxroce: xmitop cycle \n");//added by hs 
			//xmitop = 0;
			xmitop = bxroce_mpb_reg_read(base_addr,PGU_BASE,XmitCQEOp);
	//		BXROCE_PR("bxroce: xmitop cycle  is %x\n",xmitop);//added by hs 
		}
	}

	/*init wqe retrycount and timeout*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYCOUNT,0xffffffff);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYTIMER,0xffffffff);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYTIMER + 0x4,0xffffffff);

	BXROCE_PR("bxroce: bxroce_init_pgu_cq end \n");//added by hs
	return err;
	 
}

static int bxroce_init_qp(struct bxroce_dev *dev)
{
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	/*init psn*/
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,STARTINITPSN,0x0000);
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,STARTINITPSN + 0x4,0x0000);
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,STARTINITPSN + 0x8,0x0000);
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,STARTINITPSN + 0xc,0x10000);
	
/*for some reason,need init these registers*/
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,GENRSP,0x0000);
	//bxroce_mpb_reg_write(base_addr,PGU_BASE, CFGRNR,0x0000);

}

int bxroce_init_hw(struct bxroce_dev *dev)
{
	int status;
	status = bxroce_init_phd(dev);
	if (status)
		goto errphd;
	status = bxroce_init_cm(dev);
	if (status)
		goto errcm;
	status = bxroce_init_pgu_wqe(dev);
	if (status)
		goto errcm;
	status = bxroce_init_pgu_cq(dev);
	if (status)
		goto errcm;
	status = bxroce_init_qp(dev);
	if (status)
		goto errcm;
	status = bxroce_init_dev_attr(dev);
	if(status)
		goto errcm;
	return 0;
errcm:
	printk("cm init err!\n");//added by hs
errphd:
	printk("phd init err!\n");//added by hs
	return status;
}

static int bxroce_read_phd(struct bxroce_dev *dev)
{
	u32 regval;
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	int i = 0;
	u32 phd_base_addr = 0;
	for (i = 0; i <= 1; i++) {
		phd_base_addr = i == 0 ? PHD_BASE_0: PHD_BASE_1;
		BXROCE_PR("------------------bxroce read phd %d------------------\n",i);//added by hs for info
			/*read phd order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDBYTEORDER);
		BXROCE_PR("bxroce: phd_byte_order: %x\n", regval);//added by hs for info 

			/*read tx desc tail ptr_l order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDTXDESCTAILPTR_L);
		BXROCE_PR("bxroce: phd_tx_desc_tail_ptr_l: %x\n", regval);//added by hs for info 

			/*read tx desc tail ptr_h order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDTXDESCTAILPTR_H);
		BXROCE_PR("bxroce: phd_tx_desc_tail_ptr_h: %x\n", regval);//added by hs for info 

			/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDTXDESCTAILPTR_THRESDHOLD);
		BXROCE_PR("bxroce: phd_tx_desc_tail_ptr_thresdhold: %x\n", regval);//added by hs for info 

				/*read phd rx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDRXDESCTAILPTR_THRESDHOLD);
		BXROCE_PR("bxroce: phd_rx_desc_tail_ptr_thresdhold: %x\n", regval);//added by hs for info 

				/*read phd tx desc tail ptr incr step order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDRXDESCTAILPTR_INCR_STEP);
		BXROCE_PR("bxroce: phd_tx_desc_tail_ptr_incr_step: %x\n", regval);//added by hs for info 

				/*read phd rx desc tail ptr_l order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDRXDESCTAILPTR_L);
		BXROCE_PR("bxroce: phd_rx_desc_tail_ptr_l: %x\n", regval);//added by hs for info 

				/*read phd rx desc tail ptr_h thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDRXDESCTAILPTR_H);
		BXROCE_PR("bxroce: phd_rx_desc_tail_ptr_h: %x\n", regval);//added by hs for info 

				/*read phd mac source addr_l*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDMACSOURCEADDR_L);
		BXROCE_PR("bxroce: phd_mac_source_addr_l: %x\n", regval);//added by hs for info 

				/*read phd mac source addr_h*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDMACSOURCEADDR_H);
		BXROCE_PR("bxroce: phd_mac_source_addr_h: %x\n", regval);//added by hs for info 

				/*read phd mac type ipv4*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDMACTYPEIPV4);
		BXROCE_PR("bxroce: phd_mac_type_ipv4: %x\n", regval);//added by hs for info 

				/*read phd mac type ipv6*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDMACTYPEIPV6);
		BXROCE_PR("bxroce: phd_mac_type_ipv6: %x\n", regval);//added by hs for info 

				/*read phd ipv4 version*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4VERSION);
		BXROCE_PR("bxroce: : phd_ipv4_version: %x\n", regval);//added by hs for info 

				/*read phd ipv4 header len*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4HEADER_LEN);
		BXROCE_PR("bxroce: phd_ipv4_header_len: %x\n", regval);//added by hs for info 

				/*read phd ipv4 tos */
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4TOS);
		BXROCE_PR("bxroce: phd_ipv4_tos: %x\n", regval);//added by hs for info 

				/*read phd ipv4 id*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4ID);
		BXROCE_PR("bxroce: phd_ipv4_id: %x\n", regval);//added by hs for info 

				/*read phd ipv4 flag*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4FLAG);
		BXROCE_PR("bxroce: phd_ipv4_flag: %x\n", regval);//added by hs for info 

				/*read phd ipv4 offset*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4OFFSET);
		BXROCE_PR("bxroce: phd_ipv4_offset: %x\n", regval);//added by hs for info 

				/*read phd ipv4 ttl*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4TTL);
		BXROCE_PR("bxroce: phd_ipv4_ttl: %x\n", regval);//added by hs for info 

				/*read phd ipv4 protocol*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4PROTOCOL);
		BXROCE_PR("bxroce: phd_ipv4_protocol: %x\n", regval);//added by hs for info 

				/*read phd ipv4 source addr*/

		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV4SOURCEADDR);
		BXROCE_PR("bxroce: phd_ipv4_source_addr: %x\n", regval);//added by hs for info 

				/*read phd ipv6 version*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6VERSION);
		BXROCE_PR("bxroce: phd_ipv6_version: %x\n", regval);//added by hs for info 

				/*read phd ipv6_class*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6CLASS);
		BXROCE_PR("bxroce: phd_ipv6_class: %x\n", regval);//added by hs for info 

				/*read phd ipv6 flow label*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6FLOWLABLE);
		BXROCE_PR("bxroce: phd_ipv6_flow_label: %x\n", regval);//added by hs for info 

				/*read phd ipv6 next header*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6NEXTHEADER);
		BXROCE_PR("bxroce: phd_ipv6_next_header: %x\n", regval);//added by hs for info 

				/*read phd ipv6 hop limit*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6HOPLIMIT);
		BXROCE_PR("bxroce: phd_ipv6_hop_limit: %x\n", regval);//added by hs for info 

				/*read phd ipv6 source addr 0*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6SOURCEADDR_0);
		BXROCE_PR("bxroce: phd_ipv6_source_addr_0: %x\n", regval);//added by hs for info 

					/*read phd ipv6 source addr 1 order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6SOURCEADDR_1);
		BXROCE_PR("bxroce: phd_ipv6_source_addr_1: %x\n", regval);//added by hs for info 

					/*read phd ipv6 source addr 2*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6SOURCEADDR_2);
		BXROCE_PR("bxroce: phd_ipv6_source_addr_2: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDIPV6SOURCEADDR_3);
		BXROCE_PR("bxroce: phd_ipv6_source_addr_3: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDUDPSOURCEPORT);
		BXROCE_PR("bxroce: phd_udp_source_port: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDUDPDESTPORT);
		BXROCE_PR("bxroce: phd_udp_dest_port: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDUDPCHECKSUM);
		BXROCE_PR("bxroce: phd_udp_checksum: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDCONTEXT_TDES0);
		BXROCE_PR("bxroce: phd_context_tdes0: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDCONTEXT_TDES1);
		BXROCE_PR("bxroce: phd_context_tdes1: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDCONTEXT_TDES2);
		BXROCE_PR("bxroce: phd_context_tdes2: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDCONTEXT_TDES3);
		BXROCE_PR("bxroce: phd_context_tdes3: %x\n", regval);//added by hs for info 

					/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDNORMAL_TDES1);
		BXROCE_PR("bxroce: phd_normal_tdes1: %x\n", regval);//added by hs for info 

						/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDNORMAL_TDES2);
		BXROCE_PR("bxroce: phd_normal_tdes2: %x\n", regval);//added by hs for info 

						/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDNORMAL_TDES3);
		BXROCE_PR("bxroce: phd_normal_tdes3: %x\n", regval);//added by hs for info 

						/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDNORMAL_RDES1);
		BXROCE_PR("bxroce: phd_normal_rdes1: %x\n", regval);//added by hs for info 

						/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDNORMAL_RDES2);
		BXROCE_PR("bxroce: phd_normal_rdes2: %x\n", regval);//added by hs for info 

						/*read phd tx desc tail ptr thresdhold order*/;
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDNORMAL_RDES3);
		BXROCE_PR("bxroce: phd_normal_rdes3: %x\n", regval);//added by hs for info 

						/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDSRAM_RMC);
		BXROCE_PR("bxroce: phd_sram_rmc: %x\n", regval);//added by hs for info 

						/*read phd tx desc tail ptr thresdhold order*/
		regval = bxroce_mpb_reg_read(base_addr,phd_base_addr,PHDMACTYPEIPV6RECV);
		BXROCE_PR("bxroce: phd_mac_type_ipv6_recv: %x\n", regval);//added by hs for info 
		BXROCE_PR("-------------------------end of phd%d--------------------------\n",i);//added by hs 
	}
	return 0;
}
int bxroce_get_hwinfo(struct bxroce_dev *dev)
{
	int status;
	status = bxroce_read_phd(dev);
	if (status)
		goto errphd;
	return 0;
errphd:
	printk("read phd failed!\n");//added by hs
	return status;
}

int bxroce_hw_create_cq(struct bxroce_dev *dev, struct bxroce_cq *cq, int entries, u16 pd_id)
{
	BXROCE_PR("bxroce: bxroce_hw_create_cq start\n");//added by hs 
	int max_hw_cqe;
	u32 hw_pages,cqe_size,cqe_count;
	struct pci_dev *pdev = dev->devinfo.pcidev;
	int status;
	/*For kernel*/
	cq->max_hw_cqe= dev->attr.max_cqe;
	max_hw_cqe = dev->attr.max_cqe;
	cqe_size = sizeof(struct bxroce_cqe);
	
	cq->len = roundup(max_hw_cqe*cqe_size,BXROCE_MIN_Q_PAGE_SIZE);
	/*tx cq*/
	cq->txva = dma_alloc_coherent(&pdev->dev,cq->len,&cq->txpa,GFP_KERNEL); // allocate memory for tx cq
	if (!cq->txva) {
		status = -ENOMEM;
		goto mem_err;
	}
	cq->txwp = cq->txrp = 0; // because wp,rp means the offset of the phypage, shoule be 0 at first.
	


	/*rx cq*/
	cq->rxva = dma_alloc_coherent(&pdev->dev,cq->len,&cq->rxpa,GFP_KERNEL);//allocate memory for rx cq
	if (!cq->rxva) {
		status = -ENOMEM;
		goto mem_err;
	}
	cq->rxwp = cq->rxrp = 0;



	/*xmit cq*/
	cq->xmitva = dma_alloc_coherent(&pdev->dev,cq->len,&cq->xmitpa,GFP_KERNEL);//allocate memory for xmit cq
	if (!cq->xmitva) {
		status = -ENOMEM;
		goto mem_err;
	}
	cq->xmitwp = cq->xmitrp = 0;



	cqe_count = cq->len / cqe_size;
	cq->cqe_cnt = cqe_count;
	if(cqe_count > 256)
		printk("bxroce: cqe_count over 256\n");//added by hs 
	
	BXROCE_PR("bxroce: bxroce_hw_create_cq end\n");//added by hs 
	return 0;
mem_err:
	printk("bxroce:mem_err\n");//added by hs
	return 0;
}

int bxroce_alloc_cqqpresource(struct bxroce_dev *dev, unsigned long *resource_array, u32 max_resources, u32 *req_resource_num, u32 *next)
{
	u32 resource_num;
	unsigned long flags;
	spin_lock_irqsave(&dev->resource_lock,flags);
	resource_num = find_next_zero_bit(resource_array,max_resources,*next);
	if (resource_num >= max_resources) {
		resource_num = find_first_zero_bit(resource_array,max_resources);
		if (resource_num >= max_resources) {
			spin_unlock_irqrestore(&dev->resource_lock,flags);
			return -EOVERFLOW;
		}
	}
	set_bit(resource_num,resource_array);
	*next = resource_num + 1;
	if(*next == max_resources)
		*next = 0;
	*req_resource_num = resource_num;
	spin_unlock_irqrestore(&dev->resource_lock,flags);
	return 0;
}

/*allocate memory , create qp & cq in hw*/
int bxroce_hw_create_qp(struct bxroce_dev *dev, struct bxroce_qp *qp, struct bxroce_cq *cq, struct bxroce_pd *pd , struct ib_qp_init_attr *attrs)
{
	BXROCE_PR("bxroce: bxroce_hw_create_qp start \n");//added by hs
	int status;
	struct pci_dev *pdev = dev->devinfo.pcidev;
	u32 len;
	dma_addr_t pa = 0;
	/*For rq*/
	u32 max_rqe_allocated = attrs->cap.max_recv_wr + 1;
	max_rqe_allocated = min_t(u32,attrs->cap.max_recv_wr +1,dev->attr.max_qp_wr); // to sure the rqe num is under 256.
	len = sizeof(struct bxroce_rqe) * max_rqe_allocated;
	len = roundup(len,BXROCE_MIN_Q_PAGE_SIZE);
	BXROCE_PR("bxroce:RQ LEN:%d \n",len);//added by hs
	qp->rq.max_cnt= max_rqe_allocated;
	qp->rq.max_wqe_idx= max_rqe_allocated - 1;
	qp->rq.va = dma_alloc_coherent(&pdev->dev,len,&pa,GFP_KERNEL); // allocate memory for rq.
	BXROCE_PR("bxroce:RQ:va =0x%x, pa =0x%x \n",qp->rq.va,pa);//added by hs
	if(!qp->rq.va)
		return -EINVAL;
	qp->rq.len = len;
	qp->rq.pa = pa;
	qp->rq.entry_size = sizeof(struct bxroce_rqe);
	u32 pa_l = 0;
	u32 pa_h = 0;
	/*init pa ,len*/
	pa = 0;
	len = 0;
	/*For sq*/
	u32 max_wqe_allocated;
	u32 max_sges = attrs->cap.max_send_sge;
	max_wqe_allocated = min_t(u32,attrs->cap.max_send_wr +1,dev->attr.max_qp_wr);
	max_sges = min_t(u32,max_wqe_allocated,max_sges); // For a sge need a wqe, so sglist 'lenghth can't over wqe 's mounts.
	len = sizeof(struct bxroce_wqe) * max_wqe_allocated;
	len = roundup(len,BXROCE_MIN_Q_PAGE_SIZE);
	BXROCE_PR("bxroce:SQ LEN:%d \n",len);//added by hs
	qp->sq.max_cnt= max_wqe_allocated;
	qp->sq.max_wqe_idx = max_wqe_allocated -1;
	qp->sq.va = dma_alloc_coherent(&pdev->dev,len,&pa,GFP_KERNEL);
	BXROCE_PR("bxroce:SQ:va = 0x%lx, pa =0x%lx \n",qp->rq.va,pa);//added by hs
	if(!qp->sq.va)
		return -EINVAL;
	qp->sq.len = len;
	qp->sq.pa = pa;
	qp->sq.entry_size = sizeof(struct bxroce_wqe);

	BXROCE_PR("bxroce:----------------Create QP checking ---------------\n");//added by hs
	BXROCE_PR("bxroce:SQ va:0x%lx , pa 0x%lx , len:%d \n",qp->sq.va,qp->sq.pa,qp->sq.len);
	BXROCE_PR("bxroce:RQ va:0x%lx , pa 0x%lx , len:%d \n",qp->rq.va,qp->rq.pa,qp->rq.len);//added by hs
																		   /*ACCESS HardWare register*/
	u32 qpn = qp->id;
	BXROCE_PR("bxroce:QPN:%d \n",qp->id);//added by hs
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	/*init psn*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,STARTINITPSN,0x0000);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,STARTINITPSN + 0x4,0x0000);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,STARTINITPSN + 0x8,0x0000);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,STARTINITPSN + 0xc,0x10000);

	/*init qpn*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,INITQP,qpn);

	/*set psn*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,INITQPTABLE,0x1);

	/*writel receive queue START*/
	/*RECVQ DIL*/
	pa = qp->rq.pa;
	BXROCE_PR("bxroce: create_qp pa_a is %0llx\n",pa);//added by hs
	pa = pa >> 12;
	pa_l = pa; // rigth move 12 bits
	pa_h = pa >> 32;
	BXROCE_PR("bxroce: create_qp pa is %0llx\n",pa);//added by hs
	BXROCE_PR("bxroce: create_qp pa_l is %0lx\n",pa_l);//added by hs
	BXROCE_PR("bxroce: create_qp pa_h is %0lx\n",pa_h);//added by hs 
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_INF,qpn);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI,pa_l);
	/*RECVQ DIH*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI + 0x4,pa_h);

	/*Write RCVQ_WR*/
	//means base addr is written.
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_WRRD,0x1);
	/*writel receive queue END*/
	/*write wp for recevice queue*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_INF,qpn);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI,0x0);
	/*RECVQ DIH*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI + 0x4,0x0);

	/*Write RCVQ_WR*/
	//means wp is written.
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_WRRD,0x2);

	/*writel receive queue for wp end*/
	/*writel receive queue for wp start*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_INF,qpn);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI,0x0);
	/*RECVQ DIH*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI + 0x4,0x0);
	/*Write RCVQ_WR*/
	//means wp for readding is written.
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_WRRD,0x4);
	/*writel receive queue for wp end*/
	
	/*16KB pagesize and response gen CQ*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,GENRSP,0x06000000);


	pa = 0;
	pa = qp->sq.pa;
	BXROCE_PR("bxroce: create_qp sqpa_a is %0llx\n",pa);//added by hs
	pa = pa >>12;
	pa_l = pa;//SendQAddr[43:12]
	pa = pa >> 32;
	pa_h = pa + 0x00100000; // {1'b1,SendQAddr[63:44]}
	BXROCE_PR("bxroce: create_qp sqpa is %0llx\n",pa);//added by hs
	BXROCE_PR("bxroce: create_qp sqpa_l is %0lx\n",pa_l);//added by hs
	BXROCE_PR("bxroce: create_qp sqpa_h is %0lx\n",pa_h);//added by hs 
	/*writel send queue START*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,QPLISTREADQPN,qpn);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WPFORQPLIST,0x0);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WPFORQPLIST2,0x0);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RPFORQPLIST,pa_l);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RPFORQPLIST2,pa_h);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WRITEORREADQPLIST,0x1);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WRITEQPLISTMASK,0x7);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,QPLISTWRITEQPN,0x1);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,CFGSIZEOFWRENTRY,64);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,CFGSIZEOFWRENTRY + 0x4,0x0);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WRITEORREADQPLIST,0x0);
	//LINKMTU {4'h0,14'h20,14'h100}
	bxroce_mpb_reg_write(base_addr,PGU_BASE,UPLINKDOWNLINK,0x00080100);
	/*sq write end*/

	/*Init WQE*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYCOUNT,0xffffffff);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYTIMER,0xffffffff);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYTIMER + 4,0xffffffff);


	/*hw access for cq*/
	u32 txop;
	u32 rxop;
	u32 xmitop;
	len = 0;
	len = cq->len;
	pa = 0;
	pa = cq->txpa;
	pa_l = pa;
	pa_h = pa >> 32;
	BXROCE_PR("bxroce: create_qp txcqpa is %0llx\n",pa);//added by hs
	BXROCE_PR("bxroce: create_qp txcqpa_l is %0lx\n",pa_l);//added by hs
	BXROCE_PR("bxroce: create_qp txcqpa_h is %0lx\n",pa_h);//added by hs 
	/*1. writel TXCQ,because CQ need qpn,that's why we access hw here.for getting qpn.*/
	    txop = qpn<<2;
	    txop = txop + 0x3; // txop should be {{32-'QPNUM-2){1'b0}},LQP,1'b1,1'b1};
		 // upaddr  = baseaddr + len //len is the length of cq memory.
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQQUEUEUP,pa_l + len);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQQUEUEUP + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQQUEUEDOWN,pa_l);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQQUEUEDOWN + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQREADPTR,pa_l);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQREADPTR + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,CQESIZE,txop);

		while (txop & 0x1)
		{
			BXROCE_PR("bxroce:judge txcq ..\n");//added by hs
			txop = bxroce_mpb_reg_read(base_addr,PGU_BASE,CQESIZE);
		}
		BXROCE_PR("bxroce: txcq success ..\n");//added by hs
	/*2.write RXCQ.*/
		pa = 0;
		pa = cq->rxpa;
		pa_l = pa;
		pa_h = pa >> 32;
		rxop = qpn << 2;
		rxop = rxop + 0x3;
		BXROCE_PR("bxroce: create_qp rxcqpa is %0llx\n",pa);//added by hs
		BXROCE_PR("bxroce: create_qp rxcqpa_l is %0lx\n",pa_l);//added by hs
		BXROCE_PR("bxroce: create_qp rxcqpa_h is %0lx\n",pa_h);//added by hs 
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxUpAddrCQE,pa_l + len);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxUpAddrCQE + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxBaseAddrCQE,pa_l);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxBaseAddrCQE + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxCQEWP,pa_l);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxCQEWP + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,RxCQEOp,rxop);

		while (rxop & 0x1)
		{
			BXROCE_PR("bxroce:judge rxcq ... \n");//added by hs
			rxop = bxroce_mpb_reg_read(base_addr,PGU_BASE,RxCQEOp);

		}
		BXROCE_PR("bxroce:judge rxcq success \n");//added by hs
	/*3. write XMIT CQ.*/
		pa = 0;
		pa = cq->xmitpa;
		pa_l = pa;
		pa_h = pa >> 32;
		xmitop = qpn << 2;
		xmitop = xmitop + 0x3;
		BXROCE_PR("bxroce: create_qp xmitcqpa is %0llx\n",pa);//added by hs
		BXROCE_PR("bxroce: create_qp xmitcqpa_l is %0lx\n",pa_l);//added by hs
		BXROCE_PR("bxroce: create_qp xmitcqpa_h is %0lx\n",pa_h);//added by hs 
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitUpAddrCQE,pa_l + len);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitUpAddrCQE + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitBaseAddrCQE,pa_l);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitBaseAddrCQE + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitCQEWP,pa_l);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitCQEWP + 0x4,pa_h);
		bxroce_mpb_reg_write(base_addr,PGU_BASE,XmitCQEOp,xmitop);

		while (xmitop & 0x1)
		{
			BXROCE_PR("bxroce:judge xmitcq ... \n");//added by hs
			xmitop = bxroce_mpb_reg_read(base_addr,PGU_BASE,XmitCQEOp);
		}
		BXROCE_PR("bxroce: xmitcq success \n");//added by hs
	/*hw access for cq end*/
	BXROCE_PR("bxroce: bxroce_hw_create_qp end \n");//added by hs 
	return 0;
}


enum ib_qp_state get_ibqp_state(enum bxroce_qp_state qps) {
	 switch (qps) {
        case BXROCE_QPS_RST:
                return IB_QPS_RESET;
        case BXROCE_QPS_INIT:
                return IB_QPS_INIT;
        case BXROCE_QPS_RTR:
                return IB_QPS_RTR;
        case BXROCE_QPS_RTS:
                return IB_QPS_RTS;
        case BXROCE_QPS_SQD:
        case BXROCE_QPS_SQ_DRAINING:
                return IB_QPS_SQD;
        case BXROCE_QPS_SQE:
                return IB_QPS_SQE;
        case BXROCE_QPS_ERR:
                return IB_QPS_ERR;
        }
        return IB_QPS_ERR;


}

enum bxroce_qp_state get_bxroce_qp_state(enum ib_qp_state qps) {
	switch (qps) {
        case IB_QPS_RESET:
                return BXROCE_QPS_RST;
        case IB_QPS_INIT:
                return BXROCE_QPS_INIT;
        case IB_QPS_RTR:
                return BXROCE_QPS_RTR; 
        case IB_QPS_RTS:
                return BXROCE_QPS_RTS;
        case IB_QPS_SQD:
                return BXROCE_QPS_SQD;
        case IB_QPS_SQE:
                return BXROCE_QPS_SQE;
        case IB_QPS_ERR:
                return BXROCE_QPS_ERR;
        }
        return BXROCE_QPS_ERR;

}

static int bxroce_resolve_dmac(struct bxroce_dev *dev, struct rdma_ah_attr *ah_attr, u8 *mac_addr)
{
	struct in6_addr in6;
	memcpy(&in6,rdma_ah_read_grh(ah_attr)->dgid.raw,sizeof(in6));
	if(rdma_is_multicast_addr(&in6))
		rdma_get_mcast_mac(&in6,mac_addr);
	else if (rdma_link_local_addr(&in6))
		rdma_get_ll_mac(&in6,mac_addr);
	else
		memcpy(mac_addr,ah_attr->roce.dmac,ETH_ALEN);
	return 0;
}

static int bxroce_set_av_params(struct bxroce_qp *qp, struct ib_qp_attr *attrs, int attr_mask)
{
	int status;
	struct rdma_ah_attr *ah_attr = &attrs->ah_attr;
	const struct ib_gid_attr *sgid_attr;
	u32 vlan_id =0xFFFF;
	union {
		struct sockaddr		_sockaddr;
		struct sockaddr_in	_sockaddr_in;
		struct sockaddr_in6 _sockaddr_in6;
	}sgid_addr,dgid_addr;
	struct bxroce_dev *dev = get_bxroce_dev(qp->ibqp.device);
	const struct ib_global_route *grh;
	if((rdma_ah_get_ah_flags(ah_attr)& IB_AH_GRH) == 0)
				return -EINVAL;
	grh = rdma_ah_read_grh(ah_attr);
	sgid_attr = ah_attr->grh.sgid_attr;
	vlan_id = rdma_vlan_dev_vlan_id(sgid_attr->ndev);
	memcpy(qp->mac_addr,sgid_attr->ndev->dev_addr,ETH_ALEN);

	qp->sgid_idx = grh->sgid_index;
	status = bxroce_resolve_dmac(dev,ah_attr,&qp->mac_addr[0]);
	return status;
}

 int bxroce_qp_state_change(struct bxroce_qp *qp, enum ib_qp_state new_ib_state, enum ib_qp_state *old_ib_state) {
	unsigned long flags;
	enum bxroce_qp_state new_state;
	new_state = get_bxroce_qp_state(new_ib_state);
	BXROCE_PR("bxroce:%s start \n",__func__);//added by hs
	if(old_ib_state)
		*old_ib_state = get_ibqp_state(qp->qp_state);
	if (new_state == qp->qp_state) {
		return 1;
	}
	if (new_state == BXROCE_QPS_INIT) {
		BXROCE_PR("bxroce: modify_qp INIT_STATE \n");//added by hs 
		qp->sq.head= 0;
		qp->sq.tail= 0;
		qp->rq.head= 0;
		qp->rq.tail= 0;
		qp->sq.qp_foe = BXROCE_Q_EMPTY;
		qp->rq.qp_foe = BXROCE_Q_EMPTY;
	}
	else if (new_state == BXROCE_QPS_ERR) {
		printk("bxroce: modify_qp ERR_STATE \n");//added by hs 
	
	}
	qp->qp_state = new_state;
	BXROCE_PR("bxroce:%s end\n",__func__);//added by hs
	return 0;
}


int bxroce_set_qp_params(struct bxroce_qp *qp, struct ib_qp_attr *attrs, int attr_mask) {
	int status = 0;
	struct bxroce_dev *dev;
	dev = get_bxroce_dev(qp->ibqp.device);
	BXROCE_PR("bxroce:%s start \n",__func__);//added by hs
	if (attr_mask & IB_QP_PKEY_INDEX) {
		qp->pkey_index = attrs->pkey_index;
	}
	if (attr_mask & IB_QP_QKEY) {
		qp->qkey = attrs->qkey;
	}
	if (attr_mask & IB_QP_DEST_QPN) { // get dest qpn.
		qp->destqp = attrs->dest_qp_num;
	}
	if (attr_mask & IB_QP_AV) {
		status = bxroce_set_av_params(qp,attrs,attr_mask);
	}
	else if (qp->qp_type == IB_QPT_GSI || qp->qp_type == IB_QPT_UD)
	{
		memcpy(qp->mac_addr,dev->devinfo.netdev->dev_addr,ETH_ALEN);
		//GET LOCAL MAC
	}
	if (attr_mask & IB_QP_PATH_MTU) {
		if (attrs->path_mtu < IB_MTU_512 ||
			attrs->path_mtu > IB_MTU_4096) {
			printk("bxroce: not supported \n");//added by hs
			status = -EINVAL;
			}
	}
	if (attr_mask & IB_QP_TIMEOUT) {
		  printk("bxroce:timeout: %x \n",attrs->timeout);//added by hs
	}
	if (attr_mask & IB_QP_RETRY_CNT) {
		  printk("bxroce:retry_cnt: %x \n",attrs->retry_cnt);//added by hs
	}
	if (attr_mask & IB_QP_MIN_RNR_TIMER) {
		  printk("bxroce:min_rnr_timer: %x \n",attrs->min_rnr_timer);//added by hs
	}
	if (attr_mask & IB_QP_RNR_RETRY) {
		printk("bxroce:rnr_retry: %x \n",attrs->rnr_retry);//added by hs
	}
	if (attr_mask & IB_QP_SQ_PSN) {
		printk("bxroce:sq_psn: %x \n",attrs->sq_psn);//added by hs
	}
	if (attr_mask & IB_QP_RQ_PSN) {
		printk("bxroce:rq_psn: %x \n",attrs->rq_psn);//added by hs
	}
	if (attr_mask & IB_QP_MAX_QP_RD_ATOMIC) {
		printk("bxroce:max_qp_rd_atomic: %x \n",attrs->max_rd_atomic);//added by hs
	}
	if (attr_mask & IB_QP_MAX_DEST_RD_ATOMIC) {
		printk("bxroce:max_dest_rd_atomic: %x \n",attrs->max_dest_rd_atomic);//added by hs
	}
	
	BXROCE_PR("bxroce:%s end \n",__func__);//added by hs
	return status;

}
