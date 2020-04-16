//insomnia@2019/12/15 17:01:49

#include "header/bx_rnic.h"


void mpp_init(struct rnic_pdata*rnic_pdata)
{
    RNIC_PRINTK("RNIC: mpp_init start\n");
            
    #ifdef PHY_SRAM_BYPASS
        mpp_phy_sram_bypass(rnic_pdata);
    #endif
    
    mpu_init(rnic_pdata);

    #ifdef PORT_0_EN
        mpp_phy_ctl_sel(rnic_pdata,0);
        mpp_enable_port(rnic_pdata,0);
        mpp_port_rst_n_set(rnic_pdata,0);
        mpp_port_rst_n_release(rnic_pdata,0);
       
        pcs_init(rnic_pdata,0);
    #endif     
    
    #ifdef PORT_1_EN
        mpp_phy_ctl_sel(rnic_pdata,1);
        mpp_enable_port(rnic_pdata,1);
        mpp_port_rst_n_set(rnic_pdata,1);
        mpp_port_rst_n_release(rnic_pdata,1);
    
        pcs_init(rnic_pdata,1);
    #endif    

    #ifdef PORT_0_EN
        mpp_phy_ctl_sel(rnic_pdata,0);    
    #endif
    
    RNIC_PRINTK("RNIC: mpp_init done\n");
}


void mpp_mask_set(struct rnic_pdata*rnic_pdata,int data)
{
    //RNIC_PRINTK("    RNIC: mpp_mask_set \n");
    mpp_mask_pkey_set(rnic_pdata);
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_MPP_MASK,data);
    mpp_mask_pkey_clear(rnic_pdata);
}


void mpp_mask_pkey_set(struct rnic_pdata*rnic_pdata)
{
    //RNIC_PRINTK("    RNIC: mpp_mask_pkey_set\n");
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_MPP_MASK_PKEY,0x55667788);
}


void mpp_mask_pkey_clear(struct rnic_pdata*rnic_pdata)
{
    //RNIC_PRINTK("    RNIC: mpp_mask_pkey_clear\n");
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_MPP_MASK_PKEY,0x0);
}


void mpp_enable_port_mask(struct rnic_pdata*rnic_pdata,int pcs_id)
{
    int data;
    
    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_MPP_MASK);
    
    if(pcs_id == 0x0)
    {
        //RNIC_PRINTK("    RNIC: mpp_enable_port_0_mask\n"); 
        data = set_bits(data,1,0,0x3);       
    }
    else
    {
        //RNIC_PRINTK("    RNIC: mpp_enable_port_1_mask\n"); 
        data = set_bits(data,3,2,0x3);       
    }        
    
    mpp_mask_set(rnic_pdata,data);
}


void mpp_phy_sram_bypass(struct rnic_pdata*rnic_pdata)
{
    int data;

    //RNIC_PRINTK("    RNIC: mpp_phy_sram_bypass\n");
    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_PHY_PARAM);
    data = set_bits(data,PHY_PARAM_RANGE_SRAM_BYPASS,0x1);
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_PHY_PARAM,data);
}

void mpp_port_rst_n_set(struct rnic_pdata*rnic_pdata,int pcs_id)
{
    int data;
    //RNIC_PRINTK("    RNIC: mpp_port_rst_n_set %d\n",pcs_id);
    
    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_MPP_PARAM);
    
    if(pcs_id == 0x0)
        data = set_bits(data,MPP_PARMA_RANGE_PORT_0_RST_N,0x0);     
    else
        data = set_bits(data,MPP_PARMA_RANGE_PORT_1_RST_N,0x0);        
    
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_MPP_PARAM,data);
}

void mpp_port_rst_n_release(struct rnic_pdata*rnic_pdata,int pcs_id)
{
    int data;
    //RNIC_PRINTK("    RNIC: mpp_port_rst_n_release %d\n",pcs_id);

    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_MPP_PARAM);

    if(pcs_id == 0x0)    
        data = set_bits(data,MPP_PARMA_RANGE_PORT_0_RST_N,0x1);     
    else
        data = set_bits(data,MPP_PARMA_RANGE_PORT_1_RST_N,0x1);        
        
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_MPP_PARAM,data);
}

