#ifndef __BX_MAC_H__
#define __BX_MAC_H__


#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/workqueue.h>
#include <linux/phy.h>
#include <linux/if_vlan.h>
#include <linux/bitops.h>
#include <linux/timecounter.h>
#include <linux/list.h>//added by hs@20200420

/* Descriptor related parameters */
#define MAC_TX_DESC_CNT                 4096 //insomnia
#define MAC_TX_DESC_MIN_FREE            (MAC_TX_DESC_CNT >> 3)
#define MAC_TX_DESC_MAX_PROC            (MAC_TX_DESC_CNT >> 1)
#define MAC_RX_DESC_CNT                 4096 //insomnia
#define MAC_RX_DESC_MAX_DIRTY           (MAC_RX_DESC_CNT >> 3)

#define MAC_TX_MAX_BUF_SIZE             (0x3fff & ~(64 - 1))
#define MAC_RX_MIN_BUF_SIZE             (ETH_FRAME_LEN + ETH_FCS_LEN + VLAN_HLEN)
#define MAC_RX_BUF_ALIGN                64

/* Descriptors required for maximum contiguous TSO/GSO packet */
#define MAC_TX_MAX_SPLIT                ((GSO_MAX_SIZE / MAC_TX_MAX_BUF_SIZE) + 1)

/* Maximum possible descriptors needed for a SKB */
#define MAC_TX_MAX_DESC_NR              (MAX_SKB_FRAGS + MAC_TX_MAX_SPLIT + 2)


/* Maximum Size for Splitting the Header Data
 * Keep in sync with SKB_ALLOC_SIZE
 * 3'b000: 64 bytes, 3'b001: 128 bytes
 * 3'b010: 256 bytes, 3'b011: 512 bytes
 * 3'b100: 1023 bytes ,   3'b101'3'b111: Reserved
 */
#define MAC_SPH_HDSMS_SIZE              3
#define MAC_SKB_ALLOC_SIZE              512

#define MAC_MAX_FIFO                    65536
//#define MAC_MAX_FIFO                  57344
//#define MAC_MAX_FIFO                  49152
//#define MAC_MAX_FIFO                  32768
//#define MAC_MAX_FIFO                  16384

//insomnia@20200129
#define MAC_MAX_DMA_CHANNELS            7
#define MAC_DMA_STOP_TIMEOUT            5
#define MAC_DMA_INTERRUPT_MASK          0x31c7

/* Default coalescing parameters */
#define MAC_INIT_DMA_TX_USECS           1000
#define MAC_INIT_DMA_TX_FRAMES          25              
#define MAC_INIT_DMA_RX_USECS           54      //insomnia: max 54us for RWTU 256, 1:5KB
#define MAC_INIT_DMA_RX_FRAMES          25
#define MAC_MAX_DMA_RIWT                0xff
#define MAC_MIN_DMA_RIWT                0x01

/* Flow control queue count */
#define MAC_MAX_FLOW_CONTROL_QUEUES     7 //insomnia@20200201

/* System clock is 1200 MHz */
#define MAC_SYSCLOCK                    1200000000

/* Maximum MAC address hash table size (256 bits = 8 bytes) */
#define MAC_MAC_HASH_TABLE_SIZE         8

/* Receive Side Scaling */
#define MAC_RSS_HASH_KEY_SIZE           40
#define MAC_RSS_MAX_TABLE_SIZE          256
#define MAC_RSS_LOOKUP_TABLE_TYPE       0
#define MAC_RSS_HASH_KEY_TYPE           1

#define MAC_STD_PACKET_MTU              1500
#define MAC_JUMBO_PACKET_MTU            9000

/* Helper macro for descriptor handling
 *  Always use MAC_GET_DESC_DATA to access the descriptor data
 */
#define MAC_GET_DESC_DATA(ring, idx) ({                \
    typeof(ring) _ring = (ring);                    \
    ((_ring)->desc_data_head +                    \
     ((idx) & ((_ring)->dma_desc_count - 1)));            \
})

