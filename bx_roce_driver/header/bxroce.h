/*
 *
 *  This is a main header file of the driver.Important struct start here.
 *
 *
 *
 *  								------------edited by hs in 2019/6/18
 *
 *
 */
#ifndef __BXROCE_H__
#define __BXROCE_H__

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_umem.h>
#include <rdma/ib_cache.h>
#include <rdma/ib_addr.h>

//#include "../bx_roce.h"//  hs 2019/6/19
#include "../../header/bx_rnic.h"
#include "bxroce_pool.h"
#include "bxroce_verbs.h"
#include "bxroce_hw.h"
#include "bxroce_mpb_reg.h"
#include "bxroce_loc.h"
#include "bxroce_abi.h"
#include "bxroce_ah.h"


#define  BXROCE_DEBUG  				1
#define  BXROCE_HWDEBUG			    1
#define  BXROCEDRV_VER 				"1.0.0.0"
#define  BXROCE_FW_VER 				0
#define  BXROCE_MIN_Q_PAGE_SIZE		4096
#define  BXROCE_HUGEPAGE_SIZE 		0x200000
#define  BXROCE_AV_VALID			BIT(7)
#define  BXROCE_AV_VLAN_VALID		BIT(1)

/*for user space access hw reg*/
#define SQ_REG_LEN	0x1000
#define RQ_REG_LEN	0x1000
#define CQ_REG_LEN	0x1000

#define MAX_CM_MSG_4BYTE_LEN 100

#ifdef BXROCE_DEBUG
#define BXROCE_PR(fmt, args...)\
		pr_alert("[%s,%d]:" fmt, __func__, __LINE__, ## args)
#else
#define  BXROCE_PR(x...) do { } while(0)
#endif


#ifdef BXROCE_HWDEBUG
#define BXROCE_HWPR(fmt, args...)\
		pr_alert("BXROCE HWDEBUG[%s,%d]:" fmt, __func__, __LINE__, ## args)
#else
#define  BXROCE_HWPR(x...) do { } while(0)
#endif

#define MAC_RDMA_MAC_REG(devinfo, reg)					\
	((devinfo)->mac_base + (reg))


#define MAC_RDMA_MTL_REG(devinfo, n, reg)                    \
    ((devinfo)->mac_base + MTL_Q_BASE + ((n) * MTL_Q_INC) + (reg))

#define RDMA_CHANNEL 6

#define MAC_RDMA_DMA_REG(devinfo, reg)    ((devinfo)->mac_base + DMA_CH_BASE + (DMA_CH_INC * RDMA_CHANNEL) +(reg))

#endif
