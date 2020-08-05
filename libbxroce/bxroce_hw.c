#include "bxroce_main.h"

void bxroce_mpb_reg_write(void *iova, uint32_t module_addr, uint32_t regaddr, uint32_t value)
{
   

    udma_to_device_barrier();
	*(__le32 *)((uint8_t *)(iova) + MPB_WRITE_ADDR) = htole32(module_addr + regaddr);

    *(__le32 *)((uint8_t *)(iova) + MPB_RW_DATA) = htole32(value);
	
}

uint32_t bxroce_mpb_reg_read(void *iova, uint32_t module_addr, uint32_t regaddr)
{
   
    uint32_t regval = 0;

    udma_to_device_barrier();
	*(__le32 *)((uint8_t *)(iova) + MPB_WRITE_ADDR) = htole32(module_addr + regaddr);

	udma_from_device_barrier();
    regval = le32toh(*(__le32 *)((uint8_t *)(iova) + MPB_RW_DATA));
    
    return regval;
}
