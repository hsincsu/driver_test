/*
 *
 * 	this file for address handle
 * 				eidted by hs 2019/6/24
 *
 *
 */

#include <net/neighbour.h>
#include <net/netevent.h>

#include <rdma/ib_addr.h>
#include <rdma/ib_mad.h>
#include <rdma/ib_cache.h>

#include "header/bxroce.h"
//#include "header/bxroce_verbs.h"
//#include "header/bxroce_ah.h"

static u16 bxroce_hdr_type_to_proto_num(struct bxroce_dev *dev, u8 hdr_type)
{
		switch (hdr_type)
		{
		case RDMA_NETWORK_IB:
			return (u16)ETH_P_IBOE;
		case RDMA_NETWORK_IPV4:
			return (u16)0x0800;
		case RDMA_NETWORK_IPV6:
			return (u16)0x86dd;
		default:
			printk("bxroce: %s,Invalid network header\n",__func__);//added by hs 
			return 0;
		}
}


static inline int bxroce_resolve_dmac(struct bxroce_dev *dev,
                struct rdma_ah_attr *ah_attr, u8 *mac_addr)
{           
        struct in6_addr in6;
            
        memcpy(&in6, rdma_ah_read_grh(ah_attr)->dgid.raw, sizeof(in6));
        if (rdma_is_multicast_addr(&in6))
                rdma_get_mcast_mac(&in6, mac_addr);
        else if (rdma_link_local_addr(&in6))
                rdma_get_ll_mac(&in6, mac_addr);
        else
                memcpy(mac_addr, ah_attr->roce.dmac, ETH_ALEN);
        return 0;
} 

/*set bxroce_av 's attr. According to  rdma_ah_attr.*/
static inline int set_av_attr(struct bxroce_dev *dev, struct bxroce_ah *ah,
							  struct rdma_ah_attr *attr, const union ib_gid *sgid,
							  int pdid, bool *isvlan, u16 vlan_tag)
{
		int status;
        struct bxroce_eth_vlan eth;
        struct bxroce_grh grh;
        int eth_sz;
        u16 proto_num = 0;
        u8 nxthdr = 0x11;
        struct iphdr ipv4;
        const struct ib_global_route *ib_grh;
        union { 
                struct sockaddr     _sockaddr; 
                struct sockaddr_in  _sockaddr_in;
                struct sockaddr_in6 _sockaddr_in6;
        } sgid_addr, dgid_addr;
                                       
        memset(&eth, 0, sizeof(eth));  
        memset(&grh, 0, sizeof(grh));

		/*Protocol Number*/
		proto_num = bxroce_hdr_type_to_proto_num(dev,ah->hdr_type);
		if(!proto_num)
				return -EINVAL;
		nxthdr = (proto_num == ETH_P_IBOE) ? 0x1b : 0x11;

		/*VLAN*/
		if(!vlan_tag || (vlan_tag > 0xfff))
				return -EINVAL;
		if (vlan_tag) {
				eth.eth_type = cpu_to_be16(0x8100);
                eth.roce_eth_type = cpu_to_be16(proto_num);
                eth.vlan_tag = cpu_to_be16(vlan_tag);
                eth_sz = sizeof(struct bxroce_eth_vlan);
                *isvlan = true;
		}
		else {
				eth.eth_type = cpu_to_be16(proto_num);
				eth_sz = sizeof(struct bxroce_eth_basic);
		}

		/*MAC*/
	 		memcpy(eth.smac,dev->devinfo.netdev->dev_addr,ETH_ALEN);
    //		snprintf(eth.smac,ETH_ALEN,"%s",dev->devinfo.netdev>dev_addr);//added by hs
			status = bxroce_resolve_dmac(dev,attr,&eth.dmac[0]);
			if(status)
				return status;
			ib_grh = rdma_ah_read_grh(attr);
			ah->sgid_index = ib_grh->sgid_index;

		/*Eth HDR*/
			memcpy(&ah->av->eth_hdr,&eth,eth_sz);
			if (ah->hdr_type == RDMA_NETWORK_IPV4) {
				 *((__be16 *)&ipv4) = htons((4 << 12) | (5 << 8) |
                                           ib_grh->traffic_class);
                ipv4.id = cpu_to_be16(pdid);
                ipv4.frag_off = htons(IP_DF);
                ipv4.tot_len = htons(0);
                ipv4.ttl = ib_grh->hop_limit;
                ipv4.protocol = nxthdr;
                rdma_gid2ip(&sgid_addr._sockaddr, sgid);
                ipv4.saddr = sgid_addr._sockaddr_in.sin_addr.s_addr;
                rdma_gid2ip(&dgid_addr._sockaddr, &ib_grh->dgid);
                ipv4.daddr = dgid_addr._sockaddr_in.sin_addr.s_addr;
                memcpy((u8 *)ah->av + eth_sz, &ipv4, sizeof(struct iphdr));
			}
			else {
				memcpy(&grh.sgid[0], sgid->raw, sizeof(union ib_gid));
                grh.tclass_flow = cpu_to_be32((6 << 28) |
                                              (ib_grh->traffic_class << 24) |
                                              ib_grh->flow_label);
                memcpy(&grh.dgid[0], ib_grh->dgid.raw,
                       sizeof(ib_grh->dgid.raw));
                grh.pdid_hoplimit = cpu_to_be32((pdid << 16) |
                                                (nxthdr << 8) |
                                                ib_grh->hop_limit);
                memcpy((u8 *)ah->av + eth_sz, &grh, sizeof(struct bxroce_grh));
			}
			if(*isvlan)
				ah->av->valid |= BXROCE_AV_VLAN_VALID;
			ah->av->valid = cpu_to_le32(ah->av->valid);
			return status;

}

