//insomnia@2019/12/16 14:10:59

#include "header/bx_rnic.h"

void phd_init (struct rnic_pdata * rnic_pdata,int phd_id)
{
#ifdef IOC_EN
    int data;
#endif    
    int phd_base_addr;

    RNIC_PRINTK("RNIC: phd_%d_init start\n",phd_id);

    phd_base_addr = phd_id ? BASE_ADDR_PHD_1 : BASE_ADDR_PHD_0;
    
    //mpb_reg_write(rnic_pdata,phd_base_addr,PHD_REG_ADDR_IPV4_SOURCE_ADDR,0x7f000001);    //source 127.0.0.1
    mpb_reg_write(rnic_pdata,phd_base_addr,PHD_REG_ADDR_IPV4_SOURCE_ADDR,0x0100007f);  //source 127.0.0.1
        
#ifndef PFC_EN
    mpb_reg_write(rnic_pdata,phd_base_addr,PHD_REG_ADDR_CONTEXT_TDES_3,0xC0000000);
    mpb_reg_write(rnic_pdata,phd_base_addr,PHD_REG_ADDR_NORMAL_TDES_2,0x00000000);
#endif

#ifdef IOC_EN
    data = mpb_reg_read(rnic_pdata,phd_base_addr,PHD_REG_ADDR_NORMAL_TDES_2);
    data = set_bits(rnic_pdata,data,31,31,0x1);
    mpb_reg_write(rnic_pdata,phd_base_addr,PHD_REG_ADDR_NORMAL_TDES_2,data);

    data = mpb_reg_read(rnic_pdata,phd_base_addr,PHD_REG_ADDR_NORMAL_RDES_3);
    data = set_bits(rnic_pdata,data,30,30,0x1);
    mpb_reg_write(rnic_pdata,phd_base_addr,PHD_REG_ADDR_NORMAL_RDES_3,data);
#endif
            
    mpb_reg_write(rnic_pdata,phd_base_addr,PHD_REG_ADDR_PHD_START,0x1); 
        
    RNIC_PRINTK("RNIC: phd_%d_init done\n",phd_id);    
}

