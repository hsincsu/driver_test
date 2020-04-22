#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>//insomnia@20200123

#include "header/bx_rnic.h"

MODULE_LICENSE("Dual BSD/GPL");

static int debug = 0;//-1
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "ethernet debug level (0=none,...,16=all)");
static const u32 default_msg_level = (NETIF_MSG_LINK | NETIF_MSG_IFDOWN | NETIF_MSG_IFUP);

static unsigned char dev_addr[6] = {0, 0x55, 0x7b, 0xb5, 0x59, 0x05};


static void mac_read_mac_addr(struct mac_pdata *pdata)
{
    struct net_device *netdev = pdata->netdev;
    
    RNIC_TRACE_PRINT();
    
    /* Currently it uses a static mac address for test */
    memcpy(pdata->mac_addr, dev_addr, netdev->addr_len);
}

static void mac_default_config(struct mac_pdata *pdata)
{   
    RNIC_TRACE_PRINT();
    
    pdata->tx_osp_mode  = DMA_OSP_ENABLE;
    pdata->tx_sf_mode   = MTL_TSF_DISABLE;//MTL_TSF_ENABLE; 
    pdata->rx_sf_mode   = MTL_RSF_DISABLE;
    pdata->pblx8        = DMA_PBL_X8_ENABLE;
    pdata->tx_pbl       = DMA_PBL_32;
    pdata->rx_pbl       = DMA_PBL_32;
    pdata->tx_threshold = MTL_TX_THRESHOLD_128;
    pdata->rx_threshold = MTL_RX_THRESHOLD_128;
    pdata->tx_pause     = 0;//1;
    pdata->rx_pause     = 0;//1;
    pdata->sysclk_rate  = MAC_SYSCLOCK;

    strlcpy(pdata->drv_name, DRIVER_NAME,    sizeof(pdata->drv_name));
    strlcpy(pdata->drv_ver,  DRIVER_VERSION, sizeof(pdata->drv_ver));
}

static void mac_init_all_ops(struct mac_pdata *pdata)
{   
    RNIC_TRACE_PRINT();
    
    mac_init_desc_ops(&pdata->desc_ops);
    mac_init_hw_ops(&pdata->hw_ops);
}

