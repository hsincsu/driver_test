//insomnia@2019/12/12 15:37:32

#include "header/bx_rnic.h"

void pcs_init(struct rnic_pdata * rnic_pdata, int port_id)
{
    RNIC_PRINTK("RNIC: pcs %0d reg init start\n",port_id);

    pcs_sram_cfg(rnic_pdata,port_id);
    pcs_wait_soft_reset_clear(rnic_pdata,port_id);
    pcs_speed_switch(rnic_pdata,port_id);
    pcs_loopback_cfg(rnic_pdata,port_id); 
    
    pcs_an_cfg(rnic_pdata,port_id);
    pcs_krt_cfg(rnic_pdata,port_id);

    //pcs_set_am_cnt(rnic_pdata,port_id);

    RNIC_PRINTK("RNIC: pcs %0d reg init done\n",port_id);
}


void pcs_reg_write(struct rnic_pdata* rnic_pdata, int pcs_id,int offset,int data)
{
    int base_addr;
    int addr;
    
    base_addr = pcs_id ? RNIC_REG_BASE_ADDR_PCS_1 : RNIC_REG_BASE_ADDR_PCS_0;
    addr = base_addr + (offset<<2);
    reg_write(rnic_pdata,addr,data);
 
    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: pcs %0d reg write: \t addr=%0x \t data=%0x\n",pcs_id,offset,data);
    #endif
}


int pcs_reg_read(struct rnic_pdata* rnic_pdata, int pcs_id,int offset)
{
    int base_addr;
    int addr;
    int data;

    base_addr = pcs_id ? RNIC_REG_BASE_ADDR_PCS_1 : RNIC_REG_BASE_ADDR_PCS_0;   
    addr = base_addr + (offset<<2);
    data = reg_read(rnic_pdata,addr);
    
    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: pcs %0d reg read: \t addr=%0x \t data=%0x\n",pcs_id,offset,data);
    #endif
    
    return data;  
}

        
void pcs_wait_sram_init_done(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_wait_sram_init_done\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_SRAM);
    while(get_bits(data,INIT_DONE) != 0x1)
        data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_SRAM);
    
    RNIC_PRINTK("\tRNIC: pcs %0d sram_init_done\n",port_id);
}


void pcs_sram_ext_load_done(struct rnic_pdata* rnic_pdata, int port_id)
{  
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_sram_ext_load_done\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_SRAM);
    data = set_bits(data,EXT_LD_DONE,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_SRAM,data);
}


void pcs_wait_soft_reset_clear(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_wait_soft_reset_clear\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,SR_PCS_CTRL1);
    while(get_bits(data,RST) != 0x0)
        data = pcs_reg_read(rnic_pdata,port_id,SR_PCS_CTRL1);
        
    RNIC_PRINTK("\tRNIC: pcs %0d soft_reset_cleared\n",port_id);
}


int pcs_get_rlu(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;
    int rlu;
    
    data = pcs_reg_read(rnic_pdata,port_id,SR_PCS_STS1);
    rlu = get_bits(data,RLU);

    //RNIC_PRINTK("\tRNIC: pcs %0d rlu is %d\n",port_id,rlu);

    return rlu;
}


void pcs_wait_rlu(struct rnic_pdata* rnic_pdata, int port_id)
{   
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_wait_rlu\n",port_id);
    
    if(pcs_get_rlu(rnic_pdata,port_id) == 0)
        usleep_range(500,1000);

    if(pcs_get_rlu(rnic_pdata,port_id))
        RNIC_PRINTK("\tRNIC: pcs %0d rlu\n",port_id);
    else
        RNIC_PRINTK("\tRNIC: pcs %0d rlu failed\n",port_id);
}


void pcs_speed_switch(struct rnic_pdata* rnic_pdata, int port_id)
{    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_speed_switch\n",port_id); 
    pcs_speed_set(rnic_pdata,port_id);
    pcs_speed_common_cfg(rnic_pdata,port_id);
}


void pcs_speed_set(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    pcs_rs_fec_cfg(rnic_pdata,port_id);
    pcs_base_r_fec_cfg(rnic_pdata,port_id);
    pcs_type_cfg(rnic_pdata,port_id);
    pcs_pcs_speed_set(rnic_pdata,port_id);
    pcs_50g_mode_cfg(rnic_pdata,port_id);
}


void pcs_speed_common_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{
    pcs_mpll_cfg(rnic_pdata,port_id);   
    pcs_per_lane_cfg(rnic_pdata,port_id);
    pcs_set_vr_reset(rnic_pdata,port_id);
    pcs_sram_cfg(rnic_pdata,port_id);
    pcs_wait_vr_reset_clear(rnic_pdata,port_id);
    pcs_tx_eq_cfg(rnic_pdata,port_id);
}


void pcs_sram_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_sram_cfg\n",port_id);

    pcs_wait_sram_init_done(rnic_pdata,port_id);
#ifdef PHY_SRAM_UPDATE_EN        
    phy_sram_update(rnic_pdata,port_id);
#endif
    pcs_sram_ext_load_done(rnic_pdata,port_id);

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_sram_cfg done\n",port_id);
}


void pcs_type_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;
    int pcs_type;

    pcs_type    = port_id ? rnic_pdata->pcs_1_type_sel : rnic_pdata->pcs_0_type_sel;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_type_cfg\n",port_id);

    data        = pcs_reg_read(rnic_pdata,port_id,SR_PCS_CTRL2);
    data        = set_bits(data,PCS_TYPE_SEL,pcs_type);
    pcs_reg_write(rnic_pdata,port_id,SR_PCS_CTRL2,data); 
}


void pcs_pcs_speed_set(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;
    int pcs_speed;
      
    pcs_speed   = port_id ? rnic_pdata->pcs_1_speed_sel : rnic_pdata->pcs_0_speed_sel;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_pcs_speed_set\n",port_id);

    data        = pcs_reg_read(rnic_pdata,port_id,SR_PCS_CTRL1);
    data        = set_bits(data,PCS_SPEED_SEL,pcs_speed);
    pcs_reg_write(rnic_pdata,port_id,SR_PCS_CTRL1,data);
}


void pcs_50g_mode_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int data;
    int pcs_50g_mode;
    
    pcs_50g_mode= port_id ? rnic_pdata->pcs_1_50g_mode : rnic_pdata->pcs_0_50g_mode;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_50g_mode_cfg\n",port_id);

    data        = pcs_reg_read(rnic_pdata,port_id,VR_PCS_DIG_CTRL3);
    data        = set_bits(data,1,0,pcs_50g_mode);
    pcs_reg_write(rnic_pdata,port_id,VR_PCS_DIG_CTRL3,data);
}


void pcs_set_vr_reset(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_set_vr_reset\n",port_id);
    
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PCS_DIG_CTRL1);
    data = set_bits(data,VR_RST,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PCS_DIG_CTRL1,data);
}

void pcs_wait_vr_reset_clear(struct rnic_pdata* rnic_pdata, int port_id)
{  
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_wait_vr_reset_clear\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PCS_DIG_CTRL1);
    while(get_bits(data,VR_RST) != 0x0)
        data = pcs_reg_read(rnic_pdata,port_id,VR_PCS_DIG_CTRL1);

    RNIC_PRINTK("\tRNIC: pcs %0d vr_reset_cleared\n",port_id);
}


void pcs_per_lane_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    if((port_id == 0x0 && rnic_pdata->pcs_0_mpllb_sel == 0x0) || (port_id == 0x1 && rnic_pdata->pcs_1_mpllb_sel == 0x0))
        pcs_per_lane_cfg_10g(rnic_pdata,port_id);
    
    if((port_id == 0x0 && rnic_pdata->pcs_0_mpllb_sel == 0x1) || (port_id == 0x1 && rnic_pdata->pcs_1_mpllb_sel == 0x1))
        pcs_per_lane_cfg_25g(rnic_pdata,port_id);
}

