
#include <linux/ethtool.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>

#include "header/bx_rnic.h"

struct mac_stats_desc {
    char stat_string[ETH_GSTRING_LEN];
    int stat_offset;
};

#define MAC_STAT(str, var)                  \
    {                           \
        str,                        \
        offsetof(struct mac_pdata, stats.var),  \
    }

static const struct mac_stats_desc mac_gstring_stats[] = {
    /* MMC TX counters */
    MAC_STAT("tx_bytes", txoctetcount_gb),
    MAC_STAT("tx_bytes_good", txoctetcount_g),
    MAC_STAT("tx_packets", txframecount_gb),
    MAC_STAT("tx_packets_good", txframecount_g),
    MAC_STAT("tx_unicast_packets", txunicastframes_gb),
    MAC_STAT("tx_broadcast_packets", txbroadcastframes_gb),
    MAC_STAT("tx_broadcast_packets_good", txbroadcastframes_g),
    MAC_STAT("tx_multicast_packets", txmulticastframes_gb),
    MAC_STAT("tx_multicast_packets_good", txmulticastframes_g),
    MAC_STAT("tx_vlan_packets_good", txvlanframes_g),
    MAC_STAT("tx_64_byte_packets", tx64octets_gb),
    MAC_STAT("tx_65_to_127_byte_packets", tx65to127octets_gb),
    MAC_STAT("tx_128_to_255_byte_packets", tx128to255octets_gb),
    MAC_STAT("tx_256_to_511_byte_packets", tx256to511octets_gb),
    MAC_STAT("tx_512_to_1023_byte_packets", tx512to1023octets_gb),
    MAC_STAT("tx_1024_to_max_byte_packets", tx1024tomaxoctets_gb),
    MAC_STAT("tx_underflow_errors", txunderflowerror),
    MAC_STAT("tx_pause_frames", txpauseframes),

    /* MMC RX counters */
    MAC_STAT("rx_bytes", rxoctetcount_gb),
    MAC_STAT("rx_bytes_good", rxoctetcount_g),
    MAC_STAT("rx_packets", rxframecount_gb),
    MAC_STAT("rx_unicast_packets_good", rxunicastframes_g),
    MAC_STAT("rx_broadcast_packets_good", rxbroadcastframes_g),
    MAC_STAT("rx_multicast_packets_good", rxmulticastframes_g),
    MAC_STAT("rx_vlan_packets", rxvlanframes_gb),
    MAC_STAT("rx_64_byte_packets", rx64octets_gb),
    MAC_STAT("rx_65_to_127_byte_packets", rx65to127octets_gb),
    MAC_STAT("rx_128_to_255_byte_packets", rx128to255octets_gb),
    MAC_STAT("rx_256_to_511_byte_packets", rx256to511octets_gb),
    MAC_STAT("rx_512_to_1023_byte_packets", rx512to1023octets_gb),
    MAC_STAT("rx_1024_to_max_byte_packets", rx1024tomaxoctets_gb),
    MAC_STAT("rx_undersize_packets_good", rxundersize_g),
    MAC_STAT("rx_oversize_packets_good", rxoversize_g),
    MAC_STAT("rx_crc_errors", rxcrcerror),
    MAC_STAT("rx_crc_errors_small_packets", rxrunterror),
    MAC_STAT("rx_crc_errors_giant_packets", rxjabbererror),
    MAC_STAT("rx_length_errors", rxlengtherror),
    MAC_STAT("rx_out_of_range_errors", rxoutofrangetype),
    MAC_STAT("rx_fifo_overflow_errors", rxfifooverflow),
    MAC_STAT("rx_watchdog_errors", rxwatchdogerror),
    MAC_STAT("rx_pause_frames", rxpauseframes),

    /* Extra counters */
    MAC_STAT("tx_tso_packets", tx_tso_packets),
    MAC_STAT("rx_split_header_packets", rx_split_header_packets),
    MAC_STAT("tx_process_stopped", tx_process_stopped),
    MAC_STAT("rx_process_stopped", rx_process_stopped),
    MAC_STAT("tx_buffer_unavailable", tx_buffer_unavailable),
    MAC_STAT("rx_buffer_unavailable", rx_buffer_unavailable),
    MAC_STAT("fatal_bus_error", fatal_bus_error),
    MAC_STAT("tx_vlan_packets", tx_vlan_packets),
    MAC_STAT("rx_vlan_packets", rx_vlan_packets),
    MAC_STAT("napi_poll_isr", napi_poll_isr),
    MAC_STAT("napi_poll_txtimer", napi_poll_txtimer),
};

