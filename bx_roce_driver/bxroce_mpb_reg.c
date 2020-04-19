#include "header/bxroce.h"


void bxroce_mpb_reg_write(void __iomem *base_addr,int module_addr,int offset,int wdata)
{
    int addr;
    int data;
    
    addr = RNIC_REG_BASE_ADDR_MPB + MPB_STEP_ADDR_OFFSET;
    data = module_addr + offset;
    writel(data,base_addr + addr);
    
    addr = RNIC_REG_BASE_ADDR_MPB + MPB_STEP_DATA_OFFSET;
    data = wdata;
    writel(wdata,base_addr + addr);
}


int bxroce_mpb_reg_read(void __iomem *base_addr,int module_addr,int offset)
{
    int addr;
    int data;
    
    addr = RNIC_REG_BASE_ADDR_MPB + MPB_STEP_ADDR_OFFSET;
    data = module_addr + offset;
    writel(data,base_addr + addr);
    
    addr = RNIC_REG_BASE_ADDR_MPB + MPB_STEP_DATA_OFFSET;
    data = readl(base_addr + addr);

    return data;
}