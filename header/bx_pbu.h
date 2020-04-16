//insomnia@2019/12/11 15:27:51

#ifndef __BX_PBU_H_
#define __BX_PBU_H_

#define  UD_TYPE                    0b011
#define  RC_TYPE                    0b000
#define  RD_TYPE                    0b010
                                                                     
#define RC_SRC_QP_0                 0x44
#define RC_DST_QP_0                 0x06
#define RC_INIT_PSN                 0x0
#define RC_INIT_PKEY                0x8888
                                    
#define UD_SRC_QP_0                 0x44 
#define UD_DST_QP_0                 0x06    
#define UD_INIT_PSN                 0x11
#define UD_INIT_PKEY                0x8888
#define UD_INIT_QKEY                0x66666666
                                    
#define RD_SRC_QP_0                 0x44
#define RD_DST_QP_0                 0x06  
#define RD_SRC_EEC_0                0x6
#define RD_DST_EEC_0                0x9
#define RD_INIT_PSN                 0x0
#define RD_INIT_PKEY                0x8888
#define RD_INIT_QKEY                0x66666666

#define MPB_MTU_WIDTH               5
#define MTU_256                     1
#define MTU_512                     2
#define MTU_1K                      3  
#define MTU_2K                      4
#define MTU_4K                      5
#define MTU_64K                     6 
#define MTU_1M                      7

//128 entry
#define REQ_SEND_CMD_BUF_ADDR_WIDTH 7
#define REQ_SEND_DAT_BUF_ADDR_WIDTH 13
#define RSP_SEND_BUF_DEPTH          4096
#define RSP_RECV_BUF_DEPTH          4096
#define REQ_RECV_BUF_DEPTH          4096
  
//addr for indexing QP/EEC num,4K
#define QP_TAB_ADDR_WIDTH           10
#define EEC_TAB_ADDR_WIDTH          10  

//cmd fields
//#define EEC_WIDTH                 24
//#define QP_WIDTH                  24
//#define PSN_WIDTH                 24

#define PKT_LEN_WIDTH               13
#define FIRST_ADDR_WIDTH            REQ_SEND_DAT_BUF_ADDR_WIDTH
#define LAST_ADDR_WIDTH             REQ_SEND_DAT_BUF_ADDR_WIDTH  
#define ACKNUM_WIDTH                20

//packet fields in data127,0,header in order{msb,lsb},{IETH,AETH,RETH,DETH,RDETH,BTH}
//first flit,RD,{RDETH,BTH},RC,{RETH31,0,BTH}
//second flit,RD,{RETH,DETH},RC,{RETH127,32}
//third  flist RD,{,RETH}
#define BTH_WIDTH                   96
#define RDETH_WIDTH                 32
#define DETH_WIDTH                  64
#define RETH_WIDTH                  128
#define AETH_WIDTH                  32
#define IETH_WIDTH                  32
                                    
#define OPC_WIDTH                    8
#define EEC_WIDTH                   24
#define QP_WIDTH                    24
#define PSN_WIDTH                   24
#define DMALEN_WIDTH                32
#define TVER_WIDTH                  4
#define PKEY_WIDTH                  16
#define QKEY_WIDTH                  32
#define SYNDROME_WIDTH              8

#define BTH_IN_FLIT0_RANGE          95,0
#define TVER_IN_BTH_RANGE           19,16
#define PKEY_IN_BTH_RANGE           15,0
#define OPCODE_IN_BTH_RANGE         31,24
#define DEST_QP_IN_BTH_RANGE        55,32
#define PSN_IN_BTH_RANGE            87,64
#define EEC_IN_RDETH_RANGE          23,0
#define DMALEN_IN_RETH_RANG         127,96
#define QKEY_IN_DETH_RANGE          31,0
#define SRC_QP_IN_DETH_RANGE        55,32
#define SYNDROME_IN_AETH_RANGE      31,24

#define RC_RETH_P0_IN_FLIT0_RANGE   127,96
#define RC_RETH_P1_IN_FLIT1_RANGE   95,0
#define RC_AETH_IN_FLIT0_RANGE      127,96 

#define RD_RDETH_IN_FLIT0_RANGE     127,96
#define RD_DETH_IN_FLIT1_RANGE      63,0
#define RD_RETH_P0_IN_FLIT1_RANGE   127,64
#define RD_RETH_P1_IN_FLIT2_RANGE   63,0
#define RD_AETH_IN_FLIT1_RANGE      31,0

#define UD_DETH_IN_FLIT0_RANGE      127,96
#define UD_DETH_IN_FLIT1_RANGE      31,0 

//opcode
#define RC_TYPE                     0b000
#define UD_TYPE                     0b011
#define RD_TYPE                     0b010
                                    
#define OPC_RESYNC                  0b10101
#define OPC_RDMA_RD_REQ             0b01100
                                    
#define OPC_RDMA_RD_RSP_F           0b01101
#define OPC_RDMA_RD_RSP_M           0b01110
#define OPC_RDMA_RD_RSP_L           0b01111
#define OPC_RDMA_RD_RSP_O           0b10000
#define OPC_RDMA_RD_RSP_ACK         0b10001 
#define OPC_RDMA_RD_RSP_AACK        0b10010
                                    
#define RNR_NAK_OPC                 0b001
#define NNN_NAK_OPC                 0b011
                                    
//register addr                     
#define REG_OFFSET                  23,0
#define TIME_0_OUT_REG              0x00
#define TIME_1_OUT_REG              0x10
#define RETRY_NUM_REG               0x20
#define MTU_NUM_REG                 0x30
#define SRAM_RMA_REG                0x40
#define SRAM_RMB_REG                0x50
#define ERR_EN_REG                  0x60
#define ERR_CLR_REG                 0x70
#define ERR_SET_REG                 0x80
#define ERR_STA0_REG                0x90
#define ERR_STA1_REG                0xa0
                                    
#define REG_TYPE_PARTION            24,24
#define QP_PARTION                  0x1
#define EEC_PARTION                 0x0
                                    
#define REG_PARTION_ADDR            26,25
#define PSN_PARTION                 0b00
#define QKEY_PARTION                0b01
#define PKEY_PARTION                0b10
#define REG_PARTION                 0b11


//ctl info for link
#define LLP_FIFO_DEPTH              128
#define LLP_INFO_WIDTH              178
#define MAC_FLAG_IN_LLP_RANGE       177,177
#define IPVER_IN_LLP_RANGE          176,176
#define MCTL_INFO_WIDTH             (rnic_pdata,PKT_LEN_WIDTH+1)   
#define IPVER_IN_MCTL_RANGE         PKT_LEN_WIDTH,PKT_LEN_WIDTH
#define PKTLEN_IN_MCTL_RANGE        (rnic_pdata,PKT_LEN_WIDTH-1),0


void    pbu_init                    (struct rnic_pdata *);
void    pbu_reg_write               (struct rnic_pdata *,int,int);
int     pbu_reg_read                (struct rnic_pdata *,int);
void    pbu_init_for_recv_req       (struct rnic_pdata *,int,int,int,int,int,int);
void    pbu_init_for_recv_rsp       (struct rnic_pdata *,int,int,int,int);
void    pbu_init_mtu_reg            (struct rnic_pdata *,int);
void    pbu_set_general_reg         (struct rnic_pdata *,int,int);

#endif