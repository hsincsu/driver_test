// insomnia@2019/12/15 15:18:25

#include "header/bx_rnic.h"

void mac_init_cust(struct rnic_pdata*rnic_pdata,int mac_id)
{
    RNIC_PRINTK("RNIC: mac %d reg init start\n",mac_id);
    mac_soft_reset(rnic_pdata,mac_id);
    
    //mac_transmitter_disable(rnic_pdata,mac_id);
    //mac_receiver_disable(rnic_pdata,mac_id);
    mac_reg_write(rnic_pdata,mac_id,0x0000,0x30110001);   // MAC_Tx_Configuration [CS:0 | SS:011 | SARC:01 | JD:1 | Tx enable:1]
    mac_reg_write(rnic_pdata,mac_id,0x0004,0x00000005);   // MAC_Rx_Configuration [LoopBack:0 | DCRCC:1 | Rx enable:1 | ACS:1]
    mac_reg_write(rnic_pdata,mac_id,0x0008,0x00000080);   // MAC_Packet_Filter    [Receive all:0 | DNTU:0 | PCF:10 | Promiscuous Mode:0]
    mac_reg_write(rnic_pdata,mac_id,0x0140,0x00000802);   // MAC_RxQ_Enable_Ctrl0 RXQ0EN:2'b10(Queue 0 and 5 enabled for DCB)
    mac_reg_write(rnic_pdata,mac_id,0x0150,0x00000000);   // UPQ:0
    mac_reg_write(rnic_pdata,mac_id,0x0300,0x80005a5a);   // MAC_Address_0_High [AE:1 DCS:0]
    mac_reg_write(rnic_pdata,mac_id,0x0304,0x5a5a5a5a);   // MAC_Address_0_Low
    mac_reg_write(rnic_pdata,mac_id,0x1000,0x00000000);   // MTL_Operation_Mode [ETS Algorithm:00(WRR algorithm) | Receive Arbitration Algorithm:0(SP)]
    mac_reg_write(rnic_pdata,mac_id,0x1030,0x03020180);   // MTL_RxQ_DMA_Map0 [queue 0 dynamic,queue 1 -> channel 1 ...]
    mac_reg_write(rnic_pdata,mac_id,0x1034,0x07068004);   // MTL_RxQ_DMA_Map1 [queue 5 dynamic]
    mac_reg_write(rnic_pdata,mac_id,0x1038,0x0b0a0908);   // MTL_RxQ_DMA_Map2
    mac_reg_write(rnic_pdata,mac_id,0x1040,0x00000000);   // MTL_TC_Prty_Map0 [PSTC0:priority 0]
    mac_reg_write(rnic_pdata,mac_id,0x1044,0x00002000);   // MTL_TC_Prty_Map1 [PSTC5:priority 5]
    mac_reg_write(rnic_pdata,mac_id,0x3000,0x00000000);   // DMA mode [Software Reset:0 | INTR_MODE:00 | TDRP:0]
    mac_reg_write(rnic_pdata,mac_id,0x3004,0x0f0f08ff);   // DMA_SysBus_Mode[WR_OSR_LMT:0f | RD_OSR_LMT:0f |BLEN:all | UNDEF:1]
    mac_reg_write(rnic_pdata,mac_id,0x3010,0x000a0a0a);   // AXI_Tx_AR_ACE_Control [THC:4'b1010 | TEC:4'b1010 | TDRC:4'b1010]
    mac_reg_write(rnic_pdata,mac_id,0x3018,0x06060606);   // AXI_Rx_AW_ACE_Control [RDC:4'b0110 RHC:4'b0110 | RPC:4'b0110 | RDWC:4'b0110]
    mac_reg_write(rnic_pdata,mac_id,0x301c,0x00000a06);   // AXI_TxRx_AWAR_ACE_Control [RDRC:4'b1010 | TDWC:4'b0110]
    mac_reg_write(rnic_pdata,mac_id,0x3040,0x00000001);   // Tx Descriptor Pre-fetch threshold Size:0(0 descriptors)
    mac_reg_write(rnic_pdata,mac_id,0x3044,0x00000001);   // Rx Descriptor Pre-fetch threshold Size:0(0 descriptors)

    mac_ipc_on(rnic_pdata,mac_id);
    mac_recevie_all_en(rnic_pdata,mac_id);
    mac_receive_crc_check_on(rnic_pdata,mac_id);
    mac_source_addr_replace_on(rnic_pdata,mac_id);
    mac_speed_cfg(rnic_pdata,mac_id);
    
    #ifndef MAC_USE_CHANNEL_0_ONLY    
        mac_use_channel_0_only(rnic_pdata,mac_id);
    #endif
    #ifdef MPB_BLEN_TEST
        mac_set_blen_for_pcie_mps_lmt(rnic_pdata,mac_id);
    #endif    
         
    #ifdef MAC_PFC_EN
        mac_pfc_en(rnic_pdata,mac_id); 
    #endif
           
    #ifdef MAC_LOOPBACK_EN
        mac_loopback_on(rnic_pdata,mac_id);
    #endif           
    
    #ifdef JUMBO_EN
        mac_jumbo_on(rnic_pdata,mac_id);
    #endif
    
    #ifdef MPB_SIM_USE_MAC_0
        RNIC_PRINTK("\tRNIC: mpb_sim_use_mac_0\n",mac_id);
        if(mac_id == 0)
            mac_mpb_channel_cfg(rnic_pdata,mac_id);
    #endif

    #ifdef MPB_SIM_USE_MAC_1
        RNIC_PRINTK("\tRNIC: mpb_sim_use_mac_1\n",mac_id);
        if(mac_id == 1)
            mac_mpb_channel_cfg(rnic_pdata,mac_id);
    #endif

    #ifdef ETH_SIM
        mac_eth_channel_cfg(rnic_pdata,mac_id,0);
    #endif

    //mac_transmitter_enable(rnic_pdata,mac_id);
    //mac_receiver_enable(rnic_pdata,mac_id);
        
    RNIC_PRINTK("RNIC: mac %d reg init done\n",mac_id);
}