#define MAC_GET_REG_BITS(var, pos, len) ({                \
    typeof(pos) _pos = (pos);                    \
    typeof(len) _len = (len);                    \
    ((var) & GENMASK(_pos + _len - 1, _pos)) >> (_pos);        \
})

#define MAC_GET_REG_BITS_LE(var, pos, len) ({            \
    typeof(pos) _pos = (pos);                    \
    typeof(len) _len = (len);                    \
    typeof(var) _var = le32_to_cpu((var));                \
    ((_var) & GENMASK(_pos + _len - 1, _pos)) >> (_pos);        \
})

#define MAC_SET_REG_BITS(var, pos, len, val) ({            \
    typeof(var) _var = (var);                    \
    typeof(pos) _pos = (pos);                    \
    typeof(len) _len = (len);                    \
    typeof(val) _val = (val);                    \
    _val = (_val << _pos) & GENMASK(_pos + _len - 1, _pos);        \
    _var = (_var & ~GENMASK(_pos + _len - 1, _pos)) | _val;        \
})

#define MAC_SET_REG_BITS_LE(var, pos, len, val) ({            \
    typeof(var) _var = (var);                    \
    typeof(pos) _pos = (pos);                    \
    typeof(len) _len = (len);                    \
    typeof(val) _val = (val);                    \
    _val = (_val << _pos) & GENMASK(_pos + _len - 1, _pos);        \
    _var = (_var & ~GENMASK(_pos + _len - 1, _pos)) | _val;        \
    cpu_to_le32(_var);                        \
})



enum mac_int {
    MAC_INT_DMA_CH_SR_TI,
    MAC_INT_DMA_CH_SR_TPS,
    MAC_INT_DMA_CH_SR_TBU,
    MAC_INT_DMA_CH_SR_RI,
    MAC_INT_DMA_CH_SR_RBU,
    MAC_INT_DMA_CH_SR_RPS,
    MAC_INT_DMA_CH_SR_TI_RI,
    MAC_INT_DMA_CH_SR_FBE,
    MAC_INT_DMA_ALL,
};

struct mac_stats {
    /* MMC TX counters */
    u64 txoctetcount_gb;
    u64 txframecount_gb;
    u64 txbroadcastframes_g;
    u64 txmulticastframes_g;
    u64 tx64octets_gb;
    u64 tx65to127octets_gb;
    u64 tx128to255octets_gb;
    u64 tx256to511octets_gb;
    u64 tx512to1023octets_gb;
    u64 tx1024tomaxoctets_gb;
    u64 txunicastframes_gb;
    u64 txmulticastframes_gb;
    u64 txbroadcastframes_gb;
    u64 txunderflowerror;
    u64 txoctetcount_g;
    u64 txframecount_g;
    u64 txpauseframes;
    u64 txvlanframes_g;

    /* MMC RX counters */
    u64 rxframecount_gb;
    u64 rxoctetcount_gb;
    u64 rxoctetcount_g;
    u64 rxbroadcastframes_g;
    u64 rxmulticastframes_g;
    u64 rxcrcerror;
    u64 rxrunterror;
    u64 rxjabbererror;
    u64 rxundersize_g;
    u64 rxoversize_g;
    u64 rx64octets_gb;
    u64 rx65to127octets_gb;
    u64 rx128to255octets_gb;
    u64 rx256to511octets_gb;
    u64 rx512to1023octets_gb;
    u64 rx1024tomaxoctets_gb;
    u64 rxunicastframes_g;
    u64 rxlengtherror;
    u64 rxoutofrangetype;
    u64 rxpauseframes;
    u64 rxfifooverflow;
    u64 rxvlanframes_gb;
    u64 rxwatchdogerror;

