/* *
 *  *
 *   *
 *    *
 *     *      This is a header file for bx driver.It connect bx driver and bxroce driver
 *      *
 *       *
 *        *                                                                      ---------------edited by hs in 2019/9/18
 *         *
 *          *
 *           *
 * * * * * *  */

#ifndef BX_ROCE_H
#define BX_ROCE_H

#include<linux/pci.h>
#include<linux/netdevice.h>
#include<linux/phy.h>
//added by hs@20200416
struct bxroce_dev;
///* end */

struct bx_dev_info {

       // u8 mac_addr[ETH_ALEN];
		struct mac_pdata	*pdata;
		struct rnic_pdata   *rnic_pdata;
        struct device		*dev;
        struct net_device	*netdev;
		struct mac_channel	*channel_head;
		int					channel_count;
		struct pci_dev		*pcidev;
		/*mac registers base addr*/
		void __iomem		*mac_base;
		void __iomem		*base_addr;
		int					phy_speed;
		u8					mac_addr[ETH_ALEN];
		int					dev_irq;

      
};


struct bxroce_driver{
        unsigned char name[32];
        struct bxroce_dev *(*add) (struct bx_dev_info *dev_info);
        void (*remove) (struct bxroce_dev *);
		struct bxroce_dev *dev;//To pass the dev for debuging,later change  hs 2019/6/20
};

//added by hs@20200416
int mac_register_dev(struct mac_pdata *pdata);
int mac_unregister_dev(struct mac_pdata *pdata);
int bx_roce_register_driver(struct bxroce_driver *drv);//add this in bx_roce.c
void bx_roce_unregister_driver(struct bxroce_driver *drv);

#endif

