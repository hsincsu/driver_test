//insomnia@2020/2/9 23:44:25

#ifndef __BX_RNIC_H_
#define __BX_RNIC_H_

#define PORT_0_10G
//#define PORT_0_25G

//#define PORT_0_AN_EN
//#define PORT_0_RS_FEC_EN
//#define PORT_0_BASE_R_FEC_EN
//#define PORT_0_KRT_EN
//#define PCS_PHY_T2R_LB_EN
//#define PCS_AN_PDET_EN


#define RNIC_LEGACY_INT_EN
//#define RNIC_MSI_EN

//#define NAPI_POLL_WEIGHT 128
#define CUST_CHANNEL_NUM 					1
#define RNIC_MAX_MSI_CNT 					32
#define IEU_CLR_INTR_TIMER_USECS 			1000	//1ms
#define PCS_AN_MAX_RCV_CNT 					10
#define PCS_LINK_CHECK_OK_TIMER_USECS 		1000000	//1s
#define PCS_LINK_CHECK_AN_TIMER_USECS 		1000    //1ms


//#define RNIC_DEBUG
//#define RNIC_DEBUG_TRACE
//#define RNIC_DEBUG_TRACE_DESC


#define RNIC_PCI_VENDOR_ID      0x16ca
#define RNIC_PCI_DEVICE_ID      0x7312

#define DRIVER_NAME             "bx_rnic"
#define DRIVER_VERSION          "1.0.0"
#define DRIVER_DESC             "Binxin RNIC Driver"



#include "bx_rnic_com.h"
#include "bx_mac.h"
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

#include "bx_roce.h" // added by hs for roce , 20200305

#endif
