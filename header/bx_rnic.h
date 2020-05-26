//insomnia@2020/2/9 23:44:25

#ifndef __BX_RNIC_H_
#define __BX_RNIC_H_

#include "bx_cfg.h"


//#define PORT_0_LINK_UP_ONLY
//#define PORT_0_AN_EN
//#define PORT_0_RS_FEC_EN
//#define PORT_0_BASE_R_FEC_EN
//#define PORT_0_KRT_EN
//#define PCS_PHY_T2R_LB_EN
//#define PCS_AN_PDET_EN
//#define PCS_PHY_R2T_LB_EN

#define RNIC_LEGACY_INT_EN

#ifndef RNIC_LEGACY_INT_EN
#define RNIC_MSI_EN
#endif

//#define NAPI_POLL_WEIGHT 32
#define CUST_CHANNEL_NUM                    1
#define RNIC_MSI_REQ_IRQ_CNT                16      //16 for port 0 only, 32 for port 0 and 1 
#define RNIC_MAX_LINK_DOWN_CNT              3
#define IEU_CLR_INTR_TIMER_USECS            1000    //1ms
#define PCS_AN_MAX_RCV_CNT                  50
#define PCS_AN_MAX_INTR_CHECK_CNT           10000
#define PCS_LINK_CHECK_SLOW_TIMER_USECS     100000  //100ms
#define PCS_LINK_CHECK_FAST_TIMER_USECS     1       //1us
#define PCS_AN_RESTART_LOCK_TIMER_USECS     1000000 //1s


#ifndef CUST_CHANNEL_NUM
#define MAXIMIZE_RX_0_FIFO
#endif

//#define PRINT_AVERAGE
//#define RNIC_DEBUG
//#define RNIC_DEBUG_TRACE
//#define RNIC_DEBUG_TRACE_DESC

#define PCS_PHY_T2R_LB_EN //added by hs


#define RNIC_PCI_VENDOR_ID                  0x1ea9//0x16ca
#define RNIC_PCI_DEVICE_ID                  0x7312
#define DRIVER_NAME                         "binxin_rnic"
#define DRIVER_VERSION                      "1.0.0"
#define DRIVER_DESC                         "Binxin RNIC Driver"


#include "bx_rnic_com.h"
#include "bx_mac.h"
//added by hs@20200416
#include "bx_roce.h"

#include "bx_mac_reg.h"
#include "bx_mac_cfg.h"
#include "bx_pcs.h"
#include "bx_mpu.h"
#include "bx_mpp.h"
#include "bx_cm.h"
#include "bx_ieu.h"
#include "bx_pbu.h"
#include "bx_phd.h"
#include "bx_mpb.h"
#include "bx_pcie.h"


#endif