#define MAC_STATS_COUNT ARRAY_SIZE(mac_gstring_stats)

static void mac_ethtool_get_drvinfo(struct net_device *netdev,
                       struct ethtool_drvinfo *drvinfo)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    u32 ver = pdata->hw_feat.version;
    u32 bxver, devid, userver;
    
    RNIC_TRACE_PRINT();
    
    strlcpy(drvinfo->driver, pdata->drv_name, sizeof(drvinfo->driver));
    strlcpy(drvinfo->version, pdata->drv_ver, sizeof(drvinfo->version));
    strlcpy(drvinfo->bus_info, dev_name(pdata->dev),
        sizeof(drvinfo->bus_info));
    /* B|BXVER: Binxin-defined Version
     * D|DEVID: Indicates the Device family
     * U|USERVER: User-defined Version
     */
    bxver = MAC_GET_REG_BITS(ver, MAC_VR_BXVER_POS,
                      MAC_VR_BXVER_LEN);
    devid = MAC_GET_REG_BITS(ver, MAC_VR_DEVID_POS,
                    MAC_VR_DEVID_LEN);
    userver = MAC_GET_REG_BITS(ver, MAC_VR_USERVER_POS,
                      MAC_VR_USERVER_LEN);
    snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version),
         "S.D.U: %x.%x.%x", bxver, devid, userver);
}


//insomnia
static u32  mac_ethtool_op_get_link(struct net_device *netdev)
{
    struct mac_pdata *pdata = netdev_priv(netdev);

    RNIC_TRACE_PRINT();
    
    //return get_rlu(&pdata->rnic_pdata, 0) && (mac_get_link_status(&pdata->rnic_pdata, 0) == 0);
    return (mac_get_link_status(&pdata->rnic_pdata, 0) == 0);
}


static u32 mac_ethtool_get_msglevel(struct net_device *netdev)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    
    RNIC_TRACE_PRINT();
    
    return pdata->msg_enable;
}

static void mac_ethtool_set_msglevel(struct net_device *netdev,
                    u32 msglevel)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    
    RNIC_TRACE_PRINT();
    
    pdata->msg_enable = msglevel;
}

static void mac_ethtool_get_channels(struct net_device *netdev,
                    struct ethtool_channels *channel)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    
    RNIC_TRACE_PRINT();
    
    channel->max_rx = MAC_MAX_DMA_CHANNELS;
    channel->max_tx = MAC_MAX_DMA_CHANNELS;
    channel->rx_count = pdata->rx_q_count;
    channel->tx_count = pdata->tx_q_count;
}

static int mac_ethtool_get_coalesce(struct net_device *netdev,
                       struct ethtool_coalesce *ec)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    
    RNIC_TRACE_PRINT();
    
    memset(ec, 0, sizeof(struct ethtool_coalesce));
    ec->rx_coalesce_usecs = pdata->rx_usecs;
    ec->rx_max_coalesced_frames = pdata->rx_frames;
    ec->tx_max_coalesced_frames = pdata->tx_frames;

    return 0;
}

