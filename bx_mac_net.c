

#include <linux/netdevice.h>
#include <linux/tcp.h>
#include <linux/interrupt.h>
#include <linux/pci.h> //insomnia@20200213

#include "header/bx_rnic.h"

static int mac_one_poll(struct napi_struct *, int);
static int mac_all_poll(struct napi_struct *, int);

static inline unsigned int mac_tx_avail_desc(struct mac_ring *ring)
{   
    RNIC_TRACE_PRINT();
    
    return (ring->dma_desc_count - (ring->cur - ring->dirty));
}

static inline unsigned int mac_rx_dirty_desc(struct mac_ring *ring)
{   
    RNIC_TRACE_PRINT();
    
    return (ring->cur - ring->dirty);
}

static int mac_maybe_stop_tx_queue(
            struct mac_channel *channel,
            struct mac_ring *ring,
            unsigned int count)
{

    struct mac_pdata *pdata = channel->pdata;
    
    RNIC_TRACE_PRINT();
    
    if (count > mac_tx_avail_desc(ring)) {
	//insomnia@20200211
	//printk("Tx queue stopped, not enough descriptors available!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        netif_info(pdata, drv, pdata->netdev,
               "Tx queue stopped, not enough descriptors available\n");
        netif_stop_subqueue(pdata->netdev, channel->queue_index);
        ring->tx.queue_stopped = 1;

        /* If we haven't notified the hardware because of xmit_more
         * support, tell it now
         */
        if (ring->tx.xmit_more)
            pdata->hw_ops.tx_start_xmit(channel, ring);

        return NETDEV_TX_BUSY;
    }

    return 0;
}

static void mac_prep_vlan(struct sk_buff *skb,
                 struct mac_pkt_info *pkt_info)
{
    
    RNIC_TRACE_PRINT();
    
    if (skb_vlan_tag_present(skb))
        pkt_info->vlan_ctag = skb_vlan_tag_get(skb);
}

static int mac_prep_tso(struct sk_buff *skb,
               struct mac_pkt_info *pkt_info)
{
    int ret;
    
    RNIC_TRACE_PRINT();
    
    if (!MAC_GET_REG_BITS(pkt_info->attributes,
                 TX_PACKET_ATTRIBUTES_TSO_ENABLE_POS,
                 TX_PACKET_ATTRIBUTES_TSO_ENABLE_LEN))
        return 0;

    ret = skb_cow_head(skb, 0);
    if (ret)
        return ret;

    pkt_info->header_len = skb_transport_offset(skb) + tcp_hdrlen(skb);
    pkt_info->tcp_header_len = tcp_hdrlen(skb);
    pkt_info->tcp_payload_len = skb->len - pkt_info->header_len;
    pkt_info->mss = skb_shinfo(skb)->gso_size;

    RNIC_PRINTK("header_len=%u\n", pkt_info->header_len);
    RNIC_PRINTK("tcp_header_len=%u, tcp_payload_len=%u\n",
          pkt_info->tcp_header_len, pkt_info->tcp_payload_len);
    RNIC_PRINTK("mss=%u\n", pkt_info->mss);

    /* Update the number of packets that will ultimately be transmitted
     * along with the extra bytes for each extra packet
     */
    pkt_info->tx_packets = skb_shinfo(skb)->gso_segs;
    pkt_info->tx_bytes += (pkt_info->tx_packets - 1) * pkt_info->header_len;

    return 0;
}

static int mac_is_tso(struct sk_buff *skb)
{
    RNIC_TRACE_PRINT();
    
    if (skb->ip_summed != CHECKSUM_PARTIAL)
        return 0;

    if (!skb_is_gso(skb))
        return 0;

    return 1;
}

static void mac_prep_tx_pkt(struct mac_pdata *pdata,
                   struct mac_ring *ring,
                   struct sk_buff *skb,
                   struct mac_pkt_info *pkt_info)
{
    skb_frag_t *frag;
    unsigned int context_desc;
    unsigned int len;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    pkt_info->skb = skb;

    context_desc = 0;
    pkt_info->desc_count = 0;

    pkt_info->tx_packets = 1;
    pkt_info->tx_bytes = skb->len;

    if (mac_is_tso(skb)) {
        /* TSO requires an extra descriptor if mss is different */
        if (skb_shinfo(skb)->gso_size != ring->tx.cur_mss) {
            context_desc = 1;
            pkt_info->desc_count++;
        }

        /* TSO requires an extra descriptor for TSO header */
        pkt_info->desc_count++;

        pkt_info->attributes = MAC_SET_REG_BITS(
                    pkt_info->attributes,
                    TX_PACKET_ATTRIBUTES_TSO_ENABLE_POS,
                    TX_PACKET_ATTRIBUTES_TSO_ENABLE_LEN,
                    1);
        pkt_info->attributes = MAC_SET_REG_BITS(
                    pkt_info->attributes,
                    TX_PACKET_ATTRIBUTES_CSUM_ENABLE_POS,
                    TX_PACKET_ATTRIBUTES_CSUM_ENABLE_LEN,
                    1);
    } else if (skb->ip_summed == CHECKSUM_PARTIAL)
        pkt_info->attributes = MAC_SET_REG_BITS(
                    pkt_info->attributes,
                    TX_PACKET_ATTRIBUTES_CSUM_ENABLE_POS,
                    TX_PACKET_ATTRIBUTES_CSUM_ENABLE_LEN,
                    1);

    if (skb_vlan_tag_present(skb)) {
        /* VLAN requires an extra descriptor if tag is different */
        if (skb_vlan_tag_get(skb) != ring->tx.cur_vlan_ctag)
            /* We can share with the TSO context descriptor */
            if (!context_desc) {
                context_desc = 1;
                pkt_info->desc_count++;
            }

        pkt_info->attributes = MAC_SET_REG_BITS(
                    pkt_info->attributes,
                    TX_PACKET_ATTRIBUTES_VLAN_CTAG_POS,
                    TX_PACKET_ATTRIBUTES_VLAN_CTAG_LEN,
                    1);
    }

    for (len = skb_headlen(skb); len;) {
        pkt_info->desc_count++;
        len -= min_t(unsigned int, len, MAC_TX_MAX_BUF_SIZE);
    }

    for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
        frag = &skb_shinfo(skb)->frags[i];
        for (len = skb_frag_size(frag); len; ) {
            pkt_info->desc_count++;
            len -= min_t(unsigned int, len, MAC_TX_MAX_BUF_SIZE);
        }
    }
}