void pcs_mpll_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    if(rnic_pdata->mplla_en == 0x1)
        pcs_mplla_cfg(rnic_pdata,port_id);

    if(rnic_pdata->mpllb_en == 0x1)
        pcs_mpllb_cfg(rnic_pdata,port_id);
    
    pcs_mpll_en_cfg(rnic_pdata,port_id);
}


void pcs_mplla_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_mplla_cfg\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_REF_CLK_CTRL);
    data = set_bits(data,REF_RANGE,0x3);
    data = set_bits(data,REF_CLK_DIV2,0x0);
    data = set_bits(data,REF_CLK_MPLLA_DIV,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_REF_CLK_CTRL,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL0);
    data = set_bits(data,MPLLA_MULTIPLIER,0x84);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL0,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL2);
    data = set_bits(data,MPLLA_DIV_MULT,0x8);
    data = set_bits(data,MPLLA_TX_CLK_DIV,0x2);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL2,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL3);
    data = set_bits(data,MPLLA_LOW_BW,0x18);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL4);
    data = set_bits(data,MPLLA_HIGH_BW,0x18);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL4,data);
}


void pcs_per_lane_cfg_10g(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_per_lane_cfg_10g\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF0);
    data = set_bits(data,VCO_REF_LD_0,0xb);
    data = set_bits(data,VCO_REF_LD_1,0xb);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF1);
    data = set_bits(data,VCO_REF_LD_2,0xb);
    data = set_bits(data,VCO_REF_LD_3,0xb);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD0);
    data = set_bits(data,VCO_LD_VAL_0,0x5ac);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD1);
    data = set_bits(data,VCO_LD_VAL_1,0x5ac);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD2);
    data = set_bits(data,VCO_LD_VAL_2,0x5ac);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD3);
    data = set_bits(data,VCO_LD_VAL_3,0x5ac);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL0);
    data = set_bits(data,RX0_CDR_PPM_MAX,0x13);
    data = set_bits(data,RX1_CDR_PPM_MAX,0x13);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL1);
    data = set_bits(data,RX2_CDR_PPM_MAX,0x13);
    data = set_bits(data,RX3_CDR_PPM_MAX,0x13);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL1,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_MPLL_CMN_CTRL);
    data = set_bits(data,MPLLB_SEL,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_MPLL_CMN_CTRL,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_TX_GEN_CTRL5);
    data = set_bits(data,TX_ALIGN_WD_XFER_0,0x0);
    data = set_bits(data,TX_ALIGN_WD_XFER_1,0x0);
    data = set_bits(data,TX_ALIGN_WD_XFER_2,0x0);
    data = set_bits(data,TX_ALIGN_WD_XFER_3,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_TX_GEN_CTRL5,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_TX_DCC_CTRL);
    data = 0xbbbb;
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_TX_DCC_CTRL,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_AFE_RATE_CTRL);
    data = set_bits(data,EQ_AFE_RT_0,0x4);
    data = set_bits(data,EQ_AFE_RT_1,0x4);
    data = set_bits(data,EQ_AFE_RT_2,0x4);
    data = set_bits(data,EQ_AFE_RT_3,0x4);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_AFE_RATE_CTRL,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_RATE_CTRL);
    data = set_bits(data,RX0_RATE,0x1);
    data = set_bits(data,RX1_RATE,0x1);
    data = set_bits(data,RX2_RATE,0x1);
    data = set_bits(data,RX3_RATE,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_RATE_CTRL,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL0);
    data = set_bits(data,RX0_DELTA_IQ,0x6);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL1);
    data = set_bits(data,RX1_DELTA_IQ,0x6);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL2);
    data = set_bits(data,RX2_DELTA_IQ,0x6);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL3);
    data = set_bits(data,RX3_DELTA_IQ,0x6);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0);
    data = set_bits(data,CTLE_BOOST_0,0x11);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1);
    data = set_bits(data,CTLE_BOOST_1,0x11);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2);
    data = set_bits(data,CTLE_BOOST_2,0x11);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3);
    data = set_bits(data,CTLE_BOOST_3,0x11);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0);
    data = set_bits(data,CTLE_POLE_0,0x2);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1);
    data = set_bits(data,CTLE_POLE_1,0x2);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2);
    data = set_bits(data,CTLE_POLE_2,0x2);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3);
    data = set_bits(data,CTLE_POLE_3,0x2);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL0);
    data = set_bits(data,DFE_TAP1_0,0xf);
    data = set_bits(data,DFE_TAP1_1,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL1);
    data = set_bits(data,DFE_TAP1_2,0xf);
    data = set_bits(data,DFE_TAP1_3,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_GENCTRL5);
    data = set_bits(data,HF_EN_0,0x0);
    data = set_bits(data,HF_EN_3_1,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_GENCTRL5,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_GENCTRL3);
    data = set_bits(data,LF_TH_0,0x4);
    data = set_bits(data,LF_TH_1,0x4);
    data = set_bits(data,LF_TH_2,0x4);
    data = set_bits(data,LF_TH_3,0x4);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_GENCTRL3,data);
}


void pcs_mpllb_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_mpllb_cfg\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_REF_CLK_CTRL);
    data = set_bits(data,REF_RANGE,REF_RANGE_VAL);
    data = set_bits(data,REF_CLK_DIV2,REF_CLK_DIV2_VAL);
    data = set_bits(data,REF_CLK_MPLLB_DIV,REF_MPLLB_DIV_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_REF_CLK_CTRL,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL0);
    data = set_bits(data,MPLLB_MULTIPLIER,0xa5);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL2);
    data = set_bits(data,MPLLB_DIV_MULT,0x4);
    data = set_bits(data,MPLLB_TX_CLK_DIV,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL3);
    data = set_bits(data,MPLLB_LOW_BW,0x18);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL4);
    data = set_bits(data,MPLLB_HIGH_BW,0x18);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL4,data);

}       

void pcs_mpll_en_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_mpll_en_cfg\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL0);
    data = set_bits(data,MPLLA_CAL_DISABLE,!rnic_pdata->mplla_en);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLA_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL0);
    data = set_bits(data,MPLLB_CAL_DISABLE,!rnic_pdata->mpllb_en);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_MPLLB_CTRL0,data);
}   

