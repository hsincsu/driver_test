//insomnia@2019/12/17 9:15:37

#ifndef __BX_MPU_H_
#define __BX_MPU_H_

#define SPEED_100G              100
#define SPEED_50G               50
#define SPEED_25G               25
#define SPEED_40G               40
#define SPEED_10G               10

#define MAC_SPEED_40G           0b000
#define MAC_SPEED_25G           0b001
#define MAC_SPEED_50G           0b010
#define MAC_SPEED_100G          0b011
#define MAC_SPEED_10G           0b100
#define MAC_SPEED_1G            0b111
              
#define PCS_SPEED_SEL_100G      0b100
#define PCS_SPEED_SEL_50G       0b100
#define PCS_SPEED_SEL_25G       0b101
#define PCS_SPEED_SEL_40G       0b011
#define PCS_SPEED_SEL_10G       0b000
     
#define PCS_TYPE_SEL_100G       0b101
#define PCS_TYPE_SEL_50G        0b100
#define PCS_TYPE_SEL_25G        0b111
#define PCS_TYPE_SEL_40G        0b100
#define PCS_TYPE_SEL_10G        0b000


void mpu_init                   (struct rnic_pdata *);
void mpu_speed_set              (struct rnic_pdata *,int);

#endif