static int mac_calc_rx_buf_size(struct net_device *netdev, unsigned int mtu)
{
    unsigned int rx_buf_size;
    
    RNIC_TRACE_PRINT();
    
    if (mtu > MAC_JUMBO_PACKET_MTU) {
        netdev_alert(netdev, "MTU exceeds maximum supported value\n");
        return -EINVAL;
    }

    rx_buf_size = mtu + ETH_HLEN + ETH_FCS_LEN + VLAN_HLEN;
    rx_buf_size = clamp_val(rx_buf_size, MAC_RX_MIN_BUF_SIZE, PAGE_SIZE);

    rx_buf_size = (rx_buf_size + MAC_RX_BUF_ALIGN - 1) &
              ~(MAC_RX_BUF_ALIGN - 1);

    return rx_buf_size;
}

static void mac_enable_rx_tx_ints(struct mac_pdata *pdata)
{
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    struct mac_channel *channel;
    enum mac_int int_id;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
        
    
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (channel->tx_ring && channel->rx_ring)
            int_id = MAC_INT_DMA_CH_SR_TI_RI;
        else if (channel->tx_ring)
            int_id = MAC_INT_DMA_CH_SR_TI;
        else if (channel->rx_ring)
            int_id = MAC_INT_DMA_CH_SR_RI;
        else
            continue;

        hw_ops->enable_int(channel, int_id);
    }

    //insomnia@20200211
    ieu_enable_intr(&pdata->rnic_pdata);
}

static void mac_disable_rx_tx_ints(struct mac_pdata *pdata)
{
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    struct mac_channel *channel;
    enum mac_int int_id;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (channel->tx_ring && channel->rx_ring)
            int_id = MAC_INT_DMA_CH_SR_TI_RI;
        else if (channel->tx_ring)
            int_id = MAC_INT_DMA_CH_SR_TI;
        else if (channel->rx_ring)
            int_id = MAC_INT_DMA_CH_SR_RI;
        else
            continue;

        hw_ops->disable_int(channel, int_id);
    }
	
    //insomnia@20200211
    ieu_disable_intr(&pdata->rnic_pdata);    
}

static irqreturn_t mac_isr(int irq, void *data)
{
    unsigned int dma_isr, dma_ch_isr, mac_isr;
    struct mac_pdata *pdata = data;
    struct mac_channel *channel;
    struct mac_hw_ops *hw_ops;
    unsigned int i, ti, ri;
    
    RNIC_TRACE_PRINT();

    hw_ops = &pdata->hw_ops;

    /* The DMA interrupt status register also reports MAC and MTL
     * interrupts. So for polling mode, we just need to check for
     * this register to be non-zero
     */

    dma_isr = readl(pdata->mac_regs + DMA_ISR);

    //printk("===============================================dma_isr is %x=====================================================\n",dma_isr);

#ifndef RNIC_MSI_EN	
    if (!dma_isr)
        return IRQ_HANDLED;

    netif_dbg(pdata, intr, pdata->netdev, "DMA_ISR=%#010x\n", dma_isr);
#endif

    for (i = 0; i < pdata->channel_count; i++) {
		
#ifndef RNIC_MSI_EN	
        if (!(dma_isr & (1 << i)))
            continue;
#endif
        channel = pdata->channel_head + i;

        dma_ch_isr = readl(MAC_DMA_REG(channel, DMA_CH_SR));
        netif_dbg(pdata, intr, pdata->netdev, "DMA_CH%u_ISR=%#010x\n",
              i, dma_ch_isr);

        /* The TI or RI interrupt bits may still be set even if using
         * per channel DMA interrupts. Check to be sure those are not
         * enabled before using the private data napi structure.
         */
        ti = MAC_GET_REG_BITS(dma_ch_isr, DMA_CH_SR_TI_POS,
                     DMA_CH_SR_TI_LEN);
        ri = MAC_GET_REG_BITS(dma_ch_isr, DMA_CH_SR_RI_POS,
                     DMA_CH_SR_RI_LEN);
                     
        if (!pdata->per_channel_irq && (ti || ri)) {
            if (napi_schedule_prep(&pdata->napi)) {
			//insomnia@20200211
#if 1	
			#ifdef RNIC_LEGACY_INT_EN
				del_timer(&pdata->ieu_timer);
			#endif
			
                /* Disable Tx and Rx interrupts */
                mac_disable_rx_tx_ints(pdata);
#endif
                pdata->stats.napi_poll_isr++;
                /* Turn on polling */
                __napi_schedule_irqoff(&pdata->napi);
            }
        }

        if (MAC_GET_REG_BITS(dma_ch_isr, DMA_CH_SR_TPS_POS,
                    DMA_CH_SR_TPS_LEN))
            pdata->stats.tx_process_stopped++;

        if (MAC_GET_REG_BITS(dma_ch_isr, DMA_CH_SR_RPS_POS,
                    DMA_CH_SR_RPS_LEN))
            pdata->stats.rx_process_stopped++;

        if (MAC_GET_REG_BITS(dma_ch_isr, DMA_CH_SR_TBU_POS,
                    DMA_CH_SR_TBU_LEN))
            pdata->stats.tx_buffer_unavailable++;

        if (MAC_GET_REG_BITS(dma_ch_isr, DMA_CH_SR_RBU_POS,
                    DMA_CH_SR_RBU_LEN))
            pdata->stats.rx_buffer_unavailable++;

        /* Restart the device on a Fatal Bus Error */
        if (MAC_GET_REG_BITS(dma_ch_isr, DMA_CH_SR_FBE_POS,
                    DMA_CH_SR_FBE_LEN)) {
            pdata->stats.fatal_bus_error++;
            schedule_work(&pdata->restart_work);
        }

        /* Clear all interrupt signals */
        writel(dma_ch_isr, MAC_DMA_REG(channel, DMA_CH_SR)); 
    }

    if (MAC_GET_REG_BITS(dma_isr, DMA_ISR_MACIS_POS,
                DMA_ISR_MACIS_LEN)) {
        mac_isr = readl(pdata->mac_regs + MAC_ISR);

        if (MAC_GET_REG_BITS(mac_isr, MAC_ISR_MMCTXIS_POS,
                    MAC_ISR_MMCTXIS_LEN))
            hw_ops->tx_mmc_int(pdata);

        if (MAC_GET_REG_BITS(mac_isr, MAC_ISR_MMCRXIS_POS,
                    MAC_ISR_MMCRXIS_LEN))
            hw_ops->rx_mmc_int(pdata);
    }


    return IRQ_HANDLED;
}

