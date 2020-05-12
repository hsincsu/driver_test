
#include <linux/phy.h>
#include <linux/mdio.h>
#include <linux/clk.h>
#include <linux/bitrev.h>
#include <linux/crc32.h>
#include <linux/crc32poly.h>
#include <linux/dcbnl.h>

#include <linux/time.h>
#include "header/bx_rnic.h"



static int mac_tx_complete(struct mac_dma_desc *dma_desc)
{   
    RNIC_TRACE_PRINT();
    
    return !MAC_GET_REG_BITS_LE(dma_desc->desc3,
                TX_NORMAL_DESC3_OWN_POS,
                TX_NORMAL_DESC3_OWN_LEN);
}

static int mac_disable_rx_csum(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_RCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_IPC_POS,
                     MAC_RCR_IPC_LEN, 0);
    writel(regval, pdata->mac_regs + MAC_RCR);

    return 0;
}

static int mac_enable_rx_csum(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_RCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_IPC_POS,
                     MAC_RCR_IPC_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_RCR);

    return 0;
}

static int mac_set_mac_address(struct mac_pdata *pdata, u8 *addr)
{
    unsigned int mac_addr_hi, mac_addr_lo;
    
    RNIC_TRACE_PRINT();

    mac_addr_hi = (addr[5] <<  8) | (addr[4] <<  0);
    mac_addr_lo = (addr[3] << 24) | (addr[2] << 16) |
              (addr[1] <<  8) | (addr[0] <<  0);

    writel(mac_addr_hi, pdata->mac_regs + MAC_MACA0HR);
    writel(mac_addr_lo, pdata->mac_regs + MAC_MACA0LR);

    return 0;
}

static void mac_set_mac_reg(struct mac_pdata *pdata,
                   struct netdev_hw_addr *ha,
                   unsigned int *mac_reg)
{
    unsigned int mac_addr_hi, mac_addr_lo;
    u8 *mac_addr;
    
    RNIC_TRACE_PRINT();

    mac_addr_lo = 0;
    mac_addr_hi = 0;

    if (ha) {
        mac_addr = (u8 *)&mac_addr_lo;
        mac_addr[0] = ha->addr[0];
        mac_addr[1] = ha->addr[1];
        mac_addr[2] = ha->addr[2];
        mac_addr[3] = ha->addr[3];
        mac_addr = (u8 *)&mac_addr_hi;
        mac_addr[0] = ha->addr[4];
        mac_addr[1] = ha->addr[5];

        netif_dbg(pdata, drv, pdata->netdev,
              "adding mac address %pM at %#x\n",
              ha->addr, *mac_reg);

        mac_addr_hi = MAC_SET_REG_BITS(mac_addr_hi,
                          MAC_MACA1HR_AE_POS,
                        MAC_MACA1HR_AE_LEN,
                        1);
    }

    writel(mac_addr_hi, pdata->mac_regs + *mac_reg);
    *mac_reg += MAC_MACA_INC;
    writel(mac_addr_lo, pdata->mac_regs + *mac_reg);
    *mac_reg += MAC_MACA_INC;
}

static int mac_enable_rx_vlan_stripping(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_VLANTR);
    /* Put the VLAN tag in the Rx descriptor */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_EVLRXS_POS,
                     MAC_VLANTR_EVLRXS_LEN, 1);
    /* Don't check the VLAN type */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_DOVLTC_POS,
                     MAC_VLANTR_DOVLTC_LEN, 1);
    /* Check only C-TAG (0x8100) packets */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_ERSVLM_POS,
                     MAC_VLANTR_ERSVLM_LEN, 0);
    /* Don't consider an S-TAG (0x88A8) packet as a VLAN packet */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_ESVL_POS,
                     MAC_VLANTR_ESVL_LEN, 0);
    /* Enable VLAN tag stripping */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_EVLS_POS,
                     MAC_VLANTR_EVLS_LEN, 0x3);
    writel(regval, pdata->mac_regs + MAC_VLANTR);

    return 0;
}

static int mac_disable_rx_vlan_stripping(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_VLANTR);
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_EVLS_POS,
                     MAC_VLANTR_EVLS_LEN, 0);
    writel(regval, pdata->mac_regs + MAC_VLANTR);

    return 0;
}

static int mac_enable_rx_vlan_filtering(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_PFR);
    /* Enable VLAN filtering */
    regval = MAC_SET_REG_BITS(regval, MAC_PFR_VTFE_POS,
                     MAC_PFR_VTFE_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_PFR);

    regval = readl(pdata->mac_regs + MAC_VLANTR);
    /* Enable VLAN Hash Table filtering */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_VTHM_POS,
                     MAC_VLANTR_VTHM_LEN, 1);
    /* Disable VLAN tag inverse matching */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_VTIM_POS,
                     MAC_VLANTR_VTIM_LEN, 0);
    /* Only filter on the lower 12-bits of the VLAN tag */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_ETV_POS,
                     MAC_VLANTR_ETV_LEN, 1);
    /* In order for the VLAN Hash Table filtering to be effective,
     * the VLAN tag identifier in the VLAN Tag Register must not
     * be zero.  Set the VLAN tag identifier to "1" to enable the
     * VLAN Hash Table filtering.  This implies that a VLAN tag of
     * 1 will always pass filtering.
     */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANTR_VL_POS,
                     MAC_VLANTR_VL_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_VLANTR);

    return 0;
}

static int mac_disable_rx_vlan_filtering(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_PFR);
    /* Disable VLAN filtering */
    regval = MAC_SET_REG_BITS(regval, MAC_PFR_VTFE_POS,
                     MAC_PFR_VTFE_LEN, 0);
    writel(regval, pdata->mac_regs + MAC_PFR);

    return 0;
}

static u32 mac_vid_crc32_le(__le16 vid_le)
{
    unsigned char *data = (unsigned char *)&vid_le;
    unsigned char data_byte = 0;
    u32 crc = ~0;
    u32 temp = 0;
    int i, bits;
    
    RNIC_TRACE_PRINT();

    bits = get_bitmask_order(VLAN_VID_MASK);
    for (i = 0; i < bits; i++) {
        if ((i % 8) == 0)
            data_byte = data[i / 8];

        temp = ((crc & 1) ^ data_byte) & 1;
        crc >>= 1;
        data_byte >>= 1;

        if (temp)
            crc ^= CRC32_POLY_LE;
    }

    return crc;
}

static int mac_update_vlan_hash_table(struct mac_pdata *pdata)
{
    u16 vlan_hash_table = 0;
    __le16 vid_le;
    u32 regval;
    u32 crc;
    u16 vid;
    
    RNIC_TRACE_PRINT();

    /* Generate the VLAN Hash Table value */
    for_each_set_bit(vid, pdata->active_vlans, VLAN_N_VID) {
        /* Get the CRC32 value of the VLAN ID */
        vid_le = cpu_to_le16(vid);
        crc = bitrev32(~mac_vid_crc32_le(vid_le)) >> 28;

        vlan_hash_table |= (1 << crc);
    }

    regval = readl(pdata->mac_regs + MAC_VLANHTR);
    /* Set the VLAN Hash Table filtering register */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANHTR_VLHT_POS,
                     MAC_VLANHTR_VLHT_LEN, vlan_hash_table);
    writel(regval, pdata->mac_regs + MAC_VLANHTR);

    return 0;
}

static int mac_set_promiscuous_mode(struct mac_pdata *pdata,
                       unsigned int enable)
{
    unsigned int val = enable ? 1 : 0;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = MAC_GET_REG_BITS(readl(pdata->mac_regs + MAC_PFR),
                     MAC_PFR_PR_POS, MAC_PFR_PR_LEN);
    if (regval == val)
        return 0;

    netif_dbg(pdata, drv, pdata->netdev, "%s promiscuous mode\n",
          enable ? "entering" : "leaving");

    regval = readl(pdata->mac_regs + MAC_PFR);
    regval = MAC_SET_REG_BITS(regval, MAC_PFR_PR_POS,
                     MAC_PFR_PR_LEN, val);
    writel(regval, pdata->mac_regs + MAC_PFR);

    /* Hardware will still perform VLAN filtering in promiscuous mode */
    if (enable) {
        mac_disable_rx_vlan_filtering(pdata);
    } else {
        if (pdata->netdev->features & NETIF_F_HW_VLAN_CTAG_FILTER)
            mac_enable_rx_vlan_filtering(pdata);
    }

    return 0;
}

static int mac_set_all_multicast_mode(struct mac_pdata *pdata,
                     unsigned int enable)
{
    unsigned int val = enable ? 1 : 0;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = MAC_GET_REG_BITS(readl(pdata->mac_regs + MAC_PFR),
                     MAC_PFR_PM_POS, MAC_PFR_PM_LEN);
    if (regval == val)
        return 0;

    netif_dbg(pdata, drv, pdata->netdev, "%s allmulti mode\n",
          enable ? "entering" : "leaving");

    regval = readl(pdata->mac_regs + MAC_PFR);
    regval = MAC_SET_REG_BITS(regval, MAC_PFR_PM_POS,
                     MAC_PFR_PM_LEN, val);
    writel(regval, pdata->mac_regs + MAC_PFR);

    return 0;
}

static void mac_set_mac_addn_addrs(struct mac_pdata *pdata)
{
    struct net_device *netdev = pdata->netdev;
    struct netdev_hw_addr *ha;
    unsigned int addn_macs;
    unsigned int mac_reg;
    
    RNIC_TRACE_PRINT();

    mac_reg = MAC_MACA1HR;
    addn_macs = pdata->hw_feat.addn_mac;

    if (netdev_uc_count(netdev) > addn_macs) {
        mac_set_promiscuous_mode(pdata, 1);
    } else {
        netdev_for_each_uc_addr(ha, netdev) {
            mac_set_mac_reg(pdata, ha, &mac_reg);
            addn_macs--;
        }

        if (netdev_mc_count(netdev) > addn_macs) {
            mac_set_all_multicast_mode(pdata, 1);
        } else {
            netdev_for_each_mc_addr(ha, netdev) {
                mac_set_mac_reg(pdata, ha, &mac_reg);
                addn_macs--;
            }
        }
    }

    /* Clear remaining additional MAC address entries */
    while (addn_macs--)
        mac_set_mac_reg(pdata, NULL, &mac_reg);
}

static void mac_set_mac_hash_table(struct mac_pdata *pdata)
{
    unsigned int hash_table_shift, hash_table_count;
    u32 hash_table[MAC_MAC_HASH_TABLE_SIZE];
    struct net_device *netdev = pdata->netdev;
    struct netdev_hw_addr *ha;
    unsigned int hash_reg;
    unsigned int i;
    u32 crc;
    
    RNIC_TRACE_PRINT();

    hash_table_shift = 26 - (pdata->hw_feat.hash_table_size >> 7);
    hash_table_count = pdata->hw_feat.hash_table_size / 32;
    memset(hash_table, 0, sizeof(hash_table));

    /* Build the MAC Hash Table register values */
    netdev_for_each_uc_addr(ha, netdev) {
        crc = bitrev32(~crc32_le(~0, ha->addr, ETH_ALEN));
        crc >>= hash_table_shift;
        hash_table[crc >> 5] |= (1 << (crc & 0x1f));
    }

    netdev_for_each_mc_addr(ha, netdev) {
        crc = bitrev32(~crc32_le(~0, ha->addr, ETH_ALEN));
        crc >>= hash_table_shift;
        hash_table[crc >> 5] |= (1 << (crc & 0x1f));
    }

    /* Set the MAC Hash Table registers */
    hash_reg = MAC_HTR0;
    for (i = 0; i < hash_table_count; i++) {
        writel(hash_table[i], pdata->mac_regs + hash_reg);
        hash_reg += MAC_HTR_INC;
    }
}

static int mac_add_mac_addresses(struct mac_pdata *pdata)
{   
    RNIC_TRACE_PRINT();
    
    if (pdata->hw_feat.hash_table_size)
        mac_set_mac_hash_table(pdata);
    else
        mac_set_mac_addn_addrs(pdata);

    return 0;
}

static void mac_config_mac_address(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    mac_set_mac_address(pdata, pdata->netdev->dev_addr);

    /* Filtering is done using perfect filtering and hash filtering */
    if (pdata->hw_feat.hash_table_size) {
        regval = readl(pdata->mac_regs + MAC_PFR);
        regval = MAC_SET_REG_BITS(regval, MAC_PFR_HPF_POS,
                         MAC_PFR_HPF_LEN, 1);
        regval = MAC_SET_REG_BITS(regval, MAC_PFR_HUC_POS,
                         MAC_PFR_HUC_LEN, 1);
        regval = MAC_SET_REG_BITS(regval, MAC_PFR_HMC_POS,
                         MAC_PFR_HMC_LEN, 1);
        writel(regval, pdata->mac_regs + MAC_PFR);
    }
}

static void mac_config_jumbo_enable(struct mac_pdata *pdata)
{
    unsigned int val;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    val = (pdata->netdev->mtu > MAC_STD_PACKET_MTU) ? 1 : 0;

    regval = readl(pdata->mac_regs + MAC_RCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_JE_POS,
                     MAC_RCR_JE_LEN, val);
    writel(regval, pdata->mac_regs + MAC_RCR);
}