void pcs_per_lane_cfg_25g(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;
    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_per_lane_cfg_25g\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF0);
    data = set_bits(data,VCO_REF_LD_0,MPLLB_VCO_REF_LD_0_VAL);
    data = set_bits(data,VCO_REF_LD_1,MPLLB_VCO_REF_LD_1_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF1);
    data = set_bits(data,VCO_REF_LD_2,MPLLB_VCO_REF_LD_2_VAL);
    data = set_bits(data,VCO_REF_LD_3,MPLLB_VCO_REF_LD_3_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_REF1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD0);
    data = set_bits(data,VCO_LD_VAL_0,MPLLB_VCO_LD_VAL_0_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD1);
    data = set_bits(data,VCO_LD_VAL_1,MPLLB_VCO_LD_VAL_1_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD2);
    data = set_bits(data,VCO_LD_VAL_2,MPLLB_VCO_LD_VAL_2_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD3);
    data = set_bits(data,VCO_LD_VAL_3,MPLLB_VCO_LD_VAL_3_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_VCO_CAL_LD3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL0);
    data = set_bits(data,RX0_CDR_PPM_MAX,MPLLB_RX0_CDR_PPM_MAX_VAL);
    data = set_bits(data,RX1_CDR_PPM_MAX,MPLLB_RX1_CDR_PPM_MAX_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL1);
    data = set_bits(data,RX2_CDR_PPM_MAX,MPLLB_RX2_CDR_PPM_MAX_VAL);
    data = set_bits(data,RX3_CDR_PPM_MAX,MPLLB_RX3_CDR_PPM_MAX_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_PPM_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_MPLL_CMN_CTRL);
    data = set_bits(data,MPLLB_SEL,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_MPLL_CMN_CTRL,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_TX_GEN_CTRL5);
    data = set_bits(data,TX_ALIGN_WD_XFER_0,0x1);
    data = set_bits(data,TX_ALIGN_WD_XFER_1,0x1);
    data = set_bits(data,TX_ALIGN_WD_XFER_2,0x1);
    data = set_bits(data,TX_ALIGN_WD_XFER_3,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_TX_GEN_CTRL5,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_TX_DCC_CTRL);
    data = 0xcccc;
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_TX_DCC_CTRL,data);
   
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_AFE_RATE_CTRL);
    data = set_bits(data,EQ_AFE_RT_0,0x0);
    data = set_bits(data,EQ_AFE_RT_1,0x0);
    data = set_bits(data,EQ_AFE_RT_2,0x0);
    data = set_bits(data,EQ_AFE_RT_3,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_AFE_RATE_CTRL,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_RATE_CTRL);
    data = set_bits(data,RX0_RATE,0x0);
    data = set_bits(data,RX1_RATE,0x0);
    data = set_bits(data,RX2_RATE,0x0);
    data = set_bits(data,RX3_RATE,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_RATE_CTRL,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL0);
    data = set_bits(data,RX0_DELTA_IQ,0x3);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL1);
    data = set_bits(data,RX1_DELTA_IQ,0x3);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL2);
    data = set_bits(data,RX2_DELTA_IQ,0x3);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL3);
    data = set_bits(data,RX3_DELTA_IQ,0x3);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_IQ_CTRL3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0);
    data = set_bits(data,CTLE_BOOST_0,0xc);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1);
    data = set_bits(data,CTLE_BOOST_1,0xc);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2);
    data = set_bits(data,CTLE_BOOST_2,0xc);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3);
    data = set_bits(data,CTLE_BOOST_3,0x0c);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0);
    data = set_bits(data,CTLE_POLE_0,0x3);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1);
    data = set_bits(data,CTLE_POLE_1,0x3);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2);
    data = set_bits(data,CTLE_POLE_2,0x3);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3);
    data = set_bits(data,CTLE_POLE_3,0x3);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_EQ_CTRL3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL0);
    data = set_bits(data,DFE_TAP1_0,0xf);
    data = set_bits(data,DFE_TAP1_1,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL1);
    data = set_bits(data,DFE_TAP1_2,0xf);
    data = set_bits(data,DFE_TAP1_3,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_DFE_TAP_CTRL1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_GENCTRL5);
    data = set_bits(data,HF_EN_0,0x0);
    data = set_bits(data,HF_EN_3_1,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_GENCTRL5,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_GENCTRL3);
    data = set_bits(data,LF_TH_0,0x4);
    data = set_bits(data,LF_TH_1,0x4);
    data = set_bits(data,LF_TH_2,0x4);
    data = set_bits(data,LF_TH_3,0x4);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_RX_GENCTRL3,data);
}

void pcs_tx_eq_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_tx_eq_cfg\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN0);
    data = set_bits(data,TX_EQ_PRE,0x0);
    data = set_bits(data,TX_EQ_MAIN,0x18);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN1);
    data = set_bits(data,TX_EQ_PRE,0x0);
    data = set_bits(data,TX_EQ_MAIN,0x18);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN2);
    data = set_bits(data,TX_EQ_PRE,0x0);
    data = set_bits(data,TX_EQ_MAIN,0x18);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN3);
    data = set_bits(data,TX_EQ_PRE,0x0);
    data = set_bits(data,TX_EQ_MAIN,0x18);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL0_LN3,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN0);
    data = set_bits(data,TX_EQ_POST,0x0);
    data = set_bits(data,TX_EQ_OVER_RIDE,TX_EQ_OVER_RIDE_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN0,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN1);
    data = set_bits(data,TX_EQ_POST,0x0);
    data = set_bits(data,TX_EQ_OVER_RIDE,TX_EQ_OVER_RIDE_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN1,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN2);
    data = set_bits(data,TX_EQ_POST,0x0);
    data = set_bits(data,TX_EQ_OVER_RIDE,TX_EQ_OVER_RIDE_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN2,data);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN3);
    data = set_bits(data,TX_EQ_POST,0x0);
    data = set_bits(data,TX_EQ_OVER_RIDE,TX_EQ_OVER_RIDE_VAL);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_EQ_CTRL1_LN3,data);
}


void pcs_rs_fec_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_PMA_RS_FEC_CTRL);
    
    if((port_id == 0x0 && rnic_pdata->pcs_0_rs_fec_en == 0x1) || (port_id == 0x1 && rnic_pdata->pcs_1_rs_fec_en == 0x1))
    {
        data = set_bits(data,2,2,0x1);
        RNIC_PRINTK("\tRNIC: pcs %0d rs_fec en\n",port_id);
    }
    else
    {   
        data = set_bits(data,2,2,0x0); 
        RNIC_PRINTK("\tRNIC: pcs %0d rs_fec off\n",port_id);
    }
    
    pcs_reg_write(rnic_pdata,port_id,SR_PMA_RS_FEC_CTRL,data);
}


void pcs_base_r_fec_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_PMA_KR_FEC_CTRL);
    
    if((port_id == 0x0 && rnic_pdata->pcs_0_base_r_fec_en == 0x1) || (port_id == 0x1 && rnic_pdata->pcs_1_base_r_fec_en == 0x1))
    {
        data = set_bits(data,0,0,0x1);
        RNIC_PRINTK("\tRNIC: pcs %0d base_r_fec en\n",port_id);
    }
    else
    {    
        data = set_bits(data,0,0,0x0);
        RNIC_PRINTK("\tRNIC: pcs %0d base_r_fec off\n",port_id);
    }

    pcs_reg_write(rnic_pdata,port_id,SR_PMA_KR_FEC_CTRL,data);
}




void pcs_cr_wr_phy(struct rnic_pdata* rnic_pdata, int port_id,int addr,int wdata)
{      
    int data;
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    while(get_bits(data,0,0) != 0x0) 
        data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_ADDR,addr);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_DATA,wdata);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    data = set_bits(data,1,1,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    data = set_bits(data,0,0,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    while(get_bits(data,0,0) != 0x0) 
        data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    
    #ifdef PHY_CR_LOG_EN     
        RNIC_PRINTK("\tRNIC: pcs %0d pcs_cr_wr_phy: \t addr=%0x \t data=%0x\n",port_id,addr,wdata);
        pcs_cr_rd_phy(rnic_pdata,port_id,addr);
    #endif
}


int pcs_cr_rd_phy(struct rnic_pdata* rnic_pdata, int port_id,int addr)
{   
    int data;
     
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    while(get_bits(data,0,0) != 0x0) 
        data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_ADDR,addr);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    data = set_bits(data,1,1,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    data = set_bits(data,0,0,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL,data);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);
    while(get_bits(data,0,0) != 0x0) 
        data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_CTRL);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_CR_DATA);
    
    #ifdef PHY_CR_LOG_EN    
        RNIC_PRINTK("\tRNIC: pcs %0d pcs_cr_rd_phy: \t addr=%0x \t data=%0x\n",port_id,addr,data);
    #endif
    
    return data;  
}