void mac_mpb_channel_cfg (struct rnic_pdata*rnic_pdata,int mac_id)
{  
    int channel_base_addr;
    int mpb_base_addr_h;   
    
    if(mac_id == 0x0)
        mpb_base_addr_h = RNIC_BASE_ADDR_MPB_DATA_S_0_H;
    else
        mpb_base_addr_h = RNIC_BASE_ADDR_MPB_DATA_S_1_H;    

    channel_base_addr = 0x80*MAC_DMA_CHANNEL_ID_FOR_MPB;

    mac_reg_write(rnic_pdata,mac_id,0x1100+channel_base_addr,0x003f050a);                 // MTL_TxQ(#i)_Operation_Mode [TQS:7f(16KB) | Q2TCMAP:101(TC5) | TTC£º000(64) | TXQEN:10(DCB/Generic) | TSF:1 | FTQ:0]
    mac_reg_write(rnic_pdata,mac_id,0x1118+channel_base_addr,0x0000000a);                 // MTL_TC(#i)_Quantum_Weight
    mac_reg_write(rnic_pdata,mac_id,0x1140+channel_base_addr,0x00ff00b0);                 // MTL_RxQ(#i)_Operation_Mode[RQS:ff(64KB) | EHFC:1 | RSF:1 | FEF:1]
    mac_reg_write(rnic_pdata,mac_id,0x1170+channel_base_addr,0x00010000);                 // TL_Q(#i)_Interrupt_Enable [Receive Queue Overflow Interrupt Enable:1]
    mac_reg_write(rnic_pdata,mac_id,0x3100+channel_base_addr,0x00010000);                 // DMA_CH(#i)_Control [SPH:0 DSL:0 PBLx8:1]

    mac_reg_write(rnic_pdata,mac_id,0x3130+channel_base_addr,0x000003ff);                 // DMA_CH(#i)_TxDesc_Ring_Length:  (0x0080*i)+0x3130 [Transmit Descriptor Ring Length:1024]
    mac_reg_write(rnic_pdata,mac_id,0x3134+channel_base_addr,0x000003ff);                 // DMA_CH(#i)_RxDesc_Ring_Length:  (0x0080*i)+0x3134 [Receive Descriptor Ring Length :1024]

	mac_reg_write(rnic_pdata,mac_id,0x3110+channel_base_addr,0x00000000+mpb_base_addr_h); // DMA_CH(#i)_TxDesc_List_HAddress:(0x0080*i)+0x3110
    mac_reg_write(rnic_pdata,mac_id,0x3114+channel_base_addr,0x00000000);                 // DMA_CH(#i)_TxDesc_List_LAddress:(0x0080*i)+0x3114
   
	mac_reg_write(rnic_pdata,mac_id,0x3124+channel_base_addr,0x00000000);                 // DMA_CH(#i)_TxDesc_Tail_LPointer:(0x0080*i)+0x3124
	mac_reg_write(rnic_pdata,mac_id,0x3120+channel_base_addr,0x00000000+mpb_base_addr_h); // DMA_CH(#i)_TxDesc_Tail_HPointer:(0x0080*i)+0x3120
    
	mac_reg_write(rnic_pdata,mac_id,0x3118+channel_base_addr,0x00000001+mpb_base_addr_h); // DMA_CH(#i)_RxDesc_List_HAddress:(0x0080*i)+0x3118
    mac_reg_write(rnic_pdata,mac_id,0x311c+channel_base_addr,0x00000000);                 // DMA_CH(#i)_RxDesc_List_LAddress:(0x0080*i)+0x311c

    mac_reg_write(rnic_pdata,mac_id,0x312c+channel_base_addr,0x00000000);                 // DMA_CH(#i)_RxDesc_Tail_LPointer:(0x0080*i)+0x312c	
	mac_reg_write(rnic_pdata,mac_id,0x3128+channel_base_addr,0x00000001+mpb_base_addr_h); // DMA_CH(#i)_RxDesc_Tail_HPointer:(0x0080*i)+0x3128

	mac_reg_write(rnic_pdata,mac_id,0x3138+channel_base_addr,0x0000c0c5);                 // DMA_CH(#i)_Interrupt_Enable[Normal Interrupt Summary Enable:1 | Abnormal Interrupt Summary Enable.: |]
            
    mac_split_header_off(rnic_pdata,mac_id,channel_base_addr);
    mac_drop_tcpip_checksum_err_pkg_off(rnic_pdata,mac_id,channel_base_addr);
    
    #ifdef MAC_PFC_EN
        mac_rxq_flow_control_cfg(rnic_pdata,mac_id,channel_base_addr); 
    #endif
    
    mac_mpb_channel_mpb_l3_l4_filter_on(rnic_pdata,mac_id);

    mac_reg_write(rnic_pdata,mac_id,0x3104+channel_base_addr,0x00200001);                 // DMA_CH(#i)_TX_Control [TxPBL:32x8 | Start Transmission]
    mac_reg_write(rnic_pdata,mac_id,0x3108+channel_base_addr,0x00207fe1);                 // DMA_CH(#i)_RX_Control [RxPBL:32x8 | receive buffer size:16368 Bytes | Start Receive]

	
	/*added by hs for print info*/
	u32 regval;
	regval = mac_reg_read(rnic_pdata,mac_id,0x1100+channel_base_addr);                 // MTL_TxQ(#i)_Operation_Mode [TQS:7f(16KB) | Q2TCMAP:101(TC5) | TTC£º000(64) | TXQEN:10(DCB/Generic) | TSF:1 | FTQ:0]
    printk("0x1100+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x1118+channel_base_addr);                 // MTL_TC(#i)_Quantum_Weight
    printk("0x1118+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x1140+channel_base_addr);                 // MTL_RxQ(#i)_Operation_Mode[RQS:ff(64KB) | EHFC:1 | RSF:1 | FEF:1]
    printk("0x1140+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x1170+channel_base_addr);                 // TL_Q(#i)_Interrupt_Enable [Receive Queue Overflow Interrupt Enable:1]
    printk("0x1170+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3100+channel_base_addr);                 // DMA_CH(#i)_Control [SPH:0 DSL:0 PBLx8:1]
    printk("0x3100+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3138+channel_base_addr);                 // DMA_CH(#i)_Interrupt_Enable[Normal Interrupt Summary Enable:1 | Abnormal Interrupt Summary Enable.: |]
    printk("0x3138+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3110+channel_base_addr); // DMA_CH(#i)_TxDesc_List_HAddress:(0x0080*i)+0x3110
    printk("0x3110+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3114+channel_base_addr);                 // DMA_CH(#i)_TxDesc_List_LAddress:(0x0080*i)+0x3114
    printk("0x3114+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3118+channel_base_addr); // DMA_CH(#i)_RxDesc_List_HAddress:(0x0080*i)+0x3118
    printk("0x3118+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x311c+channel_base_addr);                 // DMA_CH(#i)_RxDesc_List_LAddress:(0x0080*i)+0x311c
    printk("0x311c+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3120+channel_base_addr); // DMA_CH(#i)_TxDesc_Tail_HPointer:(0x0080*i)+0x3120
    printk("TXHP:0x3120+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3124+channel_base_addr);                 // DMA_CH(#i)_TxDesc_Tail_LPointer:(0x0080*i)+0x3124
    printk("TXLP:0x3124+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3128+channel_base_addr); // DMA_CH(#i)_RxDesc_Tail_HPointer:(0x0080*i)+0x3128
    printk("RXHP:0x3128+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x312c+channel_base_addr);                 // DMA_CH(#i)_RxDesc_Tail_LPointer:(0x0080*i)+0x312c
    printk("RXLP:0x312c+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3130+channel_base_addr);                 // DMA_CH(#i)_TxDesc_Ring_Length:  (0x0080*i)+0x3130 [Transmit Descriptor Ring Length:1024]
    printk("0x3130+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3134+channel_base_addr);                 // DMA_CH(#i)_RxDesc_Ring_Length:  (0x0080*i)+0x3134 [Receive Descriptor Ring Length :1024]
	printk("0x3134+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x0c00);
	printk("0xc00: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x0c04);
	printk("0xc04: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3104+channel_base_addr);                 // DMA_CH(#i)_TX_Control [TxPBL:32x8 | Start Transmission]
    printk("0x3104+channel_base_addr: %x\n",regval);
	regval = mac_reg_read(rnic_pdata,mac_id,0x3108+channel_base_addr); 
	printk("0x3108+channel_base_addr: %x\n",regval);
}
EXPORT_SYMBOL(mac_mpb_channel_cfg);

void mac_eth_channel_cfg (struct rnic_pdata*rnic_pdata,int mac_id,int channel_id)
{
    int channel_base_addr;

    RNIC_PRINTK("\tRNIC: mac %d mac_eth_channel_cfg, using channel %d\n",mac_id,channel_id);    
   
    channel_base_addr = 0x80*channel_id;

    mac_reg_write(rnic_pdata,mac_id,0x1100+channel_base_addr,0x00ff000a);                          // MTL_TxQ(#i)_Operation_Mode [TQS:ff(64KB) | Q2TCMAP:000(TC0) | TTC£º000(64) | TXQEN:10(DCB/Generic) | TSF:1 | FTQ:0]
    mac_reg_write(rnic_pdata,mac_id,0x1118+channel_base_addr,0x0000000a);                          // MTL_TC(#i)_Quantum_Weight
    mac_reg_write(rnic_pdata,mac_id,0x1140+channel_base_addr,0x00ff00b0);                          // MTL_RxQ(#i)_Operation_Mode[RQS:ff(64KB) | EHFC:1 | RSF:1 | FEF:1]
    mac_reg_write(rnic_pdata,mac_id,0x1170+channel_base_addr,0x00010000);                          // MTL_Q(#i)_Interrupt_Enable [Receive Queue Overflow Interrupt Enable:1]
    mac_reg_write(rnic_pdata,mac_id,0x3100+channel_base_addr,0x00010000);                          // DMA_CH(#i)_Control [SPH:0 DSL:0 PBLx8:1]
    mac_reg_write(rnic_pdata,mac_id,0x3138+channel_base_addr,0x0000ffff);                          // DMA_CH(#i)_Interrupt_Enable[Enable all the Interrupt of TX and RX]
    mac_reg_write(rnic_pdata,mac_id,0x3110+channel_base_addr,rnic_pdata->malloc_high_addr);          // DMA_CH(#i)_TxDesc_List_HAddress:(0x0080*i)+0x3110
    mac_reg_write(rnic_pdata,mac_id,0x3114+channel_base_addr,rnic_pdata->tx_desc_base_addr);         // DMA_CH(#i)_TxDesc_List_LAddress:(0x0080*i)+0x3114
    mac_reg_write(rnic_pdata,mac_id,0x3118+channel_base_addr,rnic_pdata->malloc_high_addr);          // DMA_CH(#i)_RxDesc_List_HAddress:(0x0080*i)+0x3118
    mac_reg_write(rnic_pdata,mac_id,0x311c+channel_base_addr,rnic_pdata->rx_desc_base_addr);         // DMA_CH(#i)_RxDesc_List_LAddress:(0x0080*i)+0x311c
    mac_reg_write(rnic_pdata,mac_id,0x3120+channel_base_addr,rnic_pdata->malloc_high_addr);          // DMA_CH(#i)_TxDesc_Tail_HPointer:(0x0080*i)+0x3120
    mac_reg_write(rnic_pdata,mac_id,0x3124+channel_base_addr,rnic_pdata->tx_desc_base_addr);         // DMA_CH(#i)_TxDesc_Tail_LPointer:(0x0080*i)+0x3124
    mac_reg_write(rnic_pdata,mac_id,0x3128+channel_base_addr,rnic_pdata->malloc_high_addr);          // DMA_CH(#i)_RxDesc_Tail_HPointer:(0x0080*i)+0x3128
    mac_reg_write(rnic_pdata,mac_id,0x312c+channel_base_addr,rnic_pdata->rx_desc_base_addr);         // DMA_CH(#i)_RxDesc_Tail_LPointer:(0x0080*i)+0x312c
    mac_reg_write(rnic_pdata,mac_id,0x3130+channel_base_addr,0x000003ff);                          // DMA_CH(#i)_TxDesc_Ring_Length:  (0x0080*i)+0x3130 [Transmit Descriptor Ring Length:1024]
    mac_reg_write(rnic_pdata,mac_id,0x3134+channel_base_addr,0x000003ff);                          // DMA_CH(#i)_RxDesc_Ring_Length:  (0x0080*i)+0x3134 [Receive Descriptor Ring Length :1024]

    mac_set_blen_for_pcie_mps_lmt(rnic_pdata,mac_id); 
    mac_split_header_off(rnic_pdata,mac_id,channel_base_addr);                
    mac_drop_tcpip_checksum_err_pkg_off(rnic_pdata,mac_id,channel_base_addr);
    
    #ifdef MAC_PFC_EN
        mac_rxq_flow_control_cfg(rnic_pdata,mac_id,channel_base_addr); 
    #endif
    
    #ifdef PERF_XLGMAC_SIM
        mac_set_max_perf(rnic_pdata,mac_id,channel_base_addr); 
    #endif
           
    mac_reg_write(rnic_pdata,mac_id,0x3104+channel_base_addr,0x00200001);                             // DMA_CH(#i)_TX_Control [TxPBL:32x8 | Start Transmission]
    mac_reg_write(rnic_pdata,mac_id,0x3108+channel_base_addr,0x00207fe1);                             // DMA_CH(#i)_RX_Control [RxPBL:32x8 | receive buffer size:16368 Bytes | Start Receive]        
}


void mac_mpb_channel_mpb_l3_l4_filter_on (struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
     
    RNIC_PRINTK("\tRNIC: mac %d mac_mpb_channel_mpb_l3_l4_filter_on\n",mac_id);
    // write the MAC_L3_L4_Control_0
    data = mac_l3_l4_filter_cfg_reg_read(rnic_pdata,mac_id,0x0);
    printk("mac_l3l4_0_step1:0x%x \n",data);//added by hs

    data = set_bits(data,31,31,1);                                                  // DMA Channel Select Enable                        
    data = set_bits(data,27,24,MAC_DMA_CHANNEL_ID_FOR_MPB);                         // DMA Channel Number 
    //data = set_bits(data,21,21,1);                                                // Layer 4 Destination Port Inverse Match Enable.       
    data = set_bits(data,20,20,1);                                                  // Layer 4 Destination Port Match Enable.   
    data = set_bits(data,16,16,1);                                                  // Layer 4 Protocol Enable:UDP          
    
	printk("mac_l3l4_0_step2:0x%x \n",data);//added by hs                                    
    mac_l3_l4_filter_cfg_reg_write(rnic_pdata,mac_id,0x0,data);
    
    data = mac_l3_l4_filter_cfg_reg_read(rnic_pdata,mac_id,0x0);//added by hs
	printk("mac_l3l4_0_step3:0x%x \n",data);//added by hs
	// write the MAC_Layer4_Address_0
    mac_l3_l4_filter_cfg_reg_write(rnic_pdata,mac_id,0x1,4791<<16);

	data = mac_l3_l4_filter_cfg_reg_read(rnic_pdata,mac_id,0x0);//added by hs
	printk("mac_l3l4_0:0x%x \n",data);//added by hs
	data = mac_l3_l4_filter_cfg_reg_read(rnic_pdata,mac_id,0x1);//added by hs
	printk("mac_l3l4_1:0x%x \n",data);//added by hs
}


void mac_l3_l4_filter_cfg_reg_write(struct rnic_pdata*rnic_pdata,int mac_id,int addr,int wdata)
{
    int data;
    
    data = mac_reg_read(rnic_pdata,mac_id,0x0c00);
    while(get_bits(data,0,0) == 1)
        data = mac_reg_read(rnic_pdata,mac_id,0x0c00);
        
    mac_reg_write(rnic_pdata,mac_id,0x0c04,wdata); // del by hs@20200427
    
    data = mac_reg_read(rnic_pdata,mac_id,0x0c00); //del by hs@20200427
    while(get_bits(data,0,0) == 1)
        data = mac_reg_read(rnic_pdata,mac_id,0x0c00); // del by hs@20200427
    
    data = set_bits(data,15,8,addr);  //Layer4_Address
    data = set_bits(data,1,1,0);      //write
    data = set_bits(data,0,0,1);      //start write

    mac_reg_write(rnic_pdata,mac_id,0x0c00,data);

	//data = mac_reg_read(rnic_pdata,mac_id,0x0c00);//added by hs@20200427
	// while(get_bits(data,0,0) == 1) //added by hs@20200427
    //    data = mac_reg_read(rnic_pdata,mac_id,0x0c00);//added by hs@20200427

	//mac_reg_write(rnic_pdata,mac_id,0x0c04,wdata); // added by hs@20200427
}


int mac_l3_l4_filter_cfg_reg_read(struct rnic_pdata*rnic_pdata,int mac_id,int addr)
{
    int data;
    data = mac_reg_read(rnic_pdata,mac_id,0x0c00);
    while(get_bits(data,0,0) == 1)
        data = mac_reg_read(rnic_pdata,mac_id,0x0c00);

    data = set_bits(data,15,8,addr);  //Layer4_Address
    data = set_bits(data,1,1,1);      //read
    data = set_bits(data,0,0,1);      //start write

    mac_reg_write(rnic_pdata,mac_id,0x0c00,data);

    data = mac_reg_read(rnic_pdata,mac_id,0x0c00);
    while(get_bits(data,0,0) == 1)
        data = mac_reg_read(rnic_pdata,mac_id,0x0c00);
            
    data = mac_reg_read(rnic_pdata,mac_id,0x0c04);
    
    return data;
}


void mac_recevie_all_en(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_recevie_all_en\n",mac_id);
    
    data = mac_reg_read(rnic_pdata,mac_id,0x8);
    data = set_bits(data,31,31,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x8,data);        
}

void mac_speed_cfg(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int mac_speed_mode;
    
    mac_speed_mode = mac_id ? rnic_pdata->mac_1_speed_mode : rnic_pdata->mac_0_speed_mode;
        
    RNIC_PRINTK("\tRNIC: mac %d speed_mode is 0x%x\n",mac_id,mac_speed_mode);
    
    data = mac_reg_read(rnic_pdata,mac_id,0x0);
    data = set_bits(data,30,28,mac_speed_mode);     
    mac_reg_write(rnic_pdata,mac_id,0x0,data);
}


void mac_loopback_on(struct rnic_pdata*rnic_pdata,int mac_id)
{    
    int data;
    RNIC_PRINTK("\tRNIC: mac %d mac_loopback_on\n",mac_id);
   
    data = mac_reg_read(rnic_pdata,mac_id,0x0004);
    data = set_bits(data,10,10,1);
    mac_reg_write(rnic_pdata,mac_id,0x0004,data);        
}


void mac_jumbo_on(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    RNIC_PRINTK("\tRNIC: mac %d mac_jumbo_on\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x0004);
    data = set_bits(data,8,8,1);
    mac_reg_write(rnic_pdata,mac_id,0x0004,data);
}


void mac_loopback_off(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_loopback_off\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x0004);
    data = set_bits(data,10,10,0x0);
    mac_reg_write(rnic_pdata,mac_id,0x0004,data);    
}


void mac_receive_crc_check_on(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_receive_crc_check_on\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x0004);
    data = set_bits(data,3,3,0x0);
    mac_reg_write(rnic_pdata,mac_id,0x0004,data);    
}


void mac_receive_crc_check_off(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    RNIC_PRINTK("\tRNIC: mac %d mac_receive_crc_check_off\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x0004);
    data = set_bits(data,3,3,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x0004,data);    
}


void mac_axi_cfg(struct rnic_pdata*rnic_pdata,int mac_id)
{
    mac_reg_write(rnic_pdata,mac_id,0x3010,0x000a0a0a);   // AXI_Tx_AR_ACE_Control [THC:4'b1010 | TEC:4'b1010 | TDRC:4'b1010]
    mac_reg_write(rnic_pdata,mac_id,0x3018,0x06060606);   // AXI_Rx_AW_ACE_Control [RDC:4'b0110 RHC:4'b0110 | RPC:4'b0110 | RDWC:4'b0110]
    mac_reg_write(rnic_pdata,mac_id,0x301c,0x00000a06);   // AXI_TxRx_AWAR_ACE_Control [RDRC:4'b1010 | TDWC:4'b0110]

    mac_set_blen_for_pcie_mps_lmt(rnic_pdata,mac_id);
    mac_set_axi_osr_lmt(rnic_pdata,mac_id);
}


void mac_tsf_off(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int channel_id;

    for(channel_id=0;channel_id<7;channel_id++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x1100+channel_id*0x80);
        data = set_bits(data,1,1,0x0);
        mac_reg_write(rnic_pdata,mac_id,0x1100+channel_id*0x80,data);
    }
}


void mac_ttc_cfg(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int channel_id;

    for(channel_id=0;channel_id<7;channel_id++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x1100+channel_id*0x80);
        data = set_bits(data,6,4,0x3);
        mac_reg_write(rnic_pdata,mac_id,0x1100+channel_id*0x80,data);
    }
}


void mac_alloc_rx_fifo(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int channel_id;
    int value;
    for(channel_id=0;channel_id<7;channel_id++)
    {
        if(channel_id==0)
            value = 0xff;
        else
            value = 0;

        data = mac_reg_read(rnic_pdata,mac_id,0x1140+channel_id*0x80);
        data = set_bits(data,24,16,value);//rx queue size 64KB
        mac_reg_write(rnic_pdata,mac_id,0x1140+channel_id*0x80,data);
    }
}


void mac_set_blen_for_pcie_mps_lmt(struct rnic_pdata*rnic_pdata,int mac_id)
{  
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_set_blen_for_pcie_mps_lmt\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x3004);
    //data = set_bits(data,7,0,0x1);
    data = set_bits(data,7,0,0x11);
    data = set_bits(data,12,12,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x3004,data);
}


void mac_set_axi_osr_lmt(struct rnic_pdata*rnic_pdata,int mac_id)
{  
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_set_axi_osr_lmt\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x3004);
    data = set_bits(data,31,24,0x3f);
    data = set_bits(data,23,16,0x3f);
    mac_reg_write(rnic_pdata,mac_id,0x3004,data);
}


void mac_ipc_off(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_ipc_off\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x0004);
    data = set_bits(data,9,9,0x0);
    mac_reg_write(rnic_pdata,mac_id,0x0004,data);    
}


void mac_ipc_on(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_ipc_on\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x0004);
    data = set_bits(data,9,9,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x0004,data);    
}


void mac_source_addr_replace_on(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_source_addr_replace_on\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x0000);
    data = set_bits(data,22,20,0x0);
    mac_reg_write(rnic_pdata,mac_id,0x0000,data);    
}

void mac_split_header_on(struct rnic_pdata*rnic_pdata,int mac_id,int channel_base_addr)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_split_header_on\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x3100+channel_base_addr);
    data = set_bits(data,24,24,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x3100+channel_base_addr,data);    
}


void mac_split_header_off(struct rnic_pdata*rnic_pdata,int mac_id,int channel_base_addr)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_split_header_off\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x3100+channel_base_addr);
    data = set_bits(data,24,24,0);
    mac_reg_write(rnic_pdata,mac_id,0x3100+channel_base_addr,data);
}


void mac_set_dsl(struct rnic_pdata*rnic_pdata,int mac_id,int channel_base_addr)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_set_dsl\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x3100+channel_base_addr);
    data = set_bits(data,20,18,1);
    mac_reg_write(rnic_pdata,mac_id,0x3100+channel_base_addr,data);  
}


void mac_drop_tcpip_checksum_err_pkg_on(struct rnic_pdata*rnic_pdata,int mac_id,int channel_base_addr)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_drop_tcpip_checksum_err_pkg_on\n",mac_id);
    
    data = mac_reg_read(rnic_pdata,mac_id,0x1140+channel_base_addr);
    data = set_bits(data,6,6,0);
    mac_reg_write(rnic_pdata,mac_id,0x1140+channel_base_addr,data);
}


void mac_drop_tcpip_checksum_err_pkg_off(struct rnic_pdata*rnic_pdata,int mac_id,int channel_base_addr)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_drop_tcpip_checksum_err_pkg_off\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x1140+channel_base_addr);
    data = set_bits(data,6,6,1);
    mac_reg_write(rnic_pdata,mac_id,0x1140+channel_base_addr,data);
}

void mac_rxq_flow_control_cfg (struct rnic_pdata*rnic_pdata,int mac_id,int channel_base_addr)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_rxq_flow_control_cfg\n",mac_id);
    
    data = mac_reg_read(rnic_pdata,mac_id,0x1150+channel_base_addr);
    set_bits(data,22,17,30);  //RFD 30:16KB 16KB-16KB=0KB 64KB-16KB=48KB
    set_bits(data,6,1,14);    //RFA 14:8KB  16KB-8KB=8KB  64KB-8KB=58KB
    mac_reg_write(rnic_pdata,mac_id,0x1150+channel_base_addr,data); 
}


void mac_pfc_en(struct rnic_pdata*rnic_pdata,int mac_id)
{
    RNIC_PRINTK("\tRNIC: mac %d mac_pfc_en\n",mac_id);

    mac_reg_write(rnic_pdata,mac_id,0x0050,0x00600000);   // MAC_VLAN_Tag_Ctrl [EVLS:11]
    mac_reg_write(rnic_pdata,mac_id,0x0060,0x00100000);   // MAC_VLAN_Incl [VLTI:1]
    mac_reg_write(rnic_pdata,mac_id,0x0070,0x00800012);   // MAC_PRI0_Tx_Flow_Ctrl[PT:128 | PLT:001 | TFE:1]     
    //mac_reg_write(rnic_pdata,mac_id,0x0070,0x00100012);   // MAC_PRI0_Tx_Flow_Ctrl[PT:16 | PLT:001 | TFE:1]    
    //mac_reg_write(rnic_pdata,mac_id,0x0070,0x00200002);   // MAC_PRI0_Tx_Flow_Ctrl[PT:32 | PLT:000 | TFE:1]       
    mac_reg_write(rnic_pdata,mac_id,0x0084,0x00800012);   // MAC_PRI5_Tx_Flow_Ctrl[PT:128 | PLT:001 | TFE:1]        
    mac_reg_write(rnic_pdata,mac_id,0x0090,0x00000101);   // MAC_Rx_Flow_Ctrl [PFCE:1 | RFE:1]
    mac_reg_write(rnic_pdata,mac_id,0x0160,0x08040201);   // PSRQ3:3 | PSRQ2:2  | PSRQ1:1 | PSRQ0:0  (USP 0 maps to queue 0)
    mac_reg_write(rnic_pdata,mac_id,0x0164,0x00c02010);   // PSRQ7:x | PSRQ6:67 | PSRQ5:5 | PSRQ4:4

/*      
    mac_reg_write(rnic_pdata,mac_id,0x0074,0x00000200);
    mac_reg_write(rnic_pdata,mac_id,0x0078,0x00000400);
    mac_reg_write(rnic_pdata,mac_id,0x007c,0x00000800);
    mac_reg_write(rnic_pdata,mac_id,0x0080,0x00001000);
    mac_reg_write(rnic_pdata,mac_id,0x0084,0x00002000);
    mac_reg_write(rnic_pdata,mac_id,0x0088,0x00004000);
    mac_reg_write(rnic_pdata,mac_id,0x008c,0x00008000);
*/
}


void mac_pfc_off(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int i;
    RNIC_PRINTK("\tRNIC: mac %d mac_pfc_off\n",mac_id);
    
    data = mac_reg_read(rnic_pdata,mac_id,0x0090);
    data = set_bits(data,0,0,0x0);//RFE:0
    data = set_bits(data,8,8,0x0);//PFCE:0  
    mac_reg_write(rnic_pdata,mac_id,0x0090,data);

    for(i=0;i<7;i++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x70+i*0x4);
        data = set_bits(data,1,1,0x0);//TFE:0
        mac_reg_write(rnic_pdata,mac_id,0x70+i*0x4,data);
    }   
}


