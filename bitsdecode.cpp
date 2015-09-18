#include "bitsdecode.h"

BitsDecode::BitsDecode(const Parms& pm) {
    gf_nm_ = pm.gf_nm_;
    max_nid_ = pm.max_nid_;
    max_hops_ = pm.max_hops_;
    approxes_ = pm.num_approx_;
    bkt_bits_ = pm.bkt_bits_;
    more_bits_ = pm.more_bits_;
    page_sz_ = pm.page_sz_;

    // bits per nd per hop
    bits_ = int(ceil(TMath::Log2(max_nid_ + 1))) -
        bkt_bits_ + more_bits_;
    num_bkts_ = TMath::Pow2(bkt_bits_);
    bytes_per_bkt_ = approxes_ * bits_ /8;
    bytes_per_hp_ = num_bkts_ * bytes_per_bkt_;
    bytes_per_pg_hp_ = bytes_per_hp_ * page_sz_;
    size_per_pg_hp_ = bytes_per_pg_hp_ /sizeof(uint64);
    bytes_per_nd_ = bytes_per_hp_ * max_hops_;
    size_per_hp_ = bytes_per_hp_ /sizeof(uint64);
    size_per_nd_ = bytes_per_nd_ /sizeof(uint64);
    assert(bytes_per_hp_ %sizeof(uint64) == 0);

    alpha_m_ = AlphaTimesM[bkt_bits_];

    page_bits_.Gen(size_per_nd_*page_sz_);

    tm_.Tick();
}

void BitsDecode::ReadPageAtHop(const int page_id, const int hop,
                             char* buf) {
    size_t num_read;
    page_rdr_.SetFileName(GetBitsFNm(page_id, hop).CStr());
    while(page_rdr_.ReadChunk(buf, num_read)) buf += num_read;
    page_rdr_.Close();
}

// pin the page that contains the nd into memory
void BitsDecode::PinPage(const int page_id) {
    page_st_nd_ = page_id * page_sz_;
    page_ed_nd_ = min(max_nid_, (page_id + 1)*page_sz_ -1);
    char* page_ptr = (char*)(page_bits_.BegI());
    for (int hop = 1; hop <= max_hops_; hop++) {
        ReadPageAtHop(page_id, hop, page_ptr);
        page_ptr += bytes_per_pg_hp_;
    }
}

double BitsDecode::DecodeAtHop(const char* hop_bits) {
    double est_val = 0;
    for (int apx = 0; apx < approxes_; apx++) {
        double avg_bit_pos = 0, empty_bkts_r = 0;
        for (int m=0; m<num_bkts_; m++) {
            int max_bit = 0;
            for (size_t bit=0; bit<bits_; bit++) {
                if ( *(hop_bits + m * bytes_per_bkt_ +
                       (bit * approxes_ + apx)/8) &
                     (1 << (apx % 8)) )
                    max_bit = bit + 1;
            }
            avg_bit_pos += max_bit;
            if (max_bit == 0) empty_bkts_r++;
        }
        avg_bit_pos /= num_bkts_;
        empty_bkts_r /= num_bkts_;
        if (empty_bkts_r >= 0.051)
            est_val += -num_bkts_ * log(empty_bkts_r);
        else est_val += alpha_m_ * pow(2, avg_bit_pos);
    }
    return est_val / approxes_;
}

char* BitsDecode::GetNodeBitsFstHop(const int node) {
    if(node<page_st_nd_ || node>page_ed_nd_)
        PinPage(node/page_sz_);
    return (char*)(page_bits_.BegI()) +
        (node - page_st_nd_) * bytes_per_hp_;
}

void BitsDecode::GetNodeBits(const int node, char* node_bits) {
    char* hop_bits = GetNodeBitsFstHop(node);
    for(int hop=1; hop<=max_hops_; hop++) {
        memcpy(node_bits, hop_bits, bytes_per_hp_);
        node_bits += bytes_per_hp_;
        hop_bits += bytes_per_pg_hp_;
    }
}

void BitsDecode::GetNodeNbrs(const int nd, vector<double>& nbr_cnt) {
    nbr_cnt[0] = 1;
    char* hop_bits = GetNodeBitsFstHop(nd);
    for(int hop = 1; hop <= max_hops_; hop ++) {
        nbr_cnt[hop] = DecodeAtHop(hop_bits);
        hop_bits += bytes_per_pg_hp_;
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