static int mac_init(struct mac_pdata *pdata)
{
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    struct net_device *netdev = pdata->netdev;
    unsigned int i;
    int ret;
    
    RNIC_TRACE_PRINT();

    //rnic_init(&pdata->rnic_pdata); //insomnia@20200205
    
    /* Set default configuration data */
    mac_default_config(pdata);

    /* Set irq, base_addr, MAC address, */
    netdev->irq = pdata->dev_irq;
    netdev->base_addr = (unsigned long)pdata->mac_regs;
    mac_read_mac_addr(pdata);
    memcpy(netdev->dev_addr, pdata->mac_addr, netdev->addr_len);

    /* Set all the function pointers */
    mac_init_all_ops(pdata);

    /* Issue software reset to device */
    hw_ops->exit(pdata);

    /* Populate the hardware features */
    mac_get_all_hw_features(pdata);
    mac_print_all_hw_features(pdata);

    /* TODO: Set the PHY mode to XLGMII */

    /* Set the DMA mask */
    ret = dma_set_mask_and_coherent(pdata->dev,
                    DMA_BIT_MASK(pdata->hw_feat.dma_width));
    if (ret) {
        dev_err(pdata->dev, "dma_set_mask_and_coherent failed\n");
        return ret;
    }

    /* Channel and ring params initializtion
     *  pdata->channel_count;
     *  pdata->tx_ring_count;
     *  pdata->rx_ring_count;
     *  pdata->tx_desc_count;
     *  pdata->rx_desc_count;
     */
    BUILD_BUG_ON_NOT_POWER_OF_2(MAC_TX_DESC_CNT);
    pdata->tx_desc_count = MAC_TX_DESC_CNT;
    if (pdata->tx_desc_count & (pdata->tx_desc_count - 1)) {
        dev_err(pdata->dev, "tx descriptor count (%d) is not valid\n",
            pdata->tx_desc_count);
        ret = -EINVAL;
        return ret;
    }
    BUILD_BUG_ON_NOT_POWER_OF_2(MAC_RX_DESC_CNT);
    pdata->rx_desc_count = MAC_RX_DESC_CNT;
    if (pdata->rx_desc_count & (pdata->rx_desc_count - 1)) {
        dev_err(pdata->dev, "rx descriptor count (%d) is not valid\n",
            pdata->rx_desc_count);
        ret = -EINVAL;
        return ret;
    }

    pdata->tx_ring_count = min_t(unsigned int, num_online_cpus(),
                     pdata->hw_feat.tx_ch_cnt);
    pdata->tx_ring_count = min_t(unsigned int, pdata->tx_ring_count,
                     pdata->hw_feat.tx_q_cnt);
    //insomnia@20200220
#ifdef RNIC_MSI_EN    
    pdata->tx_ring_count = min_t(unsigned int, pdata->tx_ring_count, pdata->msi_irq_cnt);
#endif
    pdata->tx_q_count = pdata->tx_ring_count;
    ret = netif_set_real_num_tx_queues(netdev, pdata->tx_q_count);
    if (ret) {
        dev_err(pdata->dev, "error setting real tx queue count\n");
        return ret;
    }

    pdata->rx_ring_count = min_t(unsigned int,
                     netif_get_num_default_rss_queues(),
                     pdata->hw_feat.rx_ch_cnt);
    pdata->rx_ring_count = min_t(unsigned int, pdata->rx_ring_count,
                     pdata->hw_feat.rx_q_cnt);
    pdata->rx_q_count = pdata->rx_ring_count;
    ret = netif_set_real_num_rx_queues(netdev, pdata->rx_q_count);
    if (ret) {
        dev_err(pdata->dev, "error setting real rx queue count\n");
        return ret;
    }

    pdata->channel_count =
        max_t(unsigned int, pdata->tx_ring_count, pdata->rx_ring_count);

    /* Initialize RSS hash key and lookup table */
    netdev_rss_key_fill(pdata->rss_key, sizeof(pdata->rss_key));

    for (i = 0; i < MAC_RSS_MAX_TABLE_SIZE; i++)
        pdata->rss_table[i] = MAC_SET_REG_BITS(
                    pdata->rss_table[i],
                    MAC_RSSDR_DMCH_POS,
                    MAC_RSSDR_DMCH_LEN,
                    i % pdata->rx_ring_count);

    pdata->rss_options = MAC_SET_REG_BITS(
                pdata->rss_options,
                MAC_RSSCR_IP2TE_POS,
                MAC_RSSCR_IP2TE_LEN, 1);
    pdata->rss_options = MAC_SET_REG_BITS(
                pdata->rss_options,
                MAC_RSSCR_TCP4TE_POS,
                MAC_RSSCR_TCP4TE_LEN, 1);
    pdata->rss_options = MAC_SET_REG_BITS(
                pdata->rss_options,
                MAC_RSSCR_UDP4TE_POS,
                MAC_RSSCR_UDP4TE_LEN, 1);

    /* Set device operations */
    netdev->netdev_ops = mac_get_netdev_ops();
    netdev->ethtool_ops = mac_get_ethtool_ops();

    /* Set device features */
    if (pdata->hw_feat.tso) {
        netdev->hw_features = NETIF_F_TSO;
        netdev->hw_features |= NETIF_F_TSO6;
        netdev->hw_features |= NETIF_F_SG;
        netdev->hw_features |= NETIF_F_IP_CSUM;
        netdev->hw_features |= NETIF_F_IPV6_CSUM;
    } else if (pdata->hw_feat.tx_coe) {
        netdev->hw_features = NETIF_F_IP_CSUM;
        netdev->hw_features |= NETIF_F_IPV6_CSUM;
    }

    if (pdata->hw_feat.rx_coe) {
        netdev->hw_features |= NETIF_F_RXCSUM;
        netdev->hw_features |= NETIF_F_GRO;
    }

    if (pdata->hw_feat.rss)
        netdev->hw_features |= NETIF_F_RXHASH;

    netdev->vlan_features |= netdev->hw_features;

    netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
    if (pdata->hw_feat.sa_vlan_ins)
        netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
    if (pdata->hw_feat.vlhash)
        netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;

    netdev->features |= netdev->hw_features;
    pdata->netdev_features = netdev->features;

    netdev->priv_flags |= IFF_UNICAST_FLT;

    /* Use default watchdog timeout */
    netdev->watchdog_timeo = 0;

    /* Tx coalesce parameters initialization */
    pdata->tx_usecs = MAC_INIT_DMA_TX_USECS;
    pdata->tx_frames = MAC_INIT_DMA_TX_FRAMES;

    /* Rx coalesce parameters initialization */
    pdata->rx_riwt = hw_ops->usec_to_riwt(pdata, MAC_INIT_DMA_RX_USECS);
    pdata->rx_usecs = MAC_INIT_DMA_RX_USECS;
    pdata->rx_frames = MAC_INIT_DMA_RX_FRAMES;


#ifdef RNIC_LEGACY_INT_EN
    pdata->ieu_clr_intr_usecs = IEU_CLR_INTR_TIMER_USECS;
#endif

//insomnia@20200213
#ifdef RNIC_MSI_EN
    //pdata->per_channel_irq = 0x1;
#endif    

    //insomnia@20200212
    pdata->link_check_usecs = PCS_LINK_CHECK_SLOW_TIMER_USECS;
    pdata->link_up = 2 ;

    //insomnia@20200221
    pdata->tx_pkg_cnt = 0;
    pdata->tx_times_cnt = 0;
    pdata->rx_pkg_cnt = 0;
    pdata->rx_times_cnt = 0;

    //insomnia@20200223
    pdata->tx_desc_update_cnt = 0;
    pdata->tx_desc_cur_last = 0;
    pdata->tx_desc_cur_update_total = 0;
    pdata->tx_desc_update_time_nsec_first = 0;
    pdata->tx_desc_update_time_nsec_last = 0;

    //insomnia@20200224
    pdata->an_restart_lock_usecs = PCS_AN_RESTART_LOCK_TIMER_USECS;

    RNIC_TRACE_PRINT();

    return 0;
}


