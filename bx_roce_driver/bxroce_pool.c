/* bxroce_pool.c  --2019/10/28 hs*/
#include <rdma/ib_verbs.h>
#include "header/bxroce.h"

struct bxroce_type_info bxroce_type_info[BXROCE_NUM_TYPES] = {
	[BXROCE_TYPE_PD] = {
		.name = "bxroce-pd",
		.size = sizeof(struct bxroce_pd),
	},
	[BXROCE_TYPE_QP] = {
		.name = "bxroce-qp",
		.size = sizeof(struct bxroce_qp),
		.cleanup = bxroce_qp_cleanup,
		.flags = BXROCE_POOL_INDEX,
		.min_index = BXROCE_MIN_QP_INDEX,
		.max_index = BXROCE_MAX_QP_INDEX,
	},
	[BXROCE_TYPE_CQ] = {
		.name = "bxroce-cq",
		.size = sizeof(struct bxroce_cq),
		.cleanup = bxroce_cq_cleanup,
	},
	[BXROCE_TYPE_MR] = {
		.name = "bxroce-mr",
		.size = sizeof(struct bxroce_mr),
		.cleanup = bxroce_mem_cleanup,
		.flags = BXROCE_POOL_INDEX,
		.max_index = BXROCE_MAX_MR_INDEX,
		.min_index = BXROCE_MIN_MR_INDEX,
	},
};

static inline const char *pool_name(struct bxroce_pool *pool)
{
	return bxroce_type_info[pool->type].name;
}

static inline struct kmem_cache *pool_cache(struct bxroce_pool *pool)
{
	return bxroce_type_info[pool->type].cache;
}

static void bxroce_cache_clean(size_t cnt)
{
	int i;
	struct bxroce_type_info *type;

	for (i = 0; i < cnt; i++) {
		type = &bxroce_type_info[i];
		kmem_cache_destroy(type->cache);
		type->cache = NULL;
	}
}

int bxroce_cache_init(void)
{
	int err;
	int i;
	size_t size;
	struct bxroce_type_info *type;

	for (i = 0; i < BXROCE_NUM_TYPES; i++) {
		type = &bxroce_type_info[i];
		size = ALIGN(type->size, BXROCE_POOL_ALIGN);
		type->cache = kmem_cache_create(type->name, size,
			BXROCE_POOL_ALIGN,
			BXROCE_POOL_CACHE_FLAGS, NULL);
		if (!type->cache) {
			pr_err("Unable to init kmem cache for %s\n",
				type->name);
			err = -ENOMEM;
			goto err1;
		}
	}

	return 0;

err1:
	bxroce_cache_clean(i);

	return err;
}

void bxroce_cache_exit(void)
{
	bxroce_cache_clean(BXROCE_NUM_TYPES);
}

static int bxroce_pool_init_index(struct bxroce_pool *pool, u32 max, u32 min)
{
	int err = 0;
	size_t size;

	if ((max - min + 1) < pool->max_elem) {
		pr_warn("not enough indices for max_elem\n");
		err = -EINVAL;
		goto out;
	}

	pool->max_index = max;
	pool->min_index = min;

	size = BITS_TO_LONGS(max - min + 1) * sizeof(long);
	pool->table = kmalloc(size, GFP_KERNEL);
	if (!pool->table) {
		err = -ENOMEM;
		goto out;
	}

	pool->table_size = size;
	bitmap_zero(pool->table, max - min + 1);

out:
	return err;
}


int bxroce_pool_init(struct bxroce_dev *dev, struct bxroce_pool *pool,
	enum bxroce_elem_type type, u32 max_elem)
{
	int		err = 0;
	size_t	size = bxroce_type_info[type].size;

	memset(pool, 0, sizeof(*pool));

	pool->dev = dev;
	pool->type = type;
	pool->max_elem = max_elem;
	pool->elem_size = ALIGN(size, BXROCE_POOL_ALIGN);
	pool->flags = bxroce_type_info[type].flags;
	pool->tree = RB_ROOT;
	pool->cleanup = bxroce_type_info[type].cleanup;

	atomic_set(&pool->num_elem, 0);

	kref_init(&pool->ref_cnt);
	//BXROCE_PR("bxroce: pool_lock init \n");//added by hs 
	rwlock_init(&pool->pool_lock);

	if (bxroce_type_info[type].flags & BXROCE_POOL_INDEX) {
		err = bxroce_pool_init_index(pool,
			bxroce_type_info[type].max_index,
			bxroce_type_info[type].min_index);
		if (err)
			goto out;
	}