    /* Extra counters */
    u64 tx_tso_packets;
    u64 rx_split_header_packets;
    u64 tx_process_stopped;
    u64 rx_process_stopped;
    u64 tx_buffer_unavailable;
    u64 rx_buffer_unavailable;
    u64 fatal_bus_error;
    u64 tx_vlan_packets;
    u64 rx_vlan_packets;
    u64 napi_poll_isr;
    u64 napi_poll_txtimer;
};

struct mac_ring_buf {
    struct sk_buff *skb;
    dma_addr_t skb_dma;
    unsigned int skb_len;
};

/* Common Tx and Rx DMA hardware descriptor */
struct mac_dma_desc {
    __le32 desc0;
    __le32 desc1;
    __le32 desc2;
    __le32 desc3;
};

/* Page allocation related values */
struct mac_page_alloc {
    struct page *pages;
    unsigned int pages_len;
    unsigned int pages_offset;

    dma_addr_t pages_dma;
};

/* Ring entry buffer data */
struct mac_buffer_data {
    struct mac_page_alloc pa;
    struct mac_page_alloc pa_unmap;

    dma_addr_t dma_base;
    unsigned long dma_off;
    unsigned int dma_len;
};

/* Tx-related desc data */
struct mac_tx_desc_data {
    unsigned int packets;        /* BQL packet count */
    unsigned int bytes;        /* BQL byte count */
};

/* Rx-related desc data */
struct mac_rx_desc_data {
    struct mac_buffer_data hdr;    /* Header locations */
    struct mac_buffer_data buf;    /* Payload locations */

    unsigned short hdr_len;        /* Length of received header */
    unsigned short len;        /* Length of received packet */
};

struct mac_pkt_info {
    struct sk_buff *skb;

    unsigned int attributes;

    unsigned int errors;

    /* descriptors needed for this packet */
    unsigned int desc_count;
    unsigned int length;

    unsigned int tx_packets;
    unsigned int tx_bytes;

    unsigned int header_len;
    unsigned int tcp_header_len;
    unsigned int tcp_payload_len;
    unsigned short mss;

    unsigned short vlan_ctag;

    u64 rx_tstamp;

    u32 rss_hash;
    enum pkt_hash_types rss_hash_type;
};

struct mac_desc_data {
    /* dma_desc: Virtual address of descriptor
     *  dma_desc_addr: DMA address of descriptor
     */
    struct mac_dma_desc *dma_desc;
    dma_addr_t dma_desc_addr;

    /* skb: Virtual address of SKB
     *  skb_dma: DMA address of SKB data
     *  skb_dma_len: Length of SKB DMA area
     */
    struct sk_buff *skb;
    dma_addr_t skb_dma;
    unsigned int skb_dma_len;

    /* Tx/Rx -related data */
    struct mac_tx_desc_data tx;
    struct mac_rx_desc_data rx;

    unsigned int mapped_as_page;

    /* Incomplete receive save location.  If the budget is exhausted
     * or the last descriptor (last normal descriptor or a following
     * context descriptor) has not been DMA'd yet the current state
     * of the receive processing needs to be saved.
     */
    unsigned int state_saved;
    struct {
        struct sk_buff *skb;
        unsigned int len;
        unsigned int error;
    } state;
}; 

struct mac_ring {
    /* Per packet related information */
    struct mac_pkt_info pkt_info;

    /* Virtual/DMA addresses of DMA descriptor list and the total count */
    struct mac_dma_desc *dma_desc_head;
    dma_addr_t dma_desc_head_addr;
    unsigned int dma_desc_count;

    /* Array of descriptor data corresponding the DMA descriptor
     * (always use the MAC_GET_DESC_DATA macro to access this data)
     */
    struct mac_desc_data *desc_data_head;

    /* Page allocation for RX buffers */
    struct mac_page_alloc rx_hdr_pa;
    struct mac_page_alloc rx_buf_pa;

    /* Ring index values
     *  cur   - Tx: index of descriptor to be used for current transfer
     *          Rx: index of descriptor to check for packet availability
     *  dirty - Tx: index of descriptor to check for transfer complete
     *          Rx: index of descriptor to check for buffer reallocation
     */
    unsigned int cur;
    unsigned int dirty;

