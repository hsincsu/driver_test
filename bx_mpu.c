//insomnia@2019/12/17 8:56:00

#include "header/bx_rnic.h"

void mpu_init(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("RNIC: mpu_init start\n");
    //default 100G
    rnic_pdata -> mplla_en                  = 0x0;
    rnic_pdata -> mpllb_en                  = 0x0;
                                            
    //default 100G                          
    rnic_pdata -> port_0_speed              = SPEED_100G;
    rnic_pdata -> port_0_link_down_cnt      = 0;
    rnic_pdata -> mac_0_speed_mode          = MAC_SPEED_100G;
    rnic_pdata -> pcs_0_type_sel            = PCS_TYPE_SEL_100G;
    rnic_pdata -> pcs_0_speed_sel           = PCS_SPEED_SEL_100G;
    rnic_pdata -> pcs_0_50g_mode            = 0x0; 
    rnic_pdata -> pcs_0_mpllb_sel           = 0x1;
    rnic_pdata -> pcs_0_krt_en              = 0x0;
    rnic_pdata -> pcs_0_an_en               = 0x0;
    rnic_pdata -> pcs_0_link_up_only        = 0x0;
    rnic_pdata -> pcs_0_rs_fec_en           = 0x0;
    rnic_pdata -> pcs_0_base_r_fec_en       = 0x0;
                                            
    rnic_pdata -> pcs_0_an_bp_1             = 0x0001;
    rnic_pdata -> pcs_0_an_bp_2             = 0xF380;
    rnic_pdata -> pcs_0_an_bp_3             = 0x0000;
    rnic_pdata -> pcs_0_an_nxp_1_0          = 0xA005;
    rnic_pdata -> pcs_0_an_nxp_2_0          = 0x0353;
    rnic_pdata -> pcs_0_an_nxp_3_0          = 0x04DF;
    rnic_pdata -> pcs_0_an_nxp_1_1          = 0x0203;
    rnic_pdata -> pcs_0_an_nxp_2_1          = 0x0330;
    rnic_pdata -> pcs_0_an_nxp_3_1          = 0x0F00;
    rnic_pdata -> pcs_0_an_nxp_index        = 0x0;
    rnic_pdata -> pcs_0_krt_lane_mask       = 0xf;
    rnic_pdata -> pcs_0_krt_cl72_en         = 0x0;
    rnic_pdata -> pcs_0_krt_check           = 0x0;   
    rnic_pdata -> pcs_0_speed_switch        = 0x0;
    rnic_pdata -> pcs_0_an_start            = 0x0;
    rnic_pdata -> pcs_0_an_restart_lock     = 0x0;
    rnic_pdata -> pcs_0_an_rcv_cnt          = 0x0;
    rnic_pdata -> pcs_0_an_intr_check_cnt   = 0x0;
    rnic_pdata -> pcs_0_an_success          = 0x0;
    rnic_pdata -> pcs_0_krt_start           = 0x0;
    rnic_pdata -> pcs_0_krt_success         = 0x0;
    rnic_pdata -> pcs_0_krt_failed          = 0x0;
    
    //default 50G
    rnic_pdata -> port_1_speed              = SPEED_50G;
    rnic_pdata -> port_1_link_down_cnt      = 0;
    rnic_pdata -> mac_1_speed_mode          = MAC_SPEED_50G;
    rnic_pdata -> pcs_1_type_sel            = PCS_TYPE_SEL_50G;
    rnic_pdata -> pcs_1_speed_sel           = PCS_SPEED_SEL_50G;
    rnic_pdata -> pcs_1_50g_mode            = 0x3; 
    rnic_pdata -> pcs_1_mpllb_sel           = 0x1; 
    rnic_pdata -> pcs_1_krt_en              = 0x0;
    rnic_pdata -> pcs_1_an_en               = 0x0;  
    rnic_pdata -> pcs_1_rs_fec_en           = 0x0;
    rnic_pdata -> pcs_1_base_r_fec_en       = 0x0;  
                                            
    rnic_pdata -> pcs_1_an_bp_1             = 0x0001;
    rnic_pdata -> pcs_1_an_bp_2             = 0xC380;
    rnic_pdata -> pcs_1_an_bp_3             = 0x0000;
    rnic_pdata -> pcs_1_an_nxp_1_0          = 0xA005;
    rnic_pdata -> pcs_1_an_nxp_2_0          = 0x0353;
    rnic_pdata -> pcs_1_an_nxp_3_0          = 0x04DF;
    rnic_pdata -> pcs_1_an_nxp_1_1          = 0x0203;
    rnic_pdata -> pcs_1_an_nxp_2_1          = 0x0330;
    rnic_pdata -> pcs_1_an_nxp_3_1          = 0x0F00;
    rnic_pdata -> pcs_1_an_nxp_index        = 0x0;
    rnic_pdata -> pcs_1_krt_lane_mask       = 0x3;
    rnic_pdata -> pcs_1_krt_cl72_en         = 0x0;
    rnic_pdata -> pcs_1_krt_check           = 0x0; 
    rnic_pdata -> pcs_1_speed_switch        = 0x0;
    rnic_pdata -> pcs_1_an_start            = 0x0;
    rnic_pdata -> pcs_1_an_restart_lock     = 0x0;
    rnic_pdata -> pcs_1_an_rcv_cnt          = 0x0;
    rnic_pdata -> pcs_1_an_intr_check_cnt   = 0x0;
    rnic_pdata -> pcs_1_an_success          = 0x0;
    rnic_pdata -> pcs_1_krt_start           = 0x0;
    rnic_pdata -> pcs_1_krt_success         = 0x0;
    rnic_pdata -> pcs_1_krt_failed          = 0x0;
    
    rnic_pdata -> pcs_an_nxp_1_null         = 0x2001; 
    rnic_pdata -> pcs_an_nxp_2_null         = 0x0000; 
    rnic_pdata -> pcs_an_nxp_3_null         = 0x0000; 
                        
    #ifdef PORT_0_KRT_EN    
        rnic_pdata -> pcs_0_krt_en         = 0x1;
    #endif                  
                            
    #ifdef PORT_0_AN_EN     
        rnic_pdata -> pcs_0_an_en          = 0x1;
    #endif                  

    #ifdef PORT_0_LINK_UP_ONLY     
        rnic_pdata -> pcs_0_link_up_only   = 0x1;
    #endif     
      
    #ifdef PORT_0_RS_FEC_EN 
        rnic_pdata -> pcs_0_rs_fec_en      = 0x1;
    #endif                  

    #ifdef PORT_0_BASE_R_FEC_EN
        rnic_pdata -> pcs_0_base_r_fec_en  = 0x1;
    #endif 

    #ifdef PORT_1_KRT_EN
       rnic_pdata -> pcs_1_krt_en          = 0x1;
    #endif                  
                            
    #ifdef PORT_1_AN_EN     
        rnic_pdata -> pcs_1_an_en          = 0x1;
    #endif    
    
    #ifdef PORT_1_LINK_UP_ONLY     
        rnic_pdata -> pcs_1_link_up_only   = 0x1;
    #endif    
    
    #ifdef PORT_1_RS_FEC_EN 
        rnic_pdata -> pcs_1_rs_fec_en      = 0x1;
    #endif                  

    #ifdef PORT_1_BASE_R_FEC_EN
        rnic_pdata -> pcs_1_base_r_fec_en  = 0x1;
    #endif 

        
    #ifdef PORT_0_100G
        rnic_pdata -> port_0_speed = SPEED_100G;
    #endif

    #ifdef PORT_0_50G
        rnic_pdata -> port_0_speed = SPEED_50G;
    #endif                            

    #ifdef PORT_0_25G
        rnic_pdata -> port_0_speed = SPEED_25G;
    #endif

    #ifdef PORT_0_40G
        rnic_pdata -> port_0_speed = SPEED_40G;
    #endif            

    #ifdef PORT_0_10G 
        rnic_pdata -> port_0_speed = SPEED_10G;
    #endif

    #ifdef PORT_1_50G
        rnic_pdata -> port_1_speed = SPEED_50G;
    #endif                            

    #ifdef PORT_1_25G
        rnic_pdata -> port_1_speed = SPEED_25G;
    #endif

    #ifdef PORT_1_10G 
       rnic_pdata -> port_1_speed = SPEED_10G;
    #endif

    if(!rnic_pdata -> mplla_en & !rnic_pdata -> mpllb_en)
        rnic_pdata -> mpllb_en = 0x1;

    mpu_speed_set(rnic_pdata,0);
    mpu_speed_set(rnic_pdata,1);
    
    RNIC_PRINTK("RNIC: mpu_init done\n");
}