static int mac_ethtool_set_coalesce(struct net_device *netdev,
                       struct ethtool_coalesce *ec)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    struct mac_hw_ops *hw_ops = &pdata->hw_ops;
    unsigned int rx_frames, rx_riwt, rx_usecs;
    unsigned int tx_frames;
    
    RNIC_TRACE_PRINT();
    
    /* Check for not supported parameters */
    if ((ec->rx_coalesce_usecs_irq) || (ec->rx_max_coalesced_frames_irq) ||
        (ec->tx_coalesce_usecs) || (ec->tx_coalesce_usecs_high) ||
        (ec->tx_max_coalesced_frames_irq) || (ec->tx_coalesce_usecs_irq) ||
        (ec->stats_block_coalesce_usecs) ||  (ec->pkt_rate_low) ||
        (ec->use_adaptive_rx_coalesce) || (ec->use_adaptive_tx_coalesce) ||
        (ec->rx_max_coalesced_frames_low) || (ec->rx_coalesce_usecs_low) ||
        (ec->tx_coalesce_usecs_low) || (ec->tx_max_coalesced_frames_low) ||
        (ec->pkt_rate_high) || (ec->rx_coalesce_usecs_high) ||
        (ec->rx_max_coalesced_frames_high) ||
        (ec->tx_max_coalesced_frames_high) ||
        (ec->rate_sample_interval))
        return -EOPNOTSUPP;

    rx_usecs = ec->rx_coalesce_usecs;
    rx_riwt = hw_ops->usec_to_riwt(pdata, rx_usecs);
    rx_frames = ec->rx_max_coalesced_frames;
    tx_frames = ec->tx_max_coalesced_frames;

    if ((rx_riwt > MAC_MAX_DMA_RIWT) ||
        (rx_riwt < MAC_MIN_DMA_RIWT) ||
        (rx_frames > pdata->rx_desc_count))
        return -EINVAL;

    if (tx_frames > pdata->tx_desc_count)
        return -EINVAL;

    pdata->rx_riwt = rx_riwt;
    pdata->rx_usecs = rx_usecs;
    pdata->rx_frames = rx_frames;
    hw_ops->config_rx_coalesce(pdata);

    pdata->tx_frames = tx_frames;
    hw_ops->config_tx_coalesce(pdata);

    return 0;
}

static void mac_ethtool_get_strings(struct net_device *netdev,
                       u32 stringset, u8 *data)
{
    int i;
    
    RNIC_TRACE_PRINT();
    
    switch (stringset) {
    case ETH_SS_STATS:
        for (i = 0; i < MAC_STATS_COUNT; i++) {
            memcpy(data, mac_gstring_stats[i].stat_string,
                   ETH_GSTRING_LEN);
            data += ETH_GSTRING_LEN;
        }
        break;
    default:
        WARN_ON(1);
        break;
    }
}

static int mac_ethtool_get_sset_count(struct net_device *netdev,
                     int stringset)
{
    int ret;
    
    RNIC_TRACE_PRINT();
    
    switch (stringset) {
    case ETH_SS_STATS:
        ret = MAC_STATS_COUNT;
        break;

    default:
        ret = -EOPNOTSUPP;
    }

    return ret;
}

static void mac_ethtool_get_ethtool_stats(struct net_device *netdev,
                         struct ethtool_stats *stats,
                         u64 *data)
{
    struct mac_pdata *pdata = netdev_priv(netdev);
    u8 *stat;
    int i;
    
    RNIC_TRACE_PRINT();
    pdata->hw_ops.read_mmc_stats(pdata);
    for (i = 0; i < MAC_STATS_COUNT; i++) {
        stat = (u8 *)pdata + mac_gstring_stats[i].stat_offset;
        *data++ = *(u64 *)stat;
    }
}

static const struct ethtool_ops mac_ethtool_ops = {
    .get_drvinfo = mac_ethtool_get_drvinfo,
    //.get_link = ethtool_op_get_link,
    .get_link = mac_ethtool_op_get_link, //insomnia
    .get_msglevel = mac_ethtool_get_msglevel,
    .set_msglevel = mac_ethtool_set_msglevel,
    .get_channels = mac_ethtool_get_channels,
    .get_coalesce = mac_ethtool_get_coalesce,
    .set_coalesce = mac_ethtool_set_coalesce,
    .get_strings = mac_ethtool_get_strings,
    .get_sset_count = mac_ethtool_get_sset_count,
    .get_ethtool_stats = mac_ethtool_get_ethtool_stats,
};

const struct ethtool_ops *mac_get_ethtool_ops(void)
{   
    RNIC_TRACE_PRINT();
    
    return &mac_ethtool_ops;
}