void mac_dma_promiscuous_mode_en(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_dma_promiscuous_mode_en\n",mac_id);
    
    data = mac_reg_read(rnic_pdata,mac_id,0x8);
    data = set_bits(data,0,0,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x8,data);
}


void mac_dma_edma_cfg(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_dma_edma_cfg\n",mac_id);
    //tx
    data = mac_reg_read(rnic_pdata,mac_id,0x3040);
    data = set_bits(data,2,0,0x1);//TDPS:pre-fetch 12 descriptors
    mac_reg_write(rnic_pdata,mac_id,0x3040,data);

    //rx
    data = mac_reg_read(rnic_pdata,mac_id,0x3044);
    data = set_bits(data,2,0,0x1);//RDPS:pre-fetch 12 descriptors
    mac_reg_write(rnic_pdata,mac_id,0x3044,data);

    //osr
    //mac_set_axi_osr_lmt(rnic_pdata,mac_id);
}



void mac_dma_intr_mode_cfg(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_dma_intr_mode_cfg\n",mac_id);
    
    data = mac_reg_read(rnic_pdata,mac_id,0x3000);
    RNIC_PRINTK("\tRNIC: mac %d mac_dma_intr_mode is %x====================================================\n",mac_id,get_bits(data,13,12));
    data = set_bits(data,13,12,0x2);//0:pulse and use sbd_intr | 1:level and not use sbd_intr | 2 the same with 1 and int queued
    mac_reg_write(rnic_pdata,mac_id,0x3000,data);
}


