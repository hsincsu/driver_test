//insomnia@2019/12/15 17:01:37

#ifndef __BX_MPP_H_
#define __BX_MPP_H_


#define MPP_REG_ADDR_MAC_0_STATE_0              0x0000
#define MPP_REG_ADDR_MAC_0_STATE_1              0x0004
#define MPP_REG_ADDR_MAC_0_STATE_2              0x0008
#define MPP_REG_ADDR_MAC_1_STATE_0              0x000c
#define MPP_REG_ADDR_MAC_1_STATE_1              0x0010
#define MPP_REG_ADDR_MAC_1_STATE_2              0x0014
#define MPP_REG_ADDR_PCS_0_STATE                0x0018
#define MPP_REG_ADDR_PCS_1_STATE                0x001c
#define MPP_REG_ADDR_PHY_STATE                  0x0020

#define MPP_REG_ADDR_MPP_PARAM                  0x0024
#define MPP_REG_ADDR_MAC_0_PARAM                0x0028
#define MPP_REG_ADDR_MAC_1_PARAM                0x002c
#define MPP_REG_ADDR_PCS_0_PARAM                0x0030
#define MPP_REG_ADDR_PCS_1_PARAM                0x0034
#define MPP_REG_ADDR_PHY_PARAM                  0x0038

#define MPP_REG_ADDR_MPP_MASK_PKEY              0x0040
#define MPP_REG_ADDR_MPP_MASK                   0x0044

#define MPP_MASK_PKEY                           0x55667788

#define MPP_PARMA_RANGE_PORT_0_EN               0,0     
#define MPP_PARMA_RANGE_PORT_0_RST_N            1,1   
#define MPP_PARMA_RANGE_PORT_1_EN               2,2     
#define MPP_PARMA_RANGE_PORT_1_RST_N            3,3   
#define MPP_PARMA_RANGE_PHY_CTL_SEL             4,4
#define MPP_PARMA_RANGE_PHY_RST_MSK             5,5
#define MPP_PARMA_RANGE_PHY_FLYOVER_SEL         8,6
#define MPP_PARMA_RANGE_SRAM_RMC                13,9


#define MAC_PARAM_RANGE_SBD_FLOWCTRL            7,0
#define MAC_PARAM_RANGE_PTP_AUX_TS_TRIG         8,8
#define MAC_PARAM_RANGE_PWR_CLAMP_CTRL          9,9 
#define MAC_PARAM_RANGE_PWR_ISOLATE             10,10
#define MAC_PARAM_RANGE_PWR_DOWN_CTRL           11,11

#define PCS_PARAM_RANGE_XLGPCS_RX_LANE_EN       3,0
#define PCS_PARAM_RANGE_XLGPCS_TX_LANE_EN       7,4

#define PHY_PARAM_RANGE_NOMINAL_VP_SEL          1,0
#define PHY_PARAM_RANGE_NOMINAL_VPH_SEL         3,2
#define PHY_PARAM_RANGE_PCS_PWR_STABLE          4,4
#define PHY_PARAM_RANGE_PMA_PWR_STABLE          5,5
#define PHY_PARAM_RANGE_PG_MODE_EN              6,6
#define PHY_PARAM_RANGE_PG_RESET                7,7
#define PHY_PARAM_RANGE_MPLLA_FORCE_EN          8,8
#define PHY_PARAM_RANGE_MPLLB_FORCE_EN          9,9
#define PHY_PARAM_RANGE_SRAM_BYPASS             10,10                                         


#ifdef PORT_0_100G
    #define PORT_0_EN
#endif

#ifdef PORT_0_50G
    #ifndef PORT_0_EN
        #define PORT_0_EN
    #endif
#endif

#ifdef PORT_0_25G
    #ifndef PORT_0_EN
        #define PORT_0_EN
    #endif
#endif 

#ifdef PORT_0_40G
    #ifndef PORT_0_EN
        #define PORT_0_EN
    #endif
#endif 

#ifdef PORT_0_10G
    #ifndef PORT_0_EN
        #define PORT_0_EN
    #endif
#endif

#ifdef PORT_0_KRT_EN
    #ifndef PORT_0_EN
        #define PORT_0_EN
    #endif
    
    #define KRT_EN
#endif

#ifdef PORT_0_AN_EN
    #ifndef PORT_0_EN
        #define PORT_0_EN
    #endif
#endif

#ifdef PORT_1_50G
    #ifndef PORT_1_EN
        #define PORT_1_EN
    #endif
#endif

#ifdef PORT_1_25G
    #ifndef PORT_1_EN
        #define PORT_1_EN
    #endif
#endif 

#ifdef PORT_1_10G
    #ifndef PORT_1_EN
        #define PORT_1_EN
    #endif
#endif 

#ifdef PORT_1_KRT_EN
    #ifndef PORT_1_EN
        #define PORT_1_EN
    #endif
    #define KRT_EN
#endif

#ifdef PORT_1_AN_EN
    #ifndef PORT_1_EN
        #define PORT_1_EN
    #endif
#endif

#ifndef PORT_0_EN
    #ifndef PORT_1_EN
        #define PORT_0_EN
        #define PORT_0_100G
    #endif     
#endif



void    mpp_init                        (struct rnic_pdata*);
void    mpp_mask_set                    (struct rnic_pdata*,int);
void    mpp_mask_pkey_set               (struct rnic_pdata*);
void    mpp_mask_pkey_clear             (struct rnic_pdata*);
void    mpp_enable_port_mask            (struct rnic_pdata*,int);
void    mpp_phy_sram_bypass             (struct rnic_pdata*);
void    mpp_port_rst_n_set              (struct rnic_pdata*,int);
void    mpp_port_rst_n_release          (struct rnic_pdata*,int);
void    mpp_set_phy_pg_rst              (struct rnic_pdata*,int);
void    mpp_release_phy_pg_rst          (struct rnic_pdata*,int);
void    mpp_enable_port                 (struct rnic_pdata*,int);
void    mpp_disable_port                (struct rnic_pdata*,int);
void    mpp_phy_ctl_sel                 (struct rnic_pdata*,int);
void    mpp_reg_write                   (struct rnic_pdata*,int,int);
int     mpp_reg_read                    (struct rnic_pdata*,int);


#endif