void pcs_r2t_lb_en(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int data;
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PCS_DIG_CTRL1);
    data = set_bits(data,R2TLBE,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PCS_DIG_CTRL1,data);
    
    RNIC_PRINTK("\tRNIC: pcs %0d loopback_en\n",port_id);
}


void pcs_loopback_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_loopback_cfg\n",port_id);
    #ifdef PCS_R2T_LB_EN
        pcs_r2t_lb_en(rnic_pdata,port_id);
    #endif

    #ifdef PCS_PHY_R2T_LB_EN
        pcs_phy_r2t_lb_en(rnic_pdata,port_id);
    #endif        

    #ifdef PCS_PHY_R2T_LB_EN_PCS
        printk("not in r2t\n");//added by hs
		pcs_phy_r2t_lb_en_pcs(rnic_pdata,port_id);
    #endif
    #ifdef PCS_PHY_T2R_LB_EN
        pcs_phy_t2r_lb_en(rnic_pdata,port_id);
    #endif

    #ifdef PCS_PHY_T2R_LB_EN_PCS
        pcs_phy_t2r_lb_en_pcs(rnic_pdata,port_id);
    #endif
}


void pcs_phy_r2t_lb_en_pcs(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_MISC_CTRL0);
    data = set_bits(data,RX2TX_LB_EN,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_MISC_CTRL0,data);
    
    RNIC_PRINTK("\tRNIC: pcs %0d phy r2t lb en pcs\n",port_id);
}


void pcs_phy_t2r_lb_en_pcs(struct rnic_pdata* rnic_pdata, int port_id)
{  
    int data;
     
    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_MISC_CTRL0);
    data = set_bits(data,TX2RX_LB_EN,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_MISC_CTRL0,data);
    
    RNIC_PRINTK("\tRNIC: pcs %0d phy t2r lb en pcs\n",port_id);
}


void pcs_phy_r2t_lb_en(struct rnic_pdata* rnic_pdata, int port_id)
{    
    pcs_cr_wr_phy(rnic_pdata,port_id,0x1000,0x6);
    pcs_cr_wr_phy(rnic_pdata,port_id,0x1100,0x6);
    pcs_cr_wr_phy(rnic_pdata,port_id,0x1200,0x6);
    pcs_cr_wr_phy(rnic_pdata,port_id,0x1300,0x6);
    
    RNIC_PRINTK("\tRNIC: port %0d phy r2t lb en\n",port_id);
}


void pcs_phy_t2r_lb_en(struct rnic_pdata* rnic_pdata, int port_id)
{   
    pcs_cr_wr_phy(rnic_pdata,port_id,0x1000,0x5);
    pcs_cr_wr_phy(rnic_pdata,port_id,0x1100,0x5);
    pcs_cr_wr_phy(rnic_pdata,port_id,0x1200,0x5);
    pcs_cr_wr_phy(rnic_pdata,port_id,0x1300,0x5);

	printk("\tRNIC: pcs %0d phy t2r lb en\n",port_id);//added by hs
    RNIC_PRINTK("\tRNIC: pcs %0d phy t2r lb en\n",port_id);
}

void pcs_tx_invert_en(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_GENCTRL0);
    data = set_bits(data,7,4,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_TX_GENCTRL0,data);


    RNIC_PRINTK("\tRNIC: pcs %0d pcs_tx_invert_en\n",port_id);
}


void pcs_rx_invert_en(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_GENCTRL1);
    data = set_bits(data,3,0,0xf);
    pcs_reg_write(rnic_pdata,port_id,VR_PMA_SNPS_MP_25G_16G_RX_GENCTRL1,data);


    RNIC_PRINTK("\tRNIC: pcs %0d pcs_rx_invert_en\n",port_id);
}


void pcs_tx_rx_invert_en(struct rnic_pdata* rnic_pdata, int port_id)
{   
    pcs_tx_invert_en(rnic_pdata,port_id);
    pcs_rx_invert_en(rnic_pdata,port_id);


    RNIC_PRINTK("\tRNIC: pcs %0d pcs_tx_rx_invert_en\n",port_id);
}

void pcs_enable_prbs31(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_PCS_TP_CTRL);
    data = set_bits(data,4,4,0x1);
    pcs_reg_write(rnic_pdata,port_id,SR_PCS_TP_CTRL,data);


    RNIC_PRINTK("\tRNIC: pcs %0d pcs_enable_prbs31\n",port_id);
}


void pcs_disable_prbs31(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_PCS_TP_CTRL);
    data = set_bits(data,4,4,0x0);
    pcs_reg_write(rnic_pdata,port_id,SR_PCS_TP_CTRL,data);


    RNIC_PRINTK("\tRNIC: pcs %0d pcs_disable_prbs31\n",port_id);
}


void pcs_enable_idle_pattern(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_PCS_TP_CTRL);
    data = set_bits(data,3,3,0x1);
    data = set_bits(data,0,0,0x1);
    pcs_reg_write(rnic_pdata,port_id,SR_PCS_TP_CTRL,data);


    RNIC_PRINTK("\tRNIC: pcs %0d pcs_enable_idle_pattern\n",port_id);
}



void pcs_disable_idle_pattern(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_PCS_TP_CTRL);
    data = set_bits(data,3,3,0x0);
    data = set_bits(data,0,0,0x0);
    pcs_reg_write(rnic_pdata,port_id,SR_PCS_TP_CTRL,data);


    RNIC_PRINTK("\tRNIC: pcs %0d pcs_disable_idle_pattern\n",port_id);
}



void pcs_msk_phy_rst(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;

    RNIC_PRINTK("\tRNIC:  pcs %0d pcs_msk_phy_rst\n",port_id);
    data = pcs_reg_read(rnic_pdata,port_id,VR_PCS_DIG_CTRL3);
    data = set_bits(data,MSK_PHY_RST,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PCS_DIG_CTRL3,data);
}



void pcs_set_am_cnt(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;

    RNIC_PRINTK("\tRNIC:  pcs %0d pcs_set_am_cnt\n",port_id);
    data = pcs_reg_read(rnic_pdata,port_id,VR_PCS_AM_CNT);
    data = set_bits(data,13,0,0x80);
    pcs_reg_write(rnic_pdata,port_id,VR_PCS_AM_CNT,data);
}


//AN
void pcs_an_disable(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_CTRL);
    data = set_bits(data,12,12,0x0);
    pcs_reg_write(rnic_pdata,port_id,SR_AN_CTRL,data);

    if(port_id == 0)
    {
        rnic_pdata->pcs_0_an_start             = 0;
        rnic_pdata->pcs_0_an_rcv_cnt         = 0;
        rnic_pdata->pcs_0_an_intr_check_cnt    = 0;
    }    
    else
    {
        rnic_pdata->pcs_1_an_start             = 0;
        rnic_pdata->pcs_1_an_rcv_cnt         = 0;
        rnic_pdata->pcs_1_an_intr_check_cnt    = 0;
    }    

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_disable\n",port_id);    
}


void pcs_an_enable(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_CTRL);
    data = set_bits(data,12,12,0x1);
    pcs_reg_write(rnic_pdata,port_id,SR_AN_CTRL,data);
    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_enable\n",port_id);
}


void pcs_an_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_cfg\n",port_id);
    
    pcs_an_disable(rnic_pdata,port_id);

    pcs_an_clear_all_intr(rnic_pdata,port_id);
    
    pcs_an_enable_intr(rnic_pdata,port_id); 

    pcs_an_ext_np_ctl_en(rnic_pdata,port_id); 

    pcs_an_pdet_cfg(rnic_pdata,port_id); 

    pcs_an_base_page_cfg(rnic_pdata,port_id);
}


