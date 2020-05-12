//insomnia@2019/12/17 8:56:11

#include "header/bx_rnic.h"

void pbu_init(struct rnic_pdata * rnic_pdata)
{
    RNIC_PRINTK("RNIC: pbu_init start\n");
    printk("RNIC: pbu_init start\n");
	pbu_init_mtu_reg(rnic_pdata,MTU_1K);
    pbu_set_general_reg(rnic_pdata,ERR_EN_REG,0x1);            //err en
    pbu_set_general_reg(rnic_pdata,RETRY_NUM_REG,0x5);         //retry one
    pbu_set_general_reg(rnic_pdata,TIME_0_OUT_REG,4000000);    //40000 cycle
    pbu_set_general_reg(rnic_pdata,TIME_1_OUT_REG,0x0);
    
	u32 regval = 0;
	regval = 
	printk("RNIC: pbu_init done\n");
}
EXPORT_SYMBOL(pbu_init);


void pbu_reg_write(struct rnic_pdata * rnic_pdata,int addr,int data)
{
    mpb_reg_write(rnic_pdata,BASE_ADDR_PBU,addr,data);
    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: pbu_reg_write: addr=0x%0x,data=0x%0x\n",addr,data);
    #endif
}

int pbu_reg_read(struct rnic_pdata * rnic_pdata,int addr)
{
    int data;

    data = mpb_reg_read(rnic_pdata,BASE_ADDR_PBU,addr);

    #ifdef REG_DEBUG
        RNIC_PRINTK("\tRNIC: pbu_reg_read: addr=0x%0x,data=0x%0x\n",addr,data);
    #endif
    
    return data;
}



void pbu_init_for_recv_req(struct rnic_pdata * rnic_pdata,int service_type,int dst_qp,int dst_eec,int init_psn,int init_pkey,int init_qkey)
{
    int addr = 0;
    int wdata = 0;
	u32 regval = 0;

    pbu_reg_write(rnic_pdata,addr,wdata);
 
     //---------------rc type: {SRC_QP_0,DST_QP_0,INIT_PSN,INIT_PKEY}--------------------//
    if(service_type == RC_TYPE) 
    {
        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,QP_PARTION);    //QP,rc type
        addr    = set_bits(addr,REG_PARTION_ADDR,PSN_PARTION);   //write psn
        addr    = set_bits(addr,REG_OFFSET,dst_qp);              //src qp = 2;
        wdata   = init_psn; 
        pbu_reg_write(rnic_pdata,addr,wdata);
		//added by hs
		regval = pbu_reg_read(rnic_pdata,addr);
		printk("\tRNIC:init for recv req reg write_psn read : 0x%x \n",regval);
       
        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,QP_PARTION);    //QP,rc type
        addr    = set_bits(addr,REG_PARTION_ADDR,PKEY_PARTION);  //write pkey
        addr    = set_bits(addr,REG_OFFSET,dst_qp);              //for inbound request
        wdata   = set_bits(init_pkey,16,16,1);
        wdata   = set_bits(wdata,31,17,0);
        pbu_reg_write(rnic_pdata,addr,wdata);  
		//added by hs
		regval = pbu_reg_read(rnic_pdata,addr);
		printk("\tRNIC:init for recv req reg write pkey read: 0x%x \n",regval);

		printk("\tRNIC:init init for recv reqinfo: set pbu rc qp setting,addr is %0x,data is %0x\n",addr,wdata);
    }   
      
    //----------------rd type,{SRC_EEC_0,DST_EEC_0,INIT_PSN,INIT_PKEY}----------------//
    if(service_type == RD_TYPE) 
    {      
        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,EEC_PARTION);  //QP,rd type
        addr    = set_bits(addr,REG_PARTION_ADDR,PSN_PARTION);  //write psn
        addr    = set_bits(addr,REG_OFFSET,dst_eec);            //src EEC = 2;
        wdata   = init_psn; 
        pbu_reg_write(rnic_pdata,addr,wdata);

        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,QP_PARTION);    //QP,rd type
        addr    = set_bits(addr,REG_PARTION_ADDR,PKEY_PARTION);  //write pkey
        addr    = set_bits(addr,REG_OFFSET,dst_qp);              //for inbound request
        wdata   = set_bits(init_pkey,16,16,1);
        wdata   = set_bits(wdata,31,17,0);       
        pbu_reg_write(rnic_pdata,addr,wdata);
     
        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,QP_PARTION);    //QP,rc type
        addr    = set_bits(addr,REG_PARTION_ADDR,QKEY_PARTION);  //write qkey
        addr    = set_bits(addr,REG_OFFSET,dst_qp);              //for inbound request
        wdata   = init_qkey;      
        pbu_reg_write(rnic_pdata,addr,wdata);
		
    }
      
    //--------------------ud type-------------------------------------------------//
    if(service_type == UD_TYPE) 
    {
        //only pkey and qkey
        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,QP_PARTION);    //EEC,ud type
        addr    = set_bits(addr,REG_PARTION_ADDR,PKEY_PARTION);  //write pkey
        addr    = set_bits(addr,REG_OFFSET,dst_qp);              //for inbound request
        wdata   = set_bits(init_pkey,16,16,1);
        wdata   = set_bits(wdata,31,17,0);       
        pbu_reg_write(rnic_pdata,addr,wdata);   
 
        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,QP_PARTION);    //EEC,ud type
        addr    = set_bits(addr,REG_PARTION_ADDR,QKEY_PARTION);  //write qkey
        addr    = set_bits(addr,REG_OFFSET,dst_qp);              //for inbound request
        wdata   = init_qkey;      
        pbu_reg_write(rnic_pdata,addr,wdata);
    }    
  
	printk("\tRNIC: info: recv_req: qp/eec init done\n"); 
    RNIC_PRINTK("\tRNIC: info: recv_req: qp/eec init done\n");   
   
}
EXPORT_SYMBOL(pbu_init_for_recv_req);

