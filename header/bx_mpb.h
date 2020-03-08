//insomnia@2019/12/11 15:15:15

#ifndef __BX_MPB_H_
#define __BX_MPB_H_

#define MPB_STEP_ADDR_OFFSET    0x0
#define MPB_STEP_DATA_OFFSET    0x100
                                                  
#define BASE_ADDR_PGU           0x00000000
#define BASE_ADDR_PBU           0x10000000
#define BASE_ADDR_CMCFG         0x20000000
#define BASE_ADDR_CMMSG         0x30000000
#define BASE_ADDR_LIU           0x40000000
#define BASE_ADDR_PHD_0         0x50000000
#define BASE_ADDR_PHD_1         0x60000000

void    mpb_init                (struct rnic_pdata*);
void    mpb_reg_write           (struct rnic_pdata*,int,int,int);
int     mpb_reg_read            (struct rnic_pdata*,int,int);

#endif