static void mac_config_checksum_offload(struct mac_pdata *pdata)
{
    if (pdata->netdev->features & NETIF_F_RXCSUM)
        mac_enable_rx_csum(pdata);
    else
        mac_disable_rx_csum(pdata);
}

static void mac_config_vlan_support(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_VLANIR);
    /* Indicate that VLAN Tx CTAGs come from context descriptors */
    regval = MAC_SET_REG_BITS(regval, MAC_VLANIR_CSVL_POS,
                     MAC_VLANIR_CSVL_LEN, 0);
    regval = MAC_SET_REG_BITS(regval, MAC_VLANIR_VLTI_POS,
                     MAC_VLANIR_VLTI_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_VLANIR);

    /* Set the current VLAN Hash Table register value */
    mac_update_vlan_hash_table(pdata);

    if (pdata->netdev->features & NETIF_F_HW_VLAN_CTAG_FILTER)
        mac_enable_rx_vlan_filtering(pdata);
    else
        mac_disable_rx_vlan_filtering(pdata);

    if (pdata->netdev->features & NETIF_F_HW_VLAN_CTAG_RX)
        mac_enable_rx_vlan_stripping(pdata);
    else
        mac_disable_rx_vlan_stripping(pdata);
}

static int mac_config_rx_mode(struct mac_pdata *pdata)
{
    struct net_device *netdev = pdata->netdev;
    unsigned int pr_mode, am_mode;
    
    RNIC_TRACE_PRINT();

    pr_mode = ((netdev->flags & IFF_PROMISC) != 0);
    am_mode = ((netdev->flags & IFF_ALLMULTI) != 0);

    mac_set_promiscuous_mode(pdata, pr_mode);
    mac_set_all_multicast_mode(pdata, am_mode);

    mac_add_mac_addresses(pdata);

    return 0;
}

static void mac_prepare_tx_stop(struct mac_pdata *pdata,
                   struct mac_channel *channel)
{
    unsigned int tx_dsr, tx_pos, tx_qidx;
    unsigned long tx_timeout;
    unsigned int tx_status;
    
    RNIC_TRACE_PRINT();

    /* Calculate the status register to read and the position within */
    if (channel->queue_index < DMA_DSRX_FIRST_QUEUE) {
        tx_dsr = DMA_DSR0;
        tx_pos = (channel->queue_index * DMA_DSR_Q_LEN) +
             DMA_DSR0_TPS_START;
    } else {
        tx_qidx = channel->queue_index - DMA_DSRX_FIRST_QUEUE;

        tx_dsr = DMA_DSR1 + ((tx_qidx / DMA_DSRX_QPR) * DMA_DSRX_INC);
        tx_pos = ((tx_qidx % DMA_DSRX_QPR) * DMA_DSR_Q_LEN) +
             DMA_DSRX_TPS_START;
    }

    /* The Tx engine cannot be stopped if it is actively processing
     * descriptors. Wait for the Tx engine to enter the stopped or
     * suspended state.  Don't wait forever though...
     */
    tx_timeout = jiffies + (MAC_DMA_STOP_TIMEOUT * HZ);
    while (time_before(jiffies, tx_timeout)) {
        tx_status = readl(pdata->mac_regs + tx_dsr);
        tx_status = MAC_GET_REG_BITS(tx_status, tx_pos,
                        DMA_DSR_TPS_LEN);
        if ((tx_status == DMA_TPS_STOPPED) ||
            (tx_status == DMA_TPS_SUSPENDED))
            break;

        usleep_range(500, 1000);
    }

    if (!time_before(jiffies, tx_timeout))
        netdev_info(pdata->netdev,
                "timed out waiting for Tx DMA channel %u to stop\n",
                channel->queue_index);
}

static void mac_enable_tx(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    /* Enable each Tx DMA channel */
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_TCR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_ST_POS,
                         DMA_CH_TCR_ST_LEN, 1);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_TCR));
    }

    /* Enable each Tx queue */
    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TXQEN_POS,
                         MTL_Q_TQOMR_TXQEN_LEN,
                    MTL_Q_ENABLED);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    /* Enable MAC Tx */
    regval = readl(pdata->mac_regs + MAC_TCR);
    regval = MAC_SET_REG_BITS(regval, MAC_TCR_TE_POS,
                     MAC_TCR_TE_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_TCR);
}

static void mac_disable_tx(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    /* Prepare for Tx DMA channel stop */
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        mac_prepare_tx_stop(pdata, channel);
    }

    /* Disable MAC Tx */
    regval = readl(pdata->mac_regs + MAC_TCR);
    regval = MAC_SET_REG_BITS(regval, MAC_TCR_TE_POS,
                     MAC_TCR_TE_LEN, 0);
    writel(regval, pdata->mac_regs + MAC_TCR);

    /* Disable each Tx queue */
    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TXQEN_POS,
                         MTL_Q_TQOMR_TXQEN_LEN, 0);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    /* Disable each Tx DMA channel */
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_TCR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_ST_POS,
                         DMA_CH_TCR_ST_LEN, 0);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_TCR));
    }
}

static void mac_prepare_rx_stop(struct mac_pdata *pdata,
                   unsigned int queue)
{
    unsigned int rx_status, prxq, rxqsts;
    unsigned long rx_timeout;
    
    RNIC_TRACE_PRINT();

    /* The Rx engine cannot be stopped if it is actively processing
     * packets. Wait for the Rx queue to empty the Rx fifo.  Don't
     * wait forever though...
     */
    rx_timeout = jiffies + (MAC_DMA_STOP_TIMEOUT * HZ);
    while (time_before(jiffies, rx_timeout)) {
        rx_status = readl(MAC_MTL_REG(pdata, queue, MTL_Q_RQDR));
        prxq = MAC_GET_REG_BITS(rx_status, MTL_Q_RQDR_PRXQ_POS,
                       MTL_Q_RQDR_PRXQ_LEN);
        rxqsts = MAC_GET_REG_BITS(rx_status, MTL_Q_RQDR_RXQSTS_POS,
                         MTL_Q_RQDR_RXQSTS_LEN);
        if ((prxq == 0) && (rxqsts == 0))
            break;

        usleep_range(500, 1000);
    }

    if (!time_before(jiffies, rx_timeout))
        netdev_info(pdata->netdev,
                "timed out waiting for Rx queue %u to empty\n",
                queue);
}

static void mac_enable_rx(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int regval, i;
    
    RNIC_TRACE_PRINT();

    /* Enable each Rx DMA channel */
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_RCR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_SR_POS,
                         DMA_CH_RCR_SR_LEN, 1);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_RCR));
    }

    /* Enable each Rx queue */
    regval = 0;
    for (i = 0; i < pdata->rx_q_count; i++)
        regval |= (0x02 << (i << 1));
    writel(regval, pdata->mac_regs + MAC_RQEC);

    /* Enable MAC Rx */
    regval = readl(pdata->mac_regs + MAC_RCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_DCRCC_POS,
                     MAC_RCR_DCRCC_LEN, 1);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_CST_POS,
                     MAC_RCR_CST_LEN, 1);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_ACS_POS,
                     MAC_RCR_ACS_LEN, 1);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_RE_POS,
                     MAC_RCR_RE_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_RCR);
}

static void mac_disable_rx(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    /* Disable MAC Rx */
    regval = readl(pdata->mac_regs + MAC_RCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_DCRCC_POS,
                     MAC_RCR_DCRCC_LEN, 0);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_CST_POS,
                     MAC_RCR_CST_LEN, 0);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_ACS_POS,
                     MAC_RCR_ACS_LEN, 0);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_RE_POS,
                     MAC_RCR_RE_LEN, 0);
    writel(regval, pdata->mac_regs + MAC_RCR);

    /* Prepare for Rx DMA channel stop */
    for (i = 0; i < pdata->rx_q_count; i++)
        mac_prepare_rx_stop(pdata, i);

    /* Disable each Rx queue */
    writel(0, pdata->mac_regs + MAC_RQEC);

    /* Disable each Rx DMA channel */
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_RCR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_SR_POS,
                         DMA_CH_RCR_SR_LEN, 0);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_RCR));
    }
}

static void mac_tx_start_xmit(struct mac_channel *channel, struct mac_ring *ring)
{
    struct mac_pdata *pdata = channel->pdata;
    struct mac_desc_data *desc_data;

    int ring_cur_step;
    struct timespec64 ts64;
    ktime_get_real_ts64(&ts64);

    
    RNIC_TRACE_PRINT();
    
    /* Make sure everything is written before the register write */
    wmb();

    /* Issue a poll command to Tx DMA by writing address
     * of next immediate free descriptor
     */
    desc_data = MAC_GET_DESC_DATA(ring, ring->cur);

    ring_cur_step = (MAC_TX_DESC_CNT + ring->cur - pdata->tx_desc_cur_last) % MAC_TX_DESC_CNT;

    writel(lower_32_bits(desc_data->dma_desc_addr), MAC_DMA_REG(channel, DMA_CH_TDTR_LO));


    //printk("ring->cur is %ld\n",ring->cur);
    //printk("ring_cur_step is %ld\n",ring_cur_step);
    //printk("desc_data->dma_desc_addr is %ld\n",desc_data->dma_desc_addr);
  
    //printk("ring->pkt_info.tx_bytes is %d\n",ring->pkt_info.tx_bytes);
    

    if(pdata->tx_desc_update_cnt == 0)
    {
        //pdata->tx_desc_update_time_nsec_first = jiffies_to_usecs(jiffies);
        pdata->tx_desc_update_time_nsec_first = ts64.tv_sec * 1000000000 + ts64.tv_nsec;
        pdata->tx_desc_update_time_nsec_last  = ts64.tv_sec * 1000000000 + ts64.tv_nsec;
        //pdata->tx_desc_update_time_nsec_last  = jiffies_to_usecs(jiffies);
    }

    pdata->tx_desc_update_cnt++;
    pdata->tx_desc_cur_update_total += ring_cur_step;
    pdata->tx_desc_cur_last = ring->cur;
    
#ifdef PRINT_AVERAGE
//#if 1
    if(pdata->tx_desc_update_cnt % MAC_TX_DESC_CNT == 0)
    {
        printk("average tx desc update number    per time is %d\n",pdata->tx_desc_cur_update_total/pdata->tx_desc_update_cnt);
        printk("current tx desc update interval  per time is %lld ns\n",(ts64.tv_sec * 1000000000 + ts64.tv_nsec - pdata->tx_desc_update_time_nsec_last)/MAC_TX_DESC_CNT);
        pdata->tx_desc_update_time_nsec_last = ts64.tv_sec * 1000000000 + ts64.tv_nsec;
    }
#endif    

    /* Start the Tx timer */
    if (pdata->tx_usecs && !channel->tx_timer_active)
    {
        channel->tx_timer_active = 1;
        mod_timer(&channel->tx_timer, jiffies + usecs_to_jiffies(pdata->tx_usecs));
    }

    ring->tx.xmit_more = 0;
}