void mac_disable_all_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_disable_all_intr\n",mac_id);
    
    mac_reg_write(rnic_pdata,mac_id,0xb4,0x0); //MAC_Interrupt_Enable
    mac_reg_write(rnic_pdata,mac_id,0x80c,0x0);//MMC_Receive_Interrupt_Enable
    mac_reg_write(rnic_pdata,mac_id,0x810,0x0);//MMC_Transmit_Interrupt_Enable
            
    for(i=0;i<7;i++)
    {
        mac_reg_write(rnic_pdata,mac_id,0x1170+i*0x80,0x0);//MTL_Q(#i)_Interrupt_Enable
        mac_reg_write(rnic_pdata,mac_id,0x3138+i*0x80,0x0);//DMA_CH(#i)_Interrupt_Enable
        mac_reg_write(rnic_pdata,mac_id,0x313c+i*0x80,0x0);//DMA_CH(#i)_Rx_Interrupt_Watchdog_Timer:256 cycle
    }   
}


void mac_enable_all_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_enable_all_intr\n",mac_id);
    
    mac_reg_write(rnic_pdata,mac_id,0xb4,0xffffffff); //MAC_Interrupt_Enable
    mac_reg_write(rnic_pdata,mac_id,0x80c,0xffffffff);//MMC_Receive_Interrupt_Enable
    mac_reg_write(rnic_pdata,mac_id,0x810,0xffffffff);//MMC_Transmit_Interrupt_Enable
            
    for(i=0;i<7;i++)
    {
        mac_reg_write(rnic_pdata,mac_id,0x1170+i*0x80,0xffffffff);//MTL_Q(#i)_Interrupt_Enable
        mac_reg_write(rnic_pdata,mac_id,0x3138+i*0x80,0xffffffff);//DMA_CH(#i)_Interrupt_Enable
        mac_reg_write(rnic_pdata,mac_id,0x313c+i*0x80,0x8c);      //DMA_CH(#i)_Rx_Interrupt_Watchdog_Timer:256 cycle
    }   
}


