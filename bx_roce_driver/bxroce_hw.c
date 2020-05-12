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

unsigned int rdma_set_bits(unsigned int data,unsigned int index_h,unsigned int index_l,unsigned int new_val)
{
    int data_l;
    int data_m;
    int data_h;
    int index_tmp;     

    if(index_h < index_l)
    {
        index_tmp = index_h;
        index_h   = index_l;
        index_l   = index_tmp;
    }     
 
    if(index_h > 31 || index_h < 0 || index_l > 31 || index_l < 0)
    {
        RNIC_PRINTK("rdma_set_bits error: index overflow/underflow, the valid index is between 0 and 31\n");
        //exit(0);
    }
    
    if(index_l == 0) //gcc reason: data<<32 = data
        data_l = 0x0;
    else    
        data_l = ( data << (32 - index_l) ) >> (32 - index_l);
        
    data_m = ( new_val << (31 + index_l - index_h) ) >> (31 - index_h);
    
    if(index_h == 31)//gcc reason: data>>32 = data
        data_h = 0;
    else    
        data_h = ( data >> (index_h + 1) ) << (index_h + 1);

    //RNIC_PRINTK("data=%0x,data_l=%0x,data_m=%0x,data_h=%0x,new_data=%0x\n",data,data_l,data_m,data_h,data_h + data_m + data_l);
    
    return data_h | data_m | data_l;
}


unsigned int rdma_get_bits (unsigned int data,unsigned int index_h,unsigned int index_l)
{
    int index_tmp;

    if(index_h < index_l)
    {
        index_tmp = index_h;
        index_h   = index_l;
        index_l   = index_tmp;
    } 

    if(index_h > 31 || index_h < 0 || index_l > 31 || index_l < 0)
    {
        RNIC_PRINTK("rdma_get_bits error: index overflow/underflow, the valid index is between 0 and 31\n");
        //exit(0);
    }

    return ( data << (31 - index_h) ) >> (31 + index_l - index_h);
}



static int phd_start(struct bxroce_dev *dev)
{
	void __iomem *base_addr;
	base_addr = dev->devinfo.base_addr;
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDSTART,0x1);

#if 0
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDSTART,0x1);
#endif
	
	u32 data;
	data = bxroce_mpb_reg_read(base_addr,PHD_BASE_0,PHDSTART);
	BXROCE_PR("PHD0START:0x%x \n",data);
	data = bxroce_mpb_reg_read(base_addr,PHD_BASE_1,PHDSTART);
	BXROCE_PR("PHD1START:0x%x \n",data);
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
	struct net_device *netdev;
	struct in_device *pdev_ipaddr = NULL;
	u32 addr_k;

	base_addr = dev->devinfo.base_addr;
	netdev = dev->devinfo.netdev;
	
	pdev_ipaddr = (struct in_device *)netdev->ip_ptr;
	if(pdev_ipaddr == NULL)
	{BXROCE_PR("ipv4 NOT INIT SUCCEED1\n"); return 0; }
	if(pdev_ipaddr->ifa_list == NULL)
	{BXROCE_PR("ipv4 NOT INIT SUCCEED2\n"); return 0;}
	addr_k =pdev_ipaddr->ifa_list->ifa_local;
	addr_k = be32_to_cpu(addr_k);
	BXROCE_PR("ipv4: %x",addr_k);//added by hs for info

	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDIPV4SOURCEADDR,addr_k);
#if 0
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDIPV4SOURCEADDR,addr_k);
#endif

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

	BXROCE_PR("bxroce:macaddr_h:0x%x, faddr_l:0x%x \n",macaddr_h,macaddr_l);//added by hs	

	/*mac source addr  */
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDMACSOURCEADDR_H,macaddr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDMACSOURCEADDR_L,macaddr_l);
#if 0
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDMACSOURCEADDR_H,macaddr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDMACSOURCEADDR_L,macaddr_l);
#endif
	u32 regval = 0;
	regval = bxroce_mpb_reg_read(base_addr,PGU_BASE,SOCKETID);
        printk("bxroce: socketid mac before write: 0x%x \n",regval);	

	bxroce_mpb_reg_write(base_addr,PGU_BASE,SOCKETID,macaddr_l);
	regval = bxroce_mpb_reg_read(base_addr,PGU_BASE,SOCKETID);
	printk("bxroce: socketid mac after wirte: 0x%x \n",regval);	

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

	int i = RDMA_CHANNEL;//MAC_DMA_CHANNEL_ID_FOR_MPB;//channel_count -1;
	BXROCE_PR("channel_count:%d\n",i);
	u32 addr_h = 0;
	u32 addr_l = 0;
	
	base_addr_mac = RNIC_REG_BASE_ADDR_MAC_0 + DMA_CH_BASE + (DMA_CH_INC * i);
	addr_h = 0;
	addr_l = 0;

//	addr_h = base_addr_mac + DMA_CH_RDTR_HI;
    addr_l = base_addr_mac + DMA_CH_RDTR_LO;
	
	BXROCE_PR("base_addr:%lx, base_addr_mac0:%lx \n",base_addr,base_addr_mac);
	BXROCE_PR("addr_h0:%x, addr_l0:%x \n",addr_h,addr_l);

	/*rx_desc_tail_lptr_addr start*/
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDRXDESCTAILPTR_H,addr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDRXDESCTAILPTR_L,addr_l);

	/*end*/
