#include "bitsdecode.h"

BitsDecode::BitsDecode(const Parms& pm) {
    gf_nm_ = pm.gf_nm_;
    max_nid_ = pm.max_nid_;
    max_hops_ = pm.max_hops_;
    approxes_ = pm.num_approx_;
    bkt_bits_ = pm.bkt_bits_;
    more_bits_ = pm.more_bits_;
    page_sz_ = pm.page_sz_;
    cache_capacity_ = pm.cache_capacity_;

    // bits per nd per hop
    bits_ = int(ceil(TMath::Log2(max_nid_ + 1))) - bkt_bits_ +
        more_bits_;
    num_bkts_ = TMath::Pow2(bkt_bits_);

    assert ((approxes_ * bits_) % 8 == 0);
    bytes_per_bkt_ = approxes_ * bits_ /8;

    size_per_hp_ = num_bkts_ * bytes_per_bkt_ / sizeof(uint64);
    if (num_bkts_ * bytes_per_bkt_ % sizeof(uint64) != 0)
        size_per_hp_ ++;
    bytes_per_hp_ = size_per_hp_ * sizeof(uint64);
    size_per_pg_hp_ = size_per_hp_ * page_sz_;
    bytes_per_pg_hp_ = size_per_pg_hp_ * sizeof(uint64);

    bytes_per_nd_ = bytes_per_hp_ * max_hops_;
    size_per_nd_ = size_per_hp_ * max_hops_;
    bytes_per_pg_ = bytes_per_nd_ * page_sz_;
    size_per_pg_ = size_per_nd_ * page_sz_;
    cache_bits_.Gen(size_per_pg_ * cache_capacity_);
    cache_bits_.PutAll(0);

    // group
    group_bits_.Gen(size_per_nd_);
    group_bits_.PutAll(0);

    num_processors_ = std::thread::hardware_concurrency();
    if (num_processors_ > 8) num_processors_ = 8;

   alpha_m_ = HLLCAlpha[bkt_bits_] * num_bkts_ * num_bkts_;

#ifdef USE_EXP_WEIGHT
    SetWeights(ExpWeights);
#endif // USE_EXP_WEIGHT
#ifdef USE_INV_WEIGHT
    SetWeights(InvWeights);
#endif // USE_INV_WEIGHT

    printf("one hop: %s, page size: %s, node bytes: %s\n",
           PrettySize(bytes_per_hp_).CStr(),
           PrettySize(bytes_per_hp_ * page_sz_).CStr(),
           PrettySize(bytes_per_nd_).CStr());
    tm_.Tick();
}

void BitsDecode::ReadPageAtHop(const int page_id, const int hop,
                               char* buf) {
    size_t num_read;
    lz4::LZ4In page_rdr(GetBitsFNm(page_id, hop).CStr());
    while(page_rdr.ReadChunk(buf, num_read)) buf += num_read;
    page_rdr.Close();
}

void BitsDecode::PinPageToCacheUnit(const int page_id,
                                    const int cache_unit_id) {
    char* page_ptr =
        (char*)(cache_bits_.BegI() + cache_unit_id * size_per_pg_);
    for (int hop = 1; hop <= max_hops_; hop++) {
        ReadPageAtHop(page_id, hop, page_ptr);
        page_ptr += bytes_per_pg_hp_;
    }
}

void BitsDecode::PinPage(const int page_id) {
    int unit_id;
    if (cache_pr_lst_.Len() < cache_capacity_) {
        unit_id = cache_pr_lst_.Len();
    } else {
        CacheNd nd_to_del = cache_pr_lst_.Last();
        int page_to_del = nd_to_del->GetVal().Val1;
        unit_id = nd_to_del->GetVal().Val2;
        cache_pid_nd_map_.DelKey(page_to_del);
        cache_pr_lst_.Del(nd_to_del);
    }
    CacheNd new_nd =cache_pr_lst_.AddFront(TIntPr(page_id, unit_id));
    cache_pid_nd_map_.AddDat(page_id, new_nd);
    PinPageToCacheUnit(page_id, unit_id);
}

char* BitsDecode::GetNodeBitsFstHop(const int node) {
    int page_id = node/page_sz_;
    if(!cache_pid_nd_map_.IsKey(page_id)) {
        PinPage(page_id);
    } else {
        CacheNd nd_to_move = cache_pid_nd_map_.GetDat(page_id);
        cache_pr_lst_.PutFront(nd_to_move);
    }
    int unit_id = cache_pr_lst_.First()->GetVal().Val2;
    return (char*)(cache_bits_.BegI() + unit_id*size_per_pg_) +
        (node - page_id*page_sz_)*bytes_per_hp_;
}