void mac_enable_mac_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    RNIC_PRINTK("\tRNIC: mac %d mac_enable_mac_intr\n",mac_id);
    
    mac_reg_write(rnic_pdata,mac_id,0xb4,0xffffffff); //MAC_Interrupt_Enable  
}


void mac_disable_mac_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    RNIC_PRINTK("\tRNIC: mac %d mac_disable_mac_intr\n",mac_id);

    mac_reg_write(rnic_pdata,mac_id,0xb4,0x0);    //MAC_Interrupt_Enable
}


void mac_enable_mmc_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    RNIC_PRINTK("\tRNIC: mac %d mac_enable_mmc_intr\n",mac_id);

    mac_reg_write(rnic_pdata,mac_id,0x80c,0xffffffff);//MMC_Receive_Interrupt_Enable
    mac_reg_write(rnic_pdata,mac_id,0x810,0xffffffff);//MMC_Transmit_Interrupt_Enable
}


void mac_disable_mmc_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    RNIC_PRINTK("\tRNIC: mac %d mac_disable_mmc_intr\n",mac_id);

    mac_reg_write(rnic_pdata,mac_id,0x80c,0x0);//MMC_Receive_Interrupt_Enable
    mac_reg_write(rnic_pdata,mac_id,0x810,0x0);//MMC_Transmit_Interrupt_Enable
}


void mac_enable_mtl_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_enable_mtl_intr\n",mac_id);

    for(i=0;i<7;i++)
        mac_reg_write(rnic_pdata,mac_id,0x1170+i*0x80,0xffffffff);//MTL_Q(#i)_Interrupt_Enable
}


void mac_disable_mtl_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_disable_mtl_intr\n",mac_id);

    for(i=0;i<7;i++)
        mac_reg_write(rnic_pdata,mac_id,0x1170+i*0x80,0x0);//MTL_Q(#i)_Interrupt_Enable
}


void mac_enable_dma_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_enable_dma_intr\n",mac_id);

    for(i=0;i<7;i++)
    {
        RNIC_PRINTK("\tRNIC: mac %d dma channel %d before dma intr enable is %x\n",mac_id,i,mac_reg_read(rnic_pdata,mac_id,0x3138+i*0x80));
        mac_reg_write(rnic_pdata,mac_id,0x3138+i*0x80,0xffffffff);//DMA_CH(#i)_Interrupt_Enable
     }
}

void mac_enable_dma_intr_ri_only(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_enable_dma_intr_ri_only\n",mac_id);

    for(i=0;i<7;i++)
    {
        //RNIC_PRINTK("\tRNIC: mac %d dma channel %d before mac_enable_dma_intr_ri_only is %x\n\n\n",mac_id,i,mac_reg_read(rnic_pdata,mac_id,0x3138+i*0x80));
        mac_reg_write(rnic_pdata,mac_id,0x3138+i*0x80,0x8040);//DMA_CH(#i)_Interrupt_Enable, NI must be enabled for DMA_Interrupt_Status and sbd_intr
     }
}