#if 0
	base_addr_mac = RNIC_REG_BASE_ADDR_MAC_1 + DMA_CH_BASE + (DMA_CH_INC * i);
	addr_h = 0;
	addr_l = 0;

	//addr_h = base_addr_mac + DMA_CH_RDTR_HI;
    addr_l = base_addr_mac + DMA_CH_RDTR_LO;
	
	BXROCE_PR("base_addr:%lx, base_addr_mac1:%lx \n",base_addr,base_addr_mac);
	BXROCE_PR("addr_h1:%x, addr_l1:%x \n",addr_h,addr_l);

	/*rx_desc_tail_lptr_addr start*/
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDRXDESCTAILPTR_H,addr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDRXDESCTAILPTR_L,addr_l);
#endif

	return 0;
}
static int phd_txdesc_init(struct bxroce_dev *dev)
{
	/*对Phd的发送描述符进行初始化*/
	void __iomem *base_addr, *base_addr_mac;
	base_addr = dev->devinfo.base_addr;
	int channel_count = dev->devinfo.channel_count;
	//struct mac_pdata *pdata = channel->pdata;
	int i =RDMA_CHANNEL;//MAC_DMA_CHANNEL_ID_FOR_MPB;//channel_count -1;
	BXROCE_PR("channel_count:%d\n",i);
	u32 addr_h = 0;
	u32 addr_l = 0;
	
	base_addr_mac = RNIC_REG_BASE_ADDR_MAC_0 + DMA_CH_BASE + (DMA_CH_INC * i);
	addr_h = 0;
	addr_l = 0;
//	addr_h = base_addr_mac + DMA_CH_TDTR_HI;
	addr_l = base_addr_mac + DMA_CH_TDTR_LO;

	BXROCE_PR("base_addr:%lx, base_addr_mac0:%lx \n",base_addr,base_addr_mac);
	BXROCE_PR("addr_h0:%x, addr_l0:%x \n",addr_h,addr_l);
	/*tx_desc_tail_lptr_addr start*/
	
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDTXDESCTAILPTR_H,addr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_0,PHDTXDESCTAILPTR_L,addr_l);

	/*end*/
#if 0
	base_addr_mac = RNIC_REG_BASE_ADDR_MAC_1 + DMA_CH_BASE + (DMA_CH_INC * i);
	addr_h = 0;
	addr_l = 0;
	//addr_h = base_addr_mac + DMA_CH_TDTR_HI;
	addr_l = base_addr_mac + DMA_CH_TDTR_LO;

	BXROCE_PR("base_addr:%lx, base_addr_mac1:%lx \n",base_addr,base_addr_mac);
	BXROCE_PR("addr_h1:%x, addr_l1:%x \n",addr_h,addr_l);
	/*tx_desc_tail_lptr_addr start*/
	
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDTXDESCTAILPTR_H,addr_h);
	bxroce_mpb_reg_write(base_addr,PHD_BASE_1,PHDTXDESCTAILPTR_L,addr_l);
#endif

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
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,SOCKETID,0x0);
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
			xmitop = bxroce_mpb_reg_read(base_addr,PGU_BASE,XmitCQEOp);
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


		//added by hs for PGU INIT info printing
	printk("----------------------PGU INIT REG INFO START -----------------------------\n");

	u32 regval;

	regval = bxroce_mpb_reg_read(base_addr,PGU_BASE,SOCKETID);
	BXROCE_PR("\t TLBINIT(0x2030): 0x%x \n",regval);

	regval = bxroce_mpb_reg_read(base_addr,PGU_BASE,TLBINIT);
	BXROCE_PR("\t TLBINIT(0x202c): 0x%x \n",regval);

	regval = bxroce_mpb_reg_read(base_addr,PGU_BASE,WQERETRYCOUNT);
	BXROCE_PR("\t WQERETRYCOUNT(0x2014): 0x%x \n",regval);\

	regval = bxroce_mpb_reg_read(base_addr,PGU_BASE,WQERETRYTIMER);
	BXROCE_PR("\t WQERTRYCOUNT(0x2018): 0x%x \n",regval);

	regval = bxroce_mpb_reg_read(base_addr,PGU_BASE,WQERETRYTIMER + 0x4);
	BXROCE_PR("\t WQERTRYCOUNT +4 (0x201c): 0x%x \n",regval);

	printk("----------------------PGU INIT REG INFO END -----------------------------\n");

}

static void mac_config_mpb_mac(struct bxroce_dev *dev, int mac_id)
{
	u32 regval;
    unsigned int mac_addr_hi, mac_addr_lo;
    u8 *mac_addr,*haddr;
    u32 channel_id,reg_temp; //by lyp
    unsigned int mac_reg = MAC_MACA1HR;
    struct mac_pdata *pdata = dev->devinfo.pdata;
	struct rnic_pdata *rnic_pdata = dev->devinfo.rnic_pdata;

    RNIC_TRACE_PRINT();

    haddr = pdata->netdev->dev_addr;
    channel_id = MAC_DMA_CHANNEL_ID_FOR_MPB;

    mac_addr_lo = 0;
    mac_addr_hi = 0;

    if (haddr) 
     {
        mac_addr = (u8 *)&mac_addr_lo;
        mac_addr[0] = haddr[0];
        mac_addr[1] = haddr[1];
        mac_addr[2] = haddr[2];
        mac_addr[3] = haddr[3];
        mac_addr = (u8 *)&mac_addr_hi;
        mac_addr[0] = haddr[4];
        mac_addr[1] = haddr[5];
	
				mac_addr_hi = MAC_SET_REG_BITS(mac_addr_hi,
								  MAC_MACA1HR_AE_POS,
								MAC_MACA1HR_AE_LEN,
								1); //enable mac
		
			   reg_temp = (channel_id << 16) & 0x00ff0000;
			   mac_addr_hi |= reg_temp;
				
			 //write hi reg
			mac_reg +=MAC_MACA_INC*2*(channel_id-1); //mac_reg_addr
			writel(mac_addr_hi, pdata->mac_regs + mac_reg);
			 //write lo reg
		    mac_reg += MAC_MACA_INC;
			writel(mac_addr_lo, pdata->mac_regs + mac_reg);

			u32 regval;
			regval = readl(pdata->mac_regs + mac_reg - MAC_MACA_INC);
			printk("mac channel 6 mac addr hi:0x%x \n",regval);
			regval = readl(pdata->mac_regs + mac_reg);
			printk("mac channel 6 mac addr lo:0x%x \n",regval);
			  
	} 
}


