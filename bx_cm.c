//insomnia@2019/12/11 11:39:58

#include "header/bx_rnic.h"

void cm_init(struct rnic_pdata *rnic_pdata)
{
    RNIC_PRINTK("RNIC: cm_init start\n");
    cmcfg_reg_write(rnic_pdata,0x0,0x3);
    cmcfg_reg_write(rnic_pdata,0x1,0x3);
    cmcfg_reg_write(rnic_pdata,0x2,0x3);
    
    //cmcfg_reg_write(rnic_pdata,CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_0,0x7f000001); //dest ip  addr 127.0.0.1
    cmcfg_reg_write(rnic_pdata,CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_0,0x0100007f); //dest ip  addr 127.0.0.1
    cmcfg_reg_write(rnic_pdata,CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_4,0x5a5a5a5a); //dest mac addr 5a5a5a5a
    cmcfg_reg_write(rnic_pdata,CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_5,0x5a5a);     //dest mac addr 5a5a
    RNIC_PRINTK("RNIC: cm_init done\n");
}


void cmcfg_reg_write(struct rnic_pdata *rnic_pdata,int addr,int data)
{
    mpb_reg_write(rnic_pdata,BASE_ADDR_CMCFG,addr,data);
    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: cmcfg_reg_write: addr=0x%0x,data=0x%0x\n",addr,data);
    #endif
}

int cmcfg_reg_read(struct rnic_pdata *rnic_pdata,int addr)
{
    int data;

    data = mpb_reg_read(rnic_pdata,BASE_ADDR_CMCFG,addr);
    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: cmcfg_reg_read: addr=0x%0x,data=0x%0x\n",addr,data);
    #endif
    
    return data;
}

void cmmsg_reg_write(struct rnic_pdata *rnic_pdata,int addr,int data)
{
    mpb_reg_write(rnic_pdata,BASE_ADDR_CMMSG,addr,data);
    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: cmmsg_reg_write addr=0x%0x,data=0x%0x\n",addr,data);
    #endif
}

int cmmsg_reg_read(struct rnic_pdata *rnic_pdata,int addr)
{
    int data;

    data = mpb_reg_read(rnic_pdata,BASE_ADDR_CMMSG,addr);
    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: cmmsg_reg_read: addr=0x%0x,data=0x%0x\n",addr,data);
    #endif
    return data;
}