#if 0
//insomnia@2020216
static irqreturn_t rnic_msi_isr(int irq, void *data)
{
    struct mac_channel *channel = data;
    
    RNIC_TRACE_PRINT();
    
    /* Per channel DMA interrupts are enabled, so we use the per
     * channel napi structure and not the private data napi structure
     */
    if (napi_schedule_prep(&channel->napi)) {
        /* Disable Tx and Rx interrupts */
        disable_irq_nosync(channel->dma_irq);

        /* Turn on polling */
        __napi_schedule_irqoff(&channel->napi);
    }

    return IRQ_HANDLED;
}
#endif

static irqreturn_t mac_dma_isr(int irq, void *data)
{
    struct mac_channel *channel = data;
    
    RNIC_TRACE_PRINT();


	//printk("-------------------------------------mac_dma_isr-------------------------------------------\n");
	
    /* Per channel DMA interrupts are enabled, so we use the per
     * channel napi structure and not the private data napi structure
     */
    if (napi_schedule_prep(&channel->napi)) {

		//insomnia@20200216
		/* disable Tx and Rx interrupts */
		mac_disable_rx_tx_ints(channel->pdata);
			
        /* Disable Tx and Rx interrupts */
        disable_irq_nosync(channel->dma_irq);

        /* Turn on polling */
        __napi_schedule_irqoff(&channel->napi);
    }

    return IRQ_HANDLED;
}

//insomnia@20200209
#ifdef RNIC_LEGACY_INT_EN

static void ieu_clear_intr_timer(struct timer_list *t)
{
    struct mac_pdata *pdata = from_timer(pdata, t, ieu_timer);
	
    ieu_clear_intr(&pdata->rnic_pdata);
    mod_timer(&pdata->ieu_timer,jiffies + usecs_to_jiffies(pdata->ieu_clr_intr_usecs));
}

#endif


//insomnia@20200212
static void rnic_link_check_timer(struct timer_list *t)
{
    struct mac_pdata *pdata = from_timer(pdata, t, link_check_timer);
	int retry_cnt = 5;
	int pcs_rlu;
	int mac_ls;
	int an_intr;

	while(--retry_cnt)
	{
		pcs_rlu = pcs_get_rlu(&pdata->rnic_pdata,0);
		mac_ls  = mac_get_link_status(&pdata->rnic_pdata,0);
		if( pcs_rlu == 1 && mac_ls == 0)
			break;
	}

	if(retry_cnt > 0)
	{
    	mod_timer(&pdata->link_check_timer,jiffies + usecs_to_jiffies(pdata->link_check_usecs));
		if(pdata->link_up != 1)
		{
			printk("bx_rnic LINK UP!\n");
			pdata->link_up = 1;
		}
	}
	else
	{
		if(pdata->link_up != 0)
		{
			printk("bx_rnic LINK DOWN! RLU(%d), LS(%d)\n",pcs_rlu,mac_ls);
			pdata->link_up = 0;
		}
#if 0
		printk("LINK DOWN! RLU(%d), LS(%d)\n",pcs_rlu,mac_ls);
		mod_timer(&pdata->link_check_timer,jiffies + usecs_to_jiffies(pdata->link_check_usecs));
    	//netdev_warn(pdata->netdev, "link down, device restarting\n");
    	//schedule_work(&pdata->restart_work);
#else
		if(pdata->rnic_pdata.pcs_0_an_start == 0)
			pcs_an_start(&pdata->rnic_pdata,0);
		else
		{
			pdata->link_check_usecs = PCS_LINK_CHECK_AN_TIMER_USECS;
			an_intr = pcs_an_get_intr(&pdata->rnic_pdata,0);
			if(an_intr != 0)
			{
				if(get_bits(an_intr,AN_INT_CMPL))
				{
					pcs_an_int_cmpl(&pdata->rnic_pdata,0);
					pdata->link_check_usecs = PCS_LINK_CHECK_OK_TIMER_USECS;
				}
			
				if(get_bits(an_intr,AN_INC_LINK))
				{
					pcs_an_inc_link(&pdata->rnic_pdata,0);
					pdata->link_check_usecs = get_random_num() % PCS_LINK_CHECK_OK_TIMER_USECS;
				}
				
				if(get_bits(an_intr,AN_PG_RCV))
					pcs_an_pg_rcv(&pdata->rnic_pdata,0);
			}
		}
#endif
	
	}

	mod_timer(&pdata->link_check_timer,jiffies + usecs_to_jiffies(pdata->link_check_usecs));
}

static void mac_tx_timer(struct timer_list *t)
{
    struct mac_channel *channel = from_timer(channel, t, tx_timer);
    struct mac_pdata *pdata = channel->pdata;
    struct napi_struct *napi;
    
    RNIC_TRACE_PRINT();
    
    napi = (pdata->per_channel_irq) ? &channel->napi : &pdata->napi;

    if (napi_schedule_prep(napi)) {
        /* Disable Tx and Rx interrupts */
        if (pdata->per_channel_irq)
            disable_irq_nosync(channel->dma_irq);
        else
            mac_disable_rx_tx_ints(pdata);

        pdata->stats.napi_poll_txtimer++;
        /* Turn on polling */
        __napi_schedule(napi);
    }

    channel->tx_timer_active = 0;
}

static void mac_init_timers(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        timer_setup(&channel->tx_timer, mac_tx_timer, 0);
    }

//insomnia@20200209
#ifdef RNIC_LEGACY_INT_EN
    timer_setup(&pdata->ieu_timer, ieu_clear_intr_timer, 0);
#endif

	//insomnia@20200212
	timer_setup(&pdata->link_check_timer, rnic_link_check_timer, 0);
}

static void mac_stop_timers(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        del_timer_sync(&channel->tx_timer);
    }
    
//insomnia@20200209
#ifdef RNIC_LEGACY_INT_EN
    del_timer_sync(&pdata->ieu_timer);
#endif

	//insomnia@20200212
	del_timer_sync(&pdata->link_check_timer);
}

