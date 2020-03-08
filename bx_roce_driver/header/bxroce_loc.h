/*some definition --hs*/

#ifndef BXROCE_LOC_H
#define BXROCE_LOC_H

#define BXROCE_MAX_MR_INDEX  0x00040000
#define BXROCE_MIN_MR_INDEX  0x00000001

#define BXROCE_MIN_QP_INDEX  16
#define BXROCE_MAX_QP_INDEX  0x00020000



void bxroce_cq_cleanup(struct bxroce_pool_entry* arg);
void bxroce_qp_cleanup(struct bxroce_pool_entry* arg);
void bxroce_mem_cleanup(struct bxroce_pool_entry* arg);


#endif