int mac_drv_probe(struct pci_dev *pcidev, struct mac_resources *res)
{
    struct mac_pdata *pdata;
    struct net_device *netdev;
    int ret;

    //insomnia@20200213
    struct device *dev;    

    RNIC_TRACE_PRINT();

    //insomnia@20200213
    dev = &pcidev->dev;
    
    netdev = alloc_etherdev_mq(sizeof(struct mac_pdata),
                   MAC_MAX_DMA_CHANNELS);

    if (!netdev) {
        dev_err(dev, "alloc_etherdev failed\n");
        return -ENOMEM;
    }

    SET_NETDEV_DEV(netdev, dev);
    dev_set_drvdata(dev, netdev);
    pdata = netdev_priv(netdev);
    pdata->dev = dev;
    pdata->netdev = netdev;
    //insomnia@20200123
    pdata->pcidev = pcidev;

    pdata->dev_irq = res->irq;
    //insomnia@20200128
    pdata->mac_regs = res->addr + RNIC_REG_BASE_ADDR_MAC_0;
    
    pdata->rnic_pdata.pcie_bar_addr = res->addr;
    pdata->rnic_pdata.msi_irq_cnt     = res->msi_irq_cnt;
    pdata->rnic_pdata.mac_pdata     = pdata;

    //insomnia@20200213
#ifdef RNIC_MSI_EN
    pdata->msi_irq_cnt = res->msi_irq_cnt;

    //for(i=0;i<MAC_MAX_DMA_CHANNELS;i++)
    //    pdata->channel_irq[i] = res->irq+i*(pdata->msi_irq_cnt>1);
#endif

    mutex_init(&pdata->rss_mutex);
    pdata->msg_enable = netif_msg_init(debug, default_msg_level);
    
    ret = mac_init(pdata);
    if (ret) {
        dev_err(dev, "mac init failed\n");
        goto err_free_netdev;
    }

    ret = register_netdev(netdev);
    if (ret) {
        dev_err(dev, "net device registration failed\n");
        goto err_free_netdev;
    }

	//added by hs@20200418
	printk("bxrnic: register dev start\n");//added by hs
	ret = mac_register_dev(pdata);
	if (ret) {
		dev_err(dev,"register dev failed \n");
		goto err_free_netdev;
	}
	printk("bxrnic: register dev succ\n");//added by hs
    RNIC_TRACE_PRINT();
    
    return 0;

err_free_netdev:
    free_netdev(netdev);

    return ret;
}

int mac_drv_remove(struct pci_dev *pcidev)
{
    struct device *dev = &pcidev->dev;
    struct net_device *netdev = dev_get_drvdata(dev);

	//added by hs@20200418
    struct mac_pdata *pdata =  netdev_priv(netdev);
    
	RNIC_TRACE_PRINT();
    
	//added by hs@20200418
	mac_unregister_dev(pdata);

    unregister_netdev(netdev);
    free_netdev(netdev);

    return 0;
}


