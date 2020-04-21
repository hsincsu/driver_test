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
	dev_info.netdev = pdata->netdev;
	dev_info.dev = pdata->dev;
	dev_info.dev_irq = pdata->dev_irq;
	dev_info.pcidev = pdata->pcidev;
	dev_info.mac_base = pdata->mac_regs;
	dev_info.base_addr = pdata->rnic_pdata.pcie_bar_addr;
	dev_info.channel_head = pdata->channel_head;
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