	if (bxroce_type_info[type].flags & BXROCE_POOL_KEY) {
		pool->key_offset = bxroce_type_info[type].key_offset;
		pool->key_size = bxroce_type_info[type].key_size;
	}

	pool->state = BXROCE_POOL_STATE_VALID;

out:
	return err;

}

static void bxroce_pool_release(struct kref *kref)
{
	struct bxroce_pool *pool = container_of(kref, struct bxroce_pool, ref_cnt);

	pool->state = BXROCE_POOL_STATE_INVALID;
	kfree(pool->table);
}

static void bxroce_pool_put(struct bxroce_pool *pool)
{
	kref_put(&pool->ref_cnt, bxroce_pool_release);
}

void bxroce_pool_cleanup(struct bxroce_pool *pool)
{
	unsigned long flags;

	write_lock_irqsave(&pool->pool_lock, flags);
	pool->state = BXROCE_POOL_STATE_INVALID;
	if (atomic_read(&pool->num_elem) > 0)
		pr_warn("%s pool destroyed with unfree'd elem\n",
			pool_name(pool));
	write_unlock_irqrestore(&pool->pool_lock, flags);

	bxroce_pool_put(pool);
}

static u32 alloc_index(struct bxroce_pool *pool)
{
	//BXROCE_PR("bxroce: alloc_index start\n");//added by hs 
	u32 index;
	u32 range = pool->max_index - pool->min_index + 1;

	index = find_next_zero_bit(pool->table, range, pool->last);
	if (index >= range)
		index = find_first_zero_bit(pool->table, range);

	WARN_ON_ONCE(index >= range);
	set_bit(index, pool->table);
	pool->last = index;
	//BXROCE_PR("bxroce: alloc_index end!\n");//added by hs 
	return index + pool->min_index;
}

static void insert_index(struct bxroce_pool *pool, struct bxroce_pool_entry *new)
{
	struct rb_node **link = &pool->tree.rb_node;
	struct rb_node *parent = NULL;
	struct bxroce_pool_entry *elem;

	//BXROCE_PR("bxroce: insert_index start \n");//added by hs 
	while (*link) {
		parent = *link;
		elem = rb_entry(parent, struct bxroce_pool_entry, node);

		if (elem->index == new->index) {
			pr_warn("element already exists!\n");
			goto out;
		}

		if (elem->index > new->index)
			link = &(*link)->rb_left;
		else
			link = &(*link)->rb_right;
	}

	rb_link_node(&new->node, parent, link);
	rb_insert_color(&new->node, &pool->tree);
out:
	//BXROCE_PR("bxroce: insert_index end\n");//added by hs 
	return;
}

static void insert_key(struct bxroce_pool *pool, struct bxroce_pool_entry *new)
{
	struct rb_node **link = &pool->tree.rb_node;
	struct rb_node *parent = NULL;
	struct bxroce_pool_entry *elem;
	int cmp;

	while (*link) {
		parent = *link;
		elem = rb_entry(parent, struct bxroce_pool_entry, node);

		cmp = memcmp((u8 *)elem + pool->key_offset,
			(u8 *)new + pool->key_offset, pool->key_size);

		if (cmp == 0) {
			pr_warn("key already exists!\n");
			goto out;
		}

		if (cmp > 0)
			link = &(*link)->rb_left;
		else
			link = &(*link)->rb_right;
	}

	rb_link_node(&new->node, parent, link);
	rb_insert_color(&new->node, &pool->tree);
out:
	return;
}

void bxroce_add_key(void *arg, void *key)
{
	struct bxroce_pool_entry *elem = arg;
	struct bxroce_pool *pool = elem->pool;
	unsigned long flags;

	write_lock_irqsave(&pool->pool_lock, flags);
	memcpy((u8 *)elem + pool->key_offset, key, pool->key_size);
	insert_key(pool, elem);
	write_unlock_irqrestore(&pool->pool_lock, flags);
}

void bxroce_drop_key(void *arg)
{
	struct bxroce_pool_entry *elem = arg;
	struct bxroce_pool *pool = elem->pool;
	unsigned long flags;

	write_lock_irqsave(&pool->pool_lock, flags);
	rb_erase(&elem->node, &pool->tree);
	write_unlock_irqrestore(&pool->pool_lock, flags);
}

