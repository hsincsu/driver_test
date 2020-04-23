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


#define BXROCE_DEBUG 1
#define BXROCE_HWDEBUG 1
#define BXROCEDRV_VER "1.0.0.0"
#define BXROCE_FW_VER 0
#define  BXROCE_MIN_Q_PAGE_SIZE	4096
#define  BXROCE_AV_VALID			BIT(7)
#define  BXROCE_AV_VLAN_VALID		BIT(1)

/*for user space access hw reg*/
#define SQ_REG_LEN	0x1000
#define RQ_REG_LEN	0x1000
#define CQ_REG_LEN	0x1000

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

#endif
