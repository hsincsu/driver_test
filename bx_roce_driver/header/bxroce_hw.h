/*
*			This header file is to define the operation to hardware
*													--edited by hs
*/

#ifndef  __BXROCE_HW_H__
#define  __BXROCE_HW_H__
/*add definition for DMA reg*/
#define DMA_CH_TDTR_HI 	0x20
#define DMA_CH_RDTR_HI	0x28

unsigned int        rdma_set_bits        (unsigned int,unsigned int,unsigned int,unsigned int);
unsigned int        rdma_get_bits        (unsigned int,unsigned int,unsigned int);

int bxroce_init_hw(struct bxroce_dev *);
void bxroce_cleanup_hw(struct bxroce_dev *);
int bxroce_get_hwinfo(struct bxroce_dev *);

/*for create cq*/
int bxroce_hw_create_cq(struct bxroce_dev *,struct bxroce_cq *,int entries,u16 pd_id); //alter this later.
int bxroce_alloc_cqqpresource(struct bxroce_dev *dev, unsigned long *resource_array, u32 max_resources,u32 *req_resource_num,u32 *next );

/*bxroce_free_cqqpresource -free resouce allocated.
*
*/
static inline void  bxroce_free_cqqpresource(struct bxroce_dev *dev, unsigned long *resource_array, u32 resource_num)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->resource_lock,flags);
	clear_bit(resource_num, resource_array);
	spin_unlock_irqrestore(&dev->resource_lock ,flags);
}

/**
*bxroce_is_resource_allocated - determinate if resource is allocated
*@dev: device pointer
*@resource_array :resource array for the resource_num
*@resource_num: resource number to check
**/
static inline bool bxroce_is_resource_allocated(struct bxroce_dev *dev, unsigned long *resource_array, u32 resource_num)
{
	bool bit_is_set;
	unsigned long flags;

	spin_lock_irqsave(&dev->resource_lock, flags);
	
	bit_is_set = test_bit(resource_num, resource_array);
	spin_unlock_irqrestore(&dev->resource_lock, flags);

	return bit_is_set;
}

int bxroce_hw_create_qp(struct bxroce_dev *,struct bxroce_qp *,struct bxroce_pd *,struct ib_qp_init_attr *);
enum ib_qp_state get_ibqp_state(enum bxroce_qp_state qps);
enum bxroce_qp_state get_bxroce_qp_state(enum ib_qp_state qps);
int bxroce_qp_state_change(struct bxroce_qp *qp,enum ib_qp_state new_ib_state,enum ib_qp_state *old_ib_state);
int bxroce_set_qp_params(struct bxroce_qp *qp,struct ib_qp_attr *attrs,int attr_mask);

#endif		/*__BXROCE_HW_H__*/	