void bxroce_add_index(void *arg)
{
	//BXROCE_PR("bxroce: bxroce_add_index start\n");//added by hs 
	struct bxroce_pool_entry *elem = arg;
	struct bxroce_pool *pool = elem->pool;
	unsigned long flags;

	write_lock_irqsave(&pool->pool_lock, flags);
	elem->index = alloc_index(pool);
	//BXROCE_PR("bxroce:mr->index :%d \n",elem->index);//added by hs
	insert_index(pool, elem);
	write_unlock_irqrestore(&pool->pool_lock, flags);
	//BXROCE_PR("bxroce: bxroce_add_index end!\n");//added by hs 
}

void bxroce_drop_index(void *arg)
{
	struct bxroce_pool_entry *elem = arg;
	struct bxroce_pool *pool = elem->pool;
	unsigned long flags;

	write_lock_irqsave(&pool->pool_lock, flags);
	clear_bit(elem->index - pool->min_index, pool->table);
	rb_erase(&elem->node, &pool->tree);
	write_unlock_irqrestore(&pool->pool_lock, flags);
}

void *bxroce_alloc(struct bxroce_pool *pool)
{
	//BXROCE_PR("bxroce: --------------------bxroce_alloc start--------------- \n");//added by hs 
	struct bxroce_pool_entry *elem;
	unsigned long flags;
	//BXROCE_PR("bxroce: next is might_sleep_if\n");//added by hs 
	might_sleep_if(!(pool->flags & BXROCE_POOL_ATOMIC));
	//BXROCE_PR("bxroce: next is read_lock_irqsave\n");//added by hs 
	read_lock_irqsave(&pool->pool_lock, flags);
	if (pool->state != BXROCE_POOL_STATE_VALID) {
		printk("bxroce: pool->state is not valid\n");//added by hs 
		read_unlock_irqrestore(&pool->pool_lock, flags);
		return NULL;
	}
	//BXROCE_PR("bxroce: next is kref_get\n");//added by hs 
	kref_get(&pool->ref_cnt);
	//BXROCE_PR("bxroce: next is read_unlock_irq\n");//added by hs
	read_unlock_irqrestore(&pool->pool_lock, flags);

	//kref_get(&pool->bxroce->ref_cnt);
	//BXROCE_PR("bxroce: next is atomic_inc_return\n");//added by hs 
	//BXROCE_PR("bxorce: pool->num_elem is %x\n, max_elem is %x\n",pool->num_elem,pool->max_elem);//added by hs 
	if (atomic_inc_return(&pool->num_elem) > pool->max_elem)
		goto out_put_pool;

	//BXROCE_PR("bxroce: next is kmem_cache_zalloc\n");//added by hs 
	elem = kmem_cache_zalloc(pool_cache(pool),
		(pool->flags & BXROCE_POOL_ATOMIC) ?
		GFP_ATOMIC : GFP_KERNEL);
	if (!elem)
		goto out_put_pool;

	elem->pool = pool;
	//BXROCE_PR("bxroe: next is kref_init \n");//added by hs 
	kref_init(&elem->ref_cnt);
	//BXROCE_PR("bxroce: bxroce_alloc end\n");//added by hs 

	return elem;

out_put_pool:
	atomic_dec(&pool->num_elem);
	//bxroce_dev_put(pool->bxroce);
	bxroce_pool_put(pool);
	return NULL;
}

void bxroce_elem_release(struct kref *kref)
{
	struct bxroce_pool_entry *elem =
		container_of(kref, struct bxroce_pool_entry, ref_cnt);
	struct bxroce_pool *pool = elem->pool;

	if (pool->cleanup)
		pool->cleanup(elem);

	kmem_cache_free(pool_cache(pool), elem);
	atomic_dec(&pool->num_elem);
	//bxroce_dev_put(pool->bxroce);
	bxroce_pool_put(pool);
}

void *bxroce_pool_get_index(struct bxroce_pool *pool, u32 index)
{
	struct rb_node *node = NULL;
	struct bxroce_pool_entry *elem = NULL;
	unsigned long flags;

	read_lock_irqsave(&pool->pool_lock, flags);

	if (pool->state != BXROCE_POOL_STATE_VALID)
		goto out;

	node = pool->tree.rb_node;

	while (node) {
		elem = rb_entry(node, struct bxroce_pool_entry, node);

		if (elem->index > index)
			node = node->rb_left;
		else if (elem->index < index)
			node = node->rb_right;
		else {
			kref_get(&elem->ref_cnt);
			break;
		} 

	}

out:
	read_unlock_irqrestore(&pool->pool_lock, flags);
	return node ? elem : NULL;
}

