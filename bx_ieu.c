//insomnia@2019/12/11 11:40:07

#include "header/bx_rnic.h"


void ieu_init(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("RNIC: ieu_init start\n");

    ieu_hpb_csr_err_evt_int_en(rnic_pdata);

    ieu_hpb_csr_misc_int_en(rnic_pdata);

#ifdef RNIC_MSI_EN
    ieu_msi_cfg(rnic_pdata);
#endif

    ieu_int_direction_cfg(rnic_pdata);

    ieu_enable_intr_tx_rx_all(rnic_pdata);

    RNIC_PRINTK("RNIC: ieu_init done\n");
}

void ieu_report_pending_intr(struct rnic_pdata * rnic_pdata)
{
    printk("ieu pending_intr is %x\n",ieu_reg_read(rnic_pdata,0x0c20));
}


void ieu_msi_cfg(struct rnic_pdata * rnic_pdata)
{
    int data;
    
    //IEU_C0_MSI_MASK
    ieu_reg_write(rnic_pdata,0x0c50,0xffffffff);

    //IEU_C0_MSI_CTRL
    if(rnic_pdata->msi_irq_cnt == 1)
           ieu_reg_write(rnic_pdata,0x0c40,0x01dbb005);
    else
        ieu_reg_write(rnic_pdata,0x0c40,0x01dbb004);

    //IEU_C0_MSI_LOW_ADDR
    data =  pcie_reg_read(rnic_pdata, 0, 0x94);
    ieu_reg_write(rnic_pdata,0x0c44,data);    

    //IEU_C0_MSI_HIGH_ADDR
    data =  pcie_reg_read(rnic_pdata, 0, 0x98);
    ieu_reg_write(rnic_pdata,0x0c48,data);

    //IEU_C0_MSI_DATA
    data =  pcie_reg_read(rnic_pdata, 0, 0x9c);
    ieu_reg_write(rnic_pdata,0x0c4c,data);    



    //IEU_C1_MSI_MASK
    ieu_reg_write(rnic_pdata,0x0c70,0xffffffff);

    //IEU_C1_MSI_CTRL
    if(rnic_pdata->msi_irq_cnt == 1)
           ieu_reg_write(rnic_pdata,0x0c60,0x01dbb005);
    else
        ieu_reg_write(rnic_pdata,0x0c60,0x01dbb004);

    //IEU_C1_MSI_LOW_ADDR
    data =  pcie_reg_read(rnic_pdata, 1, 0x94);
    ieu_reg_write(rnic_pdata,0x0c64,data);

    //IEU_C1_MSI_HIGH_ADDR
    data =  pcie_reg_read(rnic_pdata, 1, 0x98);
    ieu_reg_write(rnic_pdata,0x0c68,data);

    //IEU_C1_MSI_DATA
    data =  pcie_reg_read(rnic_pdata, 1, 0x9c);
    ieu_reg_write(rnic_pdata,0x0c6c,data);
}


void ieu_hpb_csr_err_evt_int_en(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_hpb_csr_err_evt_int_en\n");

    ieu_reg_write(rnic_pdata,0x00c8,0xffffffff);
}


void ieu_hpb_csr_misc_int_en(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_hpb_csr_misc_int_en\n");

    ieu_reg_write(rnic_pdata,0x000c,0xf);
}



void ieu_int_direction_cfg(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_int_direction_cfg\n");

    ieu_reg_write(rnic_pdata,0x0c74,0x0);
}


void ieu_clear_intr_all(struct rnic_pdata * rnic_pdata)
{
    //RNIC_PRINTK("\tRNIC: ieu_clear_intr_all\n");

    ieu_reg_write(rnic_pdata,0x0c28,0xffffffff);
}


void ieu_clear_intr_one(struct rnic_pdata * rnic_pdata, int index)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: ieu_clear_intr_one %d\n",index);

    data = ieu_reg_read(rnic_pdata,0x0c28);
    data = set_bits(data,index,index,1);
    ieu_reg_write(rnic_pdata,0x0c28,data);
}


void ieu_disable_intr_one(struct rnic_pdata * rnic_pdata, int index)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: ieu_disable_intr_one %d\n",index);

    data = ieu_reg_read(rnic_pdata,0x0c80);
    data = set_bits(data,index,index,0);
    ieu_reg_write(rnic_pdata,0x0c80,data);
}


void ieu_enable_intr_one(struct rnic_pdata * rnic_pdata, int index)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: ieu_enable_intr_one %d\n",index);;

    ieu_clear_intr_one(rnic_pdata,index);

    data = ieu_reg_read(rnic_pdata,0x0c80);
    data = set_bits(data,index,index,1);
    ieu_reg_write(rnic_pdata,0x0c80,data);
}


void ieu_clear_intr_tx_one(struct rnic_pdata * rnic_pdata, int index)
{
    RNIC_PRINTK("\tRNIC: ieu_clear_intr_tx_one %d\n",index);

#if 1
    
    ieu_clear_intr_one(rnic_pdata,index);

    
#else
    int data;
        data = ieu_reg_read(rnic_pdata,0x0c28);
    data = set_bits(data,index,index,1);
    data = set_bits(data,index+7,index+7,1);
    ieu_reg_write(rnic_pdata,0x0c28,data);
#endif    
}