static int mac_mpb_flush_tx_queues(struct bxroce_dev *dev)
{
	unsigned int i,count;
	u32 regval;
	unsigned int j = MAC_DMA_CHANNEL_ID_FOR_MPB;

//added by lyp
	struct bx_dev_info *devinfo = &dev->devinfo;
	unsigned int rdma_channel = RDMA_CHANNEL;
//end added by lyp

	//write tx flush queue
#if 0 
		regval = readl(MAC_MTL_REG(pdata, j, MTL_Q_TQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_FTQ_POS,
                         MTL_Q_TQOMR_FTQ_LEN, 1);
        writel(regval, MAC_MTL_REG(pdata, j, MTL_Q_TQOMR));
#endif
	//

//added by lyp 
	 
    
     regval = readl(MAC_RDMA_MTL_REG(devinfo, rdma_channel, MTL_Q_TQOMR));
     regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_FTQ_POS,
                         MTL_Q_TQOMR_FTQ_LEN, 1);
     writel(regval, MAC_RDMA_MTL_REG(devinfo, rdma_channel, MTL_Q_TQOMR));
    

	
    /* Poll Until Poll Condition */
    
	
      count = 2000;
      regval = readl(MAC_RDMA_MTL_REG(devinfo, rdma_channel, MTL_Q_TQOMR));
      regval = MAC_GET_REG_BITS(regval, MTL_Q_TQOMR_FTQ_POS,
                         MTL_Q_TQOMR_FTQ_LEN);
        while (--count && regval)
            usleep_range(500, 600);

        if (!count)
            return -EBUSY;
    

    return 0;

//end added by lyp 


}

static int mac_mpb_config_osp_mode(struct bxroce_dev *dev)
{
	//unsigned int i,j;
	//void __iomem *base_addr;
	u32 regval;
#if 0
	base_addr = dev->devinfo.base_addr;
	regval = readl(base_addr + RNIC_REG_BASE_ADDR_MAC_0 +0x3100 + 0x80*MAC_DMA_CHANNEL_ID_FOR_MPB +  DMA_CH_TCR);
	regval = MAC_SET_REG_BITS(regval,DMA_CH_TCR_OSP_POS,DMA_CH_TCR_OSP_LEN,1);
	regval = writel(regval,base_addr + RNIC_REG_BASE_ADDR_MAC_0 + 0x3100 + 0x80*MAC_DMA_CHANNEL_ID_FOR_MPB +  DMA_CH_TCR);
#endif
//added by lyp
	struct bx_dev_info *devinfo = &dev->devinfo;
	unsigned int rdma_channel = RDMA_CHANNEL;
//end added by lyp


//added by lyp
	

     regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));
     regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_OSP_POS,
                         DMA_CH_TCR_OSP_LEN,
                    devinfo->pdata->tx_osp_mode);
	 
     writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));

//end added by lyp

	return 0;
}


static int mac_mpb_config_dis_tcp_ef_off(struct bxroce_dev *dev)
{
	u32 regval;
	struct bx_dev_info *devinfo = &dev->devinfo;
	unsigned int rdma_channel = RDMA_CHANNEL;

	regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));
	regval = MAC_SET_REG_BITS(regval,MTL_Q_RQ0MR_DIS_TCP_EF_POS,
							MTL_Q_RQ0MR_DIS_TCP_EF_LEN,
							1);
	writel(regval,MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));
	return 0;
}


//added by lyp
static int mac_rdma_config_pblx8(struct bxroce_dev *dev)
{
    u32 regval;
	struct bx_dev_info *devinfo = &dev->devinfo;
    
    RNIC_TRACE_PRINT();

   
    regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_CR));
    regval = MAC_SET_REG_BITS(regval, DMA_CH_CR_PBLX8_POS,
                         DMA_CH_CR_PBLX8_LEN,
                    devinfo->pdata->pblx8);

//	regval = MAC_SET_REG_BITS(regval, DMA_CH_CR_PBLX8_POS,
//                         DMA_CH_CR_PBLX8_LEN,0x1);

	
    writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_CR));
    

    return 0;
}



static int mac_rdma_config_tx_pbl_val(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo;
    u32 regval;
    
    RNIC_TRACE_PRINT();

 
    regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));
    regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_PBL_POS,
                         DMA_CH_TCR_PBL_LEN,
                    devinfo->pdata->tx_pbl);

//	regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_PBL_POS,
//					     DMA_CH_TCR_PBL_LEN,
//		            0x20);//added by hs
    writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));
    

    return 0;
}


static int mac_rdma_config_rx_pbl_val(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    
    

    regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RCR));
    regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_PBL_POS,
                         DMA_CH_RCR_PBL_LEN,
                    devinfo->pdata->rx_pbl);

//	 regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_PBL_POS,
//                         DMA_CH_RCR_PBL_LEN,
//						0x20);//added by hs
    writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_RCR));
    
    return 0;
}



static int mac_rdma_config_rx_coalesce(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo;
    u32 regval;
    
    RNIC_TRACE_PRINT();
    
   
    regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RIWT));
    regval = MAC_SET_REG_BITS(regval, DMA_CH_RIWT_RWT_POS,
                         DMA_CH_RIWT_RWT_LEN,
                         devinfo->pdata->rx_riwt);
    writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_RIWT));
    

    return 0;
}


static void mac_rdma_config_rx_buffer_size(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo;    
    u32 regval;
    
    RNIC_TRACE_PRINT();

  

    regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RCR));
    regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_RBSZ_POS,
                         DMA_CH_RCR_RBSZ_LEN,
                    devinfo->pdata->rx_buf_size);

//	regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_RBSZ_POS,
//						DMA_CH_RCR_RBSZ_LEN,
//					  0x3ff0);
    writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_RCR));
    
}


static void mac_rdma_config_tso_mode(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
    
  
    if (devinfo->pdata->hw_feat.tso) {
            regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));
            regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_TSE_POS,
                             DMA_CH_TCR_TSE_LEN, 0);//off by hs
            writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));
        
	}

}



static void mac_rdma_config_sph_mode(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
    

    
    regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_CR));
  //  regval = MAC_SET_REG_BITS(regval, DMA_CH_CR_SPH_POS,
  //                        DMA_CH_CR_SPH_LEN, 1);

      regval = MAC_SET_REG_BITS(regval, DMA_CH_CR_SPH_POS,
			    DMA_CH_CR_SPH_LEN, 0);//added by hs to make sph off.
			
    writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_CR));
    
}



