#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>


#include "header/bx_rnic.h"

static int rnic_probe(struct pci_dev *pcidev, const struct pci_device_id *id)
{
    struct device *dev = &pcidev->dev;
    struct mac_resources res;
    int i, ret;
    
    RNIC_TRACE_PRINT();
    ret = pcim_enable_device(pcidev);
    if (ret) {
        dev_err(dev, "ERROR: failed to enable device\n");
        return ret;
    }

    for (i = 0; i <= PCI_STD_RESOURCE_END; i++) {
        if (pci_resource_len(pcidev, i) == 0)
            continue;
        ret = pcim_iomap_regions(pcidev, BIT(i), DRIVER_NAME);
        if (ret)
            return ret;
        break;
    }

    pci_set_master(pcidev);

    memset(&res, 0, sizeof(res));

    //default
    res.msi_irq_cnt = 1;

#ifdef RNIC_MSI_EN
    ret=pci_alloc_irq_vectors_affinity(pcidev,RNIC_MSI_REQ_IRQ_CNT,RNIC_MSI_REQ_IRQ_CNT,PCI_IRQ_MSI,NULL);

    if(ret < 0)
    {
        RNIC_PRINTK("RNIC: Cannot alloc %d msi irq\n",RNIC_MSI_REQ_IRQ_CNT);
        
        ret=pci_alloc_irq_vectors_affinity(pcidev,1,1,PCI_IRQ_MSI,NULL);
        
        if(ret <= 0)
            return ret;//error
    }

    res.msi_irq_cnt = ret;
    
    RNIC_PRINTK("RNIC: msi_irq_cnt is %d\n",ret);
#endif

    res.irq = pcidev->irq;
    res.addr = pcim_iomap_table(pcidev)[i];

#ifdef SRIOV_EN
    pci_enable_sriov(pcidev, 2);
    printk("pci_enable_sriov\n");
#endif

    return mac_drv_probe(pcidev, &res);
}

static void rnic_remove(struct pci_dev *pcidev)
{   
    RNIC_TRACE_PRINT();
    
    mac_drv_remove(pcidev);
}

static const struct pci_device_id rnic_pci_tbl[] = {
    { PCI_DEVICE(RNIC_PCI_VENDOR_ID, RNIC_PCI_DEVICE_ID) },
    { 0 }
};
MODULE_DEVICE_TABLE(pci, rnic_pci_tbl);

static struct pci_driver rnic_pci_driver = {
    .name       = DRIVER_NAME,
    .id_table   = rnic_pci_tbl,
    .probe      = rnic_probe,
    .remove     = rnic_remove,
};

module_pci_driver(rnic_pci_driver);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR("insomnia <insomnia@binxin-tech.com>");
MODULE_LICENSE("Dual BSD/GPL");