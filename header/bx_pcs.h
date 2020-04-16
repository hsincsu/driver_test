//insomnia@2019/12/17 9:15:12

#ifndef __BX_PCS_H_
#define __BX_PCS_H_

#define KRT_RETRY_CNT                           10

#define SR_PMA_KR_PMD_CTRL                      0x10096
#define SR_PMA_KR_PMD_STS                       0x10097
#define SR_PMA_KR_FEC_CTRL                      0x100AB
#define SR_PMA_RS_FEC_CTRL                      0x100C8
#define VR_PMA_DIG_CTRL1                        0x18000
#define VR_PMA_KRTR_TIMER_CTRL0                 0x18006
#define VR_PMA_KRTR_TIMER_CTRL1                 0x18007
#define VR_PMA_KRTR_TIMER_CTRL2                 0x18008
#define VR_PMA_SNPS_MP_25G_16G_TX_GENCTRL0      0x18030
#define VR_PMA_SNPS_MP_25G_TX_RATE_CTRL         0x18034
#define VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN0  0x18036
#define VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN0  0x18037
#define VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN1  0x18038
#define VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN1  0x18039
#define VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN2  0x1803A
#define VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN2  0x1803B
#define VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN3  0x1803C
#define VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN3  0x1803D
#define VR_PMA_SNPS_MP_25G_TX_GEN_CTRL5         0x18046
#define VR_PMA_SNPS_MP_25G_TX_DCC_CTRL          0x18047
#define VR_PMA_SNPS_MP_25G_16G_RX_GENCTRL1      0x18051
#define VR_PMA_SNPS_MP_25G_RX_GENCTRL3          0x18053
#define VR_PMA_SNPS_MP_25G_RX_RATE_CTRL         0x18054
#define VR_PMA_SNPS_MP_25G_RX_GENCTRL5          0x18056
#define VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0      0x18058
#define VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1      0x18059
#define VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2      0x1805a
#define VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3      0x1805b
#define VR_PMA_SNPS_MP_25G_RX_AFE_RATE_CTRL     0x1805d
#define VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL0    0x1805e
#define VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL1    0x1805f
#define VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL0     0x18065
#define VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL1     0x18066
#define VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL0      0x1806A
#define VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL1      0x1806B
#define VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL2      0x1806C
#define VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL3      0x1806D
#define VR_PMA_SNPS_MP_25G_16G_MPLL_CMN_CTRL    0x18070
#define VR_PMA_SNPS_MP_25G_MPLLA_CTRL0          0x18071
#define VR_PMA_SNPS_MP_25G_MPLLA_CTRL2          0x18073
#define VR_PMA_SNPS_MP_25G_MPLLB_CTRL0          0x18074
#define VR_PMA_SNPS_MP_25G_MPLLB_CTRL2          0x18076
#define VR_PMA_SNPS_MP_25G_MPLLA_CTRL3          0x18077
#define VR_PMA_SNPS_MP_25G_MPLLB_CTRL3          0x18078
#define VR_PMA_SNPS_MP_25G_MPLLA_CTRL4          0x18079
#define VR_PMA_SNPS_MP_25G_MPLLB_CTRL4          0x1807A
#define VR_PMA_SNPS_MP_25G_16G_MISC_CTRL0       0x18090
#define VR_PMA_SNPS_MP_25G_16G_REF_CLK_CTRL     0x18091
#define VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD0      0x18092
#define VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD1      0x18093
#define VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD2      0x18094
#define VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD3      0x18095
#define VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF0     0x18096
#define VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF1     0x18097
#define VR_PMA_SNPS_MP_25G_16G_SRAM             0x1809A
#define VR_PMA_SNPS_MP_25G_16G_CR_CTRL          0x180A0
#define VR_PMA_SNPS_MP_25G_16G_CR_ADDR          0x180A1
#define VR_PMA_SNPS_MP_25G_16G_CR_DATA          0x180A2
#define SR_PCS_CTRL1                            0x30000
#define SR_PCS_STS1                             0x30001
#define SR_PCS_CTRL2                            0x30007
#define SR_PCS_TP_CTRL                          0x3002A
#define VR_PCS_DIG_CTRL1                        0x38000
#define VR_PCS_DIG_CTRL2                        0x38001
#define VR_PCS_DIG_CTRL3                        0x38003
#define VR_PCS_AM_CNT                           0x38018
#define SR_AN_CTRL                              0x70000
#define SR_AN_STS                               0x70001
#define SR_AN_ADV1                              0x70010
#define SR_AN_ADV2                              0x70011
#define SR_AN_ADV3                              0x70012
#define SR_AN_LP_ABL1                           0x70013
#define SR_AN_LP_ABL2                           0x70014
#define SR_AN_LP_ABL3                           0x70015
#define SR_AN_XNP_TX1                           0x70016
#define SR_AN_XNP_TX2                           0x70017
#define SR_AN_XNP_TX3                           0x70018
#define SR_AN_LP_XNP_ABL1                       0x70019
#define SR_AN_LP_XNP_ABL2                       0x7001a
#define SR_AN_LP_XNP_ABL3                       0x7001b
#define SR_AN_COMP_STS                          0x70030
#define VR_AN_INTR_MSK                          0x78001
#define VR_AN_INTR                              0x78002
#define VR_AN_MODE_CTRL                         0x78003
#define VR_AN_TIMER_CTRL0                       0x78004
#define VR_AN_TIMER_CTRL1                       0x78005
#define SR_VSMMD_CTRL                           0x1e0009