static void mac_dev_xmit(struct mac_channel *channel)
{
    struct mac_pdata *pdata = channel->pdata;
    struct mac_ring *ring = channel->tx_ring;
    unsigned int tso_context, vlan_context;
    struct mac_desc_data *desc_data;
    struct mac_dma_desc *dma_desc;
    struct mac_pkt_info *pkt_info;
    unsigned int csum, tso, vlan;
    int start_index = ring->cur;
    int cur_index = ring->cur;
    unsigned int tx_set_ic;
    int i;
    
    RNIC_TRACE_PRINT();

    pkt_info = &ring->pkt_info;
    csum = MAC_GET_REG_BITS(pkt_info->attributes,
                   TX_PACKET_ATTRIBUTES_CSUM_ENABLE_POS,
                TX_PACKET_ATTRIBUTES_CSUM_ENABLE_LEN);
    tso = MAC_GET_REG_BITS(pkt_info->attributes,
                  TX_PACKET_ATTRIBUTES_TSO_ENABLE_POS,
                TX_PACKET_ATTRIBUTES_TSO_ENABLE_LEN);
    vlan = MAC_GET_REG_BITS(pkt_info->attributes,
                   TX_PACKET_ATTRIBUTES_VLAN_CTAG_POS,
                TX_PACKET_ATTRIBUTES_VLAN_CTAG_LEN);

    if (tso && (pkt_info->mss != ring->tx.cur_mss))
        tso_context = 1;
    else
        tso_context = 0;

    if (vlan && (pkt_info->vlan_ctag != ring->tx.cur_vlan_ctag))
        vlan_context = 1;
    else
        vlan_context = 0;

    /* Determine if an interrupt should be generated for this Tx:
     *   Interrupt:
     *     - Tx frame count exceeds the frame count setting
     *     - Addition of Tx frame count to the frame count since the
     *       last interrupt was set exceeds the frame count setting
     *   No interrupt:
     *     - No frame count setting specified (ethtool -C ethX tx-frames 0)
     *     - Addition of Tx frame count to the frame count since the
     *       last interrupt was set does not exceed the frame count setting
     */
    ring->coalesce_count += pkt_info->tx_packets;
    if (!pdata->tx_frames)
        tx_set_ic = 0;
    else if (pkt_info->tx_packets > pdata->tx_frames)
        tx_set_ic = 1;
    else if ((ring->coalesce_count % pdata->tx_frames) <
         pkt_info->tx_packets)
        tx_set_ic = 1;
    else
        tx_set_ic = 0;

    desc_data = MAC_GET_DESC_DATA(ring, cur_index);
    dma_desc = desc_data->dma_desc;

    /* Create a context descriptor if this is a TSO pkt_info */
    if (tso_context || vlan_context) {
        if (tso_context) {
            netif_dbg(pdata, tx_queued, pdata->netdev,
                  "TSO context descriptor, mss=%u\n",
                  pkt_info->mss);

            /* Set the MSS size */
            dma_desc->desc2 = MAC_SET_REG_BITS_LE(
                        dma_desc->desc2,
                        TX_CONTEXT_DESC2_MSS_POS,
                        TX_CONTEXT_DESC2_MSS_LEN,
                        pkt_info->mss);

            /* Mark it as a CONTEXT descriptor */
            dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_CTXT_POS,
                        TX_CONTEXT_DESC3_CTXT_LEN,
                        1);

            /* Indicate this descriptor contains the MSS */
            dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_TCMSSV_POS,
                        TX_CONTEXT_DESC3_TCMSSV_LEN,
                        1);

            ring->tx.cur_mss = pkt_info->mss;
        }

        if (vlan_context) {
            netif_dbg(pdata, tx_queued, pdata->netdev,
                  "VLAN context descriptor, ctag=%u\n",
                  pkt_info->vlan_ctag);

            /* Mark it as a CONTEXT descriptor */
            dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_CTXT_POS,
                        TX_CONTEXT_DESC3_CTXT_LEN,
                        1);

            /* Set the VLAN tag */
            dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_VT_POS,
                        TX_CONTEXT_DESC3_VT_LEN,
                        pkt_info->vlan_ctag);

            /* Indicate this descriptor contains the VLAN tag */
            dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_VLTV_POS,
                        TX_CONTEXT_DESC3_VLTV_LEN,
                        1);

            ring->tx.cur_vlan_ctag = pkt_info->vlan_ctag;
        }

        cur_index++;
        desc_data = MAC_GET_DESC_DATA(ring, cur_index);
        dma_desc = desc_data->dma_desc;
    }

    /* Update buffer address (for TSO this is the header) */
    dma_desc->desc0 =  cpu_to_le32(lower_32_bits(desc_data->skb_dma));
    dma_desc->desc1 =  cpu_to_le32(upper_32_bits(desc_data->skb_dma));

    /* Update the buffer length */
    dma_desc->desc2 = MAC_SET_REG_BITS_LE(
                dma_desc->desc2,
                TX_NORMAL_DESC2_HL_B1L_POS,
                TX_NORMAL_DESC2_HL_B1L_LEN,
                desc_data->skb_dma_len);

    /* VLAN tag insertion check */
    if (vlan) {
        dma_desc->desc2 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc2,
                    TX_NORMAL_DESC2_VTIR_POS,
                    TX_NORMAL_DESC2_VTIR_LEN,
                    TX_NORMAL_DESC2_VLAN_INSERT);
        pdata->stats.tx_vlan_packets++;
    }

    /* Timestamp enablement check */
    if (MAC_GET_REG_BITS(pkt_info->attributes,
                TX_PACKET_ATTRIBUTES_PTP_POS,
                TX_PACKET_ATTRIBUTES_PTP_LEN))
        dma_desc->desc2 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc2,
                    TX_NORMAL_DESC2_TTSE_POS,
                    TX_NORMAL_DESC2_TTSE_LEN,
                    1);

    /* Mark it as First Descriptor */
    dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                TX_NORMAL_DESC3_FD_POS,
                TX_NORMAL_DESC3_FD_LEN,
                1);

    /* Mark it as a NORMAL descriptor */
    dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                TX_NORMAL_DESC3_CTXT_POS,
                TX_NORMAL_DESC3_CTXT_LEN,
                0);

    /* Set OWN bit if not the first descriptor */
    if (cur_index != start_index)
        dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_OWN_POS,
                    TX_NORMAL_DESC3_OWN_LEN,
                    1);

    if (tso) {
        /* Enable TSO */
        dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_TSE_POS,
                    TX_NORMAL_DESC3_TSE_LEN, 1);
        dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_TCPPL_POS,
                    TX_NORMAL_DESC3_TCPPL_LEN,
                    pkt_info->tcp_payload_len);
        dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_TCPHDRLEN_POS,
                    TX_NORMAL_DESC3_TCPHDRLEN_LEN,
                    pkt_info->tcp_header_len / 4);

        pdata->stats.tx_tso_packets++;
    } else {
        /* Enable CRC and Pad Insertion */
        dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_CPC_POS,
                    TX_NORMAL_DESC3_CPC_LEN, 0);

        /* Enable HW CSUM */
        if (csum)
            dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_NORMAL_DESC3_CIC_POS,
                        TX_NORMAL_DESC3_CIC_LEN,
                        0x3);

        /* Set the total length to be transmitted */
        dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_FL_POS,
                    TX_NORMAL_DESC3_FL_LEN,
                    pkt_info->length);
    }

    for (i = cur_index - start_index + 1; i < pkt_info->desc_count; i++) {
        cur_index++;
        desc_data = MAC_GET_DESC_DATA(ring, cur_index);
        dma_desc = desc_data->dma_desc;

        /* Update buffer address */
        dma_desc->desc0 =
            cpu_to_le32(lower_32_bits(desc_data->skb_dma));
        dma_desc->desc1 =
            cpu_to_le32(upper_32_bits(desc_data->skb_dma));

        /* Update the buffer length */
        dma_desc->desc2 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc2,
                    TX_NORMAL_DESC2_HL_B1L_POS,
                    TX_NORMAL_DESC2_HL_B1L_LEN,
                    desc_data->skb_dma_len);

        /* Set OWN bit */
        dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_OWN_POS,
                    TX_NORMAL_DESC3_OWN_LEN, 1);

        /* Mark it as NORMAL descriptor */
        dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_CTXT_POS,
                    TX_NORMAL_DESC3_CTXT_LEN, 0);

        /* Enable HW CSUM */
        if (csum)
            dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_NORMAL_DESC3_CIC_POS,
                        TX_NORMAL_DESC3_CIC_LEN,
                        0x3);
    }

    /* Set LAST bit for the last descriptor */
    dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                TX_NORMAL_DESC3_LD_POS,
                TX_NORMAL_DESC3_LD_LEN, 1);

    /* Set IC bit based on Tx coalescing settings */
    if (tx_set_ic)
        dma_desc->desc2 = MAC_SET_REG_BITS_LE(
                    dma_desc->desc2,
                    TX_NORMAL_DESC2_IC_POS,
                    TX_NORMAL_DESC2_IC_LEN, 1);

    /* Save the Tx info to report back during cleanup */
    desc_data->tx.packets = pkt_info->tx_packets;
    desc_data->tx.bytes = pkt_info->tx_bytes;

    /* In case the Tx DMA engine is running, make sure everything
     * is written to the descriptor(s) before setting the OWN bit
     * for the first descriptor
     */
    dma_wmb();

    /* Set OWN bit for the first descriptor */
    desc_data = MAC_GET_DESC_DATA(ring, start_index);
    dma_desc = desc_data->dma_desc;
    dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                TX_NORMAL_DESC3_OWN_POS,
                TX_NORMAL_DESC3_OWN_LEN, 1);

    if (netif_msg_tx_queued(pdata))
        mac_dump_tx_desc(pdata, ring, start_index,
                    pkt_info->desc_count, 1);

    /* Make sure ownership is written to the descriptor */
    smp_wmb();

    ring->cur = cur_index + 1;
    if (!pkt_info->skb->xmit_more || netif_xmit_stopped(netdev_get_tx_queue(pdata->netdev, channel->queue_index)))
        mac_tx_start_xmit(channel, ring);
    else
        ring->tx.xmit_more = 1;

    RNIC_PRINTK("%s: descriptors %u to %u written\n",
          channel->name, start_index & (ring->dma_desc_count - 1),
          (ring->cur - 1) & (ring->dma_desc_count - 1));
}

static void mac_get_rx_tstamp(struct mac_pkt_info *pkt_info, struct mac_dma_desc *dma_desc)
{
    u32 tsa, tsd;
    u64 nsec;
    
    RNIC_TRACE_PRINT();

    tsa = MAC_GET_REG_BITS_LE(dma_desc->desc3,
                     RX_CONTEXT_DESC3_TSA_POS,
                RX_CONTEXT_DESC3_TSA_LEN);
    tsd = MAC_GET_REG_BITS_LE(dma_desc->desc3,
                     RX_CONTEXT_DESC3_TSD_POS,
                RX_CONTEXT_DESC3_TSD_LEN);
    if (tsa && !tsd) {
        nsec = le32_to_cpu(dma_desc->desc1);
        nsec <<= 32;
        nsec |= le32_to_cpu(dma_desc->desc0);
        if (nsec != 0xffffffffffffffffULL) {
            pkt_info->rx_tstamp = nsec;
            pkt_info->attributes = MAC_SET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_RX_TSTAMP_POS,
                    RX_PACKET_ATTRIBUTES_RX_TSTAMP_LEN,
                    1);
        }
    }
}

static void mac_tx_desc_reset(struct mac_desc_data *desc_data)
{
    struct mac_dma_desc *dma_desc = desc_data->dma_desc;
    
    RNIC_TRACE_PRINT_DESC(); //insomnia

    /* Reset the Tx descriptor
     *   Set buffer 1 (lo) address to zero
     *   Set buffer 1 (hi) address to zero
     *   Reset all other control bits (IC, TTSE, B2L & B1L)
     *   Reset all other control bits (OWN, CTXT, FD, LD, CPC, CIC, etc)
     */
    dma_desc->desc0 = 0;
    dma_desc->desc1 = 0;
    dma_desc->desc2 = 0;
    dma_desc->desc3 = 0;

    /* Make sure ownership is written to the descriptor */
    dma_wmb();
}

static void mac_tx_desc_init(struct mac_channel *channel)
{
    struct mac_ring *ring = channel->tx_ring;
    struct mac_desc_data *desc_data;
    int start_index = ring->cur;
    int i;
    
    RNIC_TRACE_PRINT();

    /* Initialze all descriptors */
    for (i = 0; i < ring->dma_desc_count; i++) {
        desc_data = MAC_GET_DESC_DATA(ring, i);

        /* Initialize Tx descriptor */
        mac_tx_desc_reset(desc_data);
    }

    /* Update the total number of Tx descriptors */
    writel(ring->dma_desc_count - 1, MAC_DMA_REG(channel, DMA_CH_TDRLR));

    /* Update the starting address of descriptor ring */
    desc_data = MAC_GET_DESC_DATA(ring, start_index);
    writel(upper_32_bits(desc_data->dma_desc_addr),
           MAC_DMA_REG(channel, DMA_CH_TDLR_HI));
    writel(lower_32_bits(desc_data->dma_desc_addr),
           MAC_DMA_REG(channel, DMA_CH_TDLR_LO));
    
    writel(lower_32_bits(desc_data->dma_desc_addr),
           MAC_DMA_REG(channel, DMA_CH_TDTR_LO));
}

static void mac_rx_desc_reset(struct mac_pdata *pdata,
                 struct mac_desc_data *desc_data,
                 unsigned int index)
{
    struct mac_dma_desc *dma_desc = desc_data->dma_desc;
    unsigned int rx_frames = pdata->rx_frames;
    unsigned int rx_usecs = pdata->rx_usecs;
    dma_addr_t hdr_dma, buf_dma;
    unsigned int inte;
    
    RNIC_TRACE_PRINT_DESC(); //insomnia

    if (!rx_usecs && !rx_frames) {
        /* No coalescing, interrupt for every descriptor */
        inte = 1;
    } else {
        /* Set interrupt based on Rx frame coalescing setting */
        if (rx_frames && !((index + 1) % rx_frames))
            inte = 1;
        else
            inte = 0;
    }

    /* Reset the Rx descriptor
     *   Set buffer 1 (lo) address to header dma address (lo)
     *   Set buffer 1 (hi) address to header dma address (hi)
     *   Set buffer 2 (lo) address to buffer dma address (lo)
     *   Set buffer 2 (hi) address to buffer dma address (hi) and
     *     set control bits OWN and INTE
     */
    hdr_dma = desc_data->rx.hdr.dma_base + desc_data->rx.hdr.dma_off;
    buf_dma = desc_data->rx.buf.dma_base + desc_data->rx.buf.dma_off;
    dma_desc->desc0 = cpu_to_le32(lower_32_bits(hdr_dma));
    dma_desc->desc1 = cpu_to_le32(upper_32_bits(hdr_dma));
    dma_desc->desc2 = cpu_to_le32(lower_32_bits(buf_dma));
    dma_desc->desc3 = cpu_to_le32(upper_32_bits(buf_dma));

    dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                RX_NORMAL_DESC3_INTE_POS,
                RX_NORMAL_DESC3_INTE_LEN,
                inte);

    /* Since the Rx DMA engine is likely running, make sure everything
     * is written to the descriptor(s) before setting the OWN bit
     * for the descriptor
     */
    dma_wmb();

    dma_desc->desc3 = MAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                RX_NORMAL_DESC3_OWN_POS,
                RX_NORMAL_DESC3_OWN_LEN,
                1);


    /* Make sure ownership is written to the descriptor */
    dma_wmb();
}