void pcs_an_ext_np_ctl_en(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int data;

    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_CTRL);
    data = set_bits(data,13,13,0x1);
    pcs_reg_write(rnic_pdata,port_id,SR_AN_CTRL,data);

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_ext_np_ctl_en\n",port_id);
}


void pcs_an_enable_intr(struct rnic_pdata* rnic_pdata, int port_id)
{    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_enable_intr\n",port_id);

    pcs_reg_write(rnic_pdata,port_id,VR_AN_INTR_MSK,0x7);
}


int pcs_an_get_intr(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;
        
    //RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_get_intr\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,VR_AN_INTR);
    data = get_bits(data,2,0);

    return data;
}



void pcs_an_rst(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int data;

    pcs_msk_phy_rst(rnic_pdata,port_id);
    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_rst\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_CTRL);
    data = set_bits(data,PCS_AN_RST,0x1);
    pcs_reg_write(rnic_pdata,port_id,SR_AN_CTRL,data);
    
    RNIC_PRINTK("\tRNIC: pcs %0d wait pcs_an_rst clear\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_CTRL);
    while(get_bits(data,PCS_AN_RST)==0x1)
        data = pcs_reg_read(rnic_pdata,port_id,SR_AN_CTRL);
        
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_rst cleared\n",port_id);    

}


void pcs_an_start(struct rnic_pdata* rnic_pdata, int port_id)
{ 
    int data;
    
    if(rnic_pdata->pcs_0_an_en)
    {
        RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_start\n",port_id);
        
        pcs_an_clear_all_intr(rnic_pdata,port_id);
        
        data = pcs_reg_read(rnic_pdata,port_id,SR_AN_CTRL);
        data = set_bits(data,12,12,0x1);
        data = set_bits(data,9,9,0x1);
        pcs_reg_write(rnic_pdata,port_id,SR_AN_CTRL,data);
        
        
        if(port_id == 0)
        {
            rnic_pdata->pcs_0_an_start            = 1;
            rnic_pdata->pcs_0_an_restart_lock    = 1;
            rnic_pdata->pcs_0_an_rcv_cnt        = 0;
            rnic_pdata->pcs_0_an_intr_check_cnt = 0;
            rnic_pdata->pcs_0_an_success        = 0;
            rnic_pdata->pcs_0_an_nxp_index        = 0;
        
            rnic_pdata->pcs_0_krt_success        = 0;
            rnic_pdata->pcs_0_krt_failed        = 0;
        }    
        else
        {
            rnic_pdata->pcs_1_an_start            = 1;
            rnic_pdata->pcs_1_an_restart_lock    = 1;            
            rnic_pdata->pcs_1_an_rcv_cnt        = 0;
            rnic_pdata->pcs_1_an_intr_check_cnt = 0;
            rnic_pdata->pcs_1_an_success        = 0;
            rnic_pdata->pcs_1_an_nxp_index        = 0;
        
            rnic_pdata->pcs_1_krt_success        = 0;
            rnic_pdata->pcs_1_krt_failed        = 0;
        }
        
        rnic_pdata->mac_pdata->link_check_usecs = PCS_LINK_CHECK_FAST_TIMER_USECS;
    }
}


void pcs_an_pdet_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{  
#ifdef PCS_AN_PDET_EN

    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d enable parallel detection\n",port_id);    

    if( (port_id == 0 && (rnic_pdata->port_0_speed == SPEED_25G || rnic_pdata->port_0_speed == SPEED_10G)) ||
        (port_id == 1 && (rnic_pdata->port_1_speed == SPEED_25G || rnic_pdata->port_1_speed == SPEED_10G))
      )

    {
        data = pcs_reg_read(rnic_pdata,port_id,VR_AN_MODE_CTRL);
        data = set_bits(data,PDET_EN,0x1);
        pcs_reg_write(rnic_pdata,port_id,VR_AN_MODE_CTRL,data);
    }
#endif
}


void pcs_an_base_page_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data_array[3];

    RNIC_PRINTK("\tRNIC: pcs %0d write sr_an_adv 3,2,1\n",port_id);
    if(port_id == 0)
    {
        pcs_reg_write(rnic_pdata,port_id,SR_AN_ADV3,rnic_pdata->pcs_0_an_bp_3);
        pcs_reg_write(rnic_pdata,port_id,SR_AN_ADV2,rnic_pdata->pcs_0_an_bp_2);
        pcs_reg_write(rnic_pdata,port_id,SR_AN_ADV1,rnic_pdata->pcs_0_an_bp_1);
        data_array[0] = rnic_pdata->pcs_0_an_bp_1;
        data_array[1] = rnic_pdata->pcs_0_an_bp_2;
        data_array[2] = rnic_pdata->pcs_0_an_bp_3;
        //pcs_an_base_page_display(rnic_pdata,port_id,"ADV",data_array);
    }
    else
    {
        pcs_reg_write(rnic_pdata,port_id,SR_AN_ADV3,rnic_pdata->pcs_1_an_bp_3);
        pcs_reg_write(rnic_pdata,port_id,SR_AN_ADV2,rnic_pdata->pcs_1_an_bp_2);
        pcs_reg_write(rnic_pdata,port_id,SR_AN_ADV1,rnic_pdata->pcs_1_an_bp_1);

        data_array[0] = rnic_pdata->pcs_0_an_bp_1;
        data_array[1] = rnic_pdata->pcs_0_an_bp_2;
        data_array[2] = rnic_pdata->pcs_0_an_bp_3;
        pcs_an_base_page_display(rnic_pdata,port_id,"ADV",data_array);
    } 
}


void pcs_an_wait_sts(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d wait page received of sr_an_sts\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_STS);
    while(get_bits(data,6,6) == 0x0)
        data = pcs_reg_read(rnic_pdata,port_id,SR_AN_STS);
}


void pcs_an_clear_intr(struct rnic_pdata* rnic_pdata, int port_id,int index_h,int index_l)
{   
    int data;
    
    RNIC_PRINTK("\tRNIC: pcs %0d clear an interrupt\n",port_id);
    
    data = pcs_reg_read(rnic_pdata,port_id,VR_AN_INTR);
    data = set_bits(data,index_h,index_l,0x0);
    pcs_reg_write(rnic_pdata,port_id,VR_AN_INTR,data);
}
            

void pcs_an_clear_all_intr(struct rnic_pdata* rnic_pdata, int port_id)
{   

    RNIC_PRINTK("\tRNIC: pcs %0d clear all an intr\n",port_id);

    pcs_reg_write(rnic_pdata,port_id,VR_AN_INTR,0x0);
}
   

int pcs_an_read_lp_base_page(struct rnic_pdata* rnic_pdata, int port_id)
{
    int lp_xnp;
    int data[3];
    
    RNIC_PRINTK("\tRNIC: pcs %0d read lp base page\n",port_id);
    
    data[2] = pcs_reg_read(rnic_pdata,port_id,SR_AN_LP_ABL3);
    data[1] = pcs_reg_read(rnic_pdata,port_id,SR_AN_LP_ABL2);
    data[0] = pcs_reg_read(rnic_pdata,port_id,SR_AN_LP_ABL1);
    lp_xnp = get_bits(data[0],15,15);
    
    //pcs_an_base_page_display(rnic_pdata,port_id,"LP",data);
    
    return lp_xnp;
}