#define RST                                     15,15
#define REF_CLK_DIV2                            2,2
#define REF_RANGE                               5,3
#define REF_CLK_MPLLB_DIV                       11,9
#define REF_CLK_MPLLA_DIV                       14,12
#define INIT_DONE                               0,0
#define EXT_LD_DONE                             1,1
#define MPLLA_MULTIPLIER                        11,0
#define MPLLB_MULTIPLIER                        11,0
#define MPLLA_DIV_MULT                          7,0
#define MPLLB_DIV_MULT                          7,0
#define MPLLA_TX_CLK_DIV                        14,12
#define MPLLB_TX_CLK_DIV                        14,12
#define MPLLA_LOW_BW                            15,0
#define MPLLB_LOW_BW                            15,0
#define MPLLA_HIGH_BW                           15,0
#define MPLLB_HIGH_BW                           15,0
#define VCO_REF_LD_0                            6,0
#define VCO_REF_LD_1                            14,8
#define VCO_REF_LD_2                            6,0
#define VCO_REF_LD_3                            14,8
#define VCO_LD_VAL_0                            12,0
#define VCO_LD_VAL_1                            12,0
#define VCO_LD_VAL_2                            12,0
#define VCO_LD_VAL_3                            12,0
#define RX0_CDR_PPM_MAX                         4,0
#define RX1_CDR_PPM_MAX                         12,8
#define RX2_CDR_PPM_MAX                         4,0
#define RX3_CDR_PPM_MAX                         12,8
#define MPLLB_SEL                               7,4
#define TX_ALIGN_WD_XFER_0                      0,0
#define TX_ALIGN_WD_XFER_1                      1,1
#define TX_ALIGN_WD_XFER_2                      2,2
#define TX_ALIGN_WD_XFER_3                      3,3
#define EQ_AFE_RT_0                             2,0
#define EQ_AFE_RT_1                             6,4
#define EQ_AFE_RT_2                             10,8
#define EQ_AFE_RT_3                             14,12
#define RX0_RATE                                2,0
#define RX1_RATE                                6,4
#define RX2_RATE                                10,8
#define RX3_RATE                                14,12
#define TX0_RATE                                2,0
#define TX1_RATE                                6,4
#define TX2_RATE                                10,8
#define TX3_RATE                                14,12
#define RX0_DELTA_IQ                            11,8
#define RX1_DELTA_IQ                            11,8
#define RX2_DELTA_IQ                            11,8
#define RX3_DELTA_IQ                            11,8
#define MPLLA_CAL_DISABLE                       15,15
#define MPLLB_CAL_DISABLE                       15,15
#define TX_EQ_PRE                               5,0
#define TX_EQ_MAIN                              13,8
#define TX_EQ_POST                              5,0
#define TX_EQ_OVER_RIDE                         6,6
#define CTLE_BOOST_0                            4,0
#define CTLE_BOOST_1                            4,0
#define CTLE_BOOST_2                            4,0
#define CTLE_BOOST_3                            4,0
#define CTLE_POLE_0                             7,5
#define CTLE_POLE_1                             7,5
#define CTLE_POLE_2                             7,5
#define CTLE_POLE_3                             7,5
#define DFE_TAP1_0                              7,0
#define DFE_TAP1_1                              7,0
#define DFE_TAP1_2                              7,0
#define DFE_TAP1_3                              7,0
#define HF_EN_0                                 12,12
#define HF_EN_3_1                               15,13
#define LF_TH_0                                 2,0
#define LF_TH_1                                 5,3
#define LF_TH_2                                 8,6
#define LF_TH_3                                 11,9
#define RLU                                     2,2
#define VR_RST                                  15,15
#define TX_LANE_DIS                             15,14
#define RX_LANE_DIS                             11,10
#define AN_PG_RCV                               2,2
#define AN_INC_LINK                             1,1
#define AN_INT_CMPL                             0,0
#define R2TLBE                                  14,14
#define PDET_EN                                 0,0
#define MSK_PHY_RST                             8,8
#define PCS_AN_RST                              15,15
#define EXT_NP_CTL                              13,13
#define RX2TX_LB_EN                             7,4
#define TX2RX_LB_EN                             3,0