static void mac_rx_desc_init(struct mac_channel *channel)
{
    struct mac_pdata *pdata = channel->pdata;
    struct mac_ring *ring = channel->rx_ring;
    unsigned int start_index = ring->cur;
    struct mac_desc_data *desc_data;
    unsigned int i;
    
    RNIC_TRACE_PRINT();

    /* Initialize all descriptors */
    for (i = 0; i < ring->dma_desc_count; i++) {
        desc_data = MAC_GET_DESC_DATA(ring, i);

        /* Initialize Rx descriptor */
        mac_rx_desc_reset(pdata, desc_data, i);
    }

    /* Update the total number of Rx descriptors */
    writel(ring->dma_desc_count - 1, MAC_DMA_REG(channel, DMA_CH_RDRLR));

    /* Update the starting address of descriptor ring */
    desc_data = MAC_GET_DESC_DATA(ring, start_index);
    writel(upper_32_bits(desc_data->dma_desc_addr),
           MAC_DMA_REG(channel, DMA_CH_RDLR_HI));
    writel(lower_32_bits(desc_data->dma_desc_addr),
           MAC_DMA_REG(channel, DMA_CH_RDLR_LO));

    /* Update the Rx Descriptor Tail Pointer */
    desc_data = MAC_GET_DESC_DATA(ring, start_index +
                      ring->dma_desc_count - 1);
    writel(lower_32_bits(desc_data->dma_desc_addr),
           MAC_DMA_REG(channel, DMA_CH_RDTR_LO));
}

static int mac_is_context_desc(struct mac_dma_desc *dma_desc)
{   
    RNIC_TRACE_PRINT();
    
    /* Rx and Tx share CTXT bit, so check TDES3.CTXT bit */
    return MAC_GET_REG_BITS_LE(dma_desc->desc3,
                TX_NORMAL_DESC3_CTXT_POS,
                TX_NORMAL_DESC3_CTXT_LEN);
}

static int mac_is_last_desc(struct mac_dma_desc *dma_desc)
{   
    RNIC_TRACE_PRINT();
    
    /* Rx and Tx share LD bit, so check TDES3.LD bit */
    return MAC_GET_REG_BITS_LE(dma_desc->desc3,
                TX_NORMAL_DESC3_LD_POS,
                TX_NORMAL_DESC3_LD_LEN);
}

static int mac_disable_tx_flow_control(struct mac_pdata *pdata)
{
    unsigned int max_q_count, q_count;
    unsigned int reg, regval;
    unsigned int i;
    
    RNIC_TRACE_PRINT();

    /* Clear MTL flow control */
    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_EHFC_POS,
                         MTL_Q_RQOMR_EHFC_LEN, 0);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    /* Clear MAC flow control */
    max_q_count = MAC_MAX_FLOW_CONTROL_QUEUES;
    q_count = min_t(unsigned int, pdata->tx_q_count, max_q_count);
    reg = MAC_Q0TFCR;
    for (i = 0; i < q_count; i++) {
        regval = readl(pdata->mac_regs + reg);
        regval = MAC_SET_REG_BITS(regval,
                         MAC_Q0TFCR_TFE_POS,
                    MAC_Q0TFCR_TFE_LEN,
                    0);
        writel(regval, pdata->mac_regs + reg);

        reg += MAC_QTFCR_INC;
    }

    return 0;
}

static int mac_enable_tx_flow_control(struct mac_pdata *pdata)
{
    unsigned int max_q_count, q_count;
    unsigned int reg, regval;
    unsigned int i;
    
    RNIC_TRACE_PRINT();

    /* Set MTL flow control */
    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_EHFC_POS,
                         MTL_Q_RQOMR_EHFC_LEN, 1);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    /* Set MAC flow control */
    max_q_count = MAC_MAX_FLOW_CONTROL_QUEUES;
    q_count = min_t(unsigned int, pdata->tx_q_count, max_q_count);
    reg = MAC_Q0TFCR;
    for (i = 0; i < q_count; i++) {
        regval = readl(pdata->mac_regs + reg);

        /* Enable transmit flow control */
        regval = MAC_SET_REG_BITS(regval, MAC_Q0TFCR_TFE_POS,
                         MAC_Q0TFCR_TFE_LEN, 1);
        /* Set pause time */
        regval = MAC_SET_REG_BITS(regval, MAC_Q0TFCR_PT_POS,
                         MAC_Q0TFCR_PT_LEN, 0xffff);

        writel(regval, pdata->mac_regs + reg);

        reg += MAC_QTFCR_INC;
    }

    return 0;
}

static int mac_disable_rx_flow_control(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_RFCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RFCR_RFE_POS,
                     MAC_RFCR_RFE_LEN, 0);
    writel(regval, pdata->mac_regs + MAC_RFCR);

    return 0;
}

static int mac_enable_rx_flow_control(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(pdata->mac_regs + MAC_RFCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RFCR_RFE_POS,
                     MAC_RFCR_RFE_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_RFCR);

    return 0;
}

static int mac_config_tx_flow_control(struct mac_pdata *pdata)
{   
    RNIC_TRACE_PRINT();
    
    if (pdata->tx_pause)
        mac_enable_tx_flow_control(pdata);
    else
        mac_disable_tx_flow_control(pdata);

    return 0;
}

static int mac_config_rx_flow_control(struct mac_pdata *pdata)
{   
    RNIC_TRACE_PRINT();
    
    if (pdata->rx_pause)
        mac_enable_rx_flow_control(pdata);
    else
        mac_disable_rx_flow_control(pdata);
	
    return 0;
}

static int mac_config_rx_coalesce(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();
    
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_RIWT));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_RIWT_RWT_POS,
                         DMA_CH_RIWT_RWT_LEN,
                         pdata->rx_riwt);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_RIWT));
    }

    return 0;
}

static void mac_config_flow_control(struct mac_pdata *pdata)
{   
    RNIC_TRACE_PRINT();
    
    mac_config_tx_flow_control(pdata);
    mac_config_rx_flow_control(pdata);
}

static void mac_config_rx_fep_enable(struct mac_pdata *pdata)
{
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_FEP_POS,
                         MTL_Q_RQOMR_FEP_LEN, 1);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }
}

static void mac_config_rx_fup_enable(struct mac_pdata *pdata)
{
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_FUP_POS,
                         MTL_Q_RQOMR_FUP_LEN, 1);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }
}

static int mac_config_tx_coalesce(struct mac_pdata *pdata)
{   
    RNIC_TRACE_PRINT();

    return 0;
}

static void mac_config_rx_buffer_size(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_RCR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_RBSZ_POS,
                         DMA_CH_RCR_RBSZ_LEN,
                    pdata->rx_buf_size);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_RCR));
    }
}

static void mac_config_tso_mode(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        if (pdata->hw_feat.tso) {
            regval = readl(MAC_DMA_REG(channel, DMA_CH_TCR));
            regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_TSE_POS,
                             DMA_CH_TCR_TSE_LEN, 1);
            writel(regval, MAC_DMA_REG(channel, DMA_CH_TCR));
        }
    }
}

static void mac_config_sph_mode(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_CR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_CR_SPH_POS,
                         DMA_CH_CR_SPH_LEN, 1);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_CR));
    }

    regval = readl(pdata->mac_regs + MAC_RCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RCR_HDSMS_POS,
                     MAC_RCR_HDSMS_LEN,
                MAC_SPH_HDSMS_SIZE);
    writel(regval, pdata->mac_regs + MAC_RCR);
}

static unsigned int mac_usec_to_riwt(struct mac_pdata *pdata,
                    unsigned int usec)
{
    unsigned long rate;
    unsigned int ret;
    
    RNIC_TRACE_PRINT();

    rate = pdata->sysclk_rate;

    /* Convert the input usec value to the watchdog timer value. Each
     * watchdog timer value is equivalent to 256 clock cycles.
     * Calculate the required value as:
     *   ( usec * ( system_clock_mhz / 10^6 ) / 256
     */
    ret = (usec * (rate / 1000000)) / 256;

    return ret;
}

static unsigned int mac_riwt_to_usec(struct mac_pdata *pdata,
                    unsigned int riwt)
{
    unsigned long rate;
    unsigned int ret;
    
    RNIC_TRACE_PRINT();

    rate = pdata->sysclk_rate;

    /* Convert the input watchdog timer value to the usec value. Each
     * watchdog timer value is equivalent to 256 clock cycles.
     * Calculate the required value as:
     *   ( riwt * 256 ) / ( system_clock_mhz / 10^6 )
     */
    ret = (riwt * 256) / (rate / 1000000);

    return ret;
}

static int mac_config_rx_threshold(struct mac_pdata *pdata,
                      unsigned int val)
{
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RTC_POS,
                         MTL_Q_RQOMR_RTC_LEN, val);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    return 0;
}

static void mac_config_mtl_mode(struct mac_pdata *pdata)
{
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    /* Set Tx to weighted round robin scheduling algorithm */
    regval = readl(pdata->mac_regs + MTL_OMR);
    regval = MAC_SET_REG_BITS(regval, MTL_OMR_ETSALG_POS,
                     MTL_OMR_ETSALG_LEN, MTL_ETSALG_WRR);
    writel(regval, pdata->mac_regs + MTL_OMR);

    /* Set Tx traffic classes to use WRR algorithm with equal weights */
    for (i = 0; i < pdata->hw_feat.tc_cnt; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_TC_ETSCR));
        regval = MAC_SET_REG_BITS(regval, MTL_TC_ETSCR_TSA_POS,
                         MTL_TC_ETSCR_TSA_LEN, MTL_TSA_ETS);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_TC_ETSCR));

        regval = readl(MAC_MTL_REG(pdata, i, MTL_TC_QWR));
        regval = MAC_SET_REG_BITS(regval, MTL_TC_QWR_QW_POS,
                         MTL_TC_QWR_QW_LEN, 1);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_TC_QWR));
    }

    /* Set Rx to strict priority algorithm */
    regval = readl(pdata->mac_regs + MTL_OMR);
    regval = MAC_SET_REG_BITS(regval, MTL_OMR_RAA_POS,
                     MTL_OMR_RAA_LEN, MTL_RAA_SP);
    writel(regval, pdata->mac_regs + MTL_OMR);
}

static void mac_config_queue_mapping(struct mac_pdata *pdata)
{
    unsigned int ppq, ppq_extra, prio, prio_queues;
    unsigned int qptc, qptc_extra, queue;
    unsigned int reg, regval;
    unsigned int mask;
    unsigned int i, j;
    
    RNIC_TRACE_PRINT();

    /* Map the MTL Tx Queues to Traffic Classes
     *   Note: Tx Queues >= Traffic Classes
     */
    qptc = pdata->tx_q_count / pdata->hw_feat.tc_cnt;
    qptc_extra = pdata->tx_q_count % pdata->hw_feat.tc_cnt;

    for (i = 0, queue = 0; i < pdata->hw_feat.tc_cnt; i++) {
        for (j = 0; j < qptc; j++) {
            netif_dbg(pdata, drv, pdata->netdev,
                  "TXq%u mapped to TC%u\n", queue, i);
            regval = readl(MAC_MTL_REG(pdata, queue,
                              MTL_Q_TQOMR));
            regval = MAC_SET_REG_BITS(regval,
                             MTL_Q_TQOMR_Q2TCMAP_POS,
                             MTL_Q_TQOMR_Q2TCMAP_LEN,
                             i);
            writel(regval, MAC_MTL_REG(pdata, queue,
                              MTL_Q_TQOMR));
            queue++;
        }

        if (i < qptc_extra) {
            netif_dbg(pdata, drv, pdata->netdev,
                  "TXq%u mapped to TC%u\n", queue, i);
            regval = readl(MAC_MTL_REG(pdata, queue,
                              MTL_Q_TQOMR));
            regval = MAC_SET_REG_BITS(regval,
                             MTL_Q_TQOMR_Q2TCMAP_POS,
                             MTL_Q_TQOMR_Q2TCMAP_LEN,
                             i);
            writel(regval, MAC_MTL_REG(pdata, queue,
                              MTL_Q_TQOMR));
            queue++;
        }
    }

    /* Map the 8 VLAN priority values to available MTL Rx queues */
    prio_queues = min_t(unsigned int, IEEE_8021QAZ_MAX_TCS,
                pdata->rx_q_count);
    ppq = IEEE_8021QAZ_MAX_TCS / prio_queues;
    ppq_extra = IEEE_8021QAZ_MAX_TCS % prio_queues;

    reg = MAC_RQMC;
    regval = 0;

    
    for (i = 0, prio = 0; i < prio_queues;) {
        mask = 0;
        for (j = 0; j < ppq; j++) {
            netif_dbg(pdata, drv, pdata->netdev,
                  "PRIO%u mapped to RXq%u\n", prio, i);
            mask |= (1 << prio);
            prio++;
        }

        if (i < ppq_extra) {
            netif_dbg(pdata, drv, pdata->netdev,
                  "PRIO%u mapped to RXq%u\n", prio, i);
            mask |= (1 << prio);
            prio++;
        }

        regval |= (mask << ((i++ % MAC_RQMC_Q_PER_REG) << 3));

        if ((i % MAC_RQMC_Q_PER_REG) && (i != prio_queues))
            continue;

        writel(regval, pdata->mac_regs + reg);
        
        reg += MAC_RQMC_INC;
        regval = 0;
    }

    /* Configure one to one, MTL Rx queue to DMA Rx channel mapping
     *  ie Q0 <--> CH0, Q1 <--> CH1 ... Q11 <--> CH11
     */
    reg = MTL_RQDCM0R;
    regval = readl(pdata->mac_regs + reg);
    regval |= (MTL_RQDCM0R_Q0MDMACH | MTL_RQDCM0R_Q1MDMACH |
            MTL_RQDCM0R_Q2MDMACH | MTL_RQDCM0R_Q3MDMACH);
    writel(regval, pdata->mac_regs + reg);

    reg += MTL_RQDCM_INC;
    regval = readl(pdata->mac_regs + reg);
    regval |= (MTL_RQDCM1R_Q4MDMACH | MTL_RQDCM1R_Q5MDMACH |
            MTL_RQDCM1R_Q6MDMACH | MTL_RQDCM1R_Q7MDMACH);
    writel(regval, pdata->mac_regs + reg);

    reg += MTL_RQDCM_INC;
    regval = readl(pdata->mac_regs + reg);
    regval |= (MTL_RQDCM2R_Q8MDMACH | MTL_RQDCM2R_Q9MDMACH |
            MTL_RQDCM2R_Q10MDMACH | MTL_RQDCM2R_Q11MDMACH);
    writel(regval, pdata->mac_regs + reg);
}