static void mac_rdma_tx_desc_init(struct bxroce_dev *dev,int mac_id)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
	unsigned int mpb_base_addr_h;
    
    RNIC_TRACE_PRINT();

    if(mac_id == 0x0)
        mpb_base_addr_h = RNIC_BASE_ADDR_MPB_DATA_S_0_H;
    else
        mpb_base_addr_h = RNIC_BASE_ADDR_MPB_DATA_S_1_H;  

    /* Update the total number of Tx descriptors */
    writel(devinfo->pdata->tx_desc_count - 1, MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDRLR)); //del by hs
	//writel(0x3ff, MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDRLR));

    /* Update the starting address of descriptor ring */
    writel(0x00000000+mpb_base_addr_h,MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDLR_HI));
    writel(0x00000000, MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDLR_LO));
	
    /* Update the Tx Descriptor Tail Pointer */
    
    writel(0x00000000,MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDTR_LO));

}




static void mac_rdma_rx_desc_init(struct bxroce_dev *dev,int mac_id)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
	unsigned int mpb_base_addr_h;
    
    RNIC_TRACE_PRINT();

    if(mac_id == 0x0)
        mpb_base_addr_h = RNIC_BASE_ADDR_MPB_DATA_S_0_H;
    else
        mpb_base_addr_h = RNIC_BASE_ADDR_MPB_DATA_S_1_H; 
	
    /* Update the total number of Rx descriptors */
    writel(devinfo->pdata->rx_desc_count - 1, MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDRLR));//del by hs
	//writel(0x3ff, MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDRLR));

    /* Update the starting address of descriptor ring */
    
    writel(0x00000001+mpb_base_addr_h,
           MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDLR_HI));
	
    writel(0x00000000,
           MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDLR_LO));

    /* Update the Rx Descriptor Tail Pointer */
    
    writel(0x00000000,
           MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDTR_LO));
}



static void mac_rdma_enable_dma_interrupts(struct bxroce_dev *dev)
{
    unsigned int dma_ch_isr, dma_ch_ier;
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
    
    RNIC_TRACE_PRINT();
    

    
    /* Clear all the interrupts which are set */
    dma_ch_isr = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_SR));
    writel(dma_ch_isr, MAC_RDMA_DMA_REG(devinfo, DMA_CH_SR));
    

    /* Clear all interrupt enable bits */
    dma_ch_ier = 0;

        /* Enable following interrupts
         *   NIE  - Normal Interrupt Summary Enable
         *   AIE  - Abnormal Interrupt Summary Enable
         *   FBEE - Fatal Bus Error Enable
         */
     dma_ch_ier = MAC_SET_REG_BITS(dma_ch_ier,
                         DMA_CH_IER_NIE_POS,
                    DMA_CH_IER_NIE_LEN, 1);
     dma_ch_ier = MAC_SET_REG_BITS(dma_ch_ier,
                         DMA_CH_IER_AIE_POS,
                    DMA_CH_IER_AIE_LEN, 1);
     dma_ch_ier = MAC_SET_REG_BITS(dma_ch_ier,
                         DMA_CH_IER_FBEE_POS,
                    DMA_CH_IER_FBEE_LEN, 1);

        
            /* Enable the following Tx interrupts
             *   TIE  - Transmit Interrupt Enable (unless using
             *          per channel interrupts)
             */
      
      dma_ch_ier = MAC_SET_REG_BITS(
                        dma_ch_ier,
                        DMA_CH_IER_TIE_POS,
                        DMA_CH_IER_TIE_LEN,
                        1);
        
	  /* Enable the following Tx interrupts
             *   TBUE  - Transmit Buffer Unavailable Enable.
             */
	  dma_ch_ier = MAC_SET_REG_BITS(dma_ch_ier,
									DMA_CH_IER_TBUE_POS,
									DMA_CH_IER_TBUE_LEN,
									1); //added by hs

        
            /* Enable following Rx interrupts
             *   RBUE - Receive Buffer Unavailable Enable
             *   RIE  - Receive Interrupt Enable (unless using
             *          per channel interrupts)
             */
      dma_ch_ier = MAC_SET_REG_BITS(
                    dma_ch_ier,
                    DMA_CH_IER_RBUE_POS,
                    DMA_CH_IER_RBUE_LEN,
                    1);
        
       dma_ch_ier = MAC_SET_REG_BITS(
                        dma_ch_ier,
                        DMA_CH_IER_RIE_POS,
                        DMA_CH_IER_RIE_LEN,
                        1);
        
      writel(dma_ch_ier, MAC_RDMA_DMA_REG(devinfo, DMA_CH_IER));

	  //?? is it 0x0000c0c5 for DMA_CH_IER or not??
    
}


static int mac_rdma_config_tsf_mode(struct bxroce_dev *dev,
                  unsigned int val)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    
    u32 regval;
    
    RNIC_TRACE_PRINT();

    
	  
    regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL,MTL_Q_TQOMR));  //by lyp
    regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TSF_POS,
                         MTL_Q_TQOMR_TSF_LEN, val);
    writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_TQOMR));  //by lyp
    

    return 0;
}


static int mac_rdma_config_rsf_mode(struct bxroce_dev *dev,
                  unsigned int val)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
    
    RNIC_TRACE_PRINT();

    
	

    regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
    regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RSF_POS,
                         MTL_Q_RQOMR_RSF_LEN, val);
    writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
   

    return 0;
}



static int mac_rdma_config_tx_threshold(struct bxroce_dev *dev,
						  unsigned int val)
{
		struct bx_dev_info *devinfo = &dev->devinfo; 
		u32 regval;
	 
		
		
			
	
		regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_TQOMR)); //by lyp
		regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TTC_POS,
							 MTL_Q_TQOMR_TTC_LEN, val);
		writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_TQOMR));  //by lyp
		
	
		return 0;
}
	

static int mac_rdma_config_rx_threshold(struct bxroce_dev *dev,
						  unsigned int val)
	{
		struct bx_dev_info *devinfo = &dev->devinfo; 
		u32 regval;
	
		
		
		regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
		regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RTC_POS,
							 MTL_Q_RQOMR_RTC_LEN, val);
		writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
		
	
		return 0;
	}
	