void mac_dump_tx_desc(struct mac_pdata *pdata,
             struct mac_ring *ring,
             unsigned int idx,
             unsigned int count,
             unsigned int flag)
{
    struct mac_desc_data *desc_data;
    struct mac_dma_desc *dma_desc;

    while (count--) {
        desc_data = MAC_GET_DESC_DATA(ring, idx);
        dma_desc = desc_data->dma_desc;

        netdev_dbg(pdata->netdev, "TX: dma_desc=%p, dma_desc_addr=%pad\n",
               desc_data->dma_desc, &desc_data->dma_desc_addr);
        netdev_dbg(pdata->netdev,
               "TX_NORMAL_DESC[%d %s] = %08x:%08x:%08x:%08x\n", idx,
               (flag == 1) ? "QUEUED FOR TX" : "TX BY DEVICE",
               le32_to_cpu(dma_desc->desc0),
               le32_to_cpu(dma_desc->desc1),
               le32_to_cpu(dma_desc->desc2),
               le32_to_cpu(dma_desc->desc3));

        idx++;
    }
}

void mac_dump_rx_desc(struct mac_pdata *pdata,
             struct mac_ring *ring,
             unsigned int idx)
{
    struct mac_desc_data *desc_data;
    struct mac_dma_desc *dma_desc;
    
    RNIC_TRACE_PRINT();
    
    desc_data = MAC_GET_DESC_DATA(ring, idx);
    dma_desc = desc_data->dma_desc;

    netdev_dbg(pdata->netdev, "RX: dma_desc=%p, dma_desc_addr=%pad\n",
           desc_data->dma_desc, &desc_data->dma_desc_addr);
    netdev_dbg(pdata->netdev,
           "RX_NORMAL_DESC[%d RX BY DEVICE] = %08x:%08x:%08x:%08x\n",
           idx,
           le32_to_cpu(dma_desc->desc0),
           le32_to_cpu(dma_desc->desc1),
           le32_to_cpu(dma_desc->desc2),
           le32_to_cpu(dma_desc->desc3));
}

void mac_print_pkt(struct net_device *netdev,
              struct sk_buff *skb, bool tx_rx)
{
    struct ethhdr *eth = (struct ethhdr *)skb->data;
    unsigned char buffer[128];
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    netdev_dbg(netdev, "\n************** SKB dump ****************\n");

    netdev_dbg(netdev, "%s packet of %d bytes\n",
           (tx_rx ? "TX" : "RX"), skb->len);

    netdev_dbg(netdev, "Dst MAC addr: %pM\n", eth->h_dest);
    netdev_dbg(netdev, "Src MAC addr: %pM\n", eth->h_source);
    netdev_dbg(netdev, "Protocol: %#06hx\n", ntohs(eth->h_proto));

    for (i = 0; i < skb->len; i += 32) {
        unsigned int len = min(skb->len - i, 32U);

        hex_dump_to_buffer(&skb->data[i], len, 32, 1,
                   buffer, sizeof(buffer), false);
        netdev_dbg(netdev, "  %#06x: %s\n", i, buffer);
    }

    netdev_dbg(netdev, "\n************** SKB dump ****************\n");
}

