//insomnia@2019/12/11 11:40:17

#ifndef __BX_IEU_H_
#define __BX_IEU_H_

    
void    ieu_init                        (struct rnic_pdata *);

void    ieu_enable_intr_all             (struct rnic_pdata *);
void    ieu_disable_intr_all            (struct rnic_pdata *);
void    ieu_clear_intr_all              (struct rnic_pdata *);

void    ieu_enable_intr_one             (struct rnic_pdata *,int);
void    ieu_disable_intr_one            (struct rnic_pdata *,int);
void    ieu_clear_intr_one              (struct rnic_pdata *,int);

void    ieu_enable_intr_tx_one          (struct rnic_pdata *,int);
void    ieu_disable_intr_tx_one         (struct rnic_pdata *,int);
void    ieu_clear_intr_tx_one           (struct rnic_pdata *,int);

void    ieu_enable_intr_rx_one          (struct rnic_pdata *,int);
void    ieu_disable_intr_rx_one         (struct rnic_pdata *,int);
void    ieu_clear_intr_rx_one           (struct rnic_pdata *,int);

void    ieu_enable_intr_tx_all          (struct rnic_pdata *);
void    ieu_disable_intr_tx_all         (struct rnic_pdata *);
void    ieu_clear_intr_tx_all           (struct rnic_pdata *);

void    ieu_enable_intr_rx_all          (struct rnic_pdata *);
void    ieu_disable_intr_rx_all         (struct rnic_pdata *);
void    ieu_clear_intr_rx_all           (struct rnic_pdata *);

void    ieu_enable_intr_tx_rx_all       (struct rnic_pdata *);
void    ieu_disable_intr_tx_rx_all      (struct rnic_pdata *);
void    ieu_clear_intr_tx_rx_all        (struct rnic_pdata *);

void    ieu_msi_cfg                     (struct rnic_pdata *);
void    ieu_hpb_csr_err_evt_int_en      (struct rnic_pdata *);
void    ieu_hpb_csr_misc_int_en         (struct rnic_pdata *);
void    ieu_int_direction_cfg           (struct rnic_pdata *);
void    ieu_report_pending_intr         (struct rnic_pdata *);
void    ieu_reg_write                   (struct rnic_pdata *,int,int);
int     ieu_reg_read                    (struct rnic_pdata *,int);

#endif    