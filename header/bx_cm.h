//insomnia@2019/12/11 11:40:48

#ifndef __BX_CM_H_
#define __BX_CM_H_


#define CM_REG_ADDR_LOG_EN                          0x0000
#define CM_REG_ADDR_ERR_EN                          0x0001
#define CM_REG_ADDR_INT_EN                          0x0002
#define CM_REG_ADDR_ERR_INT_STA                     0x0003
#define CM_REG_ADDR_ERR_INT_STA_SET                 0x0004
#define CM_REG_ADDR_ERR_INT_STA_CLR                 0x0005
#define CM_REG_ADDR_ERR_INT_CNT_CLR                 0x0006
#define CM_REG_ADDR_MSG_ADDR_ERR_INFO_FIRST         0x0007
#define CM_REG_ADDR_MSG_ADDR_ERR_CNT                0x0008
                                   
#define CM_REG_ADDR_MSG_SEND_SRAM_STATE             0x0010
#define CM_REG_ADDR_MSG_SEND_MSG_4BYTE_LEN          0x0011
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_0         0x0012
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_1         0x0013
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_2         0x0014
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_3         0x0015
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_4         0x0016
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_5         0x0017
                                                                                      
#define CM_REG_ADDR_MSG_RECEIVE_SRAM_STATE          0x0020
#define CM_REG_ADDR_MSG_RECEIVE_MSG_4BYTE_LEN       0x0021
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_0      0x0022
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_1      0x0023
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_2      0x0024
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_3      0x0025
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_4      0x0026
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_5      0x0027

#define CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH         0x0030

#define CM_REG_ADDR_SRAM_RMC                        0x0040

#define CM_MSG_SEND_MSG_SRAM_WR_FINISH_RANGE        0,0   
#define CM_MSG_RECEIVE_MSG_SRAM_RD_FINISH_RANGE     1,1   


void    cm_init                                     (struct rnic_pdata *);
void    cmcfg_reg_write                             (struct rnic_pdata *,int,int);
int     cmcfg_reg_read                              (struct rnic_pdata *,int);
void    cmmsg_reg_write                             (struct rnic_pdata *,int,int);
int     cmmsg_reg_read                              (struct rnic_pdata *,int);

#endif
