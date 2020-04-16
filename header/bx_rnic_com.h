//insomnia@2019/12/11 11:40:28

#ifndef __BX_RNIC_COM_H_
#define __BX_RNIC_COM_H_


#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/workqueue.h>
#include <linux/phy.h>
#include <linux/if_vlan.h>
#include <linux/bitops.h>
#include <linux/timecounter.h>


#define RNIC_REG_BASE_ADDR_MPB      0x0000000
#define RNIC_REG_BASE_ADDR_FLASH    0x1000000
#define RNIC_REG_BASE_ADDR_PCIE_0   0x2000000
#define RNIC_REG_BASE_ADDR_PCIE_1   0x3000000
#define RNIC_REG_BASE_ADDR_PCIE_2   0x4000000
#define RNIC_REG_BASE_ADDR_IEU      0x5000000
#define RNIC_REG_BASE_ADDR_MPP      0x6000000
#define RNIC_REG_BASE_ADDR_PCS_0    0x7000000
#define RNIC_REG_BASE_ADDR_PCS_1    0x8000000
#define RNIC_REG_BASE_ADDR_CRU      0x9000000
#define RNIC_REG_BASE_ADDR_I2C      0xA000000
#define RNIC_REG_BASE_ADDR_MAC_0    0xB000000
#define RNIC_REG_BASE_ADDR_MAC_1    0xC000000



struct rnic_pdata {

    struct mac_pdata*   mac_pdata;

    int                 msi_irq_cnt;
    void __iomem *      pcie_bar_addr;
    int                 malloc_high_addr;
    int                 malloc_low_addr;
    int                 malloc_mem_len;
                     
    int                 tx_desc_base_addr;
    int                 rx_desc_base_addr;
    int                 tx_data_base_addr;
    int                 rx_data_base_addr;  
                     
    int                 mplla_en;
    int                 mpllb_en;

    int                 port_0_speed;
    int                 port_0_link_down_cnt;
    int                 mac_0_speed_mode;
    int                 pcs_0_type_sel;
    int                 pcs_0_speed_sel;
    int                 pcs_0_50g_mode;
    int                 pcs_0_mpllb_sel;
    int                 pcs_0_krt_en;
    int                 pcs_0_an_en;
    int                 pcs_0_rs_fec_en;
    int                 pcs_0_base_r_fec_en;
    int                 pcs_0_an_bp_1;
    int                 pcs_0_an_bp_2;
    int                 pcs_0_an_bp_3;
    int                 pcs_0_an_nxp_1_0;
    int                 pcs_0_an_nxp_2_0;
    int                 pcs_0_an_nxp_3_0;
    int                 pcs_0_an_nxp_1_1;
    int                 pcs_0_an_nxp_2_1;
    int                 pcs_0_an_nxp_3_1;
    int                 pcs_0_an_nxp_index;
    int                 pcs_0_krt_lane_mask;
    int                 pcs_0_krt_cl72_en;
    int                 pcs_0_krt_check;
    int                 pcs_0_speed_switch;
    int                 pcs_0_an_start;
    int                 pcs_0_an_rcv_cnt;
    int                 pcs_0_an_intr_check_cnt;
    int                 pcs_0_an_success;
    int                 pcs_0_an_restart_lock;
    int                 pcs_0_krt_start;
    int                 pcs_0_krt_success;
    int                 pcs_0_krt_failed;
    int                 pcs_0_link_up_only;

    int                 port_1_speed;
    int                 port_1_link_down_cnt;    
    int                 mac_1_speed_mode;
    int                 pcs_1_type_sel;
    int                 pcs_1_speed_sel;
    int                 pcs_1_50g_mode;
    int                 pcs_1_mpllb_sel;
    int                 pcs_1_krt_en;
    int                 pcs_1_an_en;
    int                 pcs_1_rs_fec_en;
    int                 pcs_1_base_r_fec_en;
    int                 pcs_1_an_bp_1;
    int                 pcs_1_an_bp_2;
    int                 pcs_1_an_bp_3;
    int                 pcs_1_an_nxp_1_0;
    int                 pcs_1_an_nxp_2_0;
    int                 pcs_1_an_nxp_3_0;
    int                 pcs_1_an_nxp_1_1;
    int                 pcs_1_an_nxp_2_1;
    int                 pcs_1_an_nxp_3_1;
    int                 pcs_1_an_nxp_index;
    int                 pcs_1_krt_lane_mask;
    int                 pcs_1_krt_cl72_en;
    int                 pcs_1_krt_check;
    int                 pcs_1_speed_switch;
    int                 pcs_1_an_start;
    int                 pcs_1_an_rcv_cnt;
    int                 pcs_1_an_intr_check_cnt;
    int                 pcs_1_an_success;
    int                 pcs_1_an_restart_lock;
    int                 pcs_1_krt_start;
    int                 pcs_1_krt_success;
    int                 pcs_1_krt_failed;
    int                 pcs_1_link_up_only;
    
    int                 pcs_an_nxp_1_null; //null page
    int                 pcs_an_nxp_2_null;
    int                 pcs_an_nxp_3_null; 
}____cacheline_aligned;



void                rnic_init       (struct rnic_pdata *);
void                reg_write       (struct rnic_pdata *,int,int);
int                 reg_read        (struct rnic_pdata *,int);
unsigned int        set_bits        (unsigned int,unsigned int,unsigned int,unsigned int);
unsigned int        get_bits        (unsigned int,unsigned int,unsigned int);
unsigned int        get_random_num  (void);

#ifdef RNIC_DEBUG
    #define RNIC_PRINTK(fmt, args...)       pr_alert("RNIC_DEBUG        [%s,%d]:" fmt, __func__, __LINE__, ## args)
#else
    #define RNIC_PRINTK(x...)               do { } while (0)
#endif


#ifdef RNIC_DEBUG_TRACE
    #define RNIC_TRACE_PRINT(fmt, args...)  pr_alert("RNIC_TRACE_PRINT: [%s,%d]" fmt, __func__, __LINE__, ## args)
#else
    #define RNIC_TRACE_PRINT(x...)          do { } while (0)
#endif


#ifdef RNIC_DEBUG_TRACE_DESC
    #define RNIC_TRACE_PRINT_DESC(fmt, args...) pr_alert("RNIC_DEBUG_TRACE_DESC: [%s,%d]" fmt, __func__,  __LINE__,## args)
#else
    #define RNIC_TRACE_PRINT_DESC(x...)     do { } while (0)
#endif


#endif