static unsigned int mac_calculate_per_queue_fifo(
                    unsigned int fifo_size,
                    unsigned int queue_count)
{
    unsigned int q_fifo_size;
    unsigned int p_fifo;
    
    RNIC_TRACE_PRINT();

    /* Calculate the configured fifo size */
    q_fifo_size = 1 << (fifo_size + 7);

    /* The configured value may not be the actual amount of fifo RAM */
    q_fifo_size = min_t(unsigned int, MAC_MAX_FIFO, q_fifo_size);

    q_fifo_size = q_fifo_size / queue_count;

    /* Each increment in the queue fifo size represents 256 bytes of
     * fifo, with 0 representing 256 bytes. Distribute the fifo equally
     * between the queues.
     */
    p_fifo = q_fifo_size / 256;
    if (p_fifo)
        p_fifo--;

    return p_fifo;
}

static void mac_config_tx_fifo_size(struct mac_pdata *pdata)
{
    unsigned int fifo_size;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();
#if 0
    fifo_size = mac_calculate_per_queue_fifo(
                pdata->hw_feat.tx_fifo_size,
                pdata->tx_q_count);
#endif
	fifo_size = 183;

    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TQS_POS,
                         MTL_Q_TQOMR_TQS_LEN, fifo_size);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    netif_info(pdata, drv, pdata->netdev,
           "%d Tx hardware queues, %d byte fifo per queue\n",
           pdata->tx_q_count, ((fifo_size + 1) * 256));
}

static void mac_config_rx_fifo_size(struct mac_pdata *pdata)
{
    unsigned int fifo_size;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

#if 0
    fifo_size = mac_calculate_per_queue_fifo(
                    pdata->hw_feat.rx_fifo_size,
                    pdata->rx_q_count);
#endif
	fifo_size = 183;

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RQS_POS,
                         MTL_Q_RQOMR_RQS_LEN, fifo_size);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    netif_info(pdata, drv, pdata->netdev,
           "%d Rx hardware queues, %d byte fifo per queue\n",
           pdata->rx_q_count, ((fifo_size + 1) * 256));
}

static void mac_config_flow_control_threshold(struct mac_pdata *pdata)
{
    unsigned int i;
    u32 regval;

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_RQFCR));
        /* Activate flow control when less than 4k left in fifo */
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQFCR_RFA_POS,
                         MTL_Q_RQFCR_RFA_LEN, 2);
        /* De-activate flow control when more than 6k left in fifo */
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQFCR_RFD_POS,
                         MTL_Q_RQFCR_RFD_LEN, 4);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_RQFCR));
    }
}

static int mac_config_tx_threshold(struct mac_pdata *pdata,
                      unsigned int val)
{
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TTC_POS,
                         MTL_Q_TQOMR_TTC_LEN, val);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    return 0;
}

static int mac_config_rsf_mode(struct mac_pdata *pdata,
                  unsigned int val)
{
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RSF_POS,
                         MTL_Q_RQOMR_RSF_LEN, val);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    return 0;
}

static int mac_config_tsf_mode(struct mac_pdata *pdata,
                  unsigned int val)
{
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TSF_POS,
                         MTL_Q_TQOMR_TSF_LEN, val);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    return 0;
}

static int mac_config_osp_mode(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_TCR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_OSP_POS,
                         DMA_CH_TCR_OSP_LEN,
                    pdata->tx_osp_mode);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_TCR));
    }

    return 0;
}

static int mac_config_pblx8(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        regval = readl(MAC_DMA_REG(channel, DMA_CH_CR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_CR_PBLX8_POS,
                         DMA_CH_CR_PBLX8_LEN,
                    pdata->pblx8);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_CR));
    }

    return 0;
}

static int mac_get_tx_pbl_val(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(MAC_DMA_REG(pdata->channel_head, DMA_CH_TCR));
    regval = MAC_GET_REG_BITS(regval, DMA_CH_TCR_PBL_POS,
                     DMA_CH_TCR_PBL_LEN);
    return regval;
}

static int mac_config_tx_pbl_val(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_TCR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_TCR_PBL_POS,
                         DMA_CH_TCR_PBL_LEN,
                    pdata->tx_pbl);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_TCR));
    }

    return 0;
}

static int mac_get_rx_pbl_val(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();

    regval = readl(MAC_DMA_REG(pdata->channel_head, DMA_CH_RCR));
    regval = MAC_GET_REG_BITS(regval, DMA_CH_RCR_PBL_POS,
                     DMA_CH_RCR_PBL_LEN);
    return regval;
}

static int mac_config_rx_pbl_val(struct mac_pdata *pdata)
{
    struct mac_channel *channel;
    unsigned int i;
    u32 regval;
    
    RNIC_TRACE_PRINT();

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readl(MAC_DMA_REG(channel, DMA_CH_RCR));
        regval = MAC_SET_REG_BITS(regval, DMA_CH_RCR_PBL_POS,
                         DMA_CH_RCR_PBL_LEN,
                    pdata->rx_pbl);
        writel(regval, MAC_DMA_REG(channel, DMA_CH_RCR));
    }

    return 0;
}

static u64 mac_mmc_read(struct mac_pdata *pdata, unsigned int reg_lo)
{
    bool read_hi;
    u64 val;
    
    RNIC_TRACE_PRINT_DESC(); //insomnia
    

    switch (reg_lo) {
    /* These registers are always 64 bit */
    case MMC_TXOCTETCOUNT_GB_LO:
    case MMC_TXOCTETCOUNT_G_LO:
    case MMC_RXOCTETCOUNT_GB_LO:
    case MMC_RXOCTETCOUNT_G_LO:
        read_hi = true;
        break;

    default:
        read_hi = false;
    }

    val = (u64)readl(pdata->mac_regs + reg_lo);

    if (read_hi)
        val |= ((u64)readl(pdata->mac_regs + reg_lo + 4) << 32);

    return val;
}

static void mac_tx_mmc_int(struct mac_pdata *pdata)
{
    unsigned int mmc_isr = readl(pdata->mac_regs + MMC_TISR);
    struct mac_stats *stats = &pdata->stats;
    
    RNIC_TRACE_PRINT();
    

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXOCTETCOUNT_GB_POS,
                MMC_TISR_TXOCTETCOUNT_GB_LEN))
        stats->txoctetcount_gb +=
            mac_mmc_read(pdata, MMC_TXOCTETCOUNT_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXFRAMECOUNT_GB_POS,
                MMC_TISR_TXFRAMECOUNT_GB_LEN))
        stats->txframecount_gb +=
            mac_mmc_read(pdata, MMC_TXFRAMECOUNT_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXBROADCASTFRAMES_G_POS,
                MMC_TISR_TXBROADCASTFRAMES_G_LEN))
        stats->txbroadcastframes_g +=
            mac_mmc_read(pdata, MMC_TXBROADCASTFRAMES_G_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXMULTICASTFRAMES_G_POS,
                MMC_TISR_TXMULTICASTFRAMES_G_LEN))
        stats->txmulticastframes_g +=
            mac_mmc_read(pdata, MMC_TXMULTICASTFRAMES_G_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX64OCTETS_GB_POS,
                MMC_TISR_TX64OCTETS_GB_LEN))
        stats->tx64octets_gb +=
            mac_mmc_read(pdata, MMC_TX64OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX65TO127OCTETS_GB_POS,
                MMC_TISR_TX65TO127OCTETS_GB_LEN))
        stats->tx65to127octets_gb +=
            mac_mmc_read(pdata, MMC_TX65TO127OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX128TO255OCTETS_GB_POS,
                MMC_TISR_TX128TO255OCTETS_GB_LEN))
        stats->tx128to255octets_gb +=
            mac_mmc_read(pdata, MMC_TX128TO255OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX256TO511OCTETS_GB_POS,
                MMC_TISR_TX256TO511OCTETS_GB_LEN))
        stats->tx256to511octets_gb +=
            mac_mmc_read(pdata, MMC_TX256TO511OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX512TO1023OCTETS_GB_POS,
                MMC_TISR_TX512TO1023OCTETS_GB_LEN))
        stats->tx512to1023octets_gb +=
            mac_mmc_read(pdata, MMC_TX512TO1023OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX1024TOMAXOCTETS_GB_POS,
                MMC_TISR_TX1024TOMAXOCTETS_GB_LEN))
        stats->tx1024tomaxoctets_gb +=
            mac_mmc_read(pdata, MMC_TX1024TOMAXOCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXUNICASTFRAMES_GB_POS,
                MMC_TISR_TXUNICASTFRAMES_GB_LEN))
        stats->txunicastframes_gb +=
            mac_mmc_read(pdata, MMC_TXUNICASTFRAMES_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXMULTICASTFRAMES_GB_POS,
                MMC_TISR_TXMULTICASTFRAMES_GB_LEN))
        stats->txmulticastframes_gb +=
            mac_mmc_read(pdata, MMC_TXMULTICASTFRAMES_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXBROADCASTFRAMES_GB_POS,
                MMC_TISR_TXBROADCASTFRAMES_GB_LEN))
        stats->txbroadcastframes_g +=
            mac_mmc_read(pdata, MMC_TXBROADCASTFRAMES_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXUNDERFLOWERROR_POS,
                MMC_TISR_TXUNDERFLOWERROR_LEN))
        stats->txunderflowerror +=
            mac_mmc_read(pdata, MMC_TXUNDERFLOWERROR_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXOCTETCOUNT_G_POS,
                MMC_TISR_TXOCTETCOUNT_G_LEN))
        stats->txoctetcount_g +=
            mac_mmc_read(pdata, MMC_TXOCTETCOUNT_G_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXFRAMECOUNT_G_POS,
                MMC_TISR_TXFRAMECOUNT_G_LEN))
        stats->txframecount_g +=
            mac_mmc_read(pdata, MMC_TXFRAMECOUNT_G_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXPAUSEFRAMES_POS,
                MMC_TISR_TXPAUSEFRAMES_LEN))
        stats->txpauseframes +=
            mac_mmc_read(pdata, MMC_TXPAUSEFRAMES_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXVLANFRAMES_G_POS,
                MMC_TISR_TXVLANFRAMES_G_LEN))
        stats->txvlanframes_g +=
            mac_mmc_read(pdata, MMC_TXVLANFRAMES_G_LO);
}