void mpp_set_phy_pg_rst(struct rnic_pdata*rnic_pdata,int pcs_id)
{
    int data;
    //RNIC_PRINTK("    RNIC: mpp_set_phy_pg_rst %d\n",pcs_id);
    
    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_PHY_PARAM);
    
    if(pcs_id == 0x0)
        data = set_bits(data,PHY_PARAM_RANGE_PG_RESET,0x1);     
    else
        data = set_bits(data,PHY_PARAM_RANGE_PG_RESET,0x1);        
    
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_PHY_PARAM,data);
}


void mpp_release_phy_pg_rst(struct rnic_pdata*rnic_pdata,int pcs_id)
{
    int data;
    //RNIC_PRINTK("    RNIC: mpp_release_phy_pg_rst %d\n",pcs_id);
    
    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_PHY_PARAM);
    
    if(pcs_id == 0x0)
        data = set_bits(data,PHY_PARAM_RANGE_PG_RESET,0x0);     
    else
        data = set_bits(data,PHY_PARAM_RANGE_PG_RESET,0x0);        
    
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_PHY_PARAM,data);
}



void mpp_enable_port(struct rnic_pdata*rnic_pdata,int pcs_id)
{
    int data;
    
    //RNIC_PRINTK("    RNIC: mpp_enable_port %d\n",pcs_id); 
    
    mpp_enable_port_mask(rnic_pdata,pcs_id);

    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_MPP_PARAM);

    if(pcs_id == 0x0)
        data = set_bits(data,MPP_PARMA_RANGE_PORT_0_EN,0x1);    
    else
        data = set_bits(data,MPP_PARMA_RANGE_PORT_1_EN,0x1);    

    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_MPP_PARAM,data);
}


void mpp_disable_port(struct rnic_pdata*rnic_pdata,int pcs_id)
{
    int data;
    
    //RNIC_PRINTK("    RNIC: mpp_disable_port %d\n",pcs_id); 
    
    mpp_enable_port_mask(rnic_pdata,pcs_id);

    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_MPP_PARAM);

    if(pcs_id == 0x0)
        data = set_bits(data,MPP_PARMA_RANGE_PORT_0_EN,0x0);    
    else
        data = set_bits(data,MPP_PARMA_RANGE_PORT_1_EN,0x0);    

    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_MPP_PARAM,data);
}


void mpp_phy_ctl_sel(struct rnic_pdata*rnic_pdata,int pcs_id)
{
    int data;
    
    //RNIC_PRINTK("    RNIC: mpp_phy_ctl_sel %d\n",pcs_id);
    
    data = mpp_reg_read(rnic_pdata,MPP_REG_ADDR_MPP_PARAM);
    data = set_bits(data,MPP_PARMA_RANGE_PHY_CTL_SEL,pcs_id);
    mpp_reg_write(rnic_pdata,MPP_REG_ADDR_MPP_PARAM,data);
}

void mpp_reg_write(struct rnic_pdata*rnic_pdata,int offset,int data)
{
    int addr;

    addr = RNIC_REG_BASE_ADDR_MPP + offset;
    reg_write(rnic_pdata,addr,data);
 
    #ifdef REG_DEBUG
        //RNIC_PRINTK("    RNIC: mpp reg write:      addr=%0x      data=%0x\n",offset,data);
    #endif
}


int mpp_reg_read(struct rnic_pdata*rnic_pdata,int offset)
{
    int addr;
    int data;
    
    addr = RNIC_REG_BASE_ADDR_MPP + offset;
    data = reg_read(rnic_pdata,addr);
 
    #ifdef REG_DEBUG
        //RNIC_PRINTK("    RNIC: mpp reg read:      addr=%0x      data=%0x\n",offset,data);
    #endif
   
    return data;  
}