void mpu_speed_set(struct rnic_pdata*rnic_pdata,int port_id)
{    
    if(port_id == 0x0)
    {
        RNIC_PRINTK("\tRNIC: port %0d speed set to %0dG\n",port_id,rnic_pdata -> port_0_speed);
        switch(rnic_pdata -> port_0_speed)
        {
            case SPEED_100G:
                rnic_pdata -> mpllb_en            = 0x1;
                rnic_pdata -> mac_0_speed_mode    = MAC_SPEED_100G;
                rnic_pdata -> pcs_0_type_sel      = PCS_TYPE_SEL_100G;
                rnic_pdata -> pcs_0_speed_sel     = PCS_SPEED_SEL_100G;
                rnic_pdata -> pcs_0_50g_mode      = 0x0;
                rnic_pdata -> pcs_0_mpllb_sel     = 0x1;
                rnic_pdata -> pcs_0_an_bp_1       = 0x0001;
                rnic_pdata -> pcs_0_an_bp_2       = 0xF380;
                rnic_pdata -> pcs_0_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_0_an_nxp_1_0    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_0    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_0    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_an_nxp_1_1    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_1    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_1    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_krt_lane_mask = 0xf;
                rnic_pdata -> pcs_0_krt_cl72_en   = 0x0;
                rnic_pdata -> pcs_0_krt_en        = 0x1;
                rnic_pdata -> pcs_0_an_en         = 0x1;
                rnic_pdata -> pcs_0_link_up_only  = 0x0;
                
                break;
            
            case SPEED_50G:
                rnic_pdata -> mpllb_en            = 0x1;
                rnic_pdata -> mac_0_speed_mode    = MAC_SPEED_50G;
                rnic_pdata -> pcs_0_type_sel      = PCS_TYPE_SEL_50G;
                rnic_pdata -> pcs_0_speed_sel     = PCS_SPEED_SEL_50G;
                rnic_pdata -> pcs_0_50g_mode      = 0x3;
                rnic_pdata -> pcs_0_mpllb_sel     = 0x1;
        #if 1        
                rnic_pdata -> pcs_0_an_bp_1       = 0x8001;
                rnic_pdata -> pcs_0_an_bp_2       = 0xC380;
                rnic_pdata -> pcs_0_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_0_an_nxp_1_0    = 0xA005;
                rnic_pdata -> pcs_0_an_nxp_2_0    = 0x0353;
                rnic_pdata -> pcs_0_an_nxp_3_0    = 0x04DF;
                rnic_pdata -> pcs_0_an_nxp_1_1    = 0x0203;
                rnic_pdata -> pcs_0_an_nxp_2_1    = 0x0330;
                rnic_pdata -> pcs_0_an_nxp_3_1    = 0x0F00;
        #else
                rnic_pdata -> pcs_0_an_bp_1       = 0x0001;
                rnic_pdata -> pcs_0_an_bp_2       = 0xC380;
                rnic_pdata -> pcs_0_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_0_an_nxp_1_0    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_0    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_0    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_an_nxp_1_1    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_1    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_1    = rnic_pdata -> pcs_an_nxp_3_null;
        #endif
                rnic_pdata -> pcs_0_krt_lane_mask = 0x3;
                rnic_pdata -> pcs_0_krt_cl72_en   = 0x1;
                rnic_pdata -> pcs_0_krt_en        = 0x0;
                rnic_pdata -> pcs_0_an_en         = 0x1;
                rnic_pdata -> pcs_0_link_up_only  = 0x1; 
                
                break;
            
            case SPEED_25G:
                rnic_pdata -> mpllb_en            = 0x1;
                rnic_pdata -> mac_0_speed_mode    = MAC_SPEED_25G;
                rnic_pdata -> pcs_0_type_sel      = PCS_TYPE_SEL_25G;
                rnic_pdata -> pcs_0_speed_sel     = PCS_SPEED_SEL_25G;
                rnic_pdata -> pcs_0_50g_mode      = 0x1;
                rnic_pdata -> pcs_0_mpllb_sel     = 0x1;
        #if 1        
                rnic_pdata -> pcs_0_an_bp_1       = 0x8001;
                rnic_pdata -> pcs_0_an_bp_2       = 0xC080;
                rnic_pdata -> pcs_0_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_0_an_nxp_1_0    = 0xA005;
                rnic_pdata -> pcs_0_an_nxp_2_0    = 0x0353;
                rnic_pdata -> pcs_0_an_nxp_3_0    = 0x04DF;
                rnic_pdata -> pcs_0_an_nxp_1_1    = 0x0203;
                rnic_pdata -> pcs_0_an_nxp_2_1    = 0x0030;
                rnic_pdata -> pcs_0_an_nxp_3_1    = 0x0F00;
        #else    
                rnic_pdata -> pcs_0_an_bp_1       = 0x0001;
                rnic_pdata -> pcs_0_an_bp_2       = 0xC080;
                rnic_pdata -> pcs_0_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_0_an_nxp_1_0    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_0    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_0    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_an_nxp_1_1    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_1    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_1    = rnic_pdata -> pcs_an_nxp_3_null;
        #endif        
                rnic_pdata -> pcs_0_krt_lane_mask = 0x1;
                rnic_pdata -> pcs_0_krt_cl72_en   = 0x1;
                rnic_pdata -> pcs_0_krt_en        = 0x0;
                rnic_pdata -> pcs_0_an_en         = 0x1;
                rnic_pdata -> pcs_0_link_up_only  = 0x1;
                
                break;
            
            case SPEED_40G:
                rnic_pdata -> mplla_en            = 0x1;
                rnic_pdata -> mac_0_speed_mode    = MAC_SPEED_40G;
                rnic_pdata -> pcs_0_type_sel      = PCS_TYPE_SEL_40G;
                rnic_pdata -> pcs_0_speed_sel     = PCS_SPEED_SEL_40G;
                rnic_pdata -> pcs_0_50g_mode      = 0x0;
                rnic_pdata -> pcs_0_mpllb_sel     = 0x0;
                rnic_pdata -> pcs_0_an_bp_1       = 0x0001;
                rnic_pdata -> pcs_0_an_bp_2       = 0xC380;
                rnic_pdata -> pcs_0_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_0_an_nxp_1_0    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_0    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_0    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_an_nxp_1_1    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_1    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_1    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_krt_lane_mask = 0xf;
                rnic_pdata -> pcs_0_krt_cl72_en   = 0x0;
                rnic_pdata -> pcs_0_krt_en        = 0x1;
                rnic_pdata -> pcs_0_an_en         = 0x1;
                rnic_pdata -> pcs_0_link_up_only  = 0x0;            
                break;
            
            case SPEED_10G:
                rnic_pdata -> mplla_en            = 0x1;
                rnic_pdata -> mac_0_speed_mode    = MAC_SPEED_10G;
                rnic_pdata -> pcs_0_type_sel      = PCS_TYPE_SEL_10G;
                rnic_pdata -> pcs_0_speed_sel     = PCS_SPEED_SEL_10G;
                rnic_pdata -> pcs_0_50g_mode      = 0x0;
                rnic_pdata -> pcs_0_mpllb_sel     = 0x0;                    
                rnic_pdata -> pcs_0_an_bp_1       = 0x0001;
                rnic_pdata -> pcs_0_an_bp_2       = 0x0080;
                rnic_pdata -> pcs_0_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_0_an_nxp_1_0    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_0    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_0    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_an_nxp_1_1    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_1    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_1    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_krt_lane_mask = 0x1;
                rnic_pdata -> pcs_0_krt_cl72_en   = 0x0;
                rnic_pdata -> pcs_0_krt_en        = 0x1;
                rnic_pdata -> pcs_0_an_en         = 0x1;
                rnic_pdata -> pcs_0_link_up_only  = 0x0;                
                break;
            
            default: //100G
                rnic_pdata -> mpllb_en            = 0x1;
                rnic_pdata -> mac_0_speed_mode    = MAC_SPEED_100G;
                rnic_pdata -> pcs_0_type_sel      = PCS_TYPE_SEL_100G;
                rnic_pdata -> pcs_0_speed_sel     = PCS_SPEED_SEL_100G;
                rnic_pdata -> pcs_0_50g_mode      = 0x0;
                rnic_pdata -> pcs_0_mpllb_sel     = 0x1;
                rnic_pdata -> pcs_0_an_bp_1       = 0x0001;
                rnic_pdata -> pcs_0_an_bp_2       = 0xF380;
                rnic_pdata -> pcs_0_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_0_an_nxp_1_0    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_0    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_0    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_an_nxp_1_1    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_0_an_nxp_2_1    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_0_an_nxp_3_1    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_0_krt_lane_mask = 0xf;
                rnic_pdata -> pcs_0_krt_cl72_en   = 0x0;
                rnic_pdata -> pcs_0_krt_en        = 0x1;
                rnic_pdata -> pcs_0_an_en         = 0x1;
                rnic_pdata -> pcs_0_link_up_only  = 0x0;  

        }
    }
    else
    {
        RNIC_PRINTK("\tRNIC: port %0d speed set to %0dG\n",port_id,rnic_pdata -> port_1_speed);
        switch(rnic_pdata -> port_1_speed)
        {
            case SPEED_50G:
                rnic_pdata -> mpllb_en            = 0x1;
                rnic_pdata -> mac_1_speed_mode    = MAC_SPEED_50G;
                rnic_pdata -> pcs_1_type_sel      = PCS_TYPE_SEL_50G;
                rnic_pdata -> pcs_1_speed_sel     = PCS_SPEED_SEL_50G;
                rnic_pdata -> pcs_1_50g_mode      = 0x3;
                rnic_pdata -> pcs_1_mpllb_sel     = 0x1;
                rnic_pdata -> pcs_1_an_bp_1       = 0x8001;
                rnic_pdata -> pcs_1_an_bp_2       = 0xC380;
                rnic_pdata -> pcs_1_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_1_an_bp_1       = 0x8001;
                rnic_pdata -> pcs_1_an_bp_2       = 0xC080;
                rnic_pdata -> pcs_1_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_1_an_nxp_1_0    = 0xA005;
                rnic_pdata -> pcs_1_an_nxp_2_0    = 0x0353;
                rnic_pdata -> pcs_1_an_nxp_3_0    = 0x04DF;
                rnic_pdata -> pcs_1_an_nxp_1_1    = 0x0203;
                rnic_pdata -> pcs_1_an_nxp_2_1    = 0x0330;
                rnic_pdata -> pcs_1_an_nxp_3_1    = 0x0F00;
                rnic_pdata -> pcs_1_krt_lane_mask = 0x3;
                rnic_pdata -> pcs_1_krt_cl72_en   = 0x1;
                
                break;
            
            case SPEED_25G:
                rnic_pdata -> mpllb_en            = 0x1;
                rnic_pdata -> mac_1_speed_mode    = MAC_SPEED_25G;
                rnic_pdata -> pcs_1_type_sel      = PCS_TYPE_SEL_25G;
                rnic_pdata -> pcs_1_speed_sel     = PCS_SPEED_SEL_25G;
                rnic_pdata -> pcs_1_50g_mode      = 0x1;
                rnic_pdata -> pcs_1_mpllb_sel     = 0x1;                    
                rnic_pdata -> pcs_1_an_bp_1       = 0x8001;
                rnic_pdata -> pcs_1_an_bp_2       = 0xC080;
                rnic_pdata -> pcs_1_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_1_an_bp_1       = 0x8001;
                rnic_pdata -> pcs_1_an_bp_2       = 0xC080;
                rnic_pdata -> pcs_1_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_1_an_nxp_1_0    = 0xA005;
                rnic_pdata -> pcs_1_an_nxp_2_0    = 0x0353;
                rnic_pdata -> pcs_1_an_nxp_3_0    = 0x04DF;
                rnic_pdata -> pcs_1_an_nxp_1_1    = 0x0203;
                rnic_pdata -> pcs_1_an_nxp_2_1    = 0x0030;
                rnic_pdata -> pcs_1_an_nxp_3_1    = 0x0F00;
                rnic_pdata -> pcs_1_krt_lane_mask = 0x1;
                rnic_pdata -> pcs_1_krt_cl72_en   = 0x1;
                
                break;
            
            case SPEED_10G:
                rnic_pdata -> mplla_en            = 0x1;
                rnic_pdata -> mac_1_speed_mode    = MAC_SPEED_10G;
                rnic_pdata -> pcs_1_type_sel      = PCS_TYPE_SEL_10G;
                rnic_pdata -> pcs_1_speed_sel     = PCS_SPEED_SEL_10G;
                rnic_pdata -> pcs_1_50g_mode      = 0x0;
                rnic_pdata -> pcs_1_mpllb_sel     = 0x0;                    
                rnic_pdata -> pcs_1_an_bp_1       = 0x0001;
                rnic_pdata -> pcs_1_an_bp_2       = 0x0080;
                rnic_pdata -> pcs_1_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_1_an_nxp_1_0    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_1_an_nxp_2_0    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_1_an_nxp_3_0    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_1_an_nxp_1_1    = rnic_pdata -> pcs_an_nxp_1_null;
                rnic_pdata -> pcs_1_an_nxp_2_1    = rnic_pdata -> pcs_an_nxp_2_null;
                rnic_pdata -> pcs_1_an_nxp_3_1    = rnic_pdata -> pcs_an_nxp_3_null;
                rnic_pdata -> pcs_1_krt_lane_mask = 0x1;
                rnic_pdata -> pcs_1_krt_cl72_en   = 0x0;
                
                break;
            
            default: //50G
                rnic_pdata -> mpllb_en            = 0x1;
                rnic_pdata -> mac_1_speed_mode    = MAC_SPEED_50G;
                rnic_pdata -> pcs_1_type_sel      = PCS_TYPE_SEL_50G;
                rnic_pdata -> pcs_1_speed_sel     = PCS_SPEED_SEL_50G;
                rnic_pdata -> pcs_1_50g_mode      = 0x3;
                rnic_pdata -> pcs_1_mpllb_sel     = 0x1;
                rnic_pdata -> pcs_1_an_bp_1       = 0x0001;
                rnic_pdata -> pcs_1_an_bp_2       = 0xC380;
                rnic_pdata -> pcs_1_an_bp_3       = 0x0000;
                rnic_pdata -> pcs_1_krt_lane_mask = 0x3;
                rnic_pdata -> pcs_1_krt_cl72_en   = 0x0;
        }
    }
}