static void mac_rx_mmc_int(struct mac_pdata *pdata)
{
    unsigned int mmc_isr = readl(pdata->mac_regs + MMC_RISR);
    struct mac_stats *stats = &pdata->stats;
    
    RNIC_TRACE_PRINT();
    

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXFRAMECOUNT_GB_POS,
                MMC_RISR_RXFRAMECOUNT_GB_LEN))
        stats->rxframecount_gb +=
            mac_mmc_read(pdata, MMC_RXFRAMECOUNT_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXOCTETCOUNT_GB_POS,
                MMC_RISR_RXOCTETCOUNT_GB_LEN))
        stats->rxoctetcount_gb +=
            mac_mmc_read(pdata, MMC_RXOCTETCOUNT_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXOCTETCOUNT_G_POS,
                MMC_RISR_RXOCTETCOUNT_G_LEN))
        stats->rxoctetcount_g +=
            mac_mmc_read(pdata, MMC_RXOCTETCOUNT_G_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXBROADCASTFRAMES_G_POS,
                MMC_RISR_RXBROADCASTFRAMES_G_LEN))
        stats->rxbroadcastframes_g +=
            mac_mmc_read(pdata, MMC_RXBROADCASTFRAMES_G_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXMULTICASTFRAMES_G_POS,
                MMC_RISR_RXMULTICASTFRAMES_G_LEN))
        stats->rxmulticastframes_g +=
            mac_mmc_read(pdata, MMC_RXMULTICASTFRAMES_G_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXCRCERROR_POS,
                MMC_RISR_RXCRCERROR_LEN))
        stats->rxcrcerror +=
            mac_mmc_read(pdata, MMC_RXCRCERROR_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXRUNTERROR_POS,
                MMC_RISR_RXRUNTERROR_LEN))
        stats->rxrunterror +=
            mac_mmc_read(pdata, MMC_RXRUNTERROR);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXJABBERERROR_POS,
                MMC_RISR_RXJABBERERROR_LEN))
        stats->rxjabbererror +=
            mac_mmc_read(pdata, MMC_RXJABBERERROR);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXUNDERSIZE_G_POS,
                MMC_RISR_RXUNDERSIZE_G_LEN))
        stats->rxundersize_g +=
            mac_mmc_read(pdata, MMC_RXUNDERSIZE_G);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXOVERSIZE_G_POS,
                MMC_RISR_RXOVERSIZE_G_LEN))
        stats->rxoversize_g +=
            mac_mmc_read(pdata, MMC_RXOVERSIZE_G);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX64OCTETS_GB_POS,
                MMC_RISR_RX64OCTETS_GB_LEN))
        stats->rx64octets_gb +=
            mac_mmc_read(pdata, MMC_RX64OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX65TO127OCTETS_GB_POS,
                MMC_RISR_RX65TO127OCTETS_GB_LEN))
        stats->rx65to127octets_gb +=
            mac_mmc_read(pdata, MMC_RX65TO127OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX128TO255OCTETS_GB_POS,
                MMC_RISR_RX128TO255OCTETS_GB_LEN))
        stats->rx128to255octets_gb +=
            mac_mmc_read(pdata, MMC_RX128TO255OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX256TO511OCTETS_GB_POS,
                MMC_RISR_RX256TO511OCTETS_GB_LEN))
        stats->rx256to511octets_gb +=
            mac_mmc_read(pdata, MMC_RX256TO511OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX512TO1023OCTETS_GB_POS,
                MMC_RISR_RX512TO1023OCTETS_GB_LEN))
        stats->rx512to1023octets_gb +=
            mac_mmc_read(pdata, MMC_RX512TO1023OCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX1024TOMAXOCTETS_GB_POS,
                MMC_RISR_RX1024TOMAXOCTETS_GB_LEN))
        stats->rx1024tomaxoctets_gb +=
            mac_mmc_read(pdata, MMC_RX1024TOMAXOCTETS_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXUNICASTFRAMES_G_POS,
                MMC_RISR_RXUNICASTFRAMES_G_LEN))
        stats->rxunicastframes_g +=
            mac_mmc_read(pdata, MMC_RXUNICASTFRAMES_G_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXLENGTHERROR_POS,
                MMC_RISR_RXLENGTHERROR_LEN))
        stats->rxlengtherror +=
            mac_mmc_read(pdata, MMC_RXLENGTHERROR_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXOUTOFRANGETYPE_POS,
                MMC_RISR_RXOUTOFRANGETYPE_LEN))
        stats->rxoutofrangetype +=
            mac_mmc_read(pdata, MMC_RXOUTOFRANGETYPE_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXPAUSEFRAMES_POS,
                MMC_RISR_RXPAUSEFRAMES_LEN))
        stats->rxpauseframes +=
            mac_mmc_read(pdata, MMC_RXPAUSEFRAMES_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXFIFOOVERFLOW_POS,
                MMC_RISR_RXFIFOOVERFLOW_LEN))
        stats->rxfifooverflow +=
            mac_mmc_read(pdata, MMC_RXFIFOOVERFLOW_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXVLANFRAMES_GB_POS,
                MMC_RISR_RXVLANFRAMES_GB_LEN))
        stats->rxvlanframes_gb +=
            mac_mmc_read(pdata, MMC_RXVLANFRAMES_GB_LO);

    if (MAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXWATCHDOGERROR_POS,
                MMC_RISR_RXWATCHDOGERROR_LEN))
        stats->rxwatchdogerror +=
            mac_mmc_read(pdata, MMC_RXWATCHDOGERROR);
}

static void mac_read_mmc_stats(struct mac_pdata *pdata)
{
    struct mac_stats *stats = &pdata->stats;
    u32 regval;
    
    RNIC_TRACE_PRINT();
    

    /* Freeze counters */
    regval = readl(pdata->mac_regs + MMC_CR);
    regval = MAC_SET_REG_BITS(regval, MMC_CR_MCF_POS,
                     MMC_CR_MCF_LEN, 1);
    writel(regval, pdata->mac_regs + MMC_CR);

    stats->txoctetcount_gb +=
        mac_mmc_read(pdata, MMC_TXOCTETCOUNT_GB_LO);

    stats->txframecount_gb +=
        mac_mmc_read(pdata, MMC_TXFRAMECOUNT_GB_LO);

    stats->txbroadcastframes_g +=
        mac_mmc_read(pdata, MMC_TXBROADCASTFRAMES_G_LO);

    stats->txmulticastframes_g +=
        mac_mmc_read(pdata, MMC_TXMULTICASTFRAMES_G_LO);

    stats->tx64octets_gb +=
        mac_mmc_read(pdata, MMC_TX64OCTETS_GB_LO);

    stats->tx65to127octets_gb +=
        mac_mmc_read(pdata, MMC_TX65TO127OCTETS_GB_LO);

    stats->tx128to255octets_gb +=
        mac_mmc_read(pdata, MMC_TX128TO255OCTETS_GB_LO);

    stats->tx256to511octets_gb +=
        mac_mmc_read(pdata, MMC_TX256TO511OCTETS_GB_LO);

    stats->tx512to1023octets_gb +=
        mac_mmc_read(pdata, MMC_TX512TO1023OCTETS_GB_LO);

    stats->tx1024tomaxoctets_gb +=
        mac_mmc_read(pdata, MMC_TX1024TOMAXOCTETS_GB_LO);

    stats->txunicastframes_gb +=
        mac_mmc_read(pdata, MMC_TXUNICASTFRAMES_GB_LO);

    stats->txmulticastframes_gb +=
        mac_mmc_read(pdata, MMC_TXMULTICASTFRAMES_GB_LO);

    stats->txbroadcastframes_g +=
        mac_mmc_read(pdata, MMC_TXBROADCASTFRAMES_GB_LO);

    stats->txunderflowerror +=
        mac_mmc_read(pdata, MMC_TXUNDERFLOWERROR_LO);

    stats->txoctetcount_g +=
        mac_mmc_read(pdata, MMC_TXOCTETCOUNT_G_LO);

    stats->txframecount_g +=
        mac_mmc_read(pdata, MMC_TXFRAMECOUNT_G_LO);

    stats->txpauseframes +=
        mac_mmc_read(pdata, MMC_TXPAUSEFRAMES_LO);

    stats->txvlanframes_g +=
        mac_mmc_read(pdata, MMC_TXVLANFRAMES_G_LO);

    stats->rxframecount_gb +=
        mac_mmc_read(pdata, MMC_RXFRAMECOUNT_GB_LO);

    stats->rxoctetcount_gb +=
        mac_mmc_read(pdata, MMC_RXOCTETCOUNT_GB_LO);

    stats->rxoctetcount_g +=
        mac_mmc_read(pdata, MMC_RXOCTETCOUNT_G_LO);

    stats->rxbroadcastframes_g +=
        mac_mmc_read(pdata, MMC_RXBROADCASTFRAMES_G_LO);

    stats->rxmulticastframes_g +=
        mac_mmc_read(pdata, MMC_RXMULTICASTFRAMES_G_LO);

    stats->rxcrcerror +=
        mac_mmc_read(pdata, MMC_RXCRCERROR_LO);

    stats->rxrunterror +=
        mac_mmc_read(pdata, MMC_RXRUNTERROR);

    stats->rxjabbererror +=
        mac_mmc_read(pdata, MMC_RXJABBERERROR);

    stats->rxundersize_g +=
        mac_mmc_read(pdata, MMC_RXUNDERSIZE_G);

    stats->rxoversize_g +=
        mac_mmc_read(pdata, MMC_RXOVERSIZE_G);

    stats->rx64octets_gb +=
        mac_mmc_read(pdata, MMC_RX64OCTETS_GB_LO);

    stats->rx65to127octets_gb +=
        mac_mmc_read(pdata, MMC_RX65TO127OCTETS_GB_LO);

    stats->rx128to255octets_gb +=
        mac_mmc_read(pdata, MMC_RX128TO255OCTETS_GB_LO);

    stats->rx256to511octets_gb +=
        mac_mmc_read(pdata, MMC_RX256TO511OCTETS_GB_LO);

    stats->rx512to1023octets_gb +=
        mac_mmc_read(pdata, MMC_RX512TO1023OCTETS_GB_LO);

    stats->rx1024tomaxoctets_gb +=
        mac_mmc_read(pdata, MMC_RX1024TOMAXOCTETS_GB_LO);

    stats->rxunicastframes_g +=
        mac_mmc_read(pdata, MMC_RXUNICASTFRAMES_G_LO);

    stats->rxlengtherror +=
        mac_mmc_read(pdata, MMC_RXLENGTHERROR_LO);

    stats->rxoutofrangetype +=
        mac_mmc_read(pdata, MMC_RXOUTOFRANGETYPE_LO);

    stats->rxpauseframes +=
        mac_mmc_read(pdata, MMC_RXPAUSEFRAMES_LO);

    stats->rxfifooverflow +=
        mac_mmc_read(pdata, MMC_RXFIFOOVERFLOW_LO);

    stats->rxvlanframes_gb +=
        mac_mmc_read(pdata, MMC_RXVLANFRAMES_GB_LO);

    stats->rxwatchdogerror +=
        mac_mmc_read(pdata, MMC_RXWATCHDOGERROR);

    /* Un-freeze counters */
    regval = readl(pdata->mac_regs + MMC_CR);
    regval = MAC_SET_REG_BITS(regval, MMC_CR_MCF_POS,
                     MMC_CR_MCF_LEN, 0);
    writel(regval, pdata->mac_regs + MMC_CR);
}

static void mac_config_mmc(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();
    

    regval = readl(pdata->mac_regs + MMC_CR);
    /* Set counters to reset on read */
    regval = MAC_SET_REG_BITS(regval, MMC_CR_ROR_POS,
                     MMC_CR_ROR_LEN, 1);
    /* Reset the counters */
    regval = MAC_SET_REG_BITS(regval, MMC_CR_CR_POS,
                     MMC_CR_CR_LEN, 1);
    writel(regval, pdata->mac_regs + MMC_CR);
}

static int mac_write_rss_reg(struct mac_pdata *pdata, unsigned int type,
                unsigned int index, unsigned int val)
{
    unsigned int wait;
    int ret = 0;
    u32 regval;
    
    RNIC_TRACE_PRINT_DESC(); //insomnia
    

    mutex_lock(&pdata->rss_mutex);

    regval = MAC_GET_REG_BITS(readl(pdata->mac_regs + MAC_RSSAR),
                     MAC_RSSAR_OB_POS, MAC_RSSAR_OB_LEN);
    if (regval) {
        ret = -EBUSY;
        goto unlock;
    }

    writel(val, pdata->mac_regs + MAC_RSSDR);

    regval = readl(pdata->mac_regs + MAC_RSSAR);
    regval = MAC_SET_REG_BITS(regval, MAC_RSSAR_RSSIA_POS,
                     MAC_RSSAR_RSSIA_LEN, index);
    regval = MAC_SET_REG_BITS(regval, MAC_RSSAR_ADDRT_POS,
                     MAC_RSSAR_ADDRT_LEN, type);
    regval = MAC_SET_REG_BITS(regval, MAC_RSSAR_CT_POS,
                     MAC_RSSAR_CT_LEN, 0);
    regval = MAC_SET_REG_BITS(regval, MAC_RSSAR_OB_POS,
                     MAC_RSSAR_OB_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_RSSAR);

    wait = 1000;
    while (wait--) {
        regval = MAC_GET_REG_BITS(readl(pdata->mac_regs + MAC_RSSAR),
                         MAC_RSSAR_OB_POS,
                         MAC_RSSAR_OB_LEN);
        if (!regval)
            goto unlock;

        usleep_range(1000, 1500);
    }

    ret = -EBUSY;

unlock:
    mutex_unlock(&pdata->rss_mutex);

    return ret;
}

static int mac_write_rss_hash_key(struct mac_pdata *pdata)
{
    unsigned int key_regs = sizeof(pdata->rss_key) / sizeof(u32);
    unsigned int *key = (unsigned int *)&pdata->rss_key;
    int ret;
    
    RNIC_TRACE_PRINT();
    

    while (key_regs--) {
        ret = mac_write_rss_reg(pdata, MAC_RSS_HASH_KEY_TYPE,
                       key_regs, *key++);
        if (ret)
            return ret;
    }

    return 0;
}

static int mac_write_rss_lookup_table(struct mac_pdata *pdata)
{
    unsigned int i;
    int ret;
    
    RNIC_TRACE_PRINT();
    

    for (i = 0; i < ARRAY_SIZE(pdata->rss_table); i++) {
        ret = mac_write_rss_reg(pdata,
                       MAC_RSS_LOOKUP_TABLE_TYPE, i,
                       pdata->rss_table[i]);
        if (ret)
            return ret;
    }

    return 0;
}