void mac_disable_dma_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_disable_dma_intr\n",mac_id);

    for(i=0;i<7;i++)
        mac_reg_write(rnic_pdata,mac_id,0x3138+i*0x80,0x0);//DMA_CH(#i)_Interrupt_Enable
}


void mac_disable_dma_intr_tx(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_disable_dma_intr\n",mac_id);

    for(i=0;i<7;i++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x3138+i*0x80);
        data = set_bits(data,0,0,0);//TIE
        mac_reg_write(rnic_pdata,mac_id,0x3138+i*0x80,data);
    }
}


void mac_enable_dma_intr_tx(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_disable_dma_intr\n",mac_id);

    for(i=0;i<7;i++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x3138+i*0x80);
        data = set_bits(data,0,0,1);//TIE
        mac_reg_write(rnic_pdata,mac_id,0x3138+i*0x80,data);
    }
}


void mac_enable_dma_riwt_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_enable_dma_riwt_intr\n",mac_id);

    for(i=0;i<7;i++)
        mac_reg_write(rnic_pdata,mac_id,0x313c+i*0x80,0x3f0000);       //RBCT:3f*1KB

    //DMA_CH(#i)_Rx_Interrupt_Watchdog_Timer:256 cycle
}


void mac_disable_dma_riwt_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_disable_dma_riwt_intr\n",mac_id);

    for(i=0;i<7;i++)
        mac_reg_write(rnic_pdata,mac_id,0x313c+i*0x80,0x0);       //DMA_CH(#i)_Rx_Interrupt_Watchdog_Timer:256 cycle
}


void mac_clear_all_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_eclear_all_intr\n",mac_id);

    //mac_reg_read(rnic_pdata,mac_id,0xb0);
    RNIC_PRINTK("MAC_Interrupt_Status is %x\n",mac_reg_read(rnic_pdata,mac_id,0xb0));
    RNIC_PRINTK("MAC_Interrupt_Status is %x\n",mac_reg_read(rnic_pdata,mac_id,0xb0));
    //mac_reg_read(rnic_pdata,mac_id,0xc0);
    RNIC_PRINTK("MAC_PMT_Control_Status is %x\n",mac_reg_read(rnic_pdata,mac_id,0xc0));
    RNIC_PRINTK("MAC_PMT_Control_Status is %x\n",mac_reg_read(rnic_pdata,mac_id,0xc0));
    
    for(i=0;i<7;i++)
    {
        RNIC_PRINTK("MTL_Q(%d)_Interrupt_Status is  %x\n",i,mac_reg_read(rnic_pdata,mac_id,0x1174+i*0x80));
        mac_reg_write(rnic_pdata,mac_id,0x1174+i*0x80,0xffffffff);
        RNIC_PRINTK("MTL_Q(%d)_Interrupt_Status is  %x\n",i,mac_reg_read(rnic_pdata,mac_id,0x1174+i*0x80));


        RNIC_PRINTK("DMA_CH(%d)_Status is  %x\n",i,mac_reg_read(rnic_pdata,mac_id,0x3160+i*0x80));
        mac_reg_write(rnic_pdata,mac_id,0x3160+i*0x80,0xffffffff);
        RNIC_PRINTK("DMA_CH(%d)_Status is  %x\n",i,mac_reg_read(rnic_pdata,mac_id,0x3160+i*0x80));        
    }

}


void mac_clear_dma_intr_tx(struct rnic_pdata*rnic_pdata,int mac_id, int channel_num)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d channel %d mac_clear_ti_intr\n",mac_id,channel_num);

    data = mac_reg_read(rnic_pdata,mac_id,0x3160+channel_num*0x80);
    data = set_bits(data,0,0,1);
    mac_reg_write(rnic_pdata,mac_id,0x3160+channel_num*0x80,data);
}


void mac_clear_dma_rx_intr(struct rnic_pdata*rnic_pdata,int mac_id, int channel_num)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d channel %d mac_clear_dma_rx_intr\n",mac_id,channel_num);

    data = mac_reg_read(rnic_pdata,mac_id,0x3160+channel_num*0x80);
    data = set_bits(data,6,6,1);
    mac_reg_write(rnic_pdata,mac_id,0x3160+channel_num*0x80,data);
}


void mac_enable_dspw(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_enable_dspw\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x3000);
    data = set_bits(data,8,8,1);
    mac_reg_write(rnic_pdata,mac_id,0x3000,data);
}


void mac_enable_tmrp(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_enable_tmrp\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x3000);
    data = set_bits(data,5,5,1);
    mac_reg_write(rnic_pdata,mac_id,0x3000,data);
}


void mac_enable_tdrp(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_enable_tdrp\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x3000);
    data = set_bits(data,4,4,1);
    mac_reg_write(rnic_pdata,mac_id,0x3000,data);
}



void mac_rwtu_cfg(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_rwtu_cfg\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x313c);
    data = set_bits(data,13,12,0x3);
    mac_reg_write(rnic_pdata,mac_id,0x313c,data);
}



void mac_av_cfg(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int addr;
    int i;

    RNIC_PRINTK("\tRNIC: mac %d mac_av_cfg\n",mac_id);
    
    for(i=0;i<7;i++)
    {
        addr = 0x80*i+0x1100;
        data = mac_reg_read(rnic_pdata,mac_id,0x313c);
        data = set_bits(data,3,2,0x1); 
        mac_reg_write(rnic_pdata,mac_id,0x313c,data);
    }
}


void mac_bandwidth_alloc(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int addr;
    int i;

    printk("\tRNIC: mac %d mac_bandwidth_alloc\n",mac_id);  
/*
    addr = 0x90;
    data = mac_reg_read(rnic_pdata,mac_id,addr );
    data = set_bits(data,0,0,0x0); //Disable Receive Flow Control
    data = set_bits(data,8,8,0x0); //Disable Priority Based Flow Control
    mac_reg_write(rnic_pdata,mac_id,addr ,data);


    addr = 0x140;
    //data = mac_reg_read(rnic_pdata,mac_id,addr );
    //data = set_bits(data,0,0,0x0); //Disable Receive Flow Control
    //data = set_bits(data,8,8,0x0); //Disable Priority Based Flow Control
    //data = 0; //disable all receive queue
    data = 0xaaaa; //enable all receive queue for av
    mac_reg_write(rnic_pdata,mac_id,addr ,data);

    */
    for(i=0;i<1;i++)
    {

/*        addr = 0x4*i+0x70;
        data = mac_reg_read(rnic_pdata,mac_id,addr );
        data = set_bits(data,1,1,0x0); //Disable Transmit Flow Control
        mac_reg_write(rnic_pdata,mac_id,addr ,data);


        addr = 0x80*i+0x1140;
        data = mac_reg_read(rnic_pdata,mac_id,addr );
        data = set_bits(data,7,7,0x1); //Disable Hardware Flow Control
        mac_reg_write(rnic_pdata,mac_id,addr ,data);
*/
        //TX queue
        addr = 0x80*i+0x1100;
        data = mac_reg_read(rnic_pdata,mac_id,addr );
        data = set_bits(data,10,8,0x6);//Queue mapped to TC6
        data = set_bits(data,3,2,0x1); //Enabled for AVB
        mac_reg_write(rnic_pdata,mac_id,addr ,data);

        //idleSlopeCredit
        addr = 0x80*i+0x1118;
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        data = set_bits(data,20,0,14746);
        mac_reg_write(rnic_pdata,mac_id,addr,data);

        //sendSlopeCredit
        addr = 0x80*i+0x111c;
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        data = set_bits(data,15,0,180022);
        mac_reg_write(rnic_pdata,mac_id,addr,data);

        //hiCredit
        addr = 0x80*i+0x1120;
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        data = set_bits(data,28,0,844800);
        mac_reg_write(rnic_pdata,mac_id,addr,data);

        //lowCredit
        addr = 0x80*i+0x1124;
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        data = set_bits(data,28,0,0);
        mac_reg_write(rnic_pdata,mac_id,addr,data);

        
    }

    for(i=7;i<7;i++)
    {

        addr = 0x4*i+0x70;
        data = mac_reg_read(rnic_pdata,mac_id,addr );
        data = set_bits(data,1,1,0x0); //Disable Transmit Flow Control
        mac_reg_write(rnic_pdata,mac_id,addr ,data);


        addr = 0x80*i+0x1140;
        data = mac_reg_read(rnic_pdata,mac_id,addr );
        data = set_bits(data,7,7,0x0); //Disable RX Hardware Flow Control
        mac_reg_write(rnic_pdata,mac_id,addr ,data);

    
        addr = 0x80*i+0x1100;
        data = mac_reg_read(rnic_pdata,mac_id,addr );
        data = set_bits(data,10,8,0x6);//Queue mapped to TC6
        data = set_bits(data,3,2,0x1); //Enabled for AVB
        mac_reg_write(rnic_pdata,mac_id,addr ,data);

        //idleSlopeCredit
        addr = 0x80*i+0x1118;
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        data = set_bits(data,20,0,14746);
        mac_reg_write(rnic_pdata,mac_id,addr,data);

        //sendSlopeCredit
        addr = 0x80*i+0x111c;
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        data = set_bits(data,15,0,18022);
        mac_reg_write(rnic_pdata,mac_id,addr,data);

        //hiCredit
        addr = 0x80*i+0x1120;
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        data = set_bits(data,28,0,844800);
        mac_reg_write(rnic_pdata,mac_id,addr,data);

        //lowCredit
        addr = 0x80*i+0x1124;
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        data = set_bits(data,28,0,-446600);
        mac_reg_write(rnic_pdata,mac_id,addr,data);
    }

}