void BitsDecode::GetNodeBits(const int node, char* node_bits) {
    char* src_bits = GetNodeBitsFstHop(node);
    for(int hop=1; hop<=max_hops_; hop++) {
        memcpy(node_bits, src_bits, bytes_per_hp_);
        node_bits += bytes_per_hp_;
        src_bits += bytes_per_pg_hp_;
    }
}

double BitsDecode::DecodeAtHop(const char* hop_bits) {
   return DecodeByLCHLLC_fast(hop_bits);
}

double BitsDecode::DecodeByLCHLLC(const char* hop_bits) {
    double est_val = 0;
    for (int n = 0; n < approxes_; n++) {
        double empty_bkts_ratio = 0, inv_z = 0;
        for (int m=0; m<num_bkts_; m++) {
            int max_bit = 0;
            // find the highest position with bit '1'
            for (size_t bit=0; bit<bits_; bit++) {
                if(IsSet(bit, m, n, hop_bits)) max_bit = bit + 1;
            }
            if (max_bit == 0) empty_bkts_ratio++;
            inv_z += pow(2, -max_bit);
        }
        empty_bkts_ratio /= num_bkts_;
        if (empty_bkts_ratio >= 0.1397)
            est_val += -num_bkts_ * log(empty_bkts_ratio);
        else
            est_val += alpha_m_ / inv_z;
    }
    return est_val / approxes_;
}

// need to adapt this to more general case
double BitsDecode::DecodeByLCHLLC_fast(const char* hop_bits) {
    assert(approxes_ == 8);
    vector<double> inv_Z_vec(approxes_, 0);
    vector<double> empty_bkts_vec(approxes_, 0);
    for (int bkt=0; bkt<num_bkts_; bkt++) {
        // ptr points to the last byte of current bucket
        uchar mask = 0xff;
        uchar* ptr = (uchar*)hop_bits + (bkt+1) * bytes_per_bkt_ -1;
        for (int byte=bytes_per_bkt_-1; byte>=0; byte--, ptr--) {
            if((*ptr & mask) == 0) continue;
            // if not zero, we find 1 bit
            // todo: find out which bits are 1
            uchar ucval = *ptr & mask;
            int apx = 0;
            while(ucval != 0) {
                if (ucval & 0x01) {
                    double max_bit = byte+1;
                    inv_Z_vec[apx] += pow(2, -max_bit);
                    // printf("bkt: %d, byte: %d, max bit: %d\n",
                    //        bkt, byte, byte+1);
                }
                ucval >>= 1;
                apx ++;
            }
            // update mask
            mask ^= (*ptr & mask);
            if(mask == 0) break;
        }
        // this bucket is empty in some trails
        int apx = 0;
        while (mask != 0) {
            if (mask & 0x01) {
                inv_Z_vec[apx] += 1;
                empty_bkts_vec[apx] ++;
            }
            mask >>= 1;
            apx ++;
        }
    }
    double est_val = 0;
    for (int apx=0; apx<approxes_; apx++) {
        if (empty_bkts_vec[apx] >= 0.1397 * num_bkts_)
            est_val+= -num_bkts_ *log(empty_bkts_vec[apx]/num_bkts_);
        else
            est_val+= alpha_m_ / inv_Z_vec[apx];
    }
    return est_val / approxes_;
}


void BitsDecode::GetNodeNbrs(const int nd,
                             vector<double>& nbr_cnt) {
    nbr_cnt[0] = 1;
    char* hop_bits = GetNodeBitsFstHop(nd);
    for(int hop = 1; hop <= max_hops_; hop ++) {
        nbr_cnt[hop] = DecodeAtHop(hop_bits);
        hop_bits += bytes_per_pg_hp_;
    }
}

void BitsDecode::GetGroupNbrs(const vector<int>& group,
                              vector<double>& nbr_cnts) {
    nbr_cnts.resize(max_hops_+1);
    nbr_cnts[0] = group.size();
    TBitV group_bits(size_per_nd_);
    group_bits.PutAll(0);
    for(size_t i=0; i<group.size(); i++) {
        uint64* dst = group_bits.BegI();
        uint64* src = (uint64*)GetNodeBitsFstHop(group[i]);
        for(int hop=1; hop<=max_hops_; hop++) {
            for(size_t j=0; j<size_per_hp_; j++)
                *(dst++) |= src[j];
            src += size_per_pg_hp_;
        }
    }
    char* hop_bits = (char*)(group_bits.BegI());
    double n0 = nbr_cnts[0], n1;
    for(int hop=1; hop<=max_hops_; hop++) {
        n1 = DecodeAtHop(hop_bits);
        assert(n1 >= n0);
        nbr_cnts[hop] = n1 - n0;
        hop_bits += bytes_per_hp_;
        n0 = n1;
    }
}