static int mac_set_rss_hash_key(struct mac_pdata *pdata, const u8 *key)
{   
    RNIC_TRACE_PRINT();
    
    memcpy(pdata->rss_key, key, sizeof(pdata->rss_key));

    return mac_write_rss_hash_key(pdata);
}

static int mac_set_rss_lookup_table(struct mac_pdata *pdata,
                       const u32 *table)
{
    unsigned int i;
    u32 tval;
    
    RNIC_TRACE_PRINT();
    

    for (i = 0; i < ARRAY_SIZE(pdata->rss_table); i++) {
        tval = table[i];
        pdata->rss_table[i] = MAC_SET_REG_BITS(
                        pdata->rss_table[i],
                        MAC_RSSDR_DMCH_POS,
                        MAC_RSSDR_DMCH_LEN,
                        tval);
    }

    return mac_write_rss_lookup_table(pdata);
}

static int mac_enable_rss(struct mac_pdata *pdata)
{
    u32 regval;
    int ret;
    
    RNIC_TRACE_PRINT();
    

    if (!pdata->hw_feat.rss)
        return -EOPNOTSUPP;

    /* Program the hash key */
    ret = mac_write_rss_hash_key(pdata);
    if (ret)
        return ret;

    /* Program the lookup table */
    ret = mac_write_rss_lookup_table(pdata);
    if (ret)
        return ret;

    /* Set the RSS options */
    writel(pdata->rss_options, pdata->mac_regs + MAC_RSSCR);

    /* Enable RSS */
    regval = readl(pdata->mac_regs + MAC_RSSCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RSSCR_RSSE_POS,
                     MAC_RSSCR_RSSE_LEN, 1);
    writel(regval, pdata->mac_regs + MAC_RSSCR);

    return 0;
}

static int mac_disable_rss(struct mac_pdata *pdata)
{
    u32 regval;
    
    RNIC_TRACE_PRINT();
    

    if (!pdata->hw_feat.rss)
        return -EOPNOTSUPP;

    regval = readl(pdata->mac_regs + MAC_RSSCR);
    regval = MAC_SET_REG_BITS(regval, MAC_RSSCR_RSSE_POS,
                     MAC_RSSCR_RSSE_LEN, 0);
    writel(regval, pdata->mac_regs + MAC_RSSCR);

    return 0;
}

static void mac_config_rss(struct mac_pdata *pdata)
{
    int ret;
    
    RNIC_TRACE_PRINT();
    

    if (!pdata->hw_feat.rss)
        return;

    if (pdata->netdev->features & NETIF_F_RXHASH)
        ret = mac_enable_rss(pdata);
    else
        ret = mac_disable_rss(pdata);

    if (ret)
        netdev_err(pdata->netdev,
               "error configuring RSS, RSS disabled\n");
}

static void mac_enable_dma_interrupts(struct mac_pdata *pdata)
{
    unsigned int dma_ch_isr, dma_ch_ier;
    struct mac_channel *channel;
    unsigned int i;
    
    RNIC_TRACE_PRINT();
    

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        /* Clear all the interrupts which are set */
        dma_ch_isr = readl(MAC_DMA_REG(channel, DMA_CH_SR));
        writel(dma_ch_isr, MAC_DMA_REG(channel, DMA_CH_SR));
    

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

        if (channel->tx_ring) {
            /* Enable the following Tx interrupts
             *   TIE  - Transmit Interrupt Enable (unless using
             *          per channel interrupts)
             */
            if (!pdata->per_channel_irq)//insomnia@20200216???????
                dma_ch_ier = MAC_SET_REG_BITS(
                        dma_ch_ier,
                        DMA_CH_IER_TIE_POS,
                        DMA_CH_IER_TIE_LEN,
                        1);
        }
        if (channel->rx_ring) {
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
            if (!pdata->per_channel_irq)//insomnia@20200216???????
                dma_ch_ier = MAC_SET_REG_BITS(
                        dma_ch_ier,
                        DMA_CH_IER_RIE_POS,
                        DMA_CH_IER_RIE_LEN,
                        1);
        }
        //insomnia@20200210
        //writel(dma_ch_isr, MAC_DMA_REG(channel, DMA_CH_IER));
        writel(dma_ch_ier, MAC_DMA_REG(channel, DMA_CH_IER));
    }
}

static void mac_enable_mtl_interrupts(struct mac_pdata *pdata)
{
    unsigned int q_count, i;
    unsigned int mtl_q_isr;
    
    RNIC_TRACE_PRINT();
    
    q_count = max(pdata->hw_feat.tx_q_cnt, pdata->hw_feat.rx_q_cnt);
    for (i = 0; i < q_count; i++) {
        /* Clear all the interrupts which are set */
        mtl_q_isr = readl(MAC_MTL_REG(pdata, i, MTL_Q_ISR));
        writel(mtl_q_isr, MAC_MTL_REG(pdata, i, MTL_Q_ISR));
        
        /* No MTL interrupts to be enabled */
        writel(0, MAC_MTL_REG(pdata, i, MTL_Q_IER));
    }
}

static void mac_enable_mac_interrupts(struct mac_pdata *pdata)
{
    unsigned int mac_ier = 0;
    u32 regval;
    
    RNIC_TRACE_PRINT();
    
    /* Enable Timestamp interrupt */
    mac_ier = MAC_SET_REG_BITS(mac_ier, MAC_IER_TSIE_POS,
                      MAC_IER_TSIE_LEN, 1);

    writel(mac_ier, pdata->mac_regs + MAC_IER);

    /* Enable all counter interrupts */
    regval = readl(pdata->mac_regs + MMC_RIER);
    regval = MAC_SET_REG_BITS(regval, MMC_RIER_ALL_INTERRUPTS_POS,
                     MMC_RIER_ALL_INTERRUPTS_LEN, 0xffffffff);
    writel(regval, pdata->mac_regs + MMC_RIER);
    regval = readl(pdata->mac_regs + MMC_TIER);
    regval = MAC_SET_REG_BITS(regval, MMC_TIER_ALL_INTERRUPTS_POS,
                     MMC_TIER_ALL_INTERRUPTS_LEN, 0xffffffff);
    writel(regval, pdata->mac_regs + MMC_TIER);

}


static void mac_config_mac_speed(struct mac_pdata *pdata)
{   
    RNIC_TRACE_PRINT();
    mac_speed_cfg(&pdata->rnic_pdata,0);
}

static int mac_dev_read(struct mac_channel *channel)
{
    struct mac_pdata *pdata = channel->pdata;
    struct mac_ring *ring = channel->rx_ring;
    struct net_device *netdev = pdata->netdev;
    struct mac_desc_data *desc_data;
    struct mac_dma_desc *dma_desc;
    struct mac_pkt_info *pkt_info;
    unsigned int err, etlt, l34t;
    
    RNIC_TRACE_PRINT();
    
    desc_data = MAC_GET_DESC_DATA(ring, ring->cur);
    dma_desc = desc_data->dma_desc;
    pkt_info = &ring->pkt_info;

    /* Check for data availability */
    if (MAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_OWN_POS,
                   RX_NORMAL_DESC3_OWN_LEN))
        return 1;

    /* Make sure descriptor fields are read after reading the OWN bit */
    dma_rmb();

    if (netif_msg_rx_status(pdata))
        mac_dump_rx_desc(pdata, ring, ring->cur);

    if (MAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_CTXT_POS,
                   RX_NORMAL_DESC3_CTXT_LEN)) {
        /* Timestamp Context Descriptor */
        mac_get_rx_tstamp(pkt_info, dma_desc);

        pkt_info->attributes = MAC_SET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_CONTEXT_POS,
                    RX_PACKET_ATTRIBUTES_CONTEXT_LEN,
                    1);
        pkt_info->attributes = MAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_POS,
                RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_LEN,
                0);
        return 0;
    }

    /* Normal Descriptor, be sure Context Descriptor bit is off */
    pkt_info->attributes = MAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_CONTEXT_POS,
                RX_PACKET_ATTRIBUTES_CONTEXT_LEN,
                0);

    /* Indicate if a Context Descriptor is next */
    if (MAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_CDA_POS,
                   RX_NORMAL_DESC3_CDA_LEN))
        pkt_info->attributes = MAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_POS,
                RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_LEN,
                1);

    /* Get the header length */
    if (MAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_FD_POS,
                   RX_NORMAL_DESC3_FD_LEN)) {
        desc_data->rx.hdr_len = MAC_GET_REG_BITS_LE(dma_desc->desc2,
                            RX_NORMAL_DESC2_HL_POS,
                            RX_NORMAL_DESC2_HL_LEN);
        if (desc_data->rx.hdr_len)
            pdata->stats.rx_split_header_packets++;
    }

    /* Get the RSS hash */
    if (MAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_RSV_POS,
                   RX_NORMAL_DESC3_RSV_LEN)) {
        pkt_info->attributes = MAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_RSS_HASH_POS,
                RX_PACKET_ATTRIBUTES_RSS_HASH_LEN,
                1);

        pkt_info->rss_hash = le32_to_cpu(dma_desc->desc1);

        l34t = MAC_GET_REG_BITS_LE(dma_desc->desc3,
                          RX_NORMAL_DESC3_L34T_POS,
                      RX_NORMAL_DESC3_L34T_LEN);
        switch (l34t) {
        case RX_DESC3_L34T_IPV4_TCP:
        case RX_DESC3_L34T_IPV4_UDP:
        case RX_DESC3_L34T_IPV6_TCP:
        case RX_DESC3_L34T_IPV6_UDP:
            pkt_info->rss_hash_type = PKT_HASH_TYPE_L4;
            break;
        default:
            pkt_info->rss_hash_type = PKT_HASH_TYPE_L3;
        }
    }

    /* Get the pkt_info length */
    desc_data->rx.len = MAC_GET_REG_BITS_LE(dma_desc->desc3,
                    RX_NORMAL_DESC3_PL_POS,
                    RX_NORMAL_DESC3_PL_LEN);

    if (!MAC_GET_REG_BITS_LE(dma_desc->desc3,
                    RX_NORMAL_DESC3_LD_POS,
                    RX_NORMAL_DESC3_LD_LEN)) {
        /* Not all the data has been transferred for this pkt_info */
        pkt_info->attributes = MAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_INCOMPLETE_POS,
                RX_PACKET_ATTRIBUTES_INCOMPLETE_LEN,
                1);
        return 0;
    }

    /* This is the last of the data for this pkt_info */
    pkt_info->attributes = MAC_SET_REG_BITS(
            pkt_info->attributes,
            RX_PACKET_ATTRIBUTES_INCOMPLETE_POS,
            RX_PACKET_ATTRIBUTES_INCOMPLETE_LEN,
            0);

    /* Set checksum done indicator as appropriate */
    if (netdev->features & NETIF_F_RXCSUM)
        pkt_info->attributes = MAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_CSUM_DONE_POS,
                RX_PACKET_ATTRIBUTES_CSUM_DONE_LEN,
                1);

    /* Check for errors (only valid in last descriptor) */
    err = MAC_GET_REG_BITS_LE(dma_desc->desc3,
                     RX_NORMAL_DESC3_ES_POS,
                     RX_NORMAL_DESC3_ES_LEN);
    etlt = MAC_GET_REG_BITS_LE(dma_desc->desc3,
                      RX_NORMAL_DESC3_ETLT_POS,
                      RX_NORMAL_DESC3_ETLT_LEN);
    netif_dbg(pdata, rx_status, netdev, "err=%u, etlt=%#x\n", err, etlt);

    if (!err || !etlt) {
        /* No error if err is 0 or etlt is 0 */
        if ((etlt == 0x09) &&
            (netdev->features & NETIF_F_HW_VLAN_CTAG_RX)) {
            pkt_info->attributes = MAC_SET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_VLAN_CTAG_POS,
                    RX_PACKET_ATTRIBUTES_VLAN_CTAG_LEN,
                    1);
            pkt_info->vlan_ctag =
                MAC_GET_REG_BITS_LE(dma_desc->desc0,
                               RX_NORMAL_DESC0_OVT_POS,
                           RX_NORMAL_DESC0_OVT_LEN);
            netif_dbg(pdata, rx_status, netdev, "vlan-ctag=%#06x\n",
                  pkt_info->vlan_ctag);
        }
    } else {
        if ((etlt == 0x05) || (etlt == 0x06))
            pkt_info->attributes = MAC_SET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_CSUM_DONE_POS,
                    RX_PACKET_ATTRIBUTES_CSUM_DONE_LEN,
                    0);
        else
            pkt_info->errors = MAC_SET_REG_BITS(
                    pkt_info->errors,
                    RX_PACKET_ERRORS_FRAME_POS,
                    RX_PACKET_ERRORS_FRAME_LEN,
                    1);
    }

    RNIC_PRINTK("%s - descriptor=%u (cur=%d)\n", channel->name,
          ring->cur & (ring->dma_desc_count - 1), ring->cur);

    return 0;
}

static int mac_enable_int(struct mac_channel *channel,
                 enum mac_int int_id)
{
    unsigned int dma_ch_ier;
    
    RNIC_TRACE_PRINT();
    //insomnia@20200207
    //ieu_enable_intr_tx_rx_all(&channel->pdata->rnic_pdata);