void mac_get_all_hw_features(struct mac_pdata *pdata)
{
    struct mac_hw_features *hw_feat = &pdata->hw_feat;
    unsigned int mac_hfr0, mac_hfr1, mac_hfr2;
    
    RNIC_TRACE_PRINT();
    
    mac_hfr0 = readl(pdata->mac_regs + MAC_HWF0R);
    mac_hfr1 = readl(pdata->mac_regs + MAC_HWF1R);
    mac_hfr2 = readl(pdata->mac_regs + MAC_HWF2R);

    memset(hw_feat, 0, sizeof(*hw_feat));

    hw_feat->version = readl(pdata->mac_regs + MAC_VR);

    /* Hardware feature register 0 */
    hw_feat->phyifsel    = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_PHYIFSEL_POS,
                        MAC_HWF0R_PHYIFSEL_LEN);
    hw_feat->vlhash      = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_VLHASH_POS,
                        MAC_HWF0R_VLHASH_LEN);
    hw_feat->sma         = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_SMASEL_POS,
                        MAC_HWF0R_SMASEL_LEN);
    hw_feat->rwk         = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_RWKSEL_POS,
                        MAC_HWF0R_RWKSEL_LEN);
    hw_feat->mgk         = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_MGKSEL_POS,
                        MAC_HWF0R_MGKSEL_LEN);
    hw_feat->mmc         = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_MMCSEL_POS,
                        MAC_HWF0R_MMCSEL_LEN);
    hw_feat->aoe         = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_ARPOFFSEL_POS,
                        MAC_HWF0R_ARPOFFSEL_LEN);
    hw_feat->ts          = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_TSSEL_POS,
                        MAC_HWF0R_TSSEL_LEN);
    hw_feat->eee         = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_EEESEL_POS,
                        MAC_HWF0R_EEESEL_LEN);
    hw_feat->tx_coe      = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_TXCOESEL_POS,
                        MAC_HWF0R_TXCOESEL_LEN);
    hw_feat->rx_coe      = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_RXCOESEL_POS,
                        MAC_HWF0R_RXCOESEL_LEN);
    hw_feat->addn_mac    = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_ADDMACADRSEL_POS,
                        MAC_HWF0R_ADDMACADRSEL_LEN);
    hw_feat->ts_src      = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_TSSTSSEL_POS,
                        MAC_HWF0R_TSSTSSEL_LEN);
    hw_feat->sa_vlan_ins = MAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_SAVLANINS_POS,
                        MAC_HWF0R_SAVLANINS_LEN);

    /* Hardware feature register 1 */
    hw_feat->rx_fifo_size  = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_RXFIFOSIZE_POS,
                        MAC_HWF1R_RXFIFOSIZE_LEN);
    hw_feat->tx_fifo_size  = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_TXFIFOSIZE_POS,
                        MAC_HWF1R_TXFIFOSIZE_LEN);
    hw_feat->adv_ts_hi     = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_ADVTHWORD_POS,
                        MAC_HWF1R_ADVTHWORD_LEN);
    hw_feat->dma_width     = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_ADDR64_POS,
                        MAC_HWF1R_ADDR64_LEN);
    hw_feat->dcb           = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_DCBEN_POS,
                        MAC_HWF1R_DCBEN_LEN);
    hw_feat->sph           = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_SPHEN_POS,
                        MAC_HWF1R_SPHEN_LEN);
    hw_feat->tso           = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_TSOEN_POS,
                        MAC_HWF1R_TSOEN_LEN);
    hw_feat->dma_debug     = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_DBGMEMA_POS,
                        MAC_HWF1R_DBGMEMA_LEN);
    hw_feat->rss           = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_RSSEN_POS,
                        MAC_HWF1R_RSSEN_LEN);
    hw_feat->tc_cnt        = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_NUMTC_POS,
                        MAC_HWF1R_NUMTC_LEN);
    hw_feat->hash_table_size = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_HASHTBLSZ_POS,
                        MAC_HWF1R_HASHTBLSZ_LEN);
    hw_feat->l3l4_filter_num = MAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_L3L4FNUM_POS,
                        MAC_HWF1R_L3L4FNUM_LEN);

    /* Hardware feature register 2 */
    hw_feat->rx_q_cnt     = MAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_RXQCNT_POS,
                        MAC_HWF2R_RXQCNT_LEN);
    hw_feat->tx_q_cnt     = MAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_TXQCNT_POS,
                        MAC_HWF2R_TXQCNT_LEN);
    hw_feat->rx_ch_cnt    = MAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_RXCHCNT_POS,
                        MAC_HWF2R_RXCHCNT_LEN);
    hw_feat->tx_ch_cnt    = MAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_TXCHCNT_POS,
                        MAC_HWF2R_TXCHCNT_LEN);
    hw_feat->pps_out_num  = MAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_PPSOUTNUM_POS,
                        MAC_HWF2R_PPSOUTNUM_LEN);
    hw_feat->aux_snap_num = MAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_AUXSNAPNUM_POS,
                        MAC_HWF2R_AUXSNAPNUM_LEN);
                        

    /* Translate the Hash Table size into actual number */
    switch (hw_feat->hash_table_size) {
    case 0:
        break;
    case 1:
        hw_feat->hash_table_size = 64;
        break;
    case 2:
        hw_feat->hash_table_size = 128;
        break;
    case 3:
        hw_feat->hash_table_size = 256;
        break;
    }

    /* Translate the address width setting into actual number */
    switch (hw_feat->dma_width) {
    case 0:
        hw_feat->dma_width = 32;
        break;
    case 1:
        hw_feat->dma_width = 40;
        break;
    case 2:
        hw_feat->dma_width = 48;
        break;
    default:
        hw_feat->dma_width = 32;
    }

    /* The Queue, Channel and TC counts are zero based so increment them
     * to get the actual number
     */
    hw_feat->rx_q_cnt++;
    hw_feat->tx_q_cnt++;
    hw_feat->rx_ch_cnt++;
    hw_feat->tx_ch_cnt++;
    hw_feat->tc_cnt++;

