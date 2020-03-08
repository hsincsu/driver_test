//insomnia@2019/12/17 8:55:49

#include "header/bx_rnic.h"


void mpb_init(struct rnic_pdata*rnic_pdata)
{
    RNIC_PRINTK("RNIC: mpb_init start\n");
    cm_init(rnic_pdata);
    //pbu_init(rnic_pdata);
    phd_init(rnic_pdata,0);
    phd_init(rnic_pdata,1);
    RNIC_PRINTK("RNIC: mpb_init done\n");   
}


void mpb_reg_write(struct rnic_pdata*rnic_pdata,int module_addr,int offset,int wdata)
{
    int addr;
    int data;
    
    addr = RNIC_REG_BASE_ADDR_MPB + MPB_STEP_ADDR_OFFSET;
    data = module_addr + offset;
    reg_write(rnic_pdata,addr,data);
    
    addr = RNIC_REG_BASE_ADDR_MPB + MPB_STEP_DATA_OFFSET;
    data = wdata;
    reg_write(rnic_pdata,addr,data); 
}


int mpb_reg_read(struct rnic_pdata*rnic_pdata,int module_addr,int offset)
{
    int addr;
    int data;
    
    addr = RNIC_REG_BASE_ADDR_MPB + MPB_STEP_ADDR_OFFSET;
    data = module_addr + offset;
    reg_write(rnic_pdata,addr,data);
    
    addr = RNIC_REG_BASE_ADDR_MPB + MPB_STEP_DATA_OFFSET;
    data = reg_read(rnic_pdata,addr); 

    return data;
}