int bxroce_alloc_av(struct bxroce_dev *dev, struct bxroce_ah *ah)
{
		int i;
        int status = -EINVAL; 
        struct bxroce_av *av;
        unsigned long flags;
              
        av = dev->av_tbl.va;
        spin_lock_irqsave(&dev->av_tbl.lock, flags);
        for (i = 0; i < dev->av_tbl.num_ah; i++) {
                if (av->valid == 0) {
                        av->valid = BXROCE_AV_VALID; // BIT(7) as a valid identifier.
                        ah->av = av;
                        ah->id = i;
                        status = 0; 
                        break;
                } 
                av++;
        }
        if (i == dev->av_tbl.num_ah)
                status = -EAGAIN;
        spin_unlock_irqrestore(&dev->av_tbl.lock, flags);
        return status;

}

int bxroce_free_av(struct bxroce_dev *dev, struct bxroce_ah *ah)
{
	unsigned long flags;
	spin_lock_irqsave(&dev->av_tbl.lock,flags);
	ah->av->valid = 0;
	spin_unlock_irqrestore(&dev->av_tbl.lock,flags);
	return 0;
}

static int bxroce_copy_ah_uresp(struct bxroce_dev *dev, struct bxroce_ah *ah, struct ib_udata *udata,int vlan_tag)
{
	struct bxroce_create_ah_uresp uresp;
	int eth_sz;
	struct iphdr *ipv4;
	int status;

	memset(&uresp,0,sizeof(uresp));
	if(vlan_tag)
	eth_sz = sizeof(struct bxroce_eth_vlan);
	else
	eth_sz = sizeof(struct bxroce_eth_basic);

	ipv4 = (struct iphdr *)((u8 *)ah->av + eth_sz);

	uresp.daddr = __be32_to_cpu(ipv4->daddr);
	memcpy(&uresp.dmac[0],ah->av->eth_hdr.dmac[0],ETH_ALEN);

	status = ib_copy_to_udata(udata, &uresp, sizeof(uresp));
	if(status)
	{
		printk("failed, copy ah uresp err\n");	
	}

	return status;
}

