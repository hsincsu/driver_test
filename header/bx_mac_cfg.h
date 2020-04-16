//insomnia@2019/12/11 11:41:05

#ifndef __BX_MAC_CFG_H_
#define __BX_MAC_CFG_H_
                                               
                                                
void    mac_init_cust                               (struct rnic_pdata*,int);
void    mac_reg_write                               (struct rnic_pdata*,int,int,int);
int     mac_reg_read                                (struct rnic_pdata*,int,int);
void    mac_mpb_channel_cfg                         (struct rnic_pdata*,int);
void    mac_eth_channel_cfg                         (struct rnic_pdata*,int,int);
void    mac_mpb_channel_mpb_l3_l4_filter_on         (struct rnic_pdata*,int);
void    mac_l3_l4_filter_cfg_reg_write              (struct rnic_pdata*,int,int,int);
int     mac_l3_l4_filter_cfg_reg_read               (struct rnic_pdata*,int,int);
void    mac_speed_cfg                               (struct rnic_pdata*,int);
void    mac_loopback_on                             (struct rnic_pdata*,int);
void    mac_jumbo_on                                (struct rnic_pdata*,int);
void    mac_alloc_rx_fifo                           (struct rnic_pdata*,int);
void    mac_loopback_off                            (struct rnic_pdata*,int);
void    mac_receive_crc_check_on                    (struct rnic_pdata*,int);
void    mac_receive_crc_check_off                   (struct rnic_pdata*,int);
void    mac_set_blen_for_pcie_mps_lmt               (struct rnic_pdata*,int);
void    mac_set_axi_osr_lmt                         (struct rnic_pdata*,int);
void    mac_ipc_off                                 (struct rnic_pdata*,int);
void    mac_ipc_on                                  (struct rnic_pdata*,int);
void    mac_source_addr_replace_on                  (struct rnic_pdata*,int);
void    mac_split_header_on                         (struct rnic_pdata*,int,int);
void    mac_set_dsl                                 (struct rnic_pdata*,int,int);
void    mac_split_header_off                        (struct rnic_pdata*,int,int);
void    mac_drop_tcpip_checksum_err_pkg_on          (struct rnic_pdata*,int,int);
void    mac_drop_tcpip_checksum_err_pkg_off         (struct rnic_pdata*,int,int);
void    mac_rxq_flow_control_cfg                    (struct rnic_pdata*,int,int);
void    mac_pfc_en                                  (struct rnic_pdata*,int);
void    mac_pfc_off                                 (struct rnic_pdata*,int);
void    mac_transmitter_enable                      (struct rnic_pdata*,int);
void    mac_transmitter_disable                     (struct rnic_pdata*,int);
void    mac_receiver_enable                         (struct rnic_pdata*,int);
void    mac_receiver_disable                        (struct rnic_pdata*,int);
void    mac_soft_reset                              (struct rnic_pdata*,int);
void    mac_recevie_all_en                          (struct rnic_pdata*,int);
void    mac_use_channel_0_only                      (struct rnic_pdata*,int);
void    mac_axi_cfg                                 (struct rnic_pdata*,int);
void    mac_dma_edma_cfg                            (struct rnic_pdata*,int);
void    mac_dma_intr_mode_cfg                       (struct rnic_pdata*,int);
void    mac_dma_promiscuous_mode_en                 (struct rnic_pdata*,int);
void    mac_dma_rx_int_watchdog_timer_cfg           (struct rnic_pdata*,int);
void    mac_enable_all_intr                         (struct rnic_pdata*,int);
void    mac_disable_all_intr                        (struct rnic_pdata*,int);
void    mac_clear_all_intr                          (struct rnic_pdata*,int);
void    mac_report_all_intr                         (struct rnic_pdata*,int);
void    mac_enable_mac_intr                         (struct rnic_pdata*,int);
void    mac_disable_mac_intr                        (struct rnic_pdata*,int);
void    mac_enable_mtl_intr                         (struct rnic_pdata*,int);
void    mac_disable_mtl_intr                        (struct rnic_pdata*,int);
void    mac_enable_dma_intr                         (struct rnic_pdata*,int);
void    mac_disable_dma_intr                        (struct rnic_pdata*,int);
void    mac_enable_dma_intr_tx                      (struct rnic_pdata*,int);
void    mac_disable_dma_intr_tx                     (struct rnic_pdata*,int);
void    mac_enable_dma_intr_ri_only                 (struct rnic_pdata*,int);
void    mac_enable_dma_riwt_intr                    (struct rnic_pdata*,int);
void    mac_disable_dma_riwt_intr                   (struct rnic_pdata*,int);
int     mac_get_link_status                         (struct rnic_pdata*,int);
void    mac_tsf_off                                 (struct rnic_pdata*,int);
void    mac_ttc_cfg                                 (struct rnic_pdata*,int);
void    mac_clear_dma_intr_tx                       (struct rnic_pdata*,int,int);
void    mac_clear_dma_rx_intr                       (struct rnic_pdata*,int,int);
void    mac_enable_dspw                             (struct rnic_pdata*,int);
void    mac_enable_tmrp                             (struct rnic_pdata*,int);
void    mac_enable_tdrp                             (struct rnic_pdata*,int);
void    mac_rwtu_cfg                                (struct rnic_pdata*,int);

void    mac_av_cfg                                  (struct rnic_pdata*,int);
void    mac_bandwidth_alloc                         (struct rnic_pdata*,int);


void    mac_report_status                           (struct rnic_pdata*,int);
void    mac_report_mtl_rxq_missed_pkt_overflow_cnt  (struct rnic_pdata*,int);
void    mac_report_dma_ch_miss_frame_cnt            (struct rnic_pdata*,int);
void    mac_report_tx_rx_status                     (struct rnic_pdata*,int);
void    mac_report_desc_status                      (struct rnic_pdata*,int);
void    mac_print_all_regs                          (struct rnic_pdata*,int);

#endif