static void mac_napi_enable(struct mac_pdata *pdata, unsigned int add)
{
    struct mac_channel *channel;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    if (pdata->per_channel_irq) {
        channel = pdata->channel_head;
        for (i = 0; i < pdata->channel_count; i++, channel++) {
            if (add)
                netif_napi_add(pdata->netdev, &channel->napi,
                           mac_one_poll,
                           NAPI_POLL_WEIGHT);

            napi_enable(&channel->napi);
        }
    } else {

        if (add)
            netif_napi_add(pdata->netdev, &pdata->napi,
                       mac_all_poll, NAPI_POLL_WEIGHT);

        napi_enable(&pdata->napi);
  	/* //insomnia@20200216 	
    	for(i = 0; i < pdata->msi_irq_cnt ; i++) 
		{ 
	        if (add)
	            netif_napi_add(pdata->netdev, &pdata->napi[i],
	                       mac_all_poll, NAPI_POLL_WEIGHT);

	        napi_enable(&pdata->napi[i]);
   		}
   		 */
    }
}

static void mac_napi_disable(struct mac_pdata *pdata, unsigned int del)
{
    struct mac_channel *channel;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    if (pdata->per_channel_irq) {
        channel = pdata->channel_head;
        for (i = 0; i < pdata->channel_count; i++, channel++) {
            napi_disable(&channel->napi);

            if (del)
                netif_napi_del(&channel->napi);
        }
    } else {
 
        napi_disable(&pdata->napi);

        if (del)
            netif_napi_del(&pdata->napi);
   /* insomnia@20200216
		for(i = 0; i < pdata->msi_irq_cnt ; i++) 
		{ 
	       	napi_disable(&pdata->napi[i]);
        	if (del)
            	netif_napi_del(&pdata->napi[i]);
   		}
   		*/
    }
}

static int mac_request_irqs(struct mac_pdata *pdata)
{
    struct net_device *netdev = pdata->netdev;
    struct mac_channel *channel;
    unsigned int i;
    int ret;
    
    RNIC_TRACE_PRINT();
	
#ifndef RNIC_MSI_EN
    ret = devm_request_irq(pdata->dev, pdata->dev_irq, mac_isr,
                   IRQF_SHARED, netdev->name, pdata);

    if (ret) {
        netdev_alert(netdev, "error requesting irq %d\n",
                 pdata->dev_irq);
        return ret;
    }
#endif

#if 0
    for (i = 0; i < pdata->msi_irq_cnt ; i++) {
        snprintf(pdata->msi_irq_name[i],
             sizeof(pdata->msi_irq_name[i]) - 1,
             "%s-intr-%u", netdev_name(netdev),
             i);

        ret = devm_request_irq(pdata->dev, pdata->dev_irq+i,
                       mac_isr, 0,
                       pdata->msi_irq_name[i], pdata);
        if (ret) {
            netdev_alert(netdev, "error requesting irq %d\n",
                      pdata->dev_irq+i);
            goto err_irq;
        }
    }
#endif

#if 0
   	for(i=0;i<16;i++)
	    ret = devm_request_irq(pdata->dev, pdata->dev_irq+i, mac_isr,
                   0, netdev->name, pdata);	
#endif

#if 1
    if (!pdata->per_channel_irq)
        return 0;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {

/*

        snprintf(channel->dma_irq_name,
             sizeof(channel->dma_irq_name) - 1,
             "%s-TxRx-%u", netdev_name(netdev),
             channel->queue_index);

        ret = devm_request_irq(pdata->dev, channel->dma_irq,
                       mac_dma_isr, 0,
                       channel->dma_irq_name, channel);
        if (ret) {
            netdev_alert(netdev, "error requesting irq %d\n",
                     channel->dma_irq);
            goto err_irq;
        }
	*/

        snprintf(channel->dma_irq_name_tx,
             sizeof(channel->dma_irq_name_tx) - 1,
             "%s-Tx-%u", netdev_name(netdev),
             channel->queue_index);

        ret = devm_request_irq(pdata->dev, channel->dma_irq,
                       mac_dma_isr, 0,
                       channel->dma_irq_name_tx, channel);
        if (ret) {
            netdev_alert(netdev, "error requesting irq %d\n",
                     channel->dma_irq);
            goto err_irq;
        }


        snprintf(channel->dma_irq_name_rx,
             sizeof(channel->dma_irq_name_rx) - 1,
             "%s-Rx-%u", netdev_name(netdev),
             channel->queue_index);

        ret = devm_request_irq(pdata->dev, channel->dma_irq+7,
                       mac_dma_isr, 0,
                       channel->dma_irq_name_rx, channel);
        if (ret) {
            netdev_alert(netdev, "error requesting irq %d\n",
                     channel->dma_irq+7);
            goto err_irq;
        }
	}
#endif
    return 0;

err_irq:
/*
	//insomnia@20200216
    for (i--; i < pdata->msi_irq_cnt; i--)
        devm_free_irq(pdata->dev, pdata->dev_irq+i, pdata);
*/
    /* Using an unsigned int, 'i' will go to UINT_MAX and exit */
    for (i--, channel--; i < pdata->channel_count; i--, channel--)
	{
        devm_free_irq(pdata->dev, channel->dma_irq, channel);
		devm_free_irq(pdata->dev, channel->dma_irq+7, channel); //insomnia@20200216
    }

    devm_free_irq(pdata->dev, pdata->dev_irq, pdata);

    return ret;
}

static void mac_free_irqs(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    devm_free_irq(pdata->dev, pdata->dev_irq, pdata);

    if (!pdata->per_channel_irq)
        return;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++)
        devm_free_irq(pdata->dev, channel->dma_irq, channel);
}

static void mac_free_tx_data(struct mac_pdata *pdata)
{
    struct mac_desc_ops *desc_ops = &pdata->desc_ops;
    struct mac_desc_data *desc_data;
    struct mac_channel *channel;
    struct mac_ring *ring;
    unsigned int i, j;
    
    RNIC_TRACE_PRINT();
    
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        ring = channel->tx_ring;
        if (!ring)
            break;

        for (j = 0; j < ring->dma_desc_count; j++) {
            desc_data = MAC_GET_DESC_DATA(ring, j);
            desc_ops->unmap_desc_data(pdata, desc_data);
        }
    }
}

static void mac_free_rx_data(struct mac_pdata *pdata)
{
    struct mac_desc_ops *desc_ops = &pdata->desc_ops;
    struct mac_desc_data *desc_data;
    struct mac_channel *channel;
    struct mac_ring *ring;
    unsigned int i, j;
    
    RNIC_TRACE_PRINT();
    
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        ring = channel->rx_ring;
        if (!ring)
            break;

        for (j = 0; j < ring->dma_desc_count; j++) {
            desc_data = MAC_GET_DESC_DATA(ring, j);
            desc_ops->unmap_desc_data(pdata, desc_data);
        }
    }
}