int pcs_an_read_lp_next_page(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int lp_xnp;
    int data[3];

    RNIC_PRINTK("\tRNIC: pcs %0d Read LP Next Page\n",port_id);
    
    data[2] = pcs_reg_read(rnic_pdata,port_id,SR_AN_LP_XNP_ABL3);
    data[1] = pcs_reg_read(rnic_pdata,port_id,SR_AN_LP_XNP_ABL2);
    data[0] = pcs_reg_read(rnic_pdata,port_id,SR_AN_LP_XNP_ABL1);
    
    lp_xnp = get_bits(data[0],15,15);
    
    //pcs_an_next_page_display(rnic_pdata,port_id,"LP_XNP",data);
    
    return lp_xnp;
}

int pcs_an_read_adv_base_page(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int adv_xnp;
    int data[3];

    RNIC_PRINTK("\tRNIC: pcs %0d Read ADV Base Page\n",port_id);
    
    data[2] = pcs_reg_read(rnic_pdata,port_id,SR_AN_ADV3);
    data[1] = pcs_reg_read(rnic_pdata,port_id,SR_AN_ADV2);
    data[0] = pcs_reg_read(rnic_pdata,port_id,SR_AN_ADV1);

    adv_xnp = get_bits(data[0],15,15);
    
    //pcs_an_base_page_display(rnic_pdata,port_id,"ADV",data);
    
    return adv_xnp;
}



int pcs_an_read_adv_next_page(struct rnic_pdata* rnic_pdata, int port_id)
{
    int adv_xnp;
    int data[3];

    RNIC_PRINTK("\tRNIC: pcs %0d Read ADV Next Page\n",port_id);
    
    data[2] = pcs_reg_read(rnic_pdata,port_id,SR_AN_XNP_TX3);
    data[1] = pcs_reg_read(rnic_pdata,port_id,SR_AN_XNP_TX2);
    data[0] = pcs_reg_read(rnic_pdata,port_id,SR_AN_XNP_TX1);

    adv_xnp = get_bits(data[0],15,15);
    
    //pcs_an_next_page_display(rnic_pdata,port_id,"ADV_XNP",data);
    
    return adv_xnp;
}


void pcs_an_timeout(struct rnic_pdata* rnic_pdata, int port_id)
{
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_timeout, disable an\n",port_id);
    
    pcs_an_disable(rnic_pdata,port_id);

    rnic_pdata->mac_pdata->link_check_usecs = PCS_LINK_CHECK_SLOW_TIMER_USECS;
}


void pcs_an_inc_link(struct rnic_pdata* rnic_pdata, int port_id)
{
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_inc_link received\n",port_id);

    pcs_an_clear_intr(rnic_pdata,port_id,AN_INC_LINK);

    pcs_an_disable(rnic_pdata,port_id);

    rnic_pdata->mac_pdata->link_check_usecs = (get_random_num() % PCS_LINK_CHECK_SLOW_TIMER_USECS) + 1;

    RNIC_PRINTK("AN failed. The Link Partner can be of any other speed mode!\n");
}


void pcs_an_int_cmpl(struct rnic_pdata* rnic_pdata, int port_id)
{
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_int_cmpl received\n",port_id);

    if(rnic_pdata->pcs_0_krt_success || rnic_pdata->pcs_0_krt_failed)
    {
        //pcs_an_check_comp_state(rnic_pdata,port_id);
        
        if(port_id == 0)
        {
            rnic_pdata->pcs_0_an_success = 1;
            rnic_pdata->pcs_0_an_start   = 0;
        }
        else
        {
            rnic_pdata->pcs_1_an_success = 1;
            rnic_pdata->pcs_1_an_start     = 0;
        }

        RNIC_PRINTK("\tRNIC: pcs %0d an success, current speed mode is OK!\n",port_id);

        pcs_an_disable(rnic_pdata,port_id);
    }
    else
        pcs_krt_start(rnic_pdata,port_id);

    pcs_an_clear_intr(rnic_pdata,port_id,AN_INT_CMPL);

    rnic_pdata->mac_pdata->link_check_usecs = PCS_LINK_CHECK_SLOW_TIMER_USECS;
}


void pcs_an_pg_rcv(struct rnic_pdata* rnic_pdata, int port_id)
{
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_pg_rcv received\n",port_id);
    
    if(!rnic_pdata->pcs_0_krt_start && !rnic_pdata->pcs_0_krt_success && !rnic_pdata->pcs_0_krt_failed)
    {
        pcs_an_nxp_check(rnic_pdata,port_id);    

        if(pcs_an_comp(rnic_pdata,port_id))
            pcs_an_check_comp_state(rnic_pdata,port_id);

        if(port_id == 0)
        {
            rnic_pdata->pcs_0_an_rcv_cnt ++;
            if(rnic_pdata->pcs_0_an_rcv_cnt >= PCS_AN_MAX_RCV_CNT)
                pcs_an_disable(rnic_pdata,port_id);
        }
        else
        {
            rnic_pdata->pcs_1_an_rcv_cnt ++;
            if(rnic_pdata->pcs_1_an_rcv_cnt >= PCS_AN_MAX_RCV_CNT)
                pcs_an_timeout(rnic_pdata,port_id);
        }
    }

    pcs_an_clear_intr(rnic_pdata,port_id,AN_PG_RCV);

    rnic_pdata->mac_pdata->link_check_usecs = PCS_LINK_CHECK_FAST_TIMER_USECS;
}