    /* Coalesce frame count used for interrupt bit setting */
    unsigned int coalesce_count;

    union {
        struct {
            unsigned int xmit_more;
            unsigned int queue_stopped;
            unsigned short cur_mss;
            unsigned short cur_vlan_ctag;
        } tx;
    };
} ____cacheline_aligned;

struct mac_channel {
    char name[16];

    /* Address of private data area for device */
    struct mac_pdata *pdata;

    /* Queue index and base address of queue's DMA registers */
    unsigned int queue_index;
    void __iomem *dma_regs;

    /* Per channel interrupt irq number */
    int  dma_irq;
    char dma_irq_name   [IFNAMSIZ + 32];
    char dma_irq_name_tx[IFNAMSIZ + 32];
    char dma_irq_name_rx[IFNAMSIZ + 32];

    /* Netdev related settings */
    struct napi_struct napi;

    unsigned int saved_ier;

    unsigned int tx_timer_active;
    struct timer_list tx_timer;

    struct mac_ring *tx_ring;
    struct mac_ring *rx_ring;
} ____cacheline_aligned;

struct mac_desc_ops {
    int (*alloc_channles_and_rings)(struct mac_pdata *pdata);
    void (*free_channels_and_rings)(struct mac_pdata *pdata);
    int (*map_tx_skb)(struct mac_channel *channel,
              struct sk_buff *skb);
    int (*map_rx_buffer)(struct mac_pdata *pdata,
                 struct mac_ring *ring,
            struct mac_desc_data *desc_data);
    void (*unmap_desc_data)(struct mac_pdata *pdata,
                struct mac_desc_data *desc_data);
    void (*tx_desc_init)(struct mac_pdata *pdata);
    void (*rx_desc_init)(struct mac_pdata *pdata);
};

struct mac_hw_ops {
    int (*init)(struct mac_pdata *pdata);
    int (*exit)(struct mac_pdata *pdata);

    int (*tx_complete)(struct mac_dma_desc *dma_desc);

    void (*enable_tx)(struct mac_pdata *pdata);
    void (*disable_tx)(struct mac_pdata *pdata);
    void (*enable_rx)(struct mac_pdata *pdata);
    void (*disable_rx)(struct mac_pdata *pdata);

    int (*enable_int)(struct mac_channel *channel,
              enum mac_int int_id);
    int (*disable_int)(struct mac_channel *channel,
               enum mac_int int_id);
    void (*dev_xmit)(struct mac_channel *channel);
    int (*dev_read)(struct mac_channel *channel);

    int (*set_mac_address)(struct mac_pdata *pdata, u8 *addr);
    int (*config_rx_mode)(struct mac_pdata *pdata);
    int (*enable_rx_csum)(struct mac_pdata *pdata);
    int (*disable_rx_csum)(struct mac_pdata *pdata);

    /* For descriptor related operation */
    void (*tx_desc_init)(struct mac_channel *channel);
    void (*rx_desc_init)(struct mac_channel *channel);
    void (*tx_desc_reset)(struct mac_desc_data *desc_data);
    void (*rx_desc_reset)(struct mac_pdata *pdata,
                  struct mac_desc_data *desc_data,
            unsigned int index);
    int (*is_last_desc)(struct mac_dma_desc *dma_desc);
    int (*is_context_desc)(struct mac_dma_desc *dma_desc);
    void (*tx_start_xmit)(struct mac_channel *channel,
                  struct mac_ring *ring);

    /* For Flow Control */
    int (*config_tx_flow_control)(struct mac_pdata *pdata);
    int (*config_rx_flow_control)(struct mac_pdata *pdata);

    /* For Vlan related config */
    int (*enable_rx_vlan_stripping)(struct mac_pdata *pdata);
    int (*disable_rx_vlan_stripping)(struct mac_pdata *pdata);
    int (*enable_rx_vlan_filtering)(struct mac_pdata *pdata);
    int (*disable_rx_vlan_filtering)(struct mac_pdata *pdata);
    int (*update_vlan_hash_table)(struct mac_pdata *pdata);

