//insomnia@2019/12/11 11:40:48

#ifndef __BX_CM_H_
#define __BX_CM_H_


#define CM_REG_ADDR_LOG_EN                          0x0000    //cm�ж�ģ��logʹ��
#define CM_REG_ADDR_ERR_EN                          0x0001    //cm�ж�ģ��errʹ��
#define CM_REG_ADDR_INT_EN                          0x0002    //cm�ж�ģ��intʹ��
#define CM_REG_ADDR_ERR_INT_STA                     0x0003    //cm�ж�ģ��״̬�Ĵ���
#define CM_REG_ADDR_ERR_INT_STA_SET                 0x0004    //cm�ж�ģ��״̬�Ĵ�������
#define CM_REG_ADDR_ERR_INT_STA_CLR                 0x0005    //cm�ж�ģ��״̬�Ĵ������
#define CM_REG_ADDR_ERR_INT_CNT_CLR                 0x0006    //cm�ж�ģ������Ĵ������
#define CM_REG_ADDR_MSG_ADDR_ERR_INFO_FIRST         0x0007    //cm msgģ���׸�����ĵ�ַ��Ϣ
#define CM_REG_ADDR_MSG_ADDR_ERR_CNT                0x0008    //cm msg����ĵ�ַ����

//send msg��ؼĴ���                                            
#define CM_REG_ADDR_MSG_SEND_SRAM_STATE             0x0010    //send sram��״̬��{�洢��msg������ʣ���flit����}                                 
#define CM_REG_ADDR_MSG_SEND_MSG_4BYTE_LEN          0x0011    //��ǰ����д��send sram��msg�ĳ��ȣ�4�ֽ�����
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_0         0x0012    //��ǰ����д��send sram��msg��lower layer protocol��Ϣ��[31:0]λ
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_1         0x0013    //��ǰ����д��send sram��msg��lower layer protocol��Ϣ��[63:32]λ
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_2         0x0014    //��ǰ����д��send sram��msg��lower layer protocol��Ϣ��[95:64]λ
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_3         0x0015    //��ǰ����д��send sram��msg��lower layer protocol��Ϣ��[127:96]λ
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_4         0x0016    //��ǰ����д��send sram��msg��lower layer protocol��Ϣ��[159:128]λ
#define CM_REG_ADDR_MSG_SEND_MSG_LLP_INFO_5         0x0017    //��ǰ����д��send sram��msg��lower layer protocol��Ϣ��[191:160]λ
                                                                
 //receive msg��ؼĴ���                                        
#define CM_REG_ADDR_MSG_RECEIVE_SRAM_STATE          0x0020    //receive sram��״̬��empty                                                        
#define CM_REG_ADDR_MSG_RECEIVE_MSG_4BYTE_LEN       0x0021    //��ǰ���ڴ�receive sram��ȡ��msg�ĳ��ȣ�4�ֽ�����  
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_0      0x0022    //��ǰ���ڴ�receive sram��ȡ��msg��lower layer protocol��Ϣ��[31:0]λ
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_1      0x0023    //��ǰ���ڴ�receive sram��ȡ��msg��lower layer protocol��Ϣ��[63:32]λ
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_2      0x0024    //��ǰ���ڴ�receive sram��ȡ��msg��lower layer protocol��Ϣ��[95:64]λ
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_3      0x0025    //��ǰ���ڴ�receive sram��ȡ��msg��lower layer protocol��Ϣ��[127:96]λ
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_4      0x0026    //��ǰ���ڴ�receive sram��ȡ��msg��lower layer protocol��Ϣ��[159:128]λ
#define CM_REG_ADDR_MSG_RECEIVE_MSG_LLP_INFO_5      0x0027    //��ǰ���ڴ�receive sram��ȡ��msg��lower layer protocol��Ϣ��[191:160]λ

#define CM_REG_ADDR_MSG_SRAM_OPERATE_FINISH         0x0030    //cm msg sram�Ĳ���״̬{cm_msg_receive_sram_rd_finish,cm_msg_send_sram_wr_finish}

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
