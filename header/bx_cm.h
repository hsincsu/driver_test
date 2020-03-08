//insomnia@2019/12/11 11:40:48

#ifndef __BX_CM_H_
#define __BX_CM_H_


#define CM_REG_ADDR_LOG_EN                          0x0000    //cm中断模块log使能
#define CM_REG_ADDR_ERR_EN                          0x0001    //cm中断模块err使能
#define CM_REG_ADDR_INT_EN                          0x0002    //cm中断模块int使能
#define CM_REG_ADDR_ERR_INT_STA                     0x0003    //cm中断模块状态寄存器
#define CM_REG_ADDR_ERR_INT_STA_SET                 0x0004    //cm中断模块状态寄存器设置
#define CM_REG_ADDR_ERR_INT_STA_CLR                 0x0005    //cm中断模块状态寄存器清除
#define CM_REG_ADDR_ERR_INT_CNT_CLR                 0x0006    //cm中断模块计数寄存器清除
#define CM_REG_ADDR_MSG_ADDR_ERR_INFO_FIRST         0x0007    //cm msg模块首个出错的地址信息
#define CM_REG_ADDR_MSG_ADDR_ERR_CNT                0x0008    //cm msg出错的地址个数

//send msg相关寄存器                                            
#define CM_REG_ADDR_MSG_SEND_SRAM_STATE             0x0010    //send sram的状态：{存储的msg数量，剩余的flit数量}                                 
#define CM_REG_ADDR_MSG_SEND_MSG_4BYTE_LEN          0x0011    //当前正在写入send sram的msg的长度，4字节粒度
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_0         0x0012    //当前正在写入send sram的msg的lower layer protocol信息的[31:0]位
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_1         0x0013    //当前正在写入send sram的msg的lower layer protocol信息的[63:32]位
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_2         0x0014    //当前正在写入send sram的msg的lower layer protocol信息的[95:64]位
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_3         0x0015    //当前正在写入send sram的msg的lower layer protocol信息的[127:96]位
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_4         0x0016    //当前正在写入send sram的msg的lower layer protocol信息的[159:128]位
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_5         0x0017    //当前正在写入send sram的msg的lower layer protocol信息的[191:160]位
                                                                
 //receive msg相关寄存器                                        
#define CM_REG_ADDR_MSG_RECEIVE_SRAM_STATE          0x0020    //receive sram的状态：empty                                                        
#define CM_REG_ADDR_MSG_RECEIVE_MSG_4BYTE_LEN       0x0021    //当前正在从receive sram读取的msg的长度，4字节粒度  
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_0      0x0022    //当前正在从receive sram读取的msg的lower layer protocol信息的[31:0]位
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_1      0x0023    //当前正在从receive sram读取的msg的lower layer protocol信息的[63:32]位
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_2      0x0024    //当前正在从receive sram读取的msg的lower layer protocol信息的[95:64]位
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_3      0x0025    //当前正在从receive sram读取的msg的lower layer protocol信息的[127:96]位
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_4      0x0026    //当前正在从receive sram读取的msg的lower layer protocol信息的[159:128]位
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_5      0x0027    //当前正在从receive sram读取的msg的lower layer protocol信息的[191:160]位

#define CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH         0x0030    //cm msg sram的操作状态{cm_msg_receive_sram_rd_finish,cm_msg_send_sram_wr_finish}

#define CM_REG_ADDR_SRAM_RMC                        0x0040

#define CM_MSG_SEND_MSG_SRAM_WR_FINISH_RANGE        0,0       //CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH
#define CM_MSG_RECEIVE_MSG_SRAM_RD_FINISH_RANGE     1,1       //CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH


void    cm_init                                     (struct rnic_pdata *);
void    cmcfg_reg_write                             (struct rnic_pdata *,int,int);
int     cmcfg_reg_read                              (struct rnic_pdata *,int);
void    cmmsg_reg_write                             (struct rnic_pdata *,int,int);
int     cmmsg_reg_read                              (struct rnic_pdata *,int);
                                                    
int     cmmsg_get_len                               (struct rnic_pdata *,int); 
void    cmmsg_set_len                               (struct rnic_pdata *,int,int);
void    cm_random_test                              (struct rnic_pdata *);
void    cm_random_test_msg_send                     (struct rnic_pdata *);
int     cmmsg_set_bit                               (struct rnic_pdata *,int,int,int);
int     cmmsg_get_bit                               (struct rnic_pdata *,int,int);
void    cm_random_test_msg_recv                     (struct rnic_pdata *);

#endif