    /* For RX coalescing */
    int (*config_rx_coalesce)(struct mac_pdata *pdata);
    int (*config_tx_coalesce)(struct mac_pdata *pdata);
    unsigned int (*usec_to_riwt)(struct mac_pdata *pdata,
                     unsigned int usec);
    unsigned int (*riwt_to_usec)(struct mac_pdata *pdata,
                     unsigned int riwt);

    /* For RX and TX threshold config */
    int (*config_rx_threshold)(struct mac_pdata *pdata,
                   unsigned int val);
    int (*config_tx_threshold)(struct mac_pdata *pdata,
                   unsigned int val);

    /* For RX and TX Store and Forward Mode config */
    int (*config_rsf_mode)(struct mac_pdata *pdata,
                   unsigned int val);
    int (*config_tsf_mode)(struct mac_pdata *pdata,
                   unsigned int val);

    /* For TX DMA Operate on Second Frame config */
    int (*config_osp_mode)(struct mac_pdata *pdata);

    /* For RX and TX PBL config */
    int (*config_rx_pbl_val)(struct mac_pdata *pdata);
    int (*get_rx_pbl_val)(struct mac_pdata *pdata);
    int (*config_tx_pbl_val)(struct mac_pdata *pdata);
    int (*get_tx_pbl_val)(struct mac_pdata *pdata);
    int (*config_pblx8)(struct mac_pdata *pdata);

    /* For MMC statistics */
    void (*rx_mmc_int)(struct mac_pdata *pdata);
    void (*tx_mmc_int)(struct mac_pdata *pdata);
    void (*read_mmc_stats)(struct mac_pdata *pdata);

    /* For Receive Side Scaling */
    int (*enable_rss)(struct mac_pdata *pdata);
    int (*disable_rss)(struct mac_pdata *pdata);
    int (*set_rss_hash_key)(struct mac_pdata *pdata,
                const u8 *key);
    int (*set_rss_lookup_table)(struct mac_pdata *pdata,
                    const u32 *table);
};

/* This structure contains flags that indicate what hardware features
 * or configurations are present in the device.
 */
struct mac_hw_features {
    /* HW Version */
    unsigned int version;

    /* HW Feature Register0 */
    unsigned int phyifsel;        /* PHY interface support */
    unsigned int vlhash;        /* VLAN Hash Filter */
    unsigned int sma;        /* SMA(MDIO) Interface */
    unsigned int rwk;        /* PMT remote wake-up packet */
    unsigned int mgk;        /* PMT magic packet */
    unsigned int mmc;        /* RMON module */
    unsigned int aoe;        /* ARP Offload */
    unsigned int ts;        /* IEEE 1588-2008 Advanced Timestamp */
    unsigned int eee;        /* Energy Efficient Ethernet */
    unsigned int tx_coe;        /* Tx Checksum Offload */
    unsigned int rx_coe;        /* Rx Checksum Offload */
    unsigned int addn_mac;        /* Additional MAC Addresses */
    unsigned int ts_src;        /* Timestamp Source */
    unsigned int sa_vlan_ins;    /* Source Address or VLAN Insertion */

    /* HW Feature Register1 */
    unsigned int rx_fifo_size;    /* MTL Receive FIFO Size */
    unsigned int tx_fifo_size;    /* MTL Transmit FIFO Size */
    unsigned int adv_ts_hi;        /* Advance Timestamping High Word */
    unsigned int dma_width;        /* DMA width */
    unsigned int dcb;        /* DCB Feature */
    unsigned int sph;        /* Split Header Feature */
    unsigned int tso;        /* TCP Segmentation Offload */
    unsigned int dma_debug;        /* DMA Debug Registers */
    unsigned int rss;        /* Receive Side Scaling */
    unsigned int tc_cnt;        /* Number of Traffic Classes */
    unsigned int hash_table_size;    /* Hash Table Size */
    unsigned int l3l4_filter_num;    /* Number of L3-L4 Filters */

