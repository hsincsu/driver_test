//insomnia@2020/1/1 1:49:09

#ifndef __BX_PCIE_H_
#define __BX_PCIE_H_
   
void    pcie_init                   (struct rnic_pdata *);
void    pcie_reg_write              (struct rnic_pdata *,int,int,int);
int     pcie_reg_read               (struct rnic_pdata *,int,int);
void    pcie_print_all_reg          (struct rnic_pdata *);
void    pcie_clear_link_down        (struct rnic_pdata *);
int     pcie_get_max_payload_size   (struct rnic_pdata *);
void    pcie_set_max_payload_size   (struct rnic_pdata *);
void    pcie_set_max_request_size   (struct rnic_pdata *);
void    pcie_reg_test               (struct rnic_pdata *);

#endif    