void ieu_disable_intr_tx_one(struct rnic_pdata * rnic_pdata, int index)
{
    RNIC_PRINTK("\tRNIC: ieu_disable_intr_tx_one %d\n",index);

    ieu_disable_intr_one(rnic_pdata,index);
}


void ieu_enable_intr_tx_one(struct rnic_pdata * rnic_pdata, int index)
{
    RNIC_PRINTK("\tRNIC: ieu_enable_intr_tx_one %d\n",index);

    ieu_enable_intr_one(rnic_pdata,index);
}


void ieu_clear_intr_rx_one(struct rnic_pdata * rnic_pdata, int index)
{    
    RNIC_PRINTK("\tRNIC: ieu_clear_intr_rx_one %d\n",index);

    ieu_clear_intr_one(rnic_pdata,index+7);
}


void ieu_disable_intr_rx_one(struct rnic_pdata * rnic_pdata, int index)
{    
    RNIC_PRINTK("\tRNIC: ieu_disable_intr_rx_one %d\n",index);

    ieu_disable_intr_one(rnic_pdata,index+7);
}


void ieu_enable_intr_rx_one(struct rnic_pdata * rnic_pdata, int index)
{
    RNIC_PRINTK("\tRNIC: ieu_enable_intr_rx_one %d\n",index);

    ieu_enable_intr_one(rnic_pdata,index+7);
}


void ieu_clear_intr_tx_all(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_clear_intr_tx_all\n");

    ieu_reg_write(rnic_pdata,0x0c28,0x7f);
}


void ieu_clear_intr_rx_all(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_clear_intr_rx_all\n");

    ieu_reg_write(rnic_pdata,0x0c28,0x3f80);
}


void ieu_disable_intr_all(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_disable_intr_all\n");

    ieu_reg_write(rnic_pdata,0x0c80,0x0); 

    ieu_reg_write(rnic_pdata,0x0c84,0x0);
}


void ieu_enable_intr_all(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_disable_intr_all\n");

    ieu_reg_write(rnic_pdata,0x0c80,0xffffffff); 

    ieu_reg_write(rnic_pdata,0x0c84,0xffffffff);

    ieu_clear_intr_all(rnic_pdata);
}


void ieu_disable_intr_tx_all(struct rnic_pdata * rnic_pdata)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: ieu_disable_intr_tx_all\n");

    data = ieu_reg_read(rnic_pdata,0x0c80); 
    data = set_bits(data,6,0,0x0);
    ieu_reg_write(rnic_pdata,0x0c80,data); 
}


void ieu_disable_intr_rx_all(struct rnic_pdata * rnic_pdata)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: ieu_disable_intr_rx_all\n");

    data = ieu_reg_read(rnic_pdata,0x0c80); 
    data = set_bits(data,13,7,0x0);
    ieu_reg_write(rnic_pdata,0x0c80,data); 
}


void ieu_disable_intr_tx_rx_all(struct rnic_pdata * rnic_pdata)
{    
    RNIC_PRINTK("\tRNIC: ieu_disable_intr_tx_rx_all\n");

    ieu_disable_intr_tx_all(rnic_pdata);
    ieu_disable_intr_rx_all(rnic_pdata);
}


void ieu_clear_intr_tx_rx_all(struct rnic_pdata * rnic_pdata)
{    
    RNIC_PRINTK("\tRNIC: ieu_clear_intr_tx_rx_all\n");

    ieu_clear_intr_tx_all(rnic_pdata);
    ieu_clear_intr_rx_all(rnic_pdata);
}


void ieu_enable_intr_tx_rx_all(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("\tRNIC: ieu_enable_intr_tx_rx_all\n");

    ieu_clear_intr_all(rnic_pdata);

#ifdef RNIC_MSI_EN
    ieu_reg_write(rnic_pdata,0x0c80,0x3fff); //mac0 tx rx only
#endif

#ifdef RNIC_LEGACY_INT_EN
    ieu_reg_write(rnic_pdata,0x0c84,0x1); //sbd only
#endif
}



void ieu_enable_intr_tx_all(struct rnic_pdata * rnic_pdata)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: ieu_enable_intr_tx_all\n");

    ieu_clear_intr_tx_all(rnic_pdata);

    data = ieu_reg_read(rnic_pdata,0x0c80); 
    data = set_bits(data,6,0,0x7f);
    ieu_reg_write(rnic_pdata,0x0c80,data);
}


void ieu_enable_intr_rx_all(struct rnic_pdata * rnic_pdata)
{
    int data;
    
    RNIC_PRINTK("\tRNIC: ieu_enable_intr_rx_all\n");

    ieu_clear_intr_rx_all(rnic_pdata);

    data = ieu_reg_read(rnic_pdata,0x0c80); 
    data = set_bits(data,13,7,0x7f);
    ieu_reg_write(rnic_pdata,0x0c80,data);
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