static int mac_start(struct mac_pdata *pdata)
{
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    struct net_device *netdev = pdata->netdev;
    int ret;
    
    RNIC_TRACE_PRINT();
    
    hw_ops->init(pdata);
    mac_napi_enable(pdata, 1);

    ret = mac_request_irqs(pdata);
    if (ret)
        goto err_napi;

    hw_ops->enable_tx(pdata);
    hw_ops->enable_rx(pdata);
    netif_tx_start_all_queues(netdev);

//insomnia@20200209
#ifdef RNIC_LEGACY_INT_EN
    mod_timer(&pdata->ieu_timer,jiffies + usecs_to_jiffies(pdata->ieu_clr_intr_usecs));
#endif

	//insomnia@20200212
	mod_timer(&pdata->link_check_timer,jiffies + usecs_to_jiffies(pdata->link_check_usecs));

    return 0;

err_napi:
    mac_napi_disable(pdata, 1);
    hw_ops->exit(pdata);

    return ret;
}

static void mac_stop(struct mac_pdata *pdata)
{
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    struct net_device *netdev = pdata->netdev;
    struct mac_channel *channel;
    struct netdev_queue *txq;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    netif_tx_stop_all_queues(netdev);
    mac_stop_timers(pdata);
    hw_ops->disable_tx(pdata);
    hw_ops->disable_rx(pdata);
    mac_free_irqs(pdata);
    mac_napi_disable(pdata, 1);
    hw_ops->exit(pdata);

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            continue;

        txq = netdev_get_tx_queue(netdev, channel->queue_index);
        netdev_tx_reset_queue(txq);
    }
}

static void mac_restart_dev(struct mac_pdata *pdata)
{
    
    RNIC_TRACE_PRINT();
    
    /* If not running, "restart" will happen on open */
    if (!netif_running(pdata->netdev))
        return;

    mac_stop(pdata);

    mac_free_tx_data(pdata);
    mac_free_rx_data(pdata);

    mac_start(pdata);
}

static void mac_restart(struct work_struct *work)
{
    struct mac_pdata *pdata = container_of(work,
                           struct mac_pdata,
                           restart_work);
    
    RNIC_TRACE_PRINT();
    
    rtnl_lock();

    mac_restart_dev(pdata);

    rtnl_unlock();
}

static int mac_open(struct net_device *netdev)
{

    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_desc_ops *desc_ops;
    int ret;
    desc_ops = &pdata->desc_ops;
    
    RNIC_TRACE_PRINT();
    
    /* TODO: Initialize the phy */

    /* Calculate the Rx buffer size before allocating rings */
    ret = mac_calc_rx_buf_size(netdev, netdev->mtu);
    if (ret < 0)
        return ret;
    pdata->rx_buf_size = ret;

    /* Allocate the channels and rings */
    ret = desc_ops->alloc_channles_and_rings(pdata);
    if (ret)
        return ret;

    INIT_WORK(&pdata->restart_work, mac_restart);
    mac_init_timers(pdata);

    ret = mac_start(pdata);
    if (ret)
        goto err_channels_and_rings;

    return 0;

err_channels_and_rings:
    desc_ops->free_channels_and_rings(pdata);

    return ret;
}

static int mac_close(struct net_device *netdev)
{

    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_desc_ops *desc_ops;
    
    RNIC_TRACE_PRINT();
    
    desc_ops = &pdata->desc_ops;

    /* Stop the device */
    mac_stop(pdata);

    /* Free the channels and rings */
    desc_ops->free_channels_and_rings(pdata);

    return 0;
}

static void mac_tx_timeout(struct net_device *netdev)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    
    RNIC_TRACE_PRINT();
    
    netdev_warn(netdev, "tx timeout, device restarting\n");
    schedule_work(&pdata->restart_work);
}

static int mac_xmit(struct sk_buff *skb, struct net_device *netdev)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_pkt_info *tx_pkt_info;
    struct mac_desc_ops *desc_ops;
    struct mac_channel *channel;
    struct mac_hw_ops *hw_ops;
    struct netdev_queue *txq;
    struct mac_ring *ring;
    int ret;
    
    RNIC_TRACE_PRINT();

	  //insomnia@20200216
    if(pcs_get_rlu(&pdata->rnic_pdata, 0)==0 || mac_get_link_status(&pdata->rnic_pdata, 0)!=0) 
    {
    	//netdev_warn(netdev, "link down, device restarting\n");
    	//schedule_work(&pdata->restart_work);
        return 1;
  	}
        

    desc_ops = &pdata->desc_ops;
    hw_ops = &pdata->hw_ops;

    RNIC_PRINTK("skb->len = %d\n", skb->len);

    channel = pdata->channel_head + skb->queue_mapping;
    txq = netdev_get_tx_queue(netdev, channel->queue_index);
    ring = channel->tx_ring;
    tx_pkt_info = &ring->pkt_info;

    if (skb->len == 0) {
        netif_err(pdata, tx_err, netdev,
              "empty skb received from stack\n");
        dev_kfree_skb_any(skb);
        return NETDEV_TX_OK;
    }

    /* Prepare preliminary packet info for TX */
    memset(tx_pkt_info, 0, sizeof(*tx_pkt_info));
    mac_prep_tx_pkt(pdata, ring, skb, tx_pkt_info);

    /* Check that there are enough descriptors available */
    ret = mac_maybe_stop_tx_queue(channel, ring,
                     tx_pkt_info->desc_count);
    if (ret)
        return ret;

    ret = mac_prep_tso(skb, tx_pkt_info);
    if (ret) {
        netif_err(pdata, tx_err, netdev,
              "error processing TSO packet\n");
        dev_kfree_skb_any(skb);
        return ret;
    }
    mac_prep_vlan(skb, tx_pkt_info);

    if (!desc_ops->map_tx_skb(channel, skb)) {
        dev_kfree_skb_any(skb);
        return NETDEV_TX_OK;
    }

    /* Report on the actual number of bytes (to be) sent */
    netdev_tx_sent_queue(txq, tx_pkt_info->tx_bytes);

    /* Configure required descriptor fields for transmission */
    hw_ops->dev_xmit(channel);


    if (netif_msg_pktdata(pdata))
        mac_print_pkt(netdev, skb, true);

    /* Stop the queue in advance if there may not be enough descriptors */
    mac_maybe_stop_tx_queue(channel, ring, MAC_TX_MAX_DESC_NR);


    return NETDEV_TX_OK;
}

static void mac_get_stats64(struct net_device *netdev,
                   struct rtnl_link_stats64 *s)
{   
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_stats *pstats = &pdata->stats;
    
