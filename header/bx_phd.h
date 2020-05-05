//insomnia@2019/12/11 15:45:09

#ifndef __BX_PHD_H_
#define __BX_PHD_H_


#define RNIC_BASE_ADDR_MAC_CFG_S_0_H                    0x0
#define RNIC_BASE_ADDR_MAC_CFG_S_0_L                    0xB000000
#define RNIC_BASE_ADDR_MAC_CFG_S_1_H                    0x0
#define RNIC_BASE_ADDR_MAC_CFG_S_1_L                    0xC000000

#define RNIC_BASE_ADDR_MPB_DATA_S_0_H                   0x8000
#define RNIC_BASE_ADDR_MPB_DATA_S_0_L                   0x0
#define RNIC_BASE_ADDR_MPB_DATA_S_1_H                   0xC000
#define RNIC_BASE_ADDR_MPB_DATA_S_1_L                   0x0

#define MAC_DMA_CHANNEL_ID_FOR_MPB                      6//5 by hs@20200426

#define PHD_REG_ADDR_PHD_START                          0x0000
#define PHD_REG_ADDR_BYTE_ORDER                         0x0001
#define PHD_REG_ADDR_TX_DESC_TAIL_LPTR_ADDR_L           0x0002
#define PHD_REG_ADDR_TX_DESC_TAIL_LPTR_ADDR_H           0x0003
#define PHD_REG_ADDR_TX_DESC_TAIL_PTR_WRITE_THRESDHOLD  0x0004
#define PHD_REG_ADDR_RX_DESC_TAIL_PTR_WRITE_THRESDHOLD  0x0005
#define PHD_REG_ADDR_RX_DESC_TAIL_PTR_INCR_STEP         0x0006
#define PHD_REG_ADDR_RX_DESC_TAIL_LPTR_ADDR_L           0x0007
#define PHD_REG_ADDR_RX_DESC_TAIL_LPTR_ADDR_H           0x0008
#define PHD_REG_ADDR_MAC_SOURCE_ADDR_L                  0x0009
#define PHD_REG_ADDR_MAC_SOURCE_ADDR_H                  0x000a
#define PHD_REG_ADDR_MAC_TYPE_IPV4                      0x000b
#define PHD_REG_ADDR_MAC_TYPE_IPV6                      0x000c
#define PHD_REG_ADDR_IPV4_VERSION                       0x000d
#define PHD_REG_ADDR_IPV4_HEADER_LEN                    0x000e
#define PHD_REG_ADDR_IPV4_TOS                           0x000f
#define PHD_REG_ADDR_IPV4_ID                            0x0010
#define PHD_REG_ADDR_IPV4_FLAG                          0x0011
#define PHD_REG_ADDR_IPV4_OFFSET                        0x0012
#define PHD_REG_ADDR_IPV4_TTL                           0x0013
#define PHD_REG_ADDR_IPV4_PROTOCOL                      0x0014
#define PHD_REG_ADDR_IPV4_HEADER_CHECKSUM               0x0015
#define PHD_REG_ADDR_IPV4_SOURCE_ADDR                   0x0016
#define PHD_REG_ADDR_IPV6_VERSION                       0x0017
#define PHD_REG_ADDR_IPV6_CLASS                         0x0018
#define PHD_REG_ADDR_IPV6_FLOW_LABEL                    0x0019
#define PHD_REG_ADDR_IPV6_NEXT_HEADER                   0x001a
#define PHD_REG_ADDR_IPV6_HOP_LIMIT                     0x001b
#define PHD_REG_ADDR_IPV6_SOURCE_ADDR_0                 0x001c
#define PHD_REG_ADDR_IPV6_SOURCE_ADDR_1                 0x001d
#define PHD_REG_ADDR_IPV6_SOURCE_ADDR_2                 0x001e
#define PHD_REG_ADDR_IPV6_SOURCE_ADDR_3                 0x001f
#define PHD_REG_ADDR_UDP_SOURCE_PORT                    0x0020
#define PHD_REG_ADDR_UDP_DEST_PORT                      0x0021
#define PHD_REG_ADDR_UDP_CHECKSUM                       0x0022
#define PHD_REG_ADDR_CONTEXT_TDES_0                     0x0023
#define PHD_REG_ADDR_CONTEXT_TDES_1                     0x0024
#define PHD_REG_ADDR_CONTEXT_TDES_2                     0x0025
#define PHD_REG_ADDR_CONTEXT_TDES_3                     0x0026
#define PHD_REG_ADDR_NORMAL_TDES_1                      0x0027
#define PHD_REG_ADDR_NORMAL_TDES_2                      0x0028
#define PHD_REG_ADDR_NORMAL_TDES_3                      0x0029
#define PHD_REG_ADDR_NORMAL_RDES_1                      0x002a
#define PHD_REG_ADDR_NORMAL_RDES_2                      0x002b
#define PHD_REG_ADDR_NORMAL_RDES_3                      0x002c
#define PHD_REG_ADDR_SRAM_RMC                           0x002d
#define PHD_REG_ADDR_MAC_TYPE_IPV6_RECV                 0x002e

void    phd_init    (struct rnic_pdata *,int);


#endif