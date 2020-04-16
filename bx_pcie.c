//insomnia@2020/1/1 1:48:01

#include "header/bx_rnic.h"

void pcie_init(struct rnic_pdata*rnic_pdata)
{
    RNIC_PRINTK("RNIC: pcie_init start\n");
    pcie_clear_link_down(rnic_pdata);
    //pcie_set_max_payload_size(rnic_pdata);
    //pcie_set_max_request_size(rnic_pdata);
    //pcie_reg_test(rnic_pdata);
    RNIC_PRINTK("RNIC: pcie_init done\n");
}

void pcie_reg_test(struct rnic_pdata*rnic_pdata)
{
    int data;
    printk("RNIC: pcie_reg_test\n");
    data = pcie_reg_read(rnic_pdata,0,0x1002c0);
    printk("pcie 0x1002c0 is %d\n",data);
    pcie_reg_write(rnic_pdata,0,0x1002c0,0x0);
    data = pcie_reg_read(rnic_pdata,0,0x1002c0);
    printk("pcie 0x1002c0 is %d\n",data);
}



void pcie_clear_link_down(struct rnic_pdata*rnic_pdata)
{
    RNIC_PRINTK("RNIC: pcie_clear_link_down\n");
    pcie_reg_write(rnic_pdata,0,0x400824,0x0);
}

void pcie_set_max_payload_size(struct rnic_pdata*rnic_pdata)
{
    int data;
    
    RNIC_PRINTK("RNIC: pcie_set_max_payload_size\n");
    data = pcie_reg_read(rnic_pdata,0,0xc8);
    data = set_bits(data,7,5,0x2);
    pcie_reg_write(rnic_pdata,0,0xc8,data);
}


int pcie_get_max_payload_size(struct rnic_pdata*rnic_pdata)
{
    int data;
    
    RNIC_PRINTK("RNIC: pcie_get_max_payload_size\n");
    data = pcie_reg_read(rnic_pdata,0,0xc8);
    data = get_bits(data,7,5);

    printk("pcie_get_max_payload_size is %x\n",data);

    return data;
}


void pcie_set_max_request_size(struct rnic_pdata*rnic_pdata)
{
    int data;
    
    RNIC_PRINTK("RNIC: pcie_set_max_request_size\n");
    data = pcie_reg_read(rnic_pdata,0,0xc8);
    data = set_bits(data,14,12,0x3);
    pcie_reg_write(rnic_pdata,0,0xc8,data);
}



void pcie_print_all_reg (struct rnic_pdata*rnic_pdata)
{
    int rdata;
    int pcie_id;
    int addr;
    int bit_11_2;
    int bit_22;
    int bit_20;

    RNIC_PRINTK("\tRNIC:  pcie_print_all_reg start\n");
    

    addr = 0;
    for(pcie_id=0; pcie_id<=0;pcie_id++)
    {
        for(bit_22=0; bit_22<=0x1; bit_22++)
        {
            for(bit_20=0; bit_20<=0x1; bit_20++)
            {
                if(bit_22==0 || bit_20==0)
                {
                    addr = set_bits(addr,22,22,bit_22);
                    addr = set_bits(addr,20,20,bit_20);
                    for(bit_11_2 = 0; bit_11_2<=0x3ff; bit_11_2++)
                    {
                        addr = set_bits(addr,11,2,bit_11_2);
                        {
                            rdata = pcie_reg_read(rnic_pdata,pcie_id, addr);
                            if(rdata!=0)
                                printk(" \tpcie_%d: addr(%0x) rdata(%0x)\n",pcie_id,addr,rdata);
                        } 
                    }
                }
            }
        }
        printk("\n\n");
    }

    //pcie_reg_write(rnic_pdata,0x110,0x1001);
    //pcie_reg_write(rnic_pdata,0x308,0x240);
 
    RNIC_PRINTK("\tRNIC:  pcie_print_all_reg done\n");
}

void pcie_reg_write(struct rnic_pdata * rnic_pdata,int pcie_id, int offset, int data)
{
    int addr;
    int rnic_reg_base_addr_pcie;
    
    if(pcie_id==1)
        rnic_reg_base_addr_pcie = RNIC_REG_BASE_ADDR_PCIE_1;
    else if(pcie_id==2) 
        rnic_reg_base_addr_pcie = RNIC_REG_BASE_ADDR_PCIE_2;
    else
        rnic_reg_base_addr_pcie = RNIC_REG_BASE_ADDR_PCIE_0;
                
    addr = rnic_reg_base_addr_pcie + offset;
    
    reg_write(rnic_pdata,addr,data);  
}


int pcie_reg_read(struct rnic_pdata * rnic_pdata,int pcie_id, int offset)
{
    int addr;
    int data;
    int rnic_reg_base_addr_pcie;
    
    
    if(pcie_id==1)
        rnic_reg_base_addr_pcie = RNIC_REG_BASE_ADDR_PCIE_1;
    else if(pcie_id==2) 
        rnic_reg_base_addr_pcie = RNIC_REG_BASE_ADDR_PCIE_2;
    else
        rnic_reg_base_addr_pcie = RNIC_REG_BASE_ADDR_PCIE_0;
                
    addr = rnic_reg_base_addr_pcie + offset;
    data = reg_read(rnic_pdata,addr);

    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: pcie reg write: \t addr=%0x \t data=%0x\n",offset,data);
    #endif
    
    return data;  
}