static void mac_rdma_config_tx_fifo_size(struct bxroce_dev *dev)
{
    unsigned int fifo_size;
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
    
    RNIC_TRACE_PRINT();

     //this is modified by lyp to support vf, pf fifo size is 183, corresponding to 46K, 
// every vf is 11, corresponding to 3K, every vf has 1 channel,1 queue
#if 0
    fifo_size = mac_calculate_per_queue_fifo(
                pdata->hw_feat.tx_fifo_size,
                pdata->tx_q_count);
#endif
	fifo_size = 11; //pf is 183

// end modified by lyp 20200328


    
	   
     regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_TQOMR)); //by lyp
     regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TQS_POS,
                         MTL_Q_TQOMR_TQS_LEN, fifo_size);
     writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_TQOMR));//by lyp
    

    
}


static void mac_rdma_config_rx_fifo_size(struct bxroce_dev *dev)
{
    unsigned int fifo_size;
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
    
    RNIC_TRACE_PRINT();

//this is modified by lyp to support vf, pf fifo size is 183, corresponding to 46K, 
// every vf is 11, corresponding to 3K,
#if 0
    fifo_size = mac_calculate_per_queue_fifo(
                    pdata->hw_feat.rx_fifo_size,
                    pdata->rx_q_count);
#endif
    fifo_size = 11; //pf is 183

// end modified by lyp 20200328


    
     regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
     regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RQS_POS,
                         MTL_Q_RQOMR_RQS_LEN, fifo_size);
     writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
    
}


static void mac_rdma_config_flow_control_threshold(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;

    
    regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQFCR));  //by lyp
        /* Activate flow control when less than 4k left in fifo */
    regval = MAC_SET_REG_BITS(regval, MTL_Q_RQFCR_RFA_POS,
                         MTL_Q_RQFCR_RFA_LEN, 2);
        /* De-activate flow control when more than 6k left in fifo */
    regval = MAC_SET_REG_BITS(regval, MTL_Q_RQFCR_RFD_POS,
                         MTL_Q_RQFCR_RFD_LEN, 4);
    writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQFCR));  //by lyp
    
}

static void mac_rdma_config_rx_fep_enable(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    u32 regval;
    
    RNIC_TRACE_PRINT();

    
    regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
    regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_FEP_POS,
                         MTL_Q_RQOMR_FEP_LEN, 1);
    writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
    
}




static void mac_rdma_enable_mtl_interrupts(struct bxroce_dev *dev)
{
    struct bx_dev_info *devinfo = &dev->devinfo; 
    unsigned int mtl_q_isr;
	u32 regval;
    
    RNIC_TRACE_PRINT();
    

    
    /* Clear all the interrupts which are set */
    mtl_q_isr = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_ISR));  
    writel(mtl_q_isr, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_ISR));  
        
    /* No MTL interrupts to be enabled */
    writel(0, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_IER));
   
//	regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_IER));
//	regval = MAC_SET_REG_BITS(regval,MTL_Q_IER_RXOIE_POS,
//							  MTL_Q_IER_RXOIE_LEN,1);
//	writel(regval,MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_IER)); // add by hs,enable receive queue overflow intr.

}


static int mac_rdma_disable_tx_flow_control(struct bxroce_dev *dev)
{
	
	unsigned int reg, regval;
	struct bx_dev_info *devinfo = &dev->devinfo; 

	//different with pf, start from j
	
	
	/* Clear MTL flow control */
	
	regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));  //by lyp
	regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_EHFC_POS,
						 MTL_Q_RQOMR_EHFC_LEN, 0);
	writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR)); //by lyp
	

	/* Clear MAC flow control */
	
	reg = MAC_Q0TFCR+MAC_QTFCR_INC*RDMA_CHANNEL; //by lyp
	
	regval = readl(devinfo->mac_base+ reg);
	regval = MAC_SET_REG_BITS(regval,
						 MAC_Q0TFCR_TFE_POS,
					MAC_Q0TFCR_TFE_LEN,
					0);
	writel(regval, devinfo->mac_base + reg);

	
	

	return 0;
}


static int mac_rdma_enable_tx_flow_control(struct bxroce_dev *dev)
{
		 struct bx_dev_info *devinfo = &dev->devinfo; 
		 unsigned int reg, regval;
		
	
		  
		 /* Set MTL flow control */
		 
		regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));	//by lyp
		regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_EHFC_POS,
							  MTL_Q_RQOMR_EHFC_LEN, 1);
		writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));   //by lyp
		
	
	
	 
		 /* Set MAC flow control */
		
		 reg = MAC_Q0TFCR+MAC_QTFCR_INC*RDMA_CHANNEL;
		 
		 regval = readl(devinfo->mac_base + reg);
	 
			 /* Enable transmit flow control */
		 regval = MAC_SET_REG_BITS(regval, MAC_Q0TFCR_TFE_POS,
							  MAC_Q0TFCR_TFE_LEN, 1);
			 /* Set pause time */
		 regval = MAC_SET_REG_BITS(regval, MAC_Q0TFCR_PT_POS,
							  MAC_Q0TFCR_PT_LEN, 0xffff);
	 
		 writel(regval, devinfo->mac_base + reg);
	 
			
	 
		 return 0;
}
	 





static int mac_rdma_config_tx_flow_control(struct bxroce_dev *dev)
{
	struct bx_dev_info *devinfo = &dev->devinfo; 
	
	if (devinfo->pdata->tx_pause)
		mac_rdma_enable_tx_flow_control(dev);
	else
		mac_rdma_disable_tx_flow_control(dev);
	 
	return 0;
}
	 



static void mac_rdma_config_flow_control(struct bxroce_dev *dev)
{   
    RNIC_TRACE_PRINT();
    
    mac_rdma_config_tx_flow_control(dev);
    //mac_rdma_config_rx_flow_control(dev);
}

static void mac_rdma_config_mtl_tc_quantum_weight(struct bxroce_dev *dev)
{
	 struct bx_dev_info *devinfo = &dev->devinfo; 
	 unsigned int reg, regval;

	 regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL,MTL_Q_QWR));
	 regval = MAC_SET_REG_BITS(regval,MTL_Q_QWR_QW_POS,
							   MTL_Q_QWR_QW_LEN,0xa);
}