void pcs_an_nxp_check(struct rnic_pdata* rnic_pdata, int port_id)
{
    int adv_xnp;
    int lp_xnp;
    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_an_nxp_check\n",port_id);
    
    if(port_id == 0x0)
    {
        switch(rnic_pdata->pcs_0_an_nxp_index)
        {
            case 0:
                adv_xnp = pcs_an_read_adv_base_page(rnic_pdata,port_id);
                if(adv_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d ADV need next page\n",port_id);

                lp_xnp = pcs_an_read_lp_base_page(rnic_pdata,port_id);
                if(lp_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d LP ADV need next page\n",port_id);
                
                if(adv_xnp == 0x1 || lp_xnp == 0x1)
                {
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX3,rnic_pdata->pcs_0_an_nxp_3_0);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX2,rnic_pdata->pcs_0_an_nxp_2_0);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX1,rnic_pdata->pcs_0_an_nxp_1_0);
                }
                
                break;


            case 1:
                adv_xnp = pcs_an_read_adv_next_page(rnic_pdata,port_id);
                if(adv_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d ADV NEXT need next page\n",port_id);

                lp_xnp = pcs_an_read_lp_next_page(rnic_pdata,port_id);
                if(lp_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d LP NEXT need next page\n",port_id);
                
                if(adv_xnp == 0x1 || lp_xnp == 0x1)
                {
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX3,rnic_pdata->pcs_0_an_nxp_3_1);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX2,rnic_pdata->pcs_0_an_nxp_2_1);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX1,rnic_pdata->pcs_0_an_nxp_1_1);
                }
                 
                break;

                
            default:
                adv_xnp = pcs_an_read_adv_next_page(rnic_pdata,port_id);
                if(adv_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d ADV NEXT need next page\n",port_id);

                lp_xnp = pcs_an_read_lp_next_page(rnic_pdata,port_id);
                if(lp_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d LP NEXT need next page\n",port_id);
                
                if(adv_xnp == 0x1 || lp_xnp == 0x1)
                {
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX3,rnic_pdata->pcs_an_nxp_3_null);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX2,rnic_pdata->pcs_an_nxp_2_null);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX1,rnic_pdata->pcs_an_nxp_1_null);
                } 
    
                break;
        } 
        //to be done: decide whether the an is support current speed mode
        if(rnic_pdata->port_0_speed == SPEED_25G || rnic_pdata->port_0_speed == SPEED_50G)
            rnic_pdata->pcs_0_an_nxp_index = rnic_pdata->pcs_0_an_nxp_index + 1;
    }
    else
    {
        switch(rnic_pdata->pcs_1_an_nxp_index)
        {
            case 0:
                adv_xnp = pcs_an_read_adv_base_page(rnic_pdata,port_id);
                if(adv_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d ADV need next page\n",port_id);

                lp_xnp = pcs_an_read_lp_base_page(rnic_pdata,port_id);
                if(lp_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d LP ADV need next page\n",port_id);
                
                if(adv_xnp == 0x1 || lp_xnp == 0x1)
                {
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX3,rnic_pdata->pcs_1_an_nxp_3_0);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX2,rnic_pdata->pcs_1_an_nxp_2_0);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX1,rnic_pdata->pcs_1_an_nxp_1_0);
                } 
                
                break;

            case 1:
                adv_xnp = pcs_an_read_adv_next_page(rnic_pdata,port_id);
                if(adv_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d ADV NEXT need next page\n",port_id);

                lp_xnp = pcs_an_read_lp_next_page(rnic_pdata,port_id);
                if(lp_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d LP NEXT need next page\n",port_id);
                
                if(adv_xnp == 0x1 || lp_xnp == 0x1)
                {
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX3,rnic_pdata->pcs_1_an_nxp_3_1);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX2,rnic_pdata->pcs_1_an_nxp_2_1);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX1,rnic_pdata->pcs_1_an_nxp_1_1);
                } 
                
                break;

            default:
                adv_xnp = pcs_an_read_adv_next_page(rnic_pdata,port_id);
                if(adv_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d ADV NEXT need next page\n",port_id);

                lp_xnp = pcs_an_read_lp_next_page(rnic_pdata,port_id);
                if(lp_xnp == 0x1) 
                    RNIC_PRINTK("\tRNIC: pcs %0d LP NEXT need next page\n",port_id);
                
                if(adv_xnp == 0x1 || lp_xnp == 0x1)
                {
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX3,rnic_pdata->pcs_an_nxp_3_null);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX2,rnic_pdata->pcs_an_nxp_2_null);
                    pcs_reg_write(rnic_pdata,port_id,SR_AN_XNP_TX1,rnic_pdata->pcs_an_nxp_1_null);
                } 
                
                break;
        }
        
        if(rnic_pdata->port_1_speed == SPEED_25G || rnic_pdata->port_1_speed == SPEED_50G)
            rnic_pdata->pcs_1_an_nxp_index = rnic_pdata->pcs_1_an_nxp_index + 1;
    }
        
    if(adv_xnp == 0x0 && lp_xnp == 0x0 && rnic_pdata->pcs_0_krt_start == 0 && rnic_pdata->pcs_0_krt_success == 0 && rnic_pdata->pcs_0_krt_failed == 0)
   // if(adv_xnp == 0x0 && lp_xnp == 0x0 && rnic_pdata->pcs_0_krt_start == 0)
        pcs_krt_start(rnic_pdata,port_id);
}


void pcs_an_base_page_display(struct rnic_pdata* rnic_pdata, int port_id,char* name,int* data_in)
{
    printk("\tRNIC: pcs %0d %s BASE-R FEC request    : 0x%x\n",port_id,name,get_bits(data_in[2],15,15));
    printk("\tRNIC: pcs %0d %s BASE-R FEC ability    : 0x%x\n",port_id,name,get_bits(data_in[2],14,14));
    printk("\tRNIC: pcs %0d %s 25G BASE-R FEC request: 0x%x\n",port_id,name,get_bits(data_in[2],13,13));
    printk("\tRNIC: pcs %0d %s 25G RS FEC request    : 0x%x\n",port_id,name,get_bits(data_in[2],12,12));
    printk("\tRNIC: pcs %0d %s 25GBASE-KR/CR         : 0x%x\n",port_id,name,get_bits(data_in[1],15,15));
    printk("\tRNIC: pcs %0d %s 25GBASE-KR-S/CR-S     : 0x%x\n",port_id,name,get_bits(data_in[1],14,14));
    printk("\tRNIC: pcs %0d %s 100GBASE-CR4          : 0x%x\n",port_id,name,get_bits(data_in[1],13,13));
    printk("\tRNIC: pcs %0d %s 100GBASE-KR4          : 0x%x\n",port_id,name,get_bits(data_in[1],12,12));
    printk("\tRNIC: pcs %0d %s 100GBASE-KP4          : 0x%x\n",port_id,name,get_bits(data_in[1],11,11));
    printk("\tRNIC: pcs %0d %s 100GBASE-CR10         : 0x%x\n",port_id,name,get_bits(data_in[1],10,10));
    printk("\tRNIC: pcs %0d %s 40GBASE-CR4           : 0x%x\n",port_id,name,get_bits(data_in[1],9, 9 ));
    printk("\tRNIC: pcs %0d %s 40GBASE-KR4           : 0x%x\n",port_id,name,get_bits(data_in[1],8, 8 ));
    printk("\tRNIC: pcs %0d %s 10GBASE-KR            : 0x%x\n",port_id,name,get_bits(data_in[1],7, 7 ));
    printk("\tRNIC: pcs %0d %s 10GBASE-KX4           : 0x%x\n",port_id,name,get_bits(data_in[1],6, 6 ));
    printk("\tRNIC: pcs %0d %s 1GBASE-KX             : 0x%x\n",port_id,name,get_bits(data_in[1],5, 5 ));
    printk("\tRNIC: pcs %0d %s Tx Nonce Filed        : 0x%x\n",port_id,name,get_bits(data_in[1],4, 0 ));
    printk("\tRNIC: pcs %0d %s Next Page             : 0x%x\n",port_id,name,get_bits(data_in[0],15,15));
    printk("\tRNIC: pcs %0d %s Acknowledge           : 0x%x\n",port_id,name,get_bits(data_in[0],14,14));
    printk("\tRNIC: pcs %0d %s Remote Fault          : 0x%x\n",port_id,name,get_bits(data_in[0],13,13));
    printk("\tRNIC: pcs %0d %s Pause capability      : 0x%x\n",port_id,name,get_bits(data_in[0],11,10));
    printk("\tRNIC: pcs %0d %s Echoed Nonce Field    : 0x%x\n",port_id,name,get_bits(data_in[0],9, 5 ));
    printk("\tRNIC: pcs %0d %s Selector Field        : 0x%x\n",port_id,name,get_bits(data_in[0],4, 0 ));
}