    RNIC_TRACE_PRINT();
    
    pdata->hw_ops.read_mmc_stats(pdata);

    s->rx_packets = pstats->rxframecount_gb;
    s->rx_bytes = pstats->rxoctetcount_gb;
    s->rx_errors = pstats->rxframecount_gb -
               pstats->rxbroadcastframes_g -
               pstats->rxmulticastframes_g -
               pstats->rxunicastframes_g;
    s->multicast = pstats->rxmulticastframes_g;
    s->rx_length_errors = pstats->rxlengtherror;
    s->rx_crc_errors = pstats->rxcrcerror;
    s->rx_fifo_errors = pstats->rxfifooverflow;

    s->tx_packets = pstats->txframecount_gb;
    s->tx_bytes = pstats->txoctetcount_gb;
    s->tx_errors = pstats->txframecount_gb - pstats->txframecount_g;
    s->tx_dropped = netdev->stats.tx_dropped;
}

static int mac_set_mac_address(struct net_device *netdev, void *addr)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    struct sockaddr *saddr = addr;

    if (!is_valid_ether_addr(saddr->sa_data))
        return -EADDRNOTAVAIL;

    memcpy(netdev->dev_addr, saddr->sa_data, netdev->addr_len);

    hw_ops->set_mac_address(pdata, netdev->dev_addr);

    return 0;
}

static int mac_ioctl(struct net_device *netdev,
            struct ifreq *ifreq, int cmd)
{
    if (!netif_running(netdev))
        return -ENODEV;

    return 0;
}

static int mac_change_mtu(struct net_device *netdev, int mtu)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    int ret;

    ret = mac_calc_rx_buf_size(netdev, mtu);
    if (ret < 0)
        return ret;

    pdata->rx_buf_size = ret;
    netdev->mtu = mtu;

    mac_restart_dev(pdata);

    return 0;
}

static int mac_vlan_rx_add_vid(struct net_device *netdev,
                  __be16 proto,
                  u16 vid)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    
    RNIC_TRACE_PRINT();
    
    set_bit(vid, pdata->active_vlans);
    hw_ops->update_vlan_hash_table(pdata);

    return 0;
}

static int mac_vlan_rx_kill_vid(struct net_device *netdev,
                   __be16 proto,
                   u16 vid)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;

    clear_bit(vid, pdata->active_vlans);
    hw_ops->update_vlan_hash_table(pdata);

    return 0;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void mac_poll_controller(struct net_device *netdev)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_channel *channel;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    if (pdata->per_channel_irq) {
        channel = pdata->channel_head;
        for (i = 0; i < pdata->channel_count; i++, channel++)
            mac_dma_isr(channel->dma_irq, channel);
    } else {
        disable_irq(pdata->dev_irq);
        mac_isr(pdata->dev_irq, pdata);
        enable_irq(pdata->dev_irq);
    }
}
#endif /* CONFIG_NET_POLL_CONTROLLER */

static int mac_set_features(struct net_device *netdev,
                   netdev_features_t features)
{
    netdev_features_t rxhash, rxcsum, rxvlan, rxvlan_filter;
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    int ret = 0;
    
    RNIC_TRACE_PRINT();
    
    rxhash = pdata->netdev_features & NETIF_F_RXHASH;
    rxcsum = pdata->netdev_features & NETIF_F_RXCSUM;
    rxvlan = pdata->netdev_features & NETIF_F_HW_VLAN_CTAG_RX;
    rxvlan_filter = pdata->netdev_features & NETIF_F_HW_VLAN_CTAG_FILTER;

    if ((features & NETIF_F_RXHASH) && !rxhash)
        ret = hw_ops->enable_rss(pdata);
    else if (!(features & NETIF_F_RXHASH) && rxhash)
        ret = hw_ops->disable_rss(pdata);
    if (ret)
        return ret;

    if ((features & NETIF_F_RXCSUM) && !rxcsum)
        hw_ops->enable_rx_csum(pdata);
    else if (!(features & NETIF_F_RXCSUM) && rxcsum)
        hw_ops->disable_rx_csum(pdata);

    if ((features & NETIF_F_HW_VLAN_CTAG_RX) && !rxvlan)
        hw_ops->enable_rx_vlan_stripping(pdata);
    else if (!(features & NETIF_F_HW_VLAN_CTAG_RX) && rxvlan)
        hw_ops->disable_rx_vlan_stripping(pdata);

    if ((features & NETIF_F_HW_VLAN_CTAG_FILTER) && !rxvlan_filter)
        hw_ops->enable_rx_vlan_filtering(pdata);
    else if (!(features & NETIF_F_HW_VLAN_CTAG_FILTER) && rxvlan_filter)
        hw_ops->disable_rx_vlan_filtering(pdata);

    pdata->netdev_features = features;

    return 0;
}

static void mac_set_rx_mode(struct net_device *netdev)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    
    RNIC_TRACE_PRINT();
    
    hw_ops->config_rx_mode(pdata);
}

static const struct net_device_ops mac_netdev_ops = {
    .ndo_open       = mac_open,
    .ndo_stop       = mac_close,
    .ndo_start_xmit     = mac_xmit,
    .ndo_tx_timeout     = mac_tx_timeout,
    .ndo_get_stats64    = mac_get_stats64,
    .ndo_change_mtu     = mac_change_mtu,
    .ndo_set_mac_address    = mac_set_mac_address,
    .ndo_validate_addr  = eth_validate_addr,
    .ndo_do_ioctl       = mac_ioctl,
    .ndo_vlan_rx_add_vid    = mac_vlan_rx_add_vid,
    .ndo_vlan_rx_kill_vid   = mac_vlan_rx_kill_vid,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller    = mac_poll_controller,
#endif
    .ndo_set_features   = mac_set_features,
    .ndo_set_rx_mode    = mac_set_rx_mode,
};

const struct net_device_ops *mac_get_netdev_ops(void)
{   
    RNIC_TRACE_PRINT();
    
    return &mac_netdev_ops;
}