void mac_report_all_intr(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int i;

    //RNIC_PRINTK("\tRNIC: mac %d mac_report_all_intr\n",mac_id);

    RNIC_PRINTK("MAC_Interrupt_Status       is %x\n",mac_reg_read(rnic_pdata,mac_id,0xb0));
    RNIC_PRINTK("MAC_PMT_Control_Status     is %x\n",mac_reg_read(rnic_pdata,mac_id,0xc0));
    RNIC_PRINTK("MTL_Interrupt_Status       is %x\n",mac_reg_read(rnic_pdata,mac_id,0x1020));
    
    for(i=0;i<7;i++)
    {
        RNIC_PRINTK("MTL_Q (%d)_Interrupt_Enable is %x\n",i,mac_reg_read(rnic_pdata,mac_id,0x1170+i*0x80));
        RNIC_PRINTK("MTL_Q (%d)_Interrupt_Status is %x\n",i,mac_reg_read(rnic_pdata,mac_id,0x1174+i*0x80));
    }

    for(i=0;i<7;i++)
        RNIC_PRINTK("DMA_CH(%d)_Status           is %x\n",i,mac_reg_read(rnic_pdata,mac_id,0x3160+i*0x80));   

}

int mac_get_link_status(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata, 0, 0xb0);

    data = get_bits(data,25,24);

    //RNIC_PRINTK("\tRNIC: mac %d mac link status is %x\n",mac_id,data);

    return data;
}


void mac_dma_rx_int_watchdog_timer_cfg(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int i;


    RNIC_PRINTK("\tRNIC: mac %d mac_dma_rx_int_watchdog_timer_cfg\n",mac_id);

    for(i=0;i<7;i++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x313c+i*0x80);
        RNIC_PRINTK("\tRNIC: mac %d dma_rx_int_watchdog_timer is %x====================\n",mac_id,data);
        data = set_bits(data,25,16,0x0);//disabled:Receive Byte Count Threshold
        data = set_bits(data,13,12,0x1);//512: Receive Interrupt Watchdog Timer Count Units
        data = set_bits(data,7,0,0x10); //16:Receive Interrupt Watchdog Timer Count.
        mac_reg_write(rnic_pdata,mac_id,0x313c+i*0x80,data);
    }       

}



void mac_transmitter_enable(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_transmitter_enable\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x0);
    data = set_bits(data,0,0,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x0,data);    
}


void mac_transmitter_disable(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: mac %d mac_transmitter_disable\n",mac_id);    
    
    data = mac_reg_read(rnic_pdata,mac_id,0x0);
    data = set_bits(data,0,0,0x0);
    mac_reg_write(rnic_pdata,mac_id,0x0,data);
}


void mac_receiver_enable(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_receiver_enable\n",mac_id);  

    data = mac_reg_read(rnic_pdata,mac_id,0x4);
    data = set_bits(data,0,0,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x4,data);
}


void mac_receiver_disable(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_receiver_disable\n",mac_id);

    data = mac_reg_read(rnic_pdata,mac_id,0x4);
    data = set_bits(data,0,0,0x0);
    mac_reg_write(rnic_pdata,mac_id,0x4,data);
}

void mac_use_channel_0_only(struct rnic_pdata*rnic_pdata,int mac_id)
{
    RNIC_PRINTK("\tRNIC: mac %d mac_use_channel_0_only\n",mac_id);
    
    mac_reg_write(rnic_pdata,mac_id,0x1030,0x00000000);   // MTL_RxQ_DMA_Map0 all queues mapped to channel 0
    mac_reg_write(rnic_pdata,mac_id,0x1034,0x00000000);   // MTL_RxQ_DMA_Map1
    mac_reg_write(rnic_pdata,mac_id,0x1038,0x00000000);   // MTL_RxQ_DMA_Map2
    mac_reg_write(rnic_pdata,mac_id,0x103c,0x00000000);   // MTL_RxQ_DMA_Map3
    mac_reg_write(rnic_pdata,mac_id,0x1040,0x000000ff);   // MTL_TC_Prty_Map0 all prioritys mapped to queue 0
    mac_reg_write(rnic_pdata,mac_id,0x1044,0x00000000);   // MTL_TC_Prty_Map1 
}



void mac_soft_reset(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: mac %d mac_soft_reset\n",mac_id);
   
    data = mac_reg_read(rnic_pdata,mac_id,0x3000);
    data = set_bits(data,0,0,0x1);
    mac_reg_write(rnic_pdata,mac_id,0x3000,data);
    
    data = mac_reg_read(rnic_pdata,mac_id,0x3000);
    while(get_bits(data,0,0) == 0x1)
        data = mac_reg_read(rnic_pdata,mac_id,0x3000);
        
    RNIC_PRINTK("\tRNIC: mac %d mac_soft_reset done\n",mac_id);  
}


void mac_reg_write(struct rnic_pdata*rnic_pdata,int mac_id,int offset,int wdata)
{
    int base_addr;
    int addr;
    int data;
    
    base_addr = mac_id ? RNIC_REG_BASE_ADDR_MAC_1 :RNIC_REG_BASE_ADDR_MAC_0;
    addr = base_addr + offset;
    data = wdata;
    reg_write(rnic_pdata,addr,data);

    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: mac %0d reg write: \t addr=%0x \t data=%0x\n",mac_id,offset,data);
    #endif
    
    #ifdef WRITE_REG_CONFIRM
        data = mac_reg_read(rnic_pdata,mac_id,offset);
        if(data != wdata)
            RNIC_PRINTK("\tRNIC: Warning: mac %0d  addr(%0x) \trdata(%0x) != wdata(%0x)\n",mac_id,offset,data,wdata);
    #endif    
}


int mac_reg_read(struct rnic_pdata*rnic_pdata,int mac_id,int offset)
{
    int base_addr;
    int addr;
    int data;

    base_addr = mac_id ? RNIC_REG_BASE_ADDR_MAC_1 :RNIC_REG_BASE_ADDR_MAC_0;   
    addr = base_addr + offset;
    data = reg_read(rnic_pdata,addr);
    
    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: mac %0d reg read: \t addr=%0x \t data=%0x\n",mac_id,offset,data);
    #endif
    
    return data;  
}