struct ib_ah *bxroce_create_ah(struct ib_pd *ibpd, struct rdma_ah_attr *attr,u32 flags,
		struct ib_udata *udata)
{
		BXROCE_PR("bxroce:bxroce_create_ah start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		u32 *ahid_addr;
		int status;
		bool isvlan = false;
		u16 vlan_tag = 0xffff;
		const struct ib_gid_attr *sgid_attr;
		struct bxroce_ah *ah;
		struct bxroce_pd *pd = get_bxroce_pd(ibpd);
		struct bxroce_dev *dev= get_bxroce_dev(ibpd->device);
		enum rdma_ah_attr_type ah_type = attr->type;

		if ((ah_type == RDMA_AH_ATTR_TYPE_ROCE) &&
				!(rdma_ah_get_ah_flags(attr) & IB_AH_GRH))
					return ERR_PTR(-EINVAL);

		if (ah_type == RDMA_AH_ATTR_TYPE_ROCE && udata) {
			printk("user mode\n");//added by hs 
		}

		ah = kzalloc(sizeof(*ah),GFP_ATOMIC);
		if(!ah)
			return ERR_PTR(-ENOMEM);
	
		status = bxroce_alloc_av(dev,ah);
		if(status)
			goto av_err;

		sgid_attr = attr->grh.sgid_attr;
		if(is_vlan_dev(sgid_attr->ndev))
				vlan_tag = vlan_dev_vlan_id(sgid_attr->ndev);

		/*Get network header type for this GID*/
		ah->hdr_type = rdma_gid_attr_network_type(sgid_attr);

		status = set_av_attr(dev,ah,attr,&sgid_attr->gid,pd->id,&isvlan,vlan_tag);
		if(status)
				goto av_conf_err;

		/*if pd is for user process , pass it to user space*/
		if (udata) {
			printk("From User Space. Add later \n");//added by hs
			status = bxroce_copy_ah_uresp(dev,ah,udata,vlan_tag);
			if(status)
				goto av_conf_err;
		}

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_create_ah succeed end!\n");//added by hs for printing end info
		return &ah->ibah;
	av_conf_err:
		bxroce_free_av(dev,ah);
	av_err:
		kfree(ah);
		return ERR_PTR(status);
}

int bxroce_destroy_ah(struct ib_ah *ibah,u32 flags)
{
		BXROCE_PR("bxroce:bxroce_destroy_ah start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_ah *ah = get_bxroce_ah(ibah);
		struct bxroce_dev *dev = get_bxroce_dev(ibah->device);

		bxroce_free_av(dev,ah);
		kfree(ah);
		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_destroy_ah succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_query_ah(struct ib_ah *ibah, struct rdma_ah_attr *attr)
{
		BXROCE_PR("bxroce:bxroce_query_ah start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		struct bxroce_ah *ah = get_bxroce_ah(ibah);
		struct bxroce_dev *dev = get_bxroce_dev(ibah->device);
		struct bxroce_grh *grh;
		struct bxroce_av *av = ah->av;

		attr->type = ibah->type;
		if (ah->av->valid & BXROCE_AV_VALID) {
			grh = (struct bxroce_grh *)((u8 *)ah->av +
                                sizeof(struct bxroce_eth_vlan));
            rdma_ah_set_sl(attr, be16_to_cpu(av->eth_hdr.vlan_tag) >> 13);
		}
		else {
			 grh = (struct bxroce_grh *)((u8 *)ah->av +
                                        sizeof(struct bxroce_eth_basic));
             rdma_ah_set_sl(attr, 0);
		}

		 rdma_ah_set_grh(attr, NULL,
                        be32_to_cpu(grh->tclass_flow) & 0xffffffff,
                        ah->sgid_index,
                        be32_to_cpu(grh->pdid_hoplimit) & 0xff,
                        be32_to_cpu(grh->tclass_flow) >> 24);
        rdma_ah_set_dgid_raw(attr, &grh->dgid[0]);

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_query_ah succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_modify_ah(struct ib_ah *ah,struct rdma_ah_attr *ah_attr)
{
		BXROCE_PR("bxroce:bxroce_modify_ah start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_modify_ah succeed end!\n");//added by hs for printing end info
		return 0;
}

int bxroce_pma_counters(struct bxroce_dev *dev, struct ib_mad *out_mad)
{
	return -1; // not support now;
}

int  bxroce_process_mad(struct ib_device *ibdev,
		int process_mad_flags,
		u8 port_num,
		const struct ib_wc *in_wc,
		const struct ib_grh *in_grh,
		const struct ib_mad_hdr *in, size_t in_mad_size,
		struct ib_mad_hdr *out, size_t *out_mad_size,
		u16 *out_mad_pkey_index)
{
		BXROCE_PR("bxroce:bxroce_process_mad start!\n");//added by hs for printing start info
		/*wait to add 2019/6/24*/
		int status;
		struct bxroce_dev *dev;
		const struct ib_mad *in_mad = (const struct ib_mad *)in;
		struct ib_mad *out_mad = (struct ib_mad *)out;

		 if (WARN_ON_ONCE(in_mad_size != sizeof(*in_mad) ||
                         *out_mad_size != sizeof(*out_mad)))
                return IB_MAD_RESULT_FAILURE;

		  switch (in_mad->mad_hdr.mgmt_class) {
        case IB_MGMT_CLASS_PERF_MGMT:
                dev = get_bxroce_dev(ibdev);
                if (!bxroce_pma_counters(dev, out_mad))
                        status = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
                else
                        status = IB_MAD_RESULT_SUCCESS;
                break;
        default:
                status = IB_MAD_RESULT_SUCCESS;
                break;
        }

		/*wait to add end!*/	
		BXROCE_PR("bxroce:bxroce_process_mad succeed end!\n");//added by hs for printing end info
		return status;
}


 				