void *bxroce_pool_get_key(struct bxroce_pool *pool, void *key)
{
	struct rb_node *node = NULL;
	struct bxroce_pool_entry *elem = NULL;
	int cmp;
	unsigned long flags;

	read_lock_irqsave(&pool->pool_lock, flags);

	if (pool->state != BXROCE_POOL_STATE_VALID)
		goto out;

	node = pool->tree.rb_node;

	while (node) {
		elem = rb_entry(node, struct bxroce_pool_entry, node);

		cmp = memcmp((u8 *)elem + pool->key_offset,
			key, pool->key_size);

		if (cmp > 0)
			node = node->rb_left;
		else if (cmp < 0)
			node = node->rb_right;
		else
			break;
	}

	if (node)
		kref_get(&elem->ref_cnt);

out:
	read_unlock_irqrestore(&pool->pool_lock, flags);
	return node ? elem : NULL;
}

static u8 bxroce_get_key(void)
{
	static u32 key = 1;

	key = key << 1;

	key |= (0 != (key & 0x100)) ^ (0 != (key & 0x10))
		^ (0 != (key & 0x80)) ^ (0 != (key & 0x40));

	key &= 0xff;

	return key;
}
#define IB_ACCESS_REMOTE        (IB_ACCESS_REMOTE_READ          \
                                | IB_ACCESS_REMOTE_WRITE        \
                                | IB_ACCESS_REMOTE_ATOMIC)

static void bxroce_mem_init(int access, struct bxroce_mr *mr)
{
	u32 lkey = mr->pelem.index << 8 | bxroce_get_key();
	u32 rkey = (access & IB_ACCESS_REMOTE) ? lkey : 0;

	if (mr->pelem.pool->type == BXROCE_TYPE_MR) {
		mr->ibmr.lkey = lkey;
		mr->ibmr.rkey = rkey;
	}

	mr->lkey = lkey;
	mr->rkey = rkey;
	mr->state = BXROCE_MEM_STATE_INVALID;
	mr->type = BXROCE_MEM_TYPE_NONE;
	mr->map_shift = ilog2(BXROCE_BUF_PER_MAP);

}

static int bxroce_mem_alloc(struct bxroce_mr *mr, int num_buf)
{
	BXROCE_PR("bxroce: bxroce_mem_alloc start\n");//added by hs 
	int i;
	int num_map;
	struct bxroce_map **map = mr->map;
	num_map = (num_buf + BXROCE_BUF_PER_MAP - 1) / BXROCE_BUF_PER_MAP;
	mr->map = kmalloc_array(num_map, sizeof(*map), GFP_KERNEL);
	if (!mr->map)
		goto err1;

	for (i = 0; i < num_map; i++) {
		mr->map[i] = kmalloc(sizeof(**map), GFP_KERNEL);
		if (!mr->map[i])
			goto err2;
	}

	BUILD_BUG_ON(!is_power_of_2(BXROCE_BUF_PER_MAP));

	mr->map_shift = ilog2(BXROCE_BUF_PER_MAP);
	mr->map_mask = BXROCE_BUF_PER_MAP - 1;

	mr->num_buf = num_buf;
	mr->num_map = num_map;
	mr->max_buf = num_map * BXROCE_BUF_PER_MAP;
	BXROCE_PR("bxroce: bxroce_mem_alloc end\n");//added by hs 
	return 0;

err2:
	for (i--; i >= 0; i--)
		kfree(mr->map[i]);

	kfree(mr->map);
err1:
	return -ENOMEM;


}