    /* HW Feature Register2 */
    unsigned int rx_q_cnt;        /* Number of MTL Receive Queues */
    unsigned int tx_q_cnt;        /* Number of MTL Transmit Queues */
    unsigned int rx_ch_cnt;        /* Number of DMA Receive Channels */
    unsigned int tx_ch_cnt;        /* Number of DMA Transmit Channels */
    unsigned int pps_out_num;    /* Number of PPS outputs */
    unsigned int aux_snap_num;    /* Number of Aux snapshot inputs */
};

struct mac_resources {
    void __iomem *addr;
    int irq;
    int msi_irq_cnt;//insomnia@20200213
};


struct mac_pdata {
    struct net_device *netdev;
    struct device *dev;

    struct mac_hw_ops hw_ops;
    struct mac_desc_ops desc_ops;

	//added by hs@20200416
	struct bxroce_dev *rocedev;
	struct list_head list;

    /* Device statistics */
    struct mac_stats stats;

    u32 msg_enable;

    /* MAC registers base */
    void __iomem *mac_regs;

    /* Hardware features of the device */
    struct mac_hw_features hw_feat;

    struct work_struct restart_work;

    /* Rings for Tx/Rx on a DMA channel */
    struct mac_channel *channel_head;
    unsigned int channel_count;
    unsigned int tx_ring_count;
    unsigned int rx_ring_count;
    unsigned int tx_desc_count;
    unsigned int rx_desc_count;
    unsigned int tx_q_count;
    unsigned int rx_q_count;

    /* Tx/Rx common settings */
    unsigned int pblx8;

    /* Tx settings */
    unsigned int tx_sf_mode;
    unsigned int tx_threshold;
    unsigned int tx_pbl;
    unsigned int tx_osp_mode;

    /* Rx settings */
    unsigned int rx_sf_mode;
    unsigned int rx_threshold;
    unsigned int rx_pbl;

    /* Tx coalescing settings */
    unsigned int tx_usecs;
    unsigned int tx_frames;

    /* Rx coalescing settings */
    unsigned int rx_riwt;
    unsigned int rx_usecs;
    unsigned int rx_frames;

    /* Current Rx buffer size */
    unsigned int rx_buf_size;

    /* Flow control settings */
    unsigned int tx_pause;
    unsigned int rx_pause;

    /* Device interrupt number */
    int dev_irq;
    unsigned int per_channel_irq;
    int channel_irq[MAC_MAX_DMA_CHANNELS];

    /* Netdev related settings */
    unsigned char mac_addr[ETH_ALEN];
    netdev_features_t netdev_features;
    
    struct napi_struct napi;
    
    //insomnia@202028
    //msi with 1 irq
    //struct napi_struct napi_msi;
    
    //msi with 16 or 32 irqs
    struct napi_struct napi_msi_mac_0_tx_0__pgu_0;
    struct napi_struct napi_msi_mac_0_tx_1__pgu_1;
    struct napi_struct napi_msi_mac_0_tx_2__pgu_2;
    struct napi_struct napi_msi_mac_0_tx_3__pgu_3;
    struct napi_struct napi_msi_mac_0_tx_4__pgu_4;
    struct napi_struct napi_msi_mac_0_tx_5__pbu;
    struct napi_struct napi_msi_mac_0_tx_6__cm;
    struct napi_struct napi_msi_mac_0_rx_0;
    struct napi_struct napi_msi_mac_0_rx_1;
    struct napi_struct napi_msi_mac_0_rx_2;
    struct napi_struct napi_msi_mac_0_rx_3;
    struct napi_struct napi_msi_mac_0_rx_4;
    struct napi_struct napi_msi_mac_0_rx_5;
    struct napi_struct napi_msi_mac_0_rx_6;
    struct napi_struct napi_msi_mac_0_sbd__pcie_link_0_misc;
    struct napi_struct napi_msi_mac_0_pmt__pcs_0_sbd__pcie_link_0_err;