#define PCS_SPEED_SEL                           5,2    //4:2?
#define PCS_TYPE_SEL                            3,0    //2:0?


#define AN_TIMER_VAL                            0x8
#define KRT_TIMER_VAL                           0xffff

#ifdef KRT_EN
    #define TX_EQ_OVER_RIDE_VAL                 0x0
#else
    #define TX_EQ_OVER_RIDE_VAL                 0x1
#endif   

#define REF_RANGE_VAL                           0x3
#define REF_CLK_DIV2_VAL                        0x0 

//MPLLB                                         
#define REF_MPLLB_DIV_VAL                       0x0
#define MPLLB_VCO_REF_LD_0_VAL                  0x9
#define MPLLB_VCO_REF_LD_1_VAL                  0x9
#define MPLLB_VCO_REF_LD_2_VAL                  0x9
#define MPLLB_VCO_REF_LD_3_VAL                  0x9
#define MPLLB_VCO_LD_VAL_0_VAL                  0x5CD
#define MPLLB_VCO_LD_VAL_1_VAL                  0x5CD
#define MPLLB_VCO_LD_VAL_2_VAL                  0x5CD
#define MPLLB_VCO_LD_VAL_3_VAL                  0x5CD
#define MPLLB_RX0_CDR_PPM_MAX_VAL               0x14
#define MPLLB_RX1_CDR_PPM_MAX_VAL               0x14
#define MPLLB_RX2_CDR_PPM_MAX_VAL               0x14
#define MPLLB_RX3_CDR_PPM_MAX_VAL               0x14
//MPLLA                                         
#define REF_MPLLA_DIV_VAL                       0x0
#define MPLLA_VCO_REF_LD_0_VAL                  0x15
#define MPLLA_VCO_REF_LD_1_VAL                  0x15
#define MPLLA_VCO_REF_LD_2_VAL                  0x15
#define MPLLA_VCO_REF_LD_3_VAL                  0x15
#define MPLLA_VCO_LD_VAL_0_VAL                  0x5AC
#define MPLLA_VCO_LD_VAL_1_VAL                  0x5AC
#define MPLLA_VCO_LD_VAL_2_VAL                  0x5AC
#define MPLLA_VCO_LD_VAL_3_VAL                  0x5AC
#define MPLLA_RX0_CDR_PPM_MAX_VAL               0x14
#define MPLLA_RX1_CDR_PPM_MAX_VAL               0x14
#define MPLLA_RX2_CDR_PPM_MAX_VAL               0x14
#define MPLLA_RX3_CDR_PPM_MAX_VAL               0x14

