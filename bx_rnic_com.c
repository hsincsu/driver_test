//insomnia@2019/12/11 11:41:18

#include "header/bx_rnic.h"

void rnic_init(struct rnic_pdata * rnic_pdata)
{   
    RNIC_PRINTK("RNIC: rnic_init start\n");
    
    pcie_init(rnic_pdata);
    ieu_init(rnic_pdata);
    mpp_init(rnic_pdata);
    //mpb_init(rnic_pdata);

    RNIC_PRINTK("RNIC: rnic_init done\n");
}


unsigned int set_bits(unsigned int data,unsigned int index_h,unsigned int index_l,unsigned int new_val)
{
    int data_l;
    int data_m;
    int data_h;
    int index_tmp;     

    if(index_h < index_l)
    {
        index_tmp = index_h;
        index_h   = index_l;
        index_l   = index_tmp;
    }     
 
    if(index_h > 31 || index_h < 0 || index_l > 31 || index_l < 0)
    {
        RNIC_PRINTK("set_bits error: index overflow/underflow, the valid index is between 0 and 31\n");
        //exit(0);
    }
    
    if(index_l == 0) //gcc reason: data<<32 = data
        data_l = 0x0;
    else    
        data_l = ( data << (32 - index_l) ) >> (32 - index_l);
        
    data_m = ( new_val << (31 + index_l - index_h) ) >> (31 - index_h);
    
    if(index_h == 31)//gcc reason: data>>32 = data
        data_h = 0;
    else    
        data_h = ( data >> (index_h + 1) ) << (index_h + 1);

    //RNIC_PRINTK("data=%0x,data_l=%0x,data_m=%0x,data_h=%0x,new_data=%0x\n",data,data_l,data_m,data_h,data_h + data_m + data_l);
    
    return data_h | data_m | data_l;
}


unsigned int get_bits (unsigned int data,unsigned int index_h,unsigned int index_l)
{
    int index_tmp;

    if(index_h < index_l)
    {
        index_tmp = index_h;
        index_h   = index_l;
        index_l   = index_tmp;
    } 

    if(index_h > 31 || index_h < 0 || index_l > 31 || index_l < 0)
    {
        RNIC_PRINTK("get_bits error: index overflow/underflow, the valid index is between 0 and 31\n");
        //exit(0);
    }

    return ( data << (31 - index_h) ) >> (31 + index_l - index_h);
}


void reg_write(struct rnic_pdata*rnic_pdata,int addr,int data)
{
    writel(data,rnic_pdata->pcie_bar_addr+addr);
}


int reg_read(struct rnic_pdata*rnic_pdata,int addr)
{
    return readl(rnic_pdata->pcie_bar_addr+addr);
}


unsigned int get_random_num(void) 
{
    unsigned int randNum; 
    
     get_random_bytes(&randNum, sizeof(int)); 

    return randNum; 
}