    //msi with 32 irqs
    struct napi_struct napi_msi_mac_1_tx_0;
    struct napi_struct napi_msi_mac_1_tx_1;
    struct napi_struct napi_msi_mac_1_tx_2;
    struct napi_struct napi_msi_mac_1_tx_3;
    struct napi_struct napi_msi_mac_1_tx_4;
    struct napi_struct napi_msi_mac_1_tx_5;
    struct napi_struct napi_msi_mac_1_tx_6;
    struct napi_struct napi_msi_mac_1_rx_0;
    struct napi_struct napi_msi_mac_1_rx_1;
    struct napi_struct napi_msi_mac_1_rx_2;
    struct napi_struct napi_msi_mac_1_rx_3;
    struct napi_struct napi_msi_mac_1_rx_4;
    struct napi_struct napi_msi_mac_1_rx_5;
    struct napi_struct napi_msi_mac_1_rx_6;
    struct napi_struct napi_msi_mac_1_sbd__pcie_link_1_misc;
    struct napi_struct napi_msi_mac_1_pmt__pcs_1_sbd__pcie_link_1_err;

    /* Filtering support */
    unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];

    /* Device clocks */
    unsigned long sysclk_rate;

    /* RSS addressing mutex */
    struct mutex rss_mutex;

    /* Receive Side Scaling settings */
    u8 rss_key[MAC_RSS_HASH_KEY_SIZE];
    u32 rss_table[MAC_RSS_MAX_TABLE_SIZE];
    u32 rss_options;


    char drv_name[32];
    char drv_ver[32];

    //insomnia@20200202
    struct rnic_pdata  rnic_pdata;

    //insomnia@20200209 for ieu intr clear
    struct timer_list ieu_timer;
    unsigned int      ieu_clr_intr_usecs;

    struct timer_list link_check_timer;
    unsigned int      link_check_usecs;

    //insomnia@20200213
    struct pci_dev *pcidev;

    //insomnia@20200216
    int  msi_irq_cnt;
    int  msi_irq_num [32];
    char msi_irq_name[32][IFNAMSIZ + 32];

    //insomnia@20200218
    int link_up;

    //insomnia@20200221
    int tx_pkg_cnt;
    int tx_times_cnt;
    int rx_pkg_cnt;
    int rx_times_cnt;

    //insomnia@20200223
    int tx_desc_update_cnt;
    int tx_desc_cur_last;
    int tx_desc_cur_update_total;
    long long tx_desc_update_time_nsec_first;
    long long tx_desc_update_time_nsec_last;

    //insomnia@20200224
    struct timer_list an_restart_lock_timer;
    unsigned int      an_restart_lock_usecs;    
    
}____cacheline_aligned;

void mac_init_desc_ops(struct mac_desc_ops *desc_ops);
void mac_init_hw_ops(struct mac_hw_ops *hw_ops);
const struct net_device_ops *mac_get_netdev_ops(void);
const struct ethtool_ops *mac_get_ethtool_ops(void);
void mac_dump_tx_desc(struct mac_pdata *pdata,
             struct mac_ring *ring,
             unsigned int idx,
             unsigned int count,
             unsigned int flag);
void mac_dump_rx_desc(struct mac_pdata *pdata,
             struct mac_ring *ring,
             unsigned int idx);
void mac_print_pkt(struct net_device *netdev,
              struct sk_buff *skb, bool tx_rx);
void mac_get_all_hw_features(struct mac_pdata *pdata);
void mac_print_all_hw_features(struct mac_pdata *pdata);


//int mac_drv_probe  (struct device *,struct mac_resources *);
//int mac_drv_remove (struct device *);

int mac_drv_probe  (struct pci_dev *,struct mac_resources *);
int mac_drv_remove (struct pci_dev *);


#endif /* __BX_MAC_H__ */