double BitsDecode::GetGroupGC(const TIntV& group) {
    TBitV group_bits(size_per_nd_);
    group_bits.PutAll(0);
    for(int i=0; i<group.Len(); i++) {
        uint64* dst = group_bits.BegI();
        uint64* src = (uint64*)GetNodeBitsFstHop(group[i]);
        for(int hop=1; hop<=max_hops_; hop++) {
            for(size_t j=0; j<size_per_hp_; j++) *(dst++) |= src[j];
            src += size_per_pg_hp_;
        }
    }
    char* hop_bits = (char*)(group_bits.BegI());
    double n0 = group.Len(), n1;
    double gc = n0 * weights_[0];
    for(int hop=1; hop<=max_hops_; hop++) {
        n1 = DecodeAtHop(hop_bits);
        assert(n1 >= n0);
        gc += (n1 - n0) * weights_[hop];
        hop_bits += bytes_per_hp_;
        n0 = n1;
    }
    return gc;
}

// executing in parallel
void BitsDecode::GetRewardAll() {
    const int max_id = max_nid_ / page_sz_;
    int pages_per_core = (int)ceil((max_id+1.0)/num_processors_);
    int pid_fr = 0, pid_to = 0, core = 0;
    vector< future<void> > futures;
    while(pid_fr <= max_id) {
        pid_to = min(max_id, pid_fr + pages_per_core - 1);
        futures.push_back(async(launch::async,
                                &BitsDecode::NodeRewardTask,
                                this, pid_fr, pid_to, core));
        pid_fr += pages_per_core;
        core ++;
    }
    for(auto& f: futures) f.get();
}

void BitsDecode::NodeRewardTask(const int pid_fr,
                                const int pid_to,
                                const int core_id) {
    double rwd, max_rwd = 0;
    lz4::LZ4Out wtr(GetNdRwdFNm(core_id).CStr());
    for (int pid=pid_fr; pid<=pid_to; pid++) {
        int end_nd = min(max_nid_, (pid + 1) * page_sz_ - 1);
        PinPageToCacheUnit(pid, core_id);
        for(int nd=pid*page_sz_; nd<=end_nd; nd++) {
            rwd = GetReward(nd, pid, core_id);
            wtr.Save(nd);
            wtr.Save(rwd);
            if(rwd > max_rwd) max_rwd = rwd;
        }
    }
    wtr.SetFileName(GetMxRwdFNm(core_id).CStr());
    wtr.Save(max_rwd);
}

double BitsDecode::GetReward(const int node,
                             const int page_id,
                             const int core_id) {
    double n0 = 1, n1, rwd = weights_[0];
    char* hop_bits =
        (char*)(cache_bits_.BegI() + core_id * size_per_pg_) +
        (node - page_id*page_sz_) * bytes_per_hp_;
    for(int hop=1; hop<=max_hops_; hop++) {
        n1 = DecodeAtHop(hop_bits);
        rwd += (n1 - n0) * weights_[hop];
        n0 = n1;
        hop_bits += bytes_per_pg_hp_;
    }
    return rwd;
}

////////////////////////////
// node group
double BitsDecode::GetRewardGain(const int nd,
                                 const uint64* nd_bits) {
    if(group_.IsKey(nd)) return 0;
    TBitV tmp_buf(size_per_nd_);
    for (size_t i =0; i<size_per_nd_; i++)
        tmp_buf[i] = group_bits_[i] | nd_bits[i];
    double n0 = group_.Len() + 1, n1;
    double gc = n0 * weights_[0];
    char* hop_bits = (char*)(tmp_buf.BegI());
    for (int hop=1; hop<=max_hops_; hop++) {
        n1 = DecodeAtHop(hop_bits);
        gc += (n1 - n0) * weights_[hop];
        n0 = n1;
        hop_bits += bytes_per_hp_;
    }
    return gc - gc_;
}

void BitsDecode::AddNode(const int node, const double rwd,
                         const uint64* node_bits, const bool echo,
                         FILE* fp) {
    group_.AddKey(node);
    gc_ += rwd;
    for (size_t i=0; i<size_per_nd_; i++)
        group_bits_[i] |= node_bits[i];
    if(echo) {
        printf("  %4d %9d %12.2f %12.2f\r",
               group_.Len(), node, rwd, gc_);
        fflush(stdout);
    }
    if(fp != NULL) {
        fprintf(fp, "%d\t%d\t%.6f\t%.6f\t%.2f\n",
                group_.Len(), node, rwd, gc_, tm_.GetSecs());
        fflush(fp);
    }
}
