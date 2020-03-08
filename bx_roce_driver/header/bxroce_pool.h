/*
*	 this file is to alloc memory pool for resources
*
*
*
*/

#ifndef BXROCE_POOL_H
#define BXROCE_POOL_H

#define BXROCE_POOL_ALIGN		  (16)
#define BXROCE_POOL_CACHE_FLAGS  (0)

enum bxroce_pool_flags {
	BXROCE_POOL_ATOMIC			=BIT(0),
	BXROCE_POOL_INDEX			=BIT(1),
	BXROCE_POOL_KEY			=BIT(2),
};

enum bxroce_elem_type {
//	BXROCE_TYPE_UC,
	BXROCE_TYPE_PD,
//	BXROCE_TYPE_AH,
//	BXROCE_TYPE_SRQ,
	BXROCE_TYPE_QP,
	BXROCE_TYPE_CQ,
	BXROCE_TYPE_MR,
//	BXROCE_TYPE_MW,
//	BXROCE_TYPE_MC_GRP,
//	BXROCE_TYPE_MC_ELEM,
	BXROCE_NUM_TYPES,
};

struct bxroce_pool_entry;

struct bxroce_type_info {
	const char *name;
	size_t			size;
	void			(*cleanup)(struct bxroce_pool_entry *obj);
	enum bxroce_pool_flags	flags;
	u32			max_index;
	u32			min_index;
	size_t			key_offset;
	size_t			key_size;
	struct kmem_cache *cache;
};

extern struct bxroce_type_info bxroce_type_info[];

enum bxroce_mem_state {
	BXROCE_MEM_STATE_ZOMBIE,
	BXROCE_MEM_STATE_INVALID,
	BXROCE_MEM_STATE_FREE,
	BXROCE_MEM_STATE_VALID,
};

enum bxroce_mem_type {
	BXROCE_MEM_TYPE_NONE,
	BXROCE_MEM_TYPE_DMA,
	BXROCE_MEM_TYPE_MR,
	BXROCE_MEM_TYPE_FMR,
	BXROCE_MEM_TYPE_MW,
};

enum bxroce_pool_state {
	BXROCE_POOL_STATE_INVALID,
	BXROCE_POOL_STATE_VALID,
};

struct bxroce_pool_entry {
	struct bxroce_pool			*pool;
	struct kref					ref_cnt;
	struct list_head			list;
	
	/*only userd if indexed or keyed*/
	struct rb_node				node;
	u32							index;
};

struct bxroce_pool {
	struct bxroce_dev			*dev;
	rwlock_t					pool_lock; /*protects pool add/del/search*/
	size_t						elem_size;
	struct kref					ref_cnt;
	void						(*cleanup)(struct bxroce_pool_entry *obj);
	enum bxroce_pool_state		state;
	enum bxroce_pool_flags		flags;
	enum bxroce_elem_type		type;

	unsigned int				max_elem;
	atomic_t					num_elem;

	/*only used if indexed or keyed*/
	struct rb_root				tree;
	unsigned long				*table;
	size_t						table_size;
	u32							max_index;
	u32							min_index;
	u32							last;
	size_t						key_offset;
	size_t						key_size;
};

#define BXROCE_BUF_PER_MAP			(PAGE_SIZE/sizeof(struct bxroce_phys_buf))

struct bxroce_phys_buf {
	u64				addr;
	u64				size;
};

struct bxroce_map {
	struct bxroce_phys_buf		buf[BXROCE_BUF_PER_MAP];
};



/*initialize slab caches for managed objects*/
int bxroce_cache_init(void);

/* cleanup slab caches for managed objects */
void bxroce_cache_exit(void);

/* initialize a pool of objects with given limit on
 * number of elements. gets parameters from bxroce_type_info
 * pool elements will be allocated out of a slab cache
 */
int bxroce_pool_init(struct bxroce_dev *bxroce, struct bxroce_pool *pool,
	enum bxroce_elem_type type, u32 max_elem);

/* free resources from object pool */
void bxroce_pool_cleanup(struct bxroce_pool *pool);

/* allocate an object from pool */
void *bxroce_alloc(struct bxroce_pool *pool);

/* assign an index to an indexed object and insert object into
 *  pool's rb tree
 */
void bxroce_add_index(void *elem);

/* drop an index and remove object from rb tree */
void bxroce_drop_index(void *elem);

/* assign a key to a keyed object and insert object into
 *  pool's rb tree
 */
void bxroce_add_key(void *elem, void *key);

/* remove elem from rb tree */
void bxroce_drop_key(void *elem);

/* lookup an indexed object from index. takes a reference on object */
void *bxroce_pool_get_index(struct bxroce_pool *pool, u32 index);

/* lookup keyed object from key. takes a reference on the object */
void *bxroce_pool_get_key(struct bxroce_pool *pool, void *key);

/* cleanup an object when all references are dropped */
void bxroce_elem_release(struct kref *kref);

/* take a reference on an object */
#define bxroce_add_ref(elem) kref_get(&(elem)->pelem.ref_cnt)

/* drop a reference on an object */
#define bxroce_drop_ref(elem) kref_put(&(elem)->pelem.ref_cnt, bxroce_elem_release)

#endif