void mac_print_all_regs(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int addr;
    int data;
    int i;
    
    printk("\tRNIC: mac %0d all_mac_regs start\n",mac_id);
    
    //DWC_xlgmac_map/DWCXL_CORE
    for(addr=0x0;addr<0xc4;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }
    
    for(addr=0xd0;addr<0xdc;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }   

    for(addr=0x110;addr<0x124;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }

    for(addr=0x140;addr<0x140;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }

    for(addr=0x150;addr<0x154;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }

    for(addr=0x160;addr<0x16c;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }

    for(addr=0x200;addr<0x268;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }

    for(addr=0x300;addr<0x400;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }

    for(addr=0x800;addr<0x9a0;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }
    
    for(addr=0xc00;addr<0xdbc;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }
    
    
    //DWC_xlgmac_map/DWCXL_MTL
    for(addr=0x1000;addr<0x1044;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }
    
    //DWC_xlgmac_map/DWCXL_MTL_TCQ0 Registers
    for(addr=0x1100;addr<0x1174;addr=addr+0x4)
    {
        for(i=0;i<7;i++)
        {
            data = mac_reg_read(rnic_pdata,mac_id,addr+0x80*i);
            if(data != 0)
                printk("\tRNIC: mac %0d reg addr=%0x (i=%d) \t data=%0x\n",mac_id,addr,i,data);        
        }
    }

    //DWC_xlgmac_map/MTLTrafficClass_Queue_iRegisters (for i = 0; i <= DWCXL_NUM_TXQ/2-1) Registers
    for(addr=0x111C;addr<0x1124;addr=addr+0x4)
    {
        for(i=0;i<7;i++)
        {
            data = mac_reg_read(rnic_pdata,mac_id,addr+0x80*i);
            if(data != 0)
                printk("\tRNIC: mac %0d reg addr=%0x (i=%d) \t data=%0x\n",mac_id,addr,i,data);        
        }
    }

    //DWC_xlgmac_map/DWCXL_DMA
    for(addr=0x3000;addr<0x3050;addr=addr+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,addr);
        if(data != 0)
            printk("\tRNIC: mac %0d reg addr=%0x \t data=%0x\n",mac_id,addr,data);
    }

    //DWC_xlgmac_map/DWCXL_DMA
    for(addr=0x3100;addr<0x317C;addr=addr+0x4)
    {
        for(i=0;i<7;i++)
        {
            data = mac_reg_read(rnic_pdata,mac_id,addr+0x80*i);
            if(data != 0)
                printk("\tRNIC: mac %0d reg addr=%0x (i=%d) \t data=%0x\n",mac_id,addr,i,data);        
        }
    }

}



void mac_report_status(struct rnic_pdata*rnic_pdata,int mac_id)
{    
    mac_report_mtl_rxq_missed_pkt_overflow_cnt(rnic_pdata,mac_id);
    mac_report_dma_ch_miss_frame_cnt(rnic_pdata,mac_id);
    mac_report_tx_rx_status(rnic_pdata,mac_id);
    
}


void mac_report_mtl_rxq_missed_pkt_overflow_cnt(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int i;

    for(i=0;i<7;i++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x1144);
        RNIC_PRINTK("\tRNIC: mac %d rx queue %d MTL_RxQ_Missed_Pkt_Overflow_Cnt is %d \n",mac_id,i,data);
    }
}

void mac_report_dma_ch_miss_frame_cnt(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int i;

    for(i=0;i<7;i++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x31c6);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Miss_Frame_Cnt is %d \n",mac_id,i,data);
    }
}


void mac_report_tx_rx_status(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int i;

    for(i=0x800;i<0x9a0;i=i+0x4)
    {
        data = mac_reg_read(rnic_pdata,mac_id,i);
        RNIC_PRINTK("\tRNIC: mac %d addr=0x%0x, value=0x%0x \n",mac_id,i,data);
    }
}


#if 0

void Tx_Octet_Count_Good_Bad_Low(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x814);
    RNIC_PRINTK("\tRNIC: mac %d Tx_Octet_Count_Good_Bad_Low is %d \n",mac_id,data); 
}

void Tx_Octet_Count_Good_Bad_High(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x818);
    RNIC_PRINTK("\tRNIC: mac %d Tx_Octet_Count_Good_Bad_High is %d \n",mac_id,data); 
}


void Tx_Frame_Count_Good_Bad_Low(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x81c);
    RNIC_PRINTK("\tRNIC: mac %d Tx_Frame_Count_Good_Bad_Low is %d \n",mac_id,data); 
}


void Tx_Frame_Count_Good_Bad_High(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x820);
    RNIC_PRINTK("\tRNIC: mac %d Tx_Frame_Count_Good_Bad_Low is %d \n",mac_id,data); 
}


void Tx_Broadcast_Frames_Good_Low(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x824);
    RNIC_PRINTK("\tRNIC: mac %d Tx_Broadcast_Frames_Good_Low is %d \n",mac_id,data); 
}


void Tx_Broadcast_Frames_Good_High(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x828);
    RNIC_PRINTK("\tRNIC: mac %d Tx_Broadcast_Frames_Good_High is %d \n",mac_id,data); 
}

void Tx_Multicast_Frames_Good_Low(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x82c);
    RNIC_PRINTK("\tRNIC: mac %d Tx_Multicast_Frames_Good_Low is %d \n",mac_id,data); 
}


void Tx_Multicast_Frames_Good_High(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x830);
    RNIC_PRINTK("\tRNIC: mac %d Tx_Multicast_Frames_Good_High is %d \n",mac_id,data); 
}

void Tx_64Octets_Frames_Good_Bad_Low(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x834);
    RNIC_PRINTK("\tRNIC: mac %d Tx_64Octets_Frames_Good_Bad_Low is %d \n",mac_id,data); 
}

void Tx_64Octets_Frames_Good_Bad_High(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;

    data = mac_reg_read(rnic_pdata,mac_id,0x838);
    RNIC_PRINTK("\tRNIC: mac %d Tx_64Octets_Frames_Good_Bad_High is %d \n",mac_id,data); 
}

#endif


void mac_report_desc_status(struct rnic_pdata*rnic_pdata,int mac_id)
{
    int data;
    int i;

    for(i=0;i<1;i++)
    {
        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3130);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_TxDesc_Ring_Length is %d \n",mac_id,i,data);      

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3134);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_RxDesc_Ring_Length is %d \n",mac_id,i,data);  


        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3110);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_TxDesc_List_HAddress is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3118);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_RxDesc_List_HAddress is 0x%x \n",mac_id,i,data);
        
        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3120);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_TxDesc_Tail_HPointer is 0x%x \n",mac_id,i,data);
        
        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3128);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_RxDesc_Tail_HPointer is 0x%x \n",mac_id,i,data);              

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3140);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Current_App_TxDesc_H is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3148);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Current_App_RxDesc_H is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3150);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Current_App_TxBuffer_H is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3158);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Current_App_RxBuffer_H is 0x%x \n",mac_id,i,data);



        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3114);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_TxDesc_List_LAddress is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x311c);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_RxDesc_List_LAddress is 0x%x \n",mac_id,i,data);
        
        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3124);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_TxDesc_Tail_LPointer is 0x%x \n",mac_id,i,data);
        
        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x312c);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_RxDesc_Tail_LPointer is 0x%x \n",mac_id,i,data);              

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3144);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Current_App_TxDesc_L is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x314C);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Current_App_RxDesc_L is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3154);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Current_App_TxBuffer_L is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x315C);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Current_App_RxBuffer_L is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3160);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Status(0x3160) is 0x%x \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x3164);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Debug_Status(0x3164) is 0x%x  \n",mac_id,i,data);

        data = mac_reg_read(rnic_pdata,mac_id,0x80*i+0x316C);
        RNIC_PRINTK("\tRNIC: mac %d channel %d DMA_CH_Miss_Frame_Cnt is %d \n",mac_id,i,data);
    }

        data = mac_reg_read(rnic_pdata,mac_id,0x3100);
        RNIC_PRINTK("\tRNIC: mac %d channel %d 3100 is %d \n",mac_id,0,data);
        data = mac_reg_read(rnic_pdata,mac_id,0x3104);
        RNIC_PRINTK("\tRNIC: mac %d channel %d 3104 is %d \n",mac_id,0,data);
        data = mac_reg_read(rnic_pdata,mac_id,0x3108);
        RNIC_PRINTK("\tRNIC: mac %d channel %d 3108 is %d \n",mac_id,0,data);
}