void pcs_an_next_page_display(struct rnic_pdata* rnic_pdata, int port_id,char* name,int* data_in) 
{
    printk("\tRNIC: pcs %0d %s Next Page     : 0x%x\n",port_id,name,get_bits(data_in[0],15,15));
    printk("\tRNIC: pcs %0d %s Acknowledge   : 0x%x\n",port_id,name,get_bits(data_in[0],14,14));
    printk("\tRNIC: pcs %0d %s Formatted     : 0x%x\n",port_id,name,get_bits(data_in[0],13,13));
    printk("\tRNIC: pcs %0d %s Acknowledge 2 : 0x%x\n",port_id,name,get_bits(data_in[0],12,12));
    printk("\tRNIC: pcs %0d %s Toggle bit    : 0x%x\n",port_id,name,get_bits(data_in[0],11,11));
    
    if(get_bits(data_in[0],13,13) == 0x1 && get_bits(data_in[0],10,0) == 0x1)
        printk("\tRNIC: pcs %0d NULL Next Page\n",port_id);

    if(get_bits(data_in[0],13,13) == 0x1 && get_bits(data_in[0],10,0) == 0x5 && (get_bits(data_in[1],10,0)<<14 | get_bits(data_in[2],10,0) << 2 | 0x1) == 0x6A737D)
        printk("\tRNIC: pcs %0d 25G Ethernet Consortium Next Page\n",port_id);
        
    if(get_bits(data_in[0],13,13) != 0x1 && get_bits(data_in[0],1,0) == 0x3 && get_bits(data_in[0],10,9) == 0x1)
    {
        printk("\tRNIC: pcs %0d 25G Ethernet Consortium Extended Next Page\n",port_id); 
        
        printk("\tRNIC: pcs %0d %s 50GBASE-CR2 : 0x%x\n",port_id,name,get_bits(data_in[1],9, 9));
        printk("\tRNIC: pcs %0d %s 50GBASE-KR2 : 0x%x\n",port_id,name,get_bits(data_in[1],8, 8));        
        printk("\tRNIC: pcs %0d %s 25GBASE-CR1 : 0x%x\n",port_id,name,get_bits(data_in[1],5, 5));
        printk("\tRNIC: pcs %0d %s 25GBASE-KR1 : 0x%x\n",port_id,name,get_bits(data_in[1],4, 4));
        printk("\tRNIC: pcs %0d %s FEC Control : 0x%x\n",port_id,name,get_bits(data_in[2],11,8));            
    }
}


int pcs_an_check_comp_state(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d SR_AN_COMP_STS\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_COMP_STS);

    RNIC_PRINTK("\tRNIC: pcs %0d SR_AN_COMP_STS is %x\n",port_id,data); 

    return data;
}


int pcs_an_print_state(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d SR_AN_STS\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_STS);

    data = get_bits(data,5,5);

    RNIC_PRINTK("\tRNIC: pcs %0d SR_AN_STS is %x\n",port_id,data); 

    return data;
}


int pcs_an_comp(struct rnic_pdata* rnic_pdata, int port_id)
{   
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d SR_AN_STS\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,SR_AN_STS);

    data = get_bits(data,5,5);

    return data;
}



//KRT
void pcs_krt_cfg(struct rnic_pdata* rnic_pdata, int port_id)
{   
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_krt_cfg\n",port_id);

    pcs_krt_cl72_enable(rnic_pdata,port_id); 
}


void pcs_krt_disable(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;

    RNIC_PRINTK("\tRNIC: pcs %0d pcs_krt_disable\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,SR_PMA_KR_PMD_CTRL);
    data = set_bits(data,1,1,0x0);//[1]TR_EN,0:Disable the Link training start-up protocol
    pcs_reg_write(rnic_pdata,port_id,SR_PMA_KR_PMD_CTRL,data);
}


void pcs_krt_enable(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_krt_enable\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,SR_PMA_KR_PMD_CTRL);
    data = set_bits(data,1,1,0x1);
    pcs_reg_write(rnic_pdata,port_id,SR_PMA_KR_PMD_CTRL,data);
}


void pcs_krt_restart(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_krt_restart\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,SR_PMA_KR_PMD_CTRL);
    data = set_bits(data,0,0,0x1);
    pcs_reg_write(rnic_pdata,port_id,SR_PMA_KR_PMD_CTRL,data);
}

void pcs_krt_cl72_enable(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_krt_on\n",port_id);

    data = pcs_reg_read(rnic_pdata,port_id,VR_PCS_DIG_CTRL3);
    data = set_bits(data,2,2,0x1);
    pcs_reg_write(rnic_pdata,port_id,VR_PCS_DIG_CTRL3,data);
}


void pcs_krt_start(struct rnic_pdata* rnic_pdata, int port_id)
{    
    int krt_start = 0;

    if(port_id == 0)
    {
        if(rnic_pdata->pcs_0_krt_start == 0)
        {
            if(rnic_pdata->pcs_0_krt_en)
            {
                rnic_pdata->pcs_0_krt_start = 1;
                krt_start = 1;
            }
            else
                rnic_pdata->pcs_0_krt_failed = 1;
        }
    }
    else
    {
        if(rnic_pdata->pcs_1_krt_start == 0)
        {
            if(rnic_pdata->pcs_1_krt_en)
            {
                rnic_pdata->pcs_1_krt_start = 1;
                krt_start = 1;
            }
            else
                rnic_pdata->pcs_1_krt_failed = 1;
        }
    }

    if(krt_start)
    {
        RNIC_PRINTK("\tRNIC: pcs %0d start kr training\n",port_id);        
        pcs_krt_enable(rnic_pdata,port_id);
        pcs_krt_restart(rnic_pdata,port_id);
    }
}


void pcs_krt_check_state(struct rnic_pdata* rnic_pdata, int port_id)
{
    int data;
    int krt_failed;
    int krt_success;
    int krt_lane_mask;
    
    RNIC_PRINTK("\tRNIC: pcs %0d pcs_krt_check_state\n",port_id);

    krt_failed = 0;
    krt_success = 0;
    
    krt_lane_mask = port_id ? rnic_pdata->pcs_1_krt_lane_mask : rnic_pdata->pcs_0_krt_lane_mask;

    data = pcs_reg_read(rnic_pdata,port_id,SR_PMA_KR_PMD_STS);
    
    if(  
        ((get_bits(krt_lane_mask,0,0) == 0x1) && (get_bits(data,3, 0)  == 0x1))    || 
        ((get_bits(krt_lane_mask,1,1) == 0x1) && (get_bits(data,7, 4)  == 0x1))    || 
        ((get_bits(krt_lane_mask,2,2) == 0x1) && (get_bits(data,11,8)  == 0x1))    || 
        ((get_bits(krt_lane_mask,3,3) == 0x1) && (get_bits(data,15,12) == 0x1))
    )
        krt_success = 1;
    
    if( 
        ((get_bits(krt_lane_mask,0,0) == 0x1) && (get_bits(data,3 ,3)   == 0x1))    || 
        ((get_bits(krt_lane_mask,1,1) == 0x1) && (get_bits(data,7 ,7)   == 0x1))    || 
        ((get_bits(krt_lane_mask,2,2) == 0x1) && (get_bits(data,11,11)  == 0x1))    || 
        ((get_bits(krt_lane_mask,3,3) == 0x1) && (get_bits(data,15,15)  == 0x1))
      )
        krt_failed = 1; 
    

    if(krt_failed)
    {
        if(port_id == 0)
            rnic_pdata->pcs_0_krt_failed = 1;
        else
            rnic_pdata->pcs_1_krt_failed = 1;
        
        RNIC_PRINTK("\tRNIC: pcs %0d kr training failed\n",port_id);
    }
    
    if(krt_success)
    {
        if(port_id == 0)
            rnic_pdata->pcs_0_krt_success = 1;
        else
            rnic_pdata->pcs_1_krt_success = 1;
        
        RNIC_PRINTK("\tRNIC: pcs %0d kr training success\n",port_id);
    }

    if(!krt_failed && !krt_success)
    {
        //RNIC_PRINTK("\tRNIC: pcs %0d current krt state is %x\n",port_id,data);
        rnic_pdata->mac_pdata->link_check_usecs = PCS_LINK_CHECK_FAST_TIMER_USECS;
    }
    else
    {
        if(port_id == 0)
            rnic_pdata->pcs_0_krt_start = 0;
        else
            rnic_pdata->pcs_1_krt_start = 0;

        rnic_pdata->mac_pdata->link_check_usecs = PCS_LINK_CHECK_SLOW_TIMER_USECS;

        //pcs_an_disable(rnic_pdata,0);
        //rnic_pdata->pcs_0_an_success = 1;
    }
}
