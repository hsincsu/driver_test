//insomnia@2019/12/11 11:40:17

#ifndef __BX_IEU_H_
#define __BX_IEU_H_


#define MSI_0_ADDR_H            0x00000000
#define MSI_0_ADDR_L            0xFEE10004
#define MSI_0_DATA              0x00004021

#define MSI_1_ADDR_H            0x00000000
#define MSI_1_ADDR_L            0x1FFFFFFF
#define MSI_1_DATA              0x00000000
    
void    ieu_init                (struct rnic_pdata *);
void    ieu_clear_intr          (struct rnic_pdata *);
void    ieu_disable_intr        (struct rnic_pdata *);
void    ieu_enable_intr         (struct rnic_pdata *);
void    ieu_report_pending_intr (struct rnic_pdata *);
void    ieu_reg_write           (struct rnic_pdata *,int,int);
int     ieu_reg_read            (struct rnic_pdata *,int);

#endif    