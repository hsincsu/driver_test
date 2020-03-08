//insomnia@2019/12/11 11:40:07

#include "header/bx_rnic.h"


void ieu_init(struct rnic_pdata * rnic_pdata)
{
#ifdef RNIC_MSI_EN
	int data;
#endif

    RNIC_PRINTK("RNIC: ieu_init start\n");

    //PEU_HPB_CSR_ERR_EVT_INT_EN
    ieu_reg_write(rnic_pdata,0x00c8,0xffffffff);

    //PEU_HPB_CSR_MISC_INT_EN
    ieu_reg_write(rnic_pdata,0x000c,0xf);

#ifdef RNIC_MSI_EN
    //PEU_C0_MSI_MASK
    ieu_reg_write(rnic_pdata,0x0c50,0xffffffff);

    //PEU_C0_MSI_CTRL
    ieu_reg_write(rnic_pdata,0x0c40,0x01dbb004);

    //PEU_C0_MSI_LOW_ADDR
    data =  pcie_reg_read(rnic_pdata, 0, 0x94);
    ieu_reg_write(rnic_pdata,0x0c44,data);	

    //PEU_C0_MSI_HIGH_ADDR
	data =  pcie_reg_read(rnic_pdata, 0, 0x98);
    ieu_reg_write(rnic_pdata,0x0c48,data);

    //PEU_C0_MSI_DATA
    data =  pcie_reg_read(rnic_pdata, 0, 0x9c);
    ieu_reg_write(rnic_pdata,0x0c4c,data);	



    //PEU_C1_MSI_MASK
    ieu_reg_write(rnic_pdata,0x0c70,0xffffffff);

    //PEU_C1_MSI_CTRL
    ieu_reg_write(rnic_pdata,0x0c60,0x01dbb004);

    //PEU_C1_MSI_LOW_ADDR
    data =  pcie_reg_read(rnic_pdata, 1, 0x94);
    ieu_reg_write(rnic_pdata,0x0c64,data);

    //PEU_C1_MSI_HIGH_ADDR
    data =  pcie_reg_read(rnic_pdata, 1, 0x98);
    ieu_reg_write(rnic_pdata,0x0c68,data);

    //PEU_C1_MSI_DATA
    data =  pcie_reg_read(rnic_pdata, 1, 0x9c);
    ieu_reg_write(rnic_pdata,0x0c6c,data);

#endif
    //PEU_INT_DIRECTION
    ieu_reg_write(rnic_pdata,0x0c74,0x0);

    ieu_enable_intr(rnic_pdata);

    RNIC_PRINTK("RNIC: ieu_init done\n");
}

void ieu_report_pending_intr(struct rnic_pdata * rnic_pdata)
{
    
    RNIC_PRINTK("ieu pending_intr    is %x\n",ieu_reg_read(rnic_pdata,0x0c20));
}


void ieu_clear_intr(struct rnic_pdata * rnic_pdata)
{
    ieu_reg_write(rnic_pdata,0x0c28,0xffffffff);
}


void ieu_disable_intr(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_disable_intr\n");

    ieu_reg_write(rnic_pdata,0x0c80,0x0); 

    ieu_reg_write(rnic_pdata,0x0c84,0x0);
}


void ieu_enable_intr(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_enable_intr\n");

#ifdef RNIC_MSI_EN    
    ieu_reg_write(rnic_pdata,0x0c80,0x3fff); //mac0 ri only
#endif

#ifdef RNIC_LEGACY_INT_EN
    ieu_reg_write(rnic_pdata,0x0c84,0x1); //sbd only
#endif

	ieu_clear_intr(rnic_pdata);
}



void ieu_reg_write(struct rnic_pdata * rnic_pdata,int offset,int data)
{
    int addr;
    
    addr = RNIC_REG_BASE_ADDR_IEU + offset;
    reg_write(rnic_pdata,addr,data);  
}


int ieu_reg_read(struct rnic_pdata * rnic_pdata,int offset)
{
    int addr;
    int data;
    
    addr = RNIC_REG_BASE_ADDR_IEU + offset;
    data = reg_read(rnic_pdata,addr);

    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: ieu reg write: \t addr=%0x \t data=%0x\n",offset,data);
    #endif
    
    return data;  
}