#ifdef CUST_CHANNEL_NUM
    //insomnia
    hw_feat->rx_q_cnt=CUST_CHANNEL_NUM;
    hw_feat->tx_q_cnt=CUST_CHANNEL_NUM;
    hw_feat->rx_ch_cnt=CUST_CHANNEL_NUM;
    hw_feat->tx_ch_cnt=CUST_CHANNEL_NUM;
    hw_feat->tc_cnt=CUST_CHANNEL_NUM;
    //hw_feat->sph = 0;
    //hw_feat->tso = 0;
    //hw_feat->tx_fifo_size  = 0x3ff;
#endif

}

void mac_print_all_hw_features(struct mac_pdata *pdata)
{
    char *str = NULL;
    
    RNIC_TRACE_PRINT();
    
    RNIC_PRINTK("\n");
    RNIC_PRINTK("=====================================================\n");
    RNIC_PRINTK("\n");
    RNIC_PRINTK("HW support following features\n");
    RNIC_PRINTK("\n");
    /* HW Feature Register0 */
    RNIC_PRINTK("VLAN Hash Filter Selected                   : %s\n",
          pdata->hw_feat.vlhash ? "YES" : "NO");
    RNIC_PRINTK("SMA (MDIO) Interface                        : %s\n",
          pdata->hw_feat.sma ? "YES" : "NO");
    RNIC_PRINTK("PMT Remote Wake-up Packet Enable            : %s\n",
          pdata->hw_feat.rwk ? "YES" : "NO");
    RNIC_PRINTK("PMT Magic Packet Enable                     : %s\n",
          pdata->hw_feat.mgk ? "YES" : "NO");
    RNIC_PRINTK("RMON/MMC Module Enable                      : %s\n",
          pdata->hw_feat.mmc ? "YES" : "NO");
    RNIC_PRINTK("ARP Offload Enabled                         : %s\n",
          pdata->hw_feat.aoe ? "YES" : "NO");
    RNIC_PRINTK("IEEE 1588-2008 Timestamp Enabled            : %s\n",
          pdata->hw_feat.ts ? "YES" : "NO");
    RNIC_PRINTK("Energy Efficient Ethernet Enabled           : %s\n",
          pdata->hw_feat.eee ? "YES" : "NO");
    RNIC_PRINTK("Transmit Checksum Offload Enabled           : %s\n",
          pdata->hw_feat.tx_coe ? "YES" : "NO");
    RNIC_PRINTK("Receive Checksum Offload Enabled            : %s\n",
          pdata->hw_feat.rx_coe ? "YES" : "NO");
    RNIC_PRINTK("Additional MAC Addresses 1-31 Selected      : %s\n",
          pdata->hw_feat.addn_mac ? "YES" : "NO");

    switch (pdata->hw_feat.ts_src) {
    case 0:
        str = "RESERVED";
        break;
    case 1:
        str = "INTERNAL";
        break;
    case 2:
        str = "EXTERNAL";
        break;
    case 3:
        str = "BOTH";
        break;
    }
    RNIC_PRINTK("Timestamp System Time Source                : %s\n", str);

    RNIC_PRINTK("Source Address or VLAN Insertion Enable     : %s\n",
          pdata->hw_feat.sa_vlan_ins ? "YES" : "NO");

    /* HW Feature Register1 */
    switch (pdata->hw_feat.rx_fifo_size) {
    case 0:
        str = "128 bytes";
        break;
    case 1:
        str = "256 bytes";
        break;
    case 2:
        str = "512 bytes";
        break;
    case 3:
        str = "1 KBytes";
        break;
    case 4:
        str = "2 KBytes";
        break;
    case 5:
        str = "4 KBytes";
        break;
    case 6:
        str = "8 KBytes";
        break;
    case 7:
        str = "16 KBytes";
        break;
    case 8:
        str = "32 kBytes";
        break;
    case 9:
        str = "64 KBytes";
        break;
    case 10:
        str = "128 KBytes";
        break;
    case 11:
        str = "256 KBytes";
        break;
    default:
        str = "RESERVED";
    }
    RNIC_PRINTK("MTL Receive FIFO Size                       : %s\n", str);

    switch (pdata->hw_feat.tx_fifo_size) {
    case 0:
        str = "128 bytes";
        break;
    case 1:
        str = "256 bytes";
        break;
    case 2:
        str = "512 bytes";
        break;
    case 3:
        str = "1 KBytes";
        break;
    case 4:
        str = "2 KBytes";
        break;
    case 5:
        str = "4 KBytes";
        break;
    case 6:
        str = "8 KBytes";
        break;
    case 7:
        str = "16 KBytes";
        break;
    case 8:
        str = "32 kBytes";
        break;
    case 9:
        str = "64 KBytes";
        break;
    case 10:
        str = "128 KBytes";
        break;
    case 11:
        str = "256 KBytes";
        break;
    default:
        str = "RESERVED";
    }
    RNIC_PRINTK("MTL Transmit FIFO Size                      : %s\n", str);

    RNIC_PRINTK("IEEE 1588 High Word Register Enable         : %s\n",
          pdata->hw_feat.adv_ts_hi ? "YES" : "NO");
    RNIC_PRINTK("Address width                               : %u\n",
          pdata->hw_feat.dma_width);
    RNIC_PRINTK("DCB Feature Enable                          : %s\n",
          pdata->hw_feat.dcb ? "YES" : "NO");
    RNIC_PRINTK("Split Header Feature Enable                 : %s\n",
          pdata->hw_feat.sph ? "YES" : "NO");
    RNIC_PRINTK("TCP Segmentation Offload Enable             : %s\n",
          pdata->hw_feat.tso ? "YES" : "NO");
    RNIC_PRINTK("DMA Debug Registers Enabled                 : %s\n",
          pdata->hw_feat.dma_debug ? "YES" : "NO");
    RNIC_PRINTK("RSS Feature Enabled                         : %s\n",
          pdata->hw_feat.rss ? "YES" : "NO");
    RNIC_PRINTK("Number of Traffic classes                   : %u\n",
          (pdata->hw_feat.tc_cnt));
    RNIC_PRINTK("Hash Table Size                             : %u\n",
          pdata->hw_feat.hash_table_size);
    RNIC_PRINTK("Total number of L3 or L4 Filters            : %u\n",
          pdata->hw_feat.l3l4_filter_num);

    /* HW Feature Register2 */
    RNIC_PRINTK("Number of MTL Receive Queues                : %u\n",
          pdata->hw_feat.rx_q_cnt);
    RNIC_PRINTK("Number of MTL Transmit Queues               : %u\n",
          pdata->hw_feat.tx_q_cnt);
    RNIC_PRINTK("Number of DMA Receive Channels              : %u\n",
          pdata->hw_feat.rx_ch_cnt);
    RNIC_PRINTK("Number of DMA Transmit Channels             : %u\n",
          pdata->hw_feat.tx_ch_cnt);

    switch (pdata->hw_feat.pps_out_num) {
    case 0:
        str = "No PPS output";
        break;
    case 1:
        str = "1 PPS output";
        break;
    case 2:
        str = "2 PPS output";
        break;
    case 3:
        str = "3 PPS output";
        break;
    case 4:
        str = "4 PPS output";
        break;
    default:
        str = "RESERVED";
    }
    RNIC_PRINTK("Number of PPS Outputs                       : %s\n", str);

    switch (pdata->hw_feat.aux_snap_num) {
    case 0:
        str = "No auxiliary input";
        break;
    case 1:
        str = "1 auxiliary input";
        break;
    case 2:
        str = "2 auxiliary input";
        break;
    case 3:
        str = "3 auxiliary input";
        break;
    case 4:
        str = "4 auxiliary input";
        break;
    default:
        str = "RESERVED";
    }
    RNIC_PRINTK("Number of Auxiliary Snapshot Inputs         : %s", str);

    RNIC_PRINTK("\n");
    RNIC_PRINTK("=====================================================\n");
    RNIC_PRINTK("\n");
}