int mac_rdma_l3_l4_filter_cfg_reg_read(struct bxroce_dev *dev,unsigned int addr)
{
    unsigned int data;
	struct bx_dev_info *devinfo = &dev->devinfo;

	data =readl(devinfo->mac_base + 0x0c00);
    
    while(rdma_get_bits(data,0,0) == 1)
        data =readl(devinfo->mac_base + 0x0c00);

    data = rdma_set_bits(data,15,8,addr);  //Layer4_Address
    data = rdma_set_bits(data,1,1,1);      //read
    data = rdma_set_bits(data,0,0,1);      //start write

   
	writel(data, devinfo->mac_base + 0x0c00);

    data = readl(devinfo->mac_base + 0x0c00);;
    while(rdma_get_bits(data,0,0) == 1)
        data = readl(devinfo->mac_base + 0x0c00);
            
	data = readl(devinfo->mac_base + 0x0c04);
    
    return data;
}

void mac_rdma_l3_l4_filter_cfg_reg_write(struct bxroce_dev *dev,unsigned int addr,unsigned int wdata)
{
    unsigned int data;
	struct bx_dev_info *devinfo = &dev->devinfo;
    
    data =readl(devinfo->mac_base + 0x0c00);
    while(rdma_get_bits(data,0,0) == 1)
        data =readl(devinfo->mac_base + 0x0c00);;
        
    writel(wdata, devinfo->mac_base + 0x0c04);
    
    data = readl(devinfo->mac_base + 0x0c00); //del by hs@20200427
    while(rdma_get_bits(data,0,0) == 1)
        data = readl(devinfo->mac_base + 0x0c00); // del by hs@20200427
    
    data = rdma_set_bits(data,15,8,addr);  //Layer4_Address
    data = rdma_set_bits(data,1,1,0);      //write
    data = rdma_set_bits(data,0,0,1);      //start write

   
	writel(data, devinfo->mac_base + 0x0c00);


	
}

void mac_rdma_channel_mpb_l3_l4_filter_on (struct bxroce_dev *dev)
{
    unsigned int data;
     
   
    // write the MAC_L3_L4_Control_0
    data = mac_rdma_l3_l4_filter_cfg_reg_read(dev,0x0);
    

    data = rdma_set_bits(data,31,31,1);                                                  // DMA Channel Select Enable                        
    data = rdma_set_bits(data,27,24,RDMA_CHANNEL);                         // DMA Channel Number 
    //data = rdma_set_bits(data,21,21,1);                                                // Layer 4 Destination Port Inverse Match Enable.       
    data = rdma_set_bits(data,20,20,1);                                                  // Layer 4 Destination Port Match Enable.   
    data = rdma_set_bits(data,16,16,1);                                                  // Layer 4 Protocol Enable:UDP          
    
	                           
    mac_rdma_l3_l4_filter_cfg_reg_write(dev,0x0,data);
    
	
	// write the MAC_Layer4_Address_0
    mac_rdma_l3_l4_filter_cfg_reg_write(dev,0x1,4791<<16);

	
}

#define MAC_RCR_LM_POS  10
#define MAC_RCR_LM_LEN  1

static void mac_config_loopback(struct bxroce_dev *dev)
{
	struct bx_dev_info *devinfo = &dev->devinfo;
	u32 regval;

	regval = readl(MAC_RDMA_MAC_REG(devinfo,MAC_RCR));
	regval = MAC_SET_REG_BITS(regval,MAC_RCR_LM_POS,
							  MAC_RCR_LM_LEN, 0);
	printk("MAC_RCR:regval set to 0x%x \n",regval);
	writel(regval,MAC_RDMA_MAC_REG(devinfo,MAC_RCR));

	

}

