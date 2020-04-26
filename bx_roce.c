/*
 *  
 *   
 *    
 *           This is the roce function file in mail driver.I need define some really important struct here#=!
 *      
 *       
 *        
 *         
 *          
 *                                                                 ---------edited by hs in 2019/9/19                                  
 */

#include <linux/mutex.h>
#include <linux/list.h>
//#include <linux/netdevice.h>
#include <linux/module.h>

#include "header/bx_rnic.h"
static LIST_HEAD(bxpdata_list);
/* a list for pdata and bxintf so that bx_roce can access the pdata in bx-xlgmac driver
 *                              --edied by hs                   
 */
struct bxroce_driver *bxroce;


/* mac_add_device this funtion add roce driver to bx_mac
 *
 *                                      --edited by hs
 */
static void mac_add_device(struct mac_pdata *pdata)
{
	printk("bxrnic: mac add \n");//added by hs
	struct bx_dev_info dev_info;
	dev_info.pdata = pdata;
	dev_info.rnic_pdata = &pdata->rnic_pdata;
	dev_info.netdev = pdata->netdev;
	dev_info.dev = pdata->dev;
	dev_info.dev_irq = pdata->dev_irq;
	dev_info.pcidev = pdata->pcidev;
	dev_info.mac_base = pdata->mac_regs;
	dev_info.base_addr = pdata->rnic_pdata.pcie_bar_addr;
	dev_info.channel_head = pdata->channel_head;
	dev_info.channel_count = pdata->channel_count;
	memcpy(dev_info.mac_addr, pdata->netdev->dev_addr, ETH_ALEN);	
	pdata->rocedev = bxroce->add(&dev_info);
	
}

/*mac_del_device(struct mac_pdata *pdata)
 *
 */
static void mac_del_device(struct mac_pdata *pdata)
{
	bxroce->remove(pdata->rocedev);
	pdata->rocedev =NULL;
}

//added by hs@20200416
/* xlgmac_register_dev register pdata struct into bxpdata_list
 * @xlgmac_pdata *pdata which have some private data about the nic.
 *                                          --edited by hs 
 */
int mac_register_dev(struct mac_pdata *pdata)
{
       printk("mac_register_dev \n");//added by hs
	   list_add_tail(&pdata->list,&bxpdata_list); // register pdata to bxpdata_list
	   printk("mac_register_dev \n");//added by hs
	   return 0;
}

/* xlgmac_unregister_dev unregister pdata from bxpdata_list
 *
 */
int mac_unregister_dev(struct mac_pdata *pdata)
{
	list_del(&pdata->list);
	return 0;
}
//end


/* xlgmac_register_interface register interface to bx-xlgmac driver
 * @xlgmac_interface a interface structure
 *                                        --edited by hs 
 */
int bx_roce_register_driver(struct bxroce_driver *drv)
{
        struct mac_pdata *pdata;

		bxroce = drv;
        list_for_each_entry(pdata,&bxpdata_list,list)
              mac_add_device(pdata);
		printk("bxrnic:bx_roce_register_driver \n");
        return 0;
}
EXPORT_SYMBOL(bx_roce_register_driver);

/*bx_roce_unregister_driver(struct bxroce_driver *drv)
 *
 */
void bx_roce_unregister_driver(struct bxroce_driver *drv)
{
	struct mac_pdata *pdata;
	
	bxroce = drv;
	list_for_each_entry(pdata,&bxpdata_list,list)
		mac_del_device(pdata);
}
EXPORT_SYMBOL(bx_roce_unregister_driver);