int bxroce_mem_init_user(struct bxroce_pd *pd, u64 start, u64 length, u64 iova, int access, struct ib_udata *udata, struct bxroce_mr *mr)
{
		int							entry;
		struct bxroce_map			**map;
		struct bxroce_phys_buf		*buf = NULL;
		struct ib_umem				*umem;
		struct scatterlist			*sg;
		u32							num_buf;
		dma_addr_t					paddr;//to get dma address from user memeory page.
		int err;

		struct bxroce_reg_mr_uresp  uresp; // add uresp 
		int i;
		int status = 0;
		BXROCE_PR("bxroce:%s start \n",__func__);//added by hs
		memset(&uresp, 0 ,sizeof(uresp));

		umem = ib_umem_get(pd->ibpd.uobject->context, start, length, access, 0);
		if (IS_ERR(umem)) {
			printk("err%d from ib_umem_get\n",(int)PTR_ERR(umem));//added by hs
			err = -EINVAL;
			goto err1;
		}
		
		mr->umem = umem;
		num_buf  = umem->nmap;
		bxroce_mem_init(access,mr);
		err = bxroce_mem_alloc(mr,num_buf);
		if (err) {
			printk("err%d from bxroce_mem_alloc \n",err);//added by hs
			ib_umem_release(umem);
			goto err1;
		}

		mr->page_shift	= umem->page_shift;
		printk("mr->page_shift:0x%x\n",mr->page_shift);
		mr->page_mask	= BIT(umem->page_shift) - 1;
		printk("mr->page_mask:0x%x , umem->page_shift:0x%x \n",mr->page_mask, umem->page_shift);

		num_buf			= 0;
		map				= mr->map;
		i = 0;

		if (length > 0) {
			buf = map[0]->buf;
			BXROCE_PR("bxroce:----------%s,check sg_list's dmaaddr---------\n",__func__);//added by hs
			for_each_sg(umem->sg_head.sgl, sg, umem->nmap, entry) {
				paddr = sg_dma_address(sg);
				
				buf->addr = paddr;
				buf->size = BIT(umem->page_shift);
				//to store uresp
			
				if(i == 0)
				{
				uresp.sg_phy_addr[i] = buf->addr;
				uresp.sg_phy_size[i] = buf->size;
				BXROCE_PR("bxroce:sg%d, dmaaddr:0x%lx, bufaddr:0x%lx, dmalen:%d \n",num_buf,paddr,uresp.sg_phy_addr[i],uresp.sg_phy_size[i]);//added by hs
				}
				i++;
				num_buf++;
				buf++;

				if (num_buf >= BXROCE_BUF_PER_MAP) {
						map++;
						buf = map[0]->buf;
						num_buf = 0;
				}
			}
		}

		mr->pd			=pd;
		mr->umem		=umem;
		mr->access		=access;
		mr->length		=length;
		mr->iova		=iova;
		mr->va			=start;
		mr->offset		=ib_umem_offset(umem);
		mr->state		=BXROCE_MEM_STATE_VALID;
		mr->type		=BXROCE_MEM_TYPE_MR;
		
		uresp.sg_phy_num = num_buf;
		uresp.offset	 = mr->offset;
		BXROCE_PR("bxroce:sg_phy_num:0x%x , offset: 0x%x \n",uresp.sg_phy_num, uresp.offset);
		BXROCE_PR("bxroce:uresp's size is :0x%x , udata's size: 0x%x\n",sizeof(uresp),udata->outlen);

		if(udata){
			BXROCE_PR("get in udata \n");
		status = copy_to_user(udata->outbuf +4, &uresp, sizeof(uresp)) ? -EFAULT : 0;//ib_copy_to_udata(udata + 4, &uresp, sizeof(uresp));
		if (status) {
			BXROCE_PR("%s copy error with map user addr: 0x%lx \n",__func__,mr->va);
			return -EINVAL;
			}
		}
		BXROCE_PR("bxroce:check mr..\n");//added by hs
		BXROCE_PR("bxroce:length:%d, iova:0x%lx, va:0x%lx, offset:0x%lx \n",length,iova,start,mr->offset);
		BXROCE_PR("bxroce:-------------------------%s, check end---------------------------\n",__func__);

		return 0;

err1:
		return err;
}


int bxroce_mem_init_fast(struct bxroce_pd *pd, int max_pages, struct bxroce_mr *mr)
{
	int err;

	bxroce_mem_init(0, mr);

	/* In fastreg, we also set the rkey */
	mr->ibmr.rkey = mr->ibmr.lkey;

	err = bxroce_mem_alloc(mr, max_pages);
	if (err)
		goto err1;

	mr->pd = pd;
	mr->max_buf = max_pages;
	mr->state = BXROCE_MEM_STATE_FREE;
	mr->type = BXROCE_MEM_TYPE_MR;

	return 0;

err1:
	return err;

}


int bxroce_mem_init_dma(struct bxroce_pd *pd,int access, struct bxroce_mr *mr)
{
	BXROCE_PR("bxroce: bxroce_mem_init_dma start \n");//added by hs 
	bxroce_mem_init(access,mr);

	mr->pd = pd;
	mr->access = access;
	mr->state = BXROCE_MEM_STATE_VALID;
	mr->type = BXROCE_MEM_TYPE_DMA;
	BXROCE_PR("bxroce: bxroce_mem_init_dma ");//added by hs 
	return 0;
}