static void mac_rdma_enable_tx(struct bxroce_dev *dev)
{
   struct bx_dev_info *devinfo = &dev->devinfo;
   u32 regval;
   
    
    RNIC_TRACE_PRINT();

    /* Enable each Tx DMA channel */
    
    regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));
    regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_ST_POS,
                         DMA_CH_TCR_ST_LEN, 1);
    writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));
	
    

    /* Enable each Tx queue */
    

    regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_TQOMR));  //by lyp
    regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TXQEN_POS,
                         MTL_Q_TQOMR_TXQEN_LEN,
                    MTL_Q_ENABLED);
    writel(regval, MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_TQOMR));  //by lyp



}


 
 
 static void mac_rdma_enable_rx(struct bxroce_dev *dev)
 {
	 struct bx_dev_info *devinfo = &dev->devinfo;
	 unsigned int regval,j;
	
	 
	 RNIC_TRACE_PRINT();
 
	 /* Enable each Rx DMA channel */
	
 
	 regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RCR));
	 regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_SR_POS,
						  DMA_CH_RCR_SR_LEN, 1);
	 writel(regval, MAC_RDMA_DMA_REG(devinfo, DMA_CH_RCR));
 

 

 
 
	 /* Enable each Rx queue */
	  
	
	 regval = readl(devinfo->mac_base+ MAC_RQEC); //modified by lyp
 
	
	  j = RDMA_CHANNEL;
		
	  regval |= (0x02 << (j << 1));  
	  writel(regval, devinfo->mac_base + MAC_RQEC);
	  
 
 }
 
 static void mac_rdma_print_regval(struct bxroce_dev *dev)
 {
	  struct bx_dev_info *devinfo = &dev->devinfo;
	  unsigned int rdma_channel = RDMA_CHANNEL;
	  u32 regval = 0;

	  BXROCE_PR("----------------------------MAC RDMA PRINTF INFO START-------------- \n");

	  regval = readl(MAC_RDMA_MTL_REG(devinfo, rdma_channel, MTL_Q_TQOMR));
	  BXROCE_PR("MTL_Q_TQ0MR(0x00): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQOMR));
	  BXROCE_PR("MTL_Q_RQ0MR(0x40): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_RQFCR));
	  BXROCE_PR("MTL_Q_RQFCR(0x50): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_ISR)); 
	  BXROCE_PR("MTL_Q_ISR(0x74): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_IER));
	  BXROCE_PR("MTL_Q_IER(0x70): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_MTL_REG(devinfo, RDMA_CHANNEL, MTL_Q_QWR));
	  BXROCE_PR("MTL_Q_QWR(0x18): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TCR));
	  BXROCE_PR("DMA_CH_TCR(0x04): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_CR));
	  BXROCE_PR("DMA_CH_CR(0x00): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RCR));
	  BXROCE_PR("DMA_CH_RCR(0x08): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RIWT));
	  BXROCE_PR("DMA_CH_RIWT(0x3c): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDRLR));
	  BXROCE_PR("DMA_CH_TDRLR(0x30): 0x%x \n",regval);
 
	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDLR_HI));
	  BXROCE_PR("DMA_CH_TDLR_HI(0x10): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDLR_LO));
	  BXROCE_PR("DMA_CH_TDLR_LO(0x14): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_TDTR_LO));
	  BXROCE_PR("DMA_CH_TDTR_LO(0x24): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDRLR));
	  BXROCE_PR("DMA_CH_RDRLR(0x34): 0x%x \n",regval);
 
	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDLR_HI));
	  BXROCE_PR("DMA_CH_RDLR_HI0x18): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDLR_LO));
	  BXROCE_PR("DMA_CH_RDLR_LO(0x1c): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_RDTR_LO));
	  BXROCE_PR("DMA_CH_RDTR_LO(0x2c): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_SR));
	  BXROCE_PR("DMA_CH_SR(0x60): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_DMA_REG(devinfo, DMA_CH_IER));
	  BXROCE_PR("DMA_CH_IER(0x38): 0x%x \n",regval);

	  regval = mac_rdma_l3_l4_filter_cfg_reg_read(dev,0x0);
	  BXROCE_PR("RDMA_L3_L4_FILTER0X0: 0x%x \n",regval);

	  regval = mac_rdma_l3_l4_filter_cfg_reg_read(dev,0x1);
	  BXROCE_PR("RDMA_L3_L4_FILTER0X1: 0x%x \n",regval);

	  regval = readl(devinfo->mac_base+ MAC_RQEC);
	  BXROCE_PR("MAC_RQEC(0x140): 0x%x \n",regval);

	  regval = readl(MAC_RDMA_MAC_REG(devinfo,MAC_RCR));
	  BXROCE_PR("MAC_RCR(0x0004): 0x%x \n",regval);

 
	  BXROCE_PR("----------------------------MAC RDMA PRINTF INFO END-------------- \n");
 }


static int bxroce_init_mac_channel(struct bxroce_dev *dev)
{
	void __iomem *base_addr;
	struct rnic_pdata *rnic_pdata = dev->devinfo.rnic_pdata;
	BXROCE_PR("start mac channel init \n");
	//mac_mpb_flush_tx_queues(dev);
	//mac_mpb_config_osp_mode(dev);
	
	mac_mpb_flush_tx_queues(dev);
	mac_mpb_config_osp_mode(dev);

	//added by lyp
	mac_rdma_config_pblx8(dev);
	mac_rdma_config_tx_pbl_val(dev);
	mac_rdma_config_rx_pbl_val(dev);

	mac_rdma_config_rx_coalesce(dev); //del by hs for watchdog may not need .
	mac_rdma_config_rx_buffer_size(dev);
	
	mac_rdma_config_tso_mode(dev); //may not need
	mac_rdma_config_sph_mode(dev);  //may not need, sph 1, we do not konw sph is 0 or 1 in rdma
	mac_mpb_config_dis_tcp_ef_off(dev); //add by hs

	mac_rdma_tx_desc_init(dev,0);   //may be error for descripotr addr
	mac_rdma_rx_desc_init(dev,0);  //may be error for descripotr addr
	
	
	mac_rdma_enable_dma_interrupts(dev);

	mac_rdma_config_tsf_mode(dev,dev->devinfo.pdata->tx_sf_mode);
	mac_rdma_config_rsf_mode(dev,dev->devinfo.pdata->rx_sf_mode);

//	mac_rdma_config_tsf_mode(dev,1);//added by hs
//	mac_rdma_config_rsf_mode(dev,1);//added by hs

	mac_rdma_config_tx_threshold(dev,dev->devinfo.pdata->tx_threshold);
	mac_rdma_config_rx_threshold(dev,dev->devinfo.pdata->rx_threshold);

//	mac_rdma_config_tx_threshold(dev,0);//added by hs
//	mac_rdma_config_rx_threshold(dev,0);//added by hs

	mac_rdma_config_tx_fifo_size(dev); //pf should be changed
	mac_rdma_config_rx_fifo_size(dev); //pf should be changed

	mac_rdma_config_flow_control_threshold(dev);
	mac_rdma_config_rx_fep_enable(dev);

	mac_rdma_enable_mtl_interrupts(dev);  //maybe error
	mac_rdma_config_flow_control(dev);
	mac_rdma_config_mtl_tc_quantum_weight(dev);

	mac_rdma_channel_mpb_l3_l4_filter_on(dev);

	//mac_config_loopback(dev);

	//enable tx and rx
	mac_rdma_enable_tx(dev);
	mac_rdma_enable_rx(dev);
	//end added by lyp



	mac_rdma_print_regval(dev);


	//mac_mpb_channel_cfg(rnic_pdata,0);
	//mac_config_mpb_mac(dev,0);

#if 0
	mac_mpb_channel_cfg(rnic_pdata,1);
#endif
	return 0;
}


static int bxroce_init_pbu(struct bxroce_dev *dev)
{
	struct rnic_pdata *rnic_pdata = dev->devinfo.rnic_pdata;
	pbu_init(rnic_pdata);
}

int bxroce_init_hw(struct bxroce_dev *dev)
{
	int status;
	status = bxroce_init_mac_channel(dev);
	if(status)
		goto err_mac_channel;
	status = bxroce_init_pbu(dev);
	if(status)
		goto err_pbu;
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
err_pbu:
	printk("pbu init err!\n");
err_mac_channel:
	printk("mac channel err!\n");
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
	cqe_size = sizeof(struct bxroce_txcqe);
	cq->cqe_size = cqe_size;

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
int bxroce_hw_create_qp(struct bxroce_dev *dev, struct bxroce_qp *qp, struct bxroce_pd *pd , struct ib_qp_init_attr *attrs)
{
	BXROCE_PR("bxroce: bxroce_hw_create_qp start \n");//added by hs
	int status;
	struct pci_dev *pdev = dev->devinfo.pcidev;
	struct bxroce_cq *cq = NULL;
	struct bxroce_cq *rq_cq = NULL;
	u32 len;
	dma_addr_t pa = 0;
	u32 txop = 0;
	u32 rxop = 0;
	u32 xmitop = 0;
	bool Notsharedcq = false;

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

	cq = get_bxroce_cq(attrs->send_cq);
	cq->qp_id = qp->id;
	qp->sq_cq = cq;


	rq_cq = get_bxroce_cq(attrs->recv_cq);
	rq_cq->qp_id = qp->id;
	qp->rq_cq = rq_cq;

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
	bxroce_mpb_reg_write(base_addr,PGU_BASE,INITQP,qpn);/*init qpn*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,INITQPTABLE,0x1);/*set psn*/

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
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI + 0x4,pa_h);/*RECVQ DIH*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_WRRD,0x1);/*Write RCVQ_WR*///means base addr is written.
	
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_INF,qpn);	/*writel receive queue END*//*write wp for recevice queue*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI,0x0);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI + 0x4,0x0);/*RECVQ DIH*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_WRRD,0x2);/*Write RCVQ_WR*///means wp is written.

	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_INF,qpn);/*writel receive queue for rp start*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI,0x0);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_DI + 0x4,0x0);/*RECVQ DIH*/
	bxroce_mpb_reg_write(base_addr,PGU_BASE,RCVQ_WRRD,0x4);	/*Write RCVQ_WR*///means rp for readding is written.
	/*writel receive queue for rp end*/
	
	/*16KB pagesize and response gen CQ*/
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,GENRSP,0x06000000);

#if 1 // Active QP need  behind pbu .BUT pbu need to wait rqp to map.
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
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,UPLINKDOWNLINK,0x00080100);
	bxroce_mpb_reg_write(base_addr,PGU_BASE,UPLINKDOWNLINK,0x00800400);
	/*sq write end*/
#endif
	/*Init WQE*/
	//del by hs@20200504 no need to init again.
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYCOUNT,0xffffffff);
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYTIMER,0xffffffff);
	//bxroce_mpb_reg_write(base_addr,PGU_BASE,WQERETRYTIMER + 4,0xffffffff);


	do{

	if(Notsharedcq) cq = rq_cq;

	/*hw access for cq*/
	 txop = 0;
	 rxop = 0;
	 xmitop = 0;
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

		if(cq != rq_cq) Notsharedcq = true;
		else Notsharedcq = false;

	}while(Notsharedcq);
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
	u8 hdr_type;
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
	qp->qp_change_info->sgid_idx = qp->sgid_idx;

	status = bxroce_resolve_dmac(dev,ah_attr,&qp->mac_addr[0]);
	memcpy(&qp->qp_change_info->mac_addr[0],&qp->mac_addr[0],6);

	if(status)
	{ BXROCE_PR("bxroce: resolve dmac problem!\n",__func__);return status;}

	//add resolve gid to ipv4/ipv6
	hdr_type = rdma_gid_attr_network_type(sgid_attr);
	switch (hdr_type) {
	case RDMA_NETWORK_IPV4:
		printk("ipv4\n");break;
	case RDMA_NETWORK_IPV6:
		printk("ipv6\n");break;
	case RDMA_NETWORK_IB:
		printk("IB/Roce v1 \n");

	}

	BXROCE_PR("bxroce:gid : 0x%x \n",sgid_attr->gid);
	BXROCE_PR("bxroce:global:0x%x \n",sgid_attr->gid.global);

	if (hdr_type == RDMA_NETWORK_IPV4) {
			BXROCE_PR("bxroce: sgid to ipv4\n");
			rdma_gid2ip(&sgid_addr._sockaddr,&sgid_attr->gid);
			rdma_gid2ip(&dgid_addr._sockaddr,&grh->dgid);
			memcpy(&qp->dgid[0],&dgid_addr._sockaddr_in.sin_addr.s_addr,4);
			memcpy(&qp->sgid[0],&sgid_addr._sockaddr_in.sin_addr.s_addr,4);
			memcpy(&qp->qp_change_info->dgid[0],&qp->dgid[0],4);
			memcpy(&qp->qp_change_info->sgid[0],&qp->sgid[0],4);
	}


	return status;
}

 int bxroce_qp_state_change(struct bxroce_qp *qp, enum ib_qp_state new_ib_state, enum ib_qp_state *old_ib_state) {
	unsigned long flags;
	enum bxroce_qp_state new_state;
	new_state = get_bxroce_qp_state(new_ib_state);
	BXROCE_PR("bxroce:%s start \n",__func__);//added by hs

	spin_lock_irqsave(&qp->q_lock, flags);

	if(old_ib_state)
		*old_ib_state = get_ibqp_state(qp->qp_state);
	if (new_state == qp->qp_state) {
		spin_unlock_irqrestore(&qp->q_lock,flags);
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

	spin_unlock_irqrestore(&qp->q_lock, flags);
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
		qp->qp_change_info->pkey_index = qp->pkey_index;
	}
	if (attr_mask & IB_QP_QKEY) {
		qp->qkey = attrs->qkey;
		qp->qp_change_info->qkey = qp->qkey;
	}
	if (attr_mask & IB_QP_DEST_QPN) { // get dest qpn.
		qp->destqp = attrs->dest_qp_num;
		qp->qp_change_info->destqp = qp->destqp;
	}
	if (attr_mask & IB_QP_AV) {
		status = bxroce_set_av_params(qp,attrs,attr_mask);
	}
	else if (qp->qp_type == IB_QPT_GSI || qp->qp_type == IB_QPT_UD)
	{
		// hw have no place to resotre.by hs@20200501
		//memcpy(qp->mac_addr,dev->devinfo.netdev->dev_addr,ETH_ALEN);
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
		qp->init_sqpsn = attrs->sq_psn;
	}
	if (attr_mask & IB_QP_RQ_PSN) {
		printk("bxroce:rq_psn: %x \n",attrs->rq_psn);//added by hs
		qp->init_rqpsn = attrs->rq_psn;
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