void    pcs_init                                (struct rnic_pdata*,int);
void    pcs_reg_write                           (struct rnic_pdata*,int,int,int);
int     pcs_reg_read                            (struct rnic_pdata*,int,int);        
void    pcs_wait_sram_init_done                 (struct rnic_pdata*,int);
void    pcs_sram_ext_load_done                  (struct rnic_pdata*,int);
void    pcs_wait_soft_reset_clear               (struct rnic_pdata*,int);
int     pcs_get_rlu                             (struct rnic_pdata*,int);
void    pcs_wait_rlu                            (struct rnic_pdata*,int);
void    pcs_speed_switch                        (struct rnic_pdata*,int);
void    pcs_speed_set                           (struct rnic_pdata*,int);
void    pcs_speed_common_cfg                    (struct rnic_pdata*,int);
void    pcs_sram_cfg                            (struct rnic_pdata*,int);
void    pcs_type_cfg                            (struct rnic_pdata*,int);
void    pcs_pcs_speed_set                       (struct rnic_pdata*,int);
void    pcs_50g_mode_cfg                        (struct rnic_pdata*,int);
void    pcs_set_vr_reset                        (struct rnic_pdata*,int);
void    pcs_wait_vr_reset_clear                 (struct rnic_pdata*,int);
void    pcs_per_lane_cfg                        (struct rnic_pdata*,int);
void    pcs_mpll_cfg                            (struct rnic_pdata*,int);
void    pcs_mplla_cfg                           (struct rnic_pdata*,int);
void    pcs_per_lane_cfg_10g                    (struct rnic_pdata*,int);
void    pcs_mpllb_cfg                           (struct rnic_pdata*,int);
void    pcs_mpll_en_cfg                         (struct rnic_pdata*,int);
void    pcs_per_lane_cfg_25g                    (struct rnic_pdata*,int);
void    pcs_tx_eq_cfg                           (struct rnic_pdata*,int);
void    pcs_rs_fec_cfg                          (struct rnic_pdata*,int);
void    pcs_base_r_fec_cfg                      (struct rnic_pdata*,int);
void    pcs_cr_wr_phy                           (struct rnic_pdata*,int,int,int);
int     pcs_cr_rd_phy                           (struct rnic_pdata*,int,int);
void    pcs_r2t_lb_en                           (struct rnic_pdata*,int);
void    pcs_loopback_cfg                        (struct rnic_pdata*,int);
void    pcs_phy_r2t_lb_en_pcs                   (struct rnic_pdata*,int);
void    pcs_phy_t2r_lb_en_pcs                   (struct rnic_pdata*,int);
void    pcs_phy_r2t_lb_en                       (struct rnic_pdata*,int);
void    pcs_phy_t2r_lb_en                       (struct rnic_pdata*,int);
void    pcs_tx_invert_en                        (struct rnic_pdata*,int);
void    pcs_rx_invert_en                        (struct rnic_pdata*,int);
void    pcs_tx_rx_invert_en                     (struct rnic_pdata*,int);
void    pcs_enable_prbs31                       (struct rnic_pdata*,int);
void    pcs_disable_prbs31                      (struct rnic_pdata*,int);
void    pcs_enable_idle_pattern                 (struct rnic_pdata*,int);
void    pcs_disable_idle_pattern                (struct rnic_pdata*,int);
void    pcs_msk_phy_rst                         (struct rnic_pdata*,int);
void    pcs_set_am_cnt                          (struct rnic_pdata*,int);

//AN                                            
void    pcs_an_enable_intr                      (struct rnic_pdata*,int);
int     pcs_an_get_intr                         (struct rnic_pdata*,int);
void    pcs_an_disable                          (struct rnic_pdata*,int);
void    pcs_an_enable                           (struct rnic_pdata*,int);
void    pcs_an_cfg                              (struct rnic_pdata*,int);
void    pcs_an_rst                              (struct rnic_pdata*,int);
void    pcs_an_start                            (struct rnic_pdata*,int);
void    pcs_an_pdet_cfg                         (struct rnic_pdata*,int);
void    pcs_an_base_page_cfg                    (struct rnic_pdata*,int);
void    pcs_an_wait_sts                         (struct rnic_pdata*,int);
void    pcs_an_pg_rcv                           (struct rnic_pdata*,int);
void    pcs_an_clear_intr                       (struct rnic_pdata*,int,int,int);    
void    pcs_an_clear_all_intr                   (struct rnic_pdata*,int);  
int     pcs_an_read_lp_base_page                (struct rnic_pdata*,int);
int     pcs_an_read_lp_next_page                (struct rnic_pdata*,int);
int     pcs_an_read_adv_base_page               (struct rnic_pdata*,int);
int     pcs_an_read_adv_next_page               (struct rnic_pdata*,int);
void    pcs_an_inc_link                         (struct rnic_pdata*,int);
void    pcs_an_int_cmpl                         (struct rnic_pdata*,int);
void    pcs_an_nxp_check                        (struct rnic_pdata*,int);
void    pcs_an_base_page_display                (struct rnic_pdata*,int,char*,int*);
void    pcs_an_next_page_display                (struct rnic_pdata*,int,char*,int*); 
int     pcs_an_check_comp_state                 (struct rnic_pdata*,int);
int     pcs_an_print_state                      (struct rnic_pdata*,int);
int     pcs_an_comp                             (struct rnic_pdata*,int);
void    pcs_an_ext_np_ctl_en                    (struct rnic_pdata*,int);
void    pcs_an_timeout                          (struct rnic_pdata*,int);
                                                
//KRT
void    pcs_krt_start                           (struct rnic_pdata*,int);
void    pcs_krt_cfg                             (struct rnic_pdata*,int);
void    pcs_krt_enable                          (struct rnic_pdata*,int);
void    pcs_krt_disable                         (struct rnic_pdata*,int);
void    pcs_krt_restart                         (struct rnic_pdata*,int);
void    pcs_krt_cl72_enable                     (struct rnic_pdata*,int);
void    pcs_krt_check_state                     (struct rnic_pdata*,int);


#endif