static void mac_rx_refresh(struct mac_channel *channel)
{
    struct mac_pdata *pdata = channel->pdata;
    struct mac_ring *ring = channel->rx_ring;
    struct mac_desc_data *desc_data;
    struct mac_desc_ops *desc_ops;
    struct mac_hw_ops *hw_ops;
    
    RNIC_TRACE_PRINT();
    
    desc_ops = &pdata->desc_ops;
    hw_ops = &pdata->hw_ops;

    while (ring->dirty != ring->cur) {
        desc_data = MAC_GET_DESC_DATA(ring, ring->dirty);

        /* Reset desc_data values */
        desc_ops->unmap_desc_data(pdata, desc_data);

        if (desc_ops->map_rx_buffer(pdata, ring, desc_data))
            break;

        hw_ops->rx_desc_reset(pdata, desc_data, ring->dirty);

        ring->dirty++;
    }

    /* Make sure everything is written before the register write */
    wmb();

    /* Update the Rx Tail Pointer Register with address of
     * the last cleaned entry
     */
    desc_data = MAC_GET_DESC_DATA(ring, ring->dirty - 1);
    
    writel(lower_32_bits(desc_data->dma_desc_addr),
           MAC_DMA_REG(channel, DMA_CH_RDTR_LO));
}

static struct sk_buff *mac_create_skb(struct mac_pdata *pdata,
                     struct napi_struct *napi,
                     struct mac_desc_data *desc_data,
                     unsigned int len)
{
    unsigned int copy_len;
    struct sk_buff *skb;
    u8 *packet;
    
    RNIC_TRACE_PRINT();
    
    skb = napi_alloc_skb(napi, desc_data->rx.hdr.dma_len);
    if (!skb)
        return NULL;

    /* Start with the header buffer which may contain just the header
     * or the header plus data
     */
    dma_sync_single_range_for_cpu(pdata->dev, desc_data->rx.hdr.dma_base,
                      desc_data->rx.hdr.dma_off,
                      desc_data->rx.hdr.dma_len,
                      DMA_FROM_DEVICE);

    packet = page_address(desc_data->rx.hdr.pa.pages) +
         desc_data->rx.hdr.pa.pages_offset;
    copy_len = (desc_data->rx.hdr_len) ? desc_data->rx.hdr_len : len;
    copy_len = min(desc_data->rx.hdr.dma_len, copy_len);
    skb_copy_to_linear_data(skb, packet, copy_len);
    skb_put(skb, copy_len);

    len -= copy_len;
    if (len) {
        /* Add the remaining data as a frag */
        dma_sync_single_range_for_cpu(pdata->dev,
                          desc_data->rx.buf.dma_base,
                          desc_data->rx.buf.dma_off,
                          desc_data->rx.buf.dma_len,
                          DMA_FROM_DEVICE);

        skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags,
                desc_data->rx.buf.pa.pages,
                desc_data->rx.buf.pa.pages_offset,
                len, desc_data->rx.buf.dma_len);
        desc_data->rx.buf.pa.pages = NULL;
    }

    return skb;
}

static int mac_tx_poll(struct mac_channel *channel)
{
    struct mac_pdata *pdata = channel->pdata;
    struct mac_ring *ring = channel->tx_ring;
    struct net_device *netdev = pdata->netdev;
    unsigned int tx_packets = 0, tx_bytes = 0;
    struct mac_desc_data *desc_data;
    struct mac_dma_desc *dma_desc;
    struct mac_desc_ops *desc_ops;
    struct mac_hw_ops *hw_ops;
    struct netdev_queue *txq;
    int processed = 0;
    unsigned int cur;
    
    RNIC_TRACE_PRINT();
    
    desc_ops = &pdata->desc_ops;
    hw_ops = &pdata->hw_ops;

    /* Nothing to do if there isn't a Tx ring for this channel */
    if (!ring)
        return 0;

    cur = ring->cur;

    /* Be sure we get ring->cur before accessing descriptor data */
    smp_rmb();

    txq = netdev_get_tx_queue(netdev, channel->queue_index);

    while ((processed < MAC_TX_DESC_MAX_PROC) &&
           (ring->dirty != cur)) {
        desc_data = MAC_GET_DESC_DATA(ring, ring->dirty);
        dma_desc = desc_data->dma_desc;

        if (!hw_ops->tx_complete(dma_desc))
            break;

        /* Make sure descriptor fields are read after reading
         * the OWN bit
         */
        dma_rmb();

        if (netif_msg_tx_done(pdata))
            mac_dump_tx_desc(pdata, ring, ring->dirty, 1, 0);

        if (hw_ops->is_last_desc(dma_desc)) {
            tx_packets += desc_data->tx.packets;
            tx_bytes += desc_data->tx.bytes;
        }

        /* Free the SKB and reset the descriptor for re-use */
        desc_ops->unmap_desc_data(pdata, desc_data);
        hw_ops->tx_desc_reset(desc_data);

        processed++;
        ring->dirty++;
    }

    if (!processed)
        return 0;

    netdev_tx_completed_queue(txq, tx_packets, tx_bytes);

    if ((ring->tx.queue_stopped == 1) &&
        (mac_tx_avail_desc(ring) > MAC_TX_DESC_MIN_FREE)) {
        ring->tx.queue_stopped = 0;
        netif_tx_wake_queue(txq);
    }

    RNIC_PRINTK("channel = %d, processed=%d\n", channel->queue_index, processed);

    return processed;
}