void pbu_init_for_recv_rsp(struct rnic_pdata * rnic_pdata,int service_type,int src_qp,int src_eec,int init_pkey)
{
    int addr;
    int wdata; 
	u32 regval = 0;//added by hs
  
    //---------------rc type: {SRC_QP_0,DST_QP_0,INIT_PSN,INIT_PKEY}--------------------//
    if(service_type == RC_TYPE) 
    {       
        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,QP_PARTION);    //QP,rc type
        addr    = set_bits(addr,REG_PARTION_ADDR,PKEY_PARTION);  //write pkey
        addr    = set_bits(addr,REG_OFFSET,src_qp);              //for inbound request
        wdata   = set_bits(init_pkey,16,16,1);
        wdata   = set_bits(wdata,31,17,0);
        pbu_reg_write(rnic_pdata,addr,wdata);  

		//added by hs
		regval = pbu_reg_read(rnic_pdata,addr);
		printk("\tRNIC: init for recv_rsp reg read: 0x%x \n",regval);
		printk("\tRNIC: init for recv_resp info: set pbu rc qp setting,addr is %0x,data is %0x\n",addr,wdata);
    }    
      
      //----------------rd type,{SRC_EEC_0,DST_EEC_0,INIT_PSN,INIT_PKEY}----------------//
    if(service_type == RD_TYPE) 
    {
        addr    = 0x0;
        addr    = set_bits(addr,REG_TYPE_PARTION,QP_PARTION);    //QP,rd type
        addr    = set_bits(addr,REG_PARTION_ADDR,PKEY_PARTION);  //write pkey
        addr    = set_bits(addr,REG_OFFSET,src_eec);              //for inbound request
        wdata   = set_bits(init_pkey,16,16,1);
        wdata   = set_bits(wdata,31,17,0);
        pbu_reg_write(rnic_pdata,addr,wdata);
    }
      
      //--------------------ud type-------------------------------------------------//
   //no
	printk("\tRNIC: info: RECV_RSP: QP/EEC Init Done\n");   
  RNIC_PRINTK("\tRNIC: info: RECV_RSP: QP/EEC Init Done\n");   
   
}  
EXPORT_SYMBOL(pbu_init_for_recv_rsp);

void pbu_init_mtu_reg(struct rnic_pdata * rnic_pdata,int mtu_size) //1:256B,2:512B,3:1024B,4: 2048B,5: 4KB,6: 64KB,7: 1MB 
{
    int addr;
    int wdata;
	u32 regval = 0;

    addr    = 0x0;
    addr    = set_bits(addr,REG_TYPE_PARTION,EEC_PARTION);      //no matter
    addr    = set_bits(addr,REG_PARTION_ADDR,REG_PARTION);     //write general regs
    addr    = set_bits(addr,REG_OFFSET,MTU_NUM_REG);            //mtu
    wdata   = set_bits(0x0,4,0,mtu_size);
    pbu_reg_write(rnic_pdata,addr,wdata);
	
	//added by hs
	regval = pbu_reg_read(rnic_pdata,addr);
	printk("\tRNIC: info: mtu reg read: 0x%x \n",regval);
	printk("\tRNIC: info: mtu init done\n"); 

    RNIC_PRINTK("\tRNIC: info: mtu init done\n"); 
}

void pbu_set_general_reg(struct rnic_pdata * rnic_pdata,int offset,int wdata)
{   
    int addr;
	u32 regval = 0;
    
    addr    = 0x0;
    addr    = set_bits(addr,REG_TYPE_PARTION,EEC_PARTION);      //no matter
    addr    = set_bits(addr,REG_PARTION_ADDR,REG_PARTION);     //write general regs
    addr    = set_bits(addr,REG_OFFSET,offset);                 //
    pbu_reg_write(rnic_pdata,addr,wdata);

	//added by hs
	regval = pbu_reg_read(rnic_pdata,addr);
	printk("\tRNIC: info: general register reg read: 0x%x \n",regval);
	printk("\tRNIC: info: set general register ok,addr is %0x,data is %0x\n",addr,wdata);
    RNIC_PRINTK("\tRNIC: info: set general register ok,addr is %0x,data is %0x\n",addr,wdata);
}
      





                        