    dma_ch_ier = readl(MAC_DMA_REG(channel, DMA_CH_IER));

    switch (int_id) {
    case MAC_INT_DMA_CH_SR_TI:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TIE_POS,
                DMA_CH_IER_TIE_LEN, 1);
        break;
    case MAC_INT_DMA_CH_SR_TPS:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TXSE_POS,
                DMA_CH_IER_TXSE_LEN, 1);
        break;
    case MAC_INT_DMA_CH_SR_TBU:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TBUE_POS,
                DMA_CH_IER_TBUE_LEN, 1);
        break;
    case MAC_INT_DMA_CH_SR_RI:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RIE_POS,
                DMA_CH_IER_RIE_LEN, 1);
        break;
    case MAC_INT_DMA_CH_SR_RBU:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RBUE_POS,
                DMA_CH_IER_RBUE_LEN, 1);
        break;
    case MAC_INT_DMA_CH_SR_RPS:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RSE_POS,
                DMA_CH_IER_RSE_LEN, 1);
        break;
    case MAC_INT_DMA_CH_SR_TI_RI:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TIE_POS,
                DMA_CH_IER_TIE_LEN, 1);
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RIE_POS,
                DMA_CH_IER_RIE_LEN, 1);
        break;
    case MAC_INT_DMA_CH_SR_FBE:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_FBEE_POS,
                DMA_CH_IER_FBEE_LEN, 1);
        break;
    case MAC_INT_DMA_ALL:
        dma_ch_ier |= channel->saved_ier;
        break;
    default:
        return -1;
    }

    writel(dma_ch_ier, MAC_DMA_REG(channel, DMA_CH_IER));
    
    return 0;
}

static int mac_disable_int(struct mac_channel *channel,
                  enum mac_int int_id)
{
    unsigned int dma_ch_ier;

    RNIC_TRACE_PRINT();

    dma_ch_ier = readl(MAC_DMA_REG(channel, DMA_CH_IER));

    switch (int_id) {
    case MAC_INT_DMA_CH_SR_TI:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TIE_POS,
                DMA_CH_IER_TIE_LEN, 0);
        break;
    case MAC_INT_DMA_CH_SR_TPS:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TXSE_POS,
                DMA_CH_IER_TXSE_LEN, 0);
        break;
    case MAC_INT_DMA_CH_SR_TBU:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TBUE_POS,
                DMA_CH_IER_TBUE_LEN, 0);
        break;
    case MAC_INT_DMA_CH_SR_RI:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RIE_POS,
                DMA_CH_IER_RIE_LEN, 0);
        break;
    case MAC_INT_DMA_CH_SR_RBU:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RBUE_POS,
                DMA_CH_IER_RBUE_LEN, 0);
        break;
    case MAC_INT_DMA_CH_SR_RPS:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RSE_POS,
                DMA_CH_IER_RSE_LEN, 0);
        break;
    case MAC_INT_DMA_CH_SR_TI_RI:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TIE_POS,
                DMA_CH_IER_TIE_LEN, 0);
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RIE_POS,
                DMA_CH_IER_RIE_LEN, 0);
        break;
    case MAC_INT_DMA_CH_SR_FBE:
        dma_ch_ier = MAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_FBEE_POS,
                DMA_CH_IER_FBEE_LEN, 0);
        break;
    case MAC_INT_DMA_ALL:
        channel->saved_ier = dma_ch_ier & MAC_DMA_INTERRUPT_MASK;
        dma_ch_ier &= ~MAC_DMA_INTERRUPT_MASK;
        break;
    default:
        return -1;
    }

    writel(dma_ch_ier, MAC_DMA_REG(channel, DMA_CH_IER));
        
    return 0;
}

static int mac_flush_tx_queues(struct mac_pdata *pdata)
{
    unsigned int i, count;
    u32 regval;
    
    RNIC_TRACE_PRINT();
    

    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = MAC_SET_REG_BITS(regval, MTL_Q_TQOMR_FTQ_POS,
                         MTL_Q_TQOMR_FTQ_LEN, 1);
        writel(regval, MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    /* Poll Until Poll Condition */
    for (i = 0; i < pdata->tx_q_count; i++) {
        count = 2000;
        regval = readl(MAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = MAC_GET_REG_BITS(regval, MTL_Q_TQOMR_FTQ_POS,
                         MTL_Q_TQOMR_FTQ_LEN);
        while (--count && regval)
            usleep_range(500, 600);

        if (!count)
            return -EBUSY;
    }

    return 0;
}

static void mac_config_dma_bus(struct mac_pdata *pdata)
{
    u32 regval;

    regval = readl(pdata->mac_regs + DMA_SBMR);
    /* Set enhanced addressing mode */
    regval = MAC_SET_REG_BITS(regval, DMA_SBMR_EAME_POS,
                     DMA_SBMR_EAME_LEN, 1);
    /* Set the System Bus mode */
    regval = MAC_SET_REG_BITS(regval, DMA_SBMR_UNDEF_POS,
                     DMA_SBMR_UNDEF_LEN, 1);
    regval = MAC_SET_REG_BITS(regval, DMA_SBMR_BLEN_256_POS,
                     DMA_SBMR_BLEN_256_LEN, 1);
    writel(regval, pdata->mac_regs + DMA_SBMR);
}

static int mac_hw_init(struct mac_pdata *pdata)
{
    struct mac_desc_ops *desc_ops = &pdata->desc_ops;
    int ret;
    
    RNIC_TRACE_PRINT();

	//added by hs for loopback in pcs
#if 1
	struct rnic_pdata *rnic_pdata = &pdata->rnic_pdata;
	pcs_loopback_cfg(rnic_pdata,0);
#endif
    
    /* Flush Tx queues */
    ret = mac_flush_tx_queues(pdata);
    if (ret)
        return ret;

    /* Initialize DMA related features */
    mac_config_dma_bus(pdata);
    mac_config_osp_mode(pdata);
    mac_config_pblx8(pdata);
    mac_config_tx_pbl_val(pdata);
    mac_config_rx_pbl_val(pdata);
    mac_config_rx_coalesce(pdata);
    mac_config_tx_coalesce(pdata);
    mac_config_rx_buffer_size(pdata);
    mac_config_tso_mode(pdata);
    mac_config_sph_mode(pdata);
    mac_config_rss(pdata);
    desc_ops->tx_desc_init(pdata);
    desc_ops->rx_desc_init(pdata);
    mac_enable_dma_interrupts(pdata);

    /* Initialize MTL related features */
    mac_config_mtl_mode(pdata);
    mac_config_queue_mapping(pdata);
    mac_config_tsf_mode(pdata, pdata->tx_sf_mode);
    mac_config_rsf_mode(pdata, pdata->rx_sf_mode);
    mac_config_tx_threshold(pdata, pdata->tx_threshold);
    mac_config_rx_threshold(pdata, pdata->rx_threshold);
    mac_config_tx_fifo_size(pdata);
    mac_config_rx_fifo_size(pdata);
    mac_config_flow_control_threshold(pdata);
    mac_config_rx_fep_enable(pdata);
    mac_config_rx_fup_enable(pdata);
    mac_enable_mtl_interrupts(pdata);

    /* Initialize MAC related features */
    mac_config_mac_address(pdata);
    mac_config_rx_mode(pdata);
    mac_config_jumbo_enable(pdata);
    mac_config_flow_control(pdata);
    mac_config_mac_speed(pdata);
    mac_config_checksum_offload(pdata);
    mac_config_vlan_support(pdata);
    mac_config_mmc(pdata);
    mac_enable_mac_interrupts(pdata);


    //insomnia@20200205
    /* Initialize MAC axi ports features */
    mac_axi_cfg(&pdata->rnic_pdata,0);
    //mac_rwtu_cfg(&pdata->rnic_pdata,0);
    //mac_enable_dspw(&pdata->rnic_pdata,0);
    
#ifdef MAXIMIZE_RX_0_FIFO
    mac_alloc_rx_fifo(&pdata->rnic_pdata,0);
#endif

#ifdef RNIC_MSI_EN
    mac_dma_intr_mode_cfg(&pdata->rnic_pdata,0);
    mac_enable_dma_intr(&pdata->rnic_pdata,0);
#endif  
    //mac_print_all_regs(&pdata->rnic_pdata,0);
    //pcie_print_all_reg(&pdata->rnic_pdata);

    //mac_bandwidth_alloc(&pdata->rnic_pdata,0);
    
    return 0;
}

static int mac_hw_exit(struct mac_pdata *pdata)
{
    RNIC_TRACE_PRINT();

#if 1    
    rnic_init(&pdata->rnic_pdata); //insomnia@20200205

//by insomnia@20200205
#else
    unsigned int count = 2000;
    u32 regval;

    /* Issue a software reset */
    regval = readl(pdata->mac_regs + DMA_MR);
    regval = MAC_SET_REG_BITS(regval, DMA_MR_SWR_POS,
                     DMA_MR_SWR_LEN, 1);
    writel(regval, pdata->mac_regs + DMA_MR);
    usleep_range(10, 15);

    /* Poll Until Poll Condition */
    while (--count &&
           MAC_GET_REG_BITS(readl(pdata->mac_regs + DMA_MR),
                   DMA_MR_SWR_POS, DMA_MR_SWR_LEN))
        usleep_range(500, 600);

    if (!count)
        return -EBUSY;
#endif
    return 0;
}

void mac_init_hw_ops(struct mac_hw_ops *hw_ops)
{   
    RNIC_TRACE_PRINT();
    
    hw_ops->init = mac_hw_init;
    hw_ops->exit = mac_hw_exit;

    hw_ops->tx_complete = mac_tx_complete;

    hw_ops->enable_tx = mac_enable_tx;
    hw_ops->disable_tx = mac_disable_tx;
    hw_ops->enable_rx = mac_enable_rx;
    hw_ops->disable_rx = mac_disable_rx;

    hw_ops->dev_xmit = mac_dev_xmit;
    hw_ops->dev_read = mac_dev_read;
    hw_ops->enable_int = mac_enable_int;
    hw_ops->disable_int = mac_disable_int;

    hw_ops->set_mac_address = mac_set_mac_address;
    hw_ops->config_rx_mode = mac_config_rx_mode;
    hw_ops->enable_rx_csum = mac_enable_rx_csum;
    hw_ops->disable_rx_csum = mac_disable_rx_csum;

    /* For descriptor related operation */
    hw_ops->tx_desc_init = mac_tx_desc_init;
    hw_ops->rx_desc_init = mac_rx_desc_init;
    hw_ops->tx_desc_reset = mac_tx_desc_reset;
    hw_ops->rx_desc_reset = mac_rx_desc_reset;
    hw_ops->is_last_desc = mac_is_last_desc;
    hw_ops->is_context_desc = mac_is_context_desc;
    hw_ops->tx_start_xmit = mac_tx_start_xmit;

    /* For Flow Control */
    hw_ops->config_tx_flow_control = mac_config_tx_flow_control;
    hw_ops->config_rx_flow_control = mac_config_rx_flow_control;

    /* For Vlan related config */
    hw_ops->enable_rx_vlan_stripping = mac_enable_rx_vlan_stripping;
    hw_ops->disable_rx_vlan_stripping = mac_disable_rx_vlan_stripping;
    hw_ops->enable_rx_vlan_filtering = mac_enable_rx_vlan_filtering;
    hw_ops->disable_rx_vlan_filtering = mac_disable_rx_vlan_filtering;
    hw_ops->update_vlan_hash_table = mac_update_vlan_hash_table;

    /* For RX coalescing */
    hw_ops->config_rx_coalesce = mac_config_rx_coalesce;
    hw_ops->config_tx_coalesce = mac_config_tx_coalesce;
    hw_ops->usec_to_riwt = mac_usec_to_riwt;
    hw_ops->riwt_to_usec = mac_riwt_to_usec;

    /* For RX and TX threshold config */
    hw_ops->config_rx_threshold = mac_config_rx_threshold;
    hw_ops->config_tx_threshold = mac_config_tx_threshold;

    /* For RX and TX Store and Forward Mode config */
    hw_ops->config_rsf_mode = mac_config_rsf_mode;
    hw_ops->config_tsf_mode = mac_config_tsf_mode;

    /* For TX DMA Operating on Second Frame config */
    hw_ops->config_osp_mode = mac_config_osp_mode;

    /* For RX and TX PBL config */
    hw_ops->config_rx_pbl_val = mac_config_rx_pbl_val;
    hw_ops->get_rx_pbl_val = mac_get_rx_pbl_val;
    hw_ops->config_tx_pbl_val = mac_config_tx_pbl_val;
    hw_ops->get_tx_pbl_val = mac_get_tx_pbl_val;
    hw_ops->config_pblx8 = mac_config_pblx8;

    /* For MMC statistics support */
    hw_ops->tx_mmc_int = mac_tx_mmc_int;
    hw_ops->rx_mmc_int = mac_rx_mmc_int;
    hw_ops->read_mmc_stats = mac_read_mmc_stats;

    /* For Receive Side Scaling */
    hw_ops->enable_rss = mac_enable_rss;
    hw_ops->disable_rss = mac_disable_rss;
    hw_ops->set_rss_hash_key = mac_set_rss_hash_key;
    hw_ops->set_rss_lookup_table = mac_set_rss_lookup_table;
}