static int mac_rx_poll(struct mac_channel *channel, int budget)
{
    struct mac_pdata *pdata = channel->pdata;
    struct mac_ring *ring = channel->rx_ring;
    struct net_device *netdev = pdata->netdev;
    unsigned int len, dma_desc_len, max_len;
    unsigned int context_next, context;
    struct mac_desc_data *desc_data;
    struct mac_pkt_info *pkt_info;
    unsigned int incomplete, error;
    struct mac_hw_ops *hw_ops;
    unsigned int received = 0;
    struct napi_struct *napi;
    struct sk_buff *skb;
    int packet_count = 0;
    
    RNIC_TRACE_PRINT();
    
    hw_ops = &pdata->hw_ops;

    /* Nothing to do if there isn't a Rx ring for this channel */
    if (!ring)
        return 0;

    incomplete = 0;
    context_next = 0;

    napi = (pdata->per_channel_irq) ? &channel->napi : &pdata->napi;

    desc_data = MAC_GET_DESC_DATA(ring, ring->cur);

    pkt_info = &ring->pkt_info;
    while (packet_count < budget) {
        /* First time in loop see if we need to restore state */
        if (!received && desc_data->state_saved) {
            skb = desc_data->state.skb;
            error = desc_data->state.error;
            len = desc_data->state.len;
        } else {
            memset(pkt_info, 0, sizeof(*pkt_info));
            skb = NULL;
            error = 0;
            len = 0;
        }

read_again:
        desc_data = MAC_GET_DESC_DATA(ring, ring->cur);
        
        RNIC_PRINTK("ring->cur = %d\n",ring->cur);

        if (mac_rx_dirty_desc(ring) > MAC_RX_DESC_MAX_DIRTY)
            mac_rx_refresh(channel);

        if (hw_ops->dev_read(channel))
            break;

        received++;
        ring->cur++;

        incomplete = MAC_GET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_INCOMPLETE_POS,
                    RX_PACKET_ATTRIBUTES_INCOMPLETE_LEN);
        context_next = MAC_GET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_POS,
                    RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_LEN);
        context = MAC_GET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_CONTEXT_POS,
                    RX_PACKET_ATTRIBUTES_CONTEXT_LEN);

        /* Earlier error, just drain the remaining data */
        if ((incomplete || context_next) && error)
            goto read_again;

        if (error || pkt_info->errors) {
            if (pkt_info->errors)
                netif_err(pdata, rx_err, netdev,
                      "error in received packet\n");
            dev_kfree_skb(skb);
            goto next_packet;
        }

        if (!context) {
            /* Length is cumulative, get this descriptor's length */
            dma_desc_len = desc_data->rx.len - len;
            len += dma_desc_len;

            if (dma_desc_len && !skb) {
                skb = mac_create_skb(pdata, napi, desc_data,
                            dma_desc_len);
                if (!skb)
                    error = 1;
            } else if (dma_desc_len) {
                dma_sync_single_range_for_cpu(
                        pdata->dev,
                        desc_data->rx.buf.dma_base,
                        desc_data->rx.buf.dma_off,
                        desc_data->rx.buf.dma_len,
                        DMA_FROM_DEVICE);

                skb_add_rx_frag(
                    skb, skb_shinfo(skb)->nr_frags,
                    desc_data->rx.buf.pa.pages,
                    desc_data->rx.buf.pa.pages_offset,
                    dma_desc_len,
                    desc_data->rx.buf.dma_len);
                desc_data->rx.buf.pa.pages = NULL;
            }
        }

        if (incomplete || context_next)
            goto read_again;

        if (!skb)
            goto next_packet;

        /* Be sure we don't exceed the configured MTU */
        max_len = netdev->mtu + ETH_HLEN;
        if (!(netdev->features & NETIF_F_HW_VLAN_CTAG_RX) &&
            (skb->protocol == htons(ETH_P_8021Q)))
            max_len += VLAN_HLEN;

        if (skb->len > max_len) {
            netif_err(pdata, rx_err, netdev,
                  "packet length exceeds configured MTU\n");
            dev_kfree_skb(skb);
            goto next_packet;
        }

        if (netif_msg_pktdata(pdata))
            mac_print_pkt(netdev, skb, false);

        skb_checksum_none_assert(skb);
        if (MAC_GET_REG_BITS(pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_CSUM_DONE_POS,
                    RX_PACKET_ATTRIBUTES_CSUM_DONE_LEN))
            skb->ip_summed = CHECKSUM_UNNECESSARY;

        if (MAC_GET_REG_BITS(pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_VLAN_CTAG_POS,
                    RX_PACKET_ATTRIBUTES_VLAN_CTAG_LEN)) {
            __vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
                           pkt_info->vlan_ctag);
            pdata->stats.rx_vlan_packets++;
        }

        if (MAC_GET_REG_BITS(pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_RSS_HASH_POS,
                RX_PACKET_ATTRIBUTES_RSS_HASH_LEN))
            skb_set_hash(skb, pkt_info->rss_hash,
                     pkt_info->rss_hash_type);

        skb->dev = netdev;
        skb->protocol = eth_type_trans(skb, netdev);
        skb_record_rx_queue(skb, channel->queue_index);

        napi_gro_receive(napi, skb);

next_packet:
        packet_count++;
    }

    /* Check if we need to save state before leaving */
    if (received && (incomplete || context_next)) {
        desc_data = MAC_GET_DESC_DATA(ring, ring->cur);
        desc_data->state_saved = 1;
        desc_data->state.skb = skb;
        desc_data->state.len = len;
        desc_data->state.error = error;
    }

    RNIC_PRINTK("channel = %d, packet_count = %d\n",channel->queue_index, packet_count);

    return packet_count;
}

static int mac_one_poll(struct napi_struct *napi, int budget)
{
    struct mac_channel *channel = container_of(napi,
                        struct mac_channel,
                        napi);
    int processed = 0;
    
    RNIC_TRACE_PRINT();
    
    RNIC_PRINTK("budget=%d\n", budget);

    /* Cleanup Tx ring first */
    mac_tx_poll(channel);

    /* Process Rx ring next */
    processed = mac_rx_poll(channel, budget);

    /* If we processed everything, we are done */
    if (processed < budget) {
        /* Turn off polling */
        napi_complete_done(napi, processed);

        /* Enable Tx and Rx interrupts */
        enable_irq(channel->dma_irq);
    }

	//insomnia@20200216
    /* Enable Tx and Rx interrupts */
    mac_enable_rx_tx_ints(channel->pdata);
	
    RNIC_PRINTK("received = %d\n", processed);

    return processed;
}

static int mac_all_poll(struct napi_struct *napi, int budget)
{
    struct mac_pdata *pdata = container_of(napi,
                           struct mac_pdata,
                           napi);
    struct mac_channel *channel;
    int processed, last_processed;
    int ring_budget;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    
    RNIC_PRINTK("budget=%d\n", budget);

    processed = 0;
    ring_budget = budget / pdata->rx_ring_count;
    do {
        last_processed = processed;

        channel = pdata->channel_head;
        for (i = 0; i < pdata->channel_count; i++, channel++) {
            /* Cleanup Tx ring first */
            mac_tx_poll(channel);

            /* Process Rx ring next */
            if (ring_budget > (budget - processed))
                ring_budget = budget - processed;
            processed += mac_rx_poll(channel, ring_budget);
        }
    } while ((processed < budget) && (processed != last_processed));

    /* If we processed everything, we are done */
    if (processed < budget) {
        /* Turn off polling */
        napi_complete_done(napi, processed);

        /* Enable Tx and Rx interrupts */
        mac_enable_rx_tx_ints(pdata);

	}

    RNIC_PRINTK("received = %d\n", processed);

//insomnia@20200211
#ifdef RNIC_LEGACY_INT_EN
	mod_timer(&pdata->ieu_timer,jiffies + usecs_to_jiffies(pdata->ieu_clr_intr_usecs));
#endif


    return processed;
}