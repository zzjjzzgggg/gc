#ifndef __BITSDECODE_H__
#define __BITSDECODE_H__

#include "stdafx.h"
#include "utils.h"

#ifdef USE_SNAPPY
#include "snappy/snappyio.h"
#endif // USE_SNAPPY

#ifdef USE_LZ4
#include "lz4/lz4io.h"
#endif // USE_LZ4


class BitsDecode {
protected:
    TStr gf_nm_;
    int max_hops_, approxes_, bkt_bits_, more_bits_, page_sz_,
        num_bkts_, max_nid_, page_st_nd_=-1, page_ed_nd_=-1;
    size_t bits_, bytes_per_bkt_, bytes_per_hp_, bytes_per_pg_hp_,
        bytes_per_nd_, size_per_hp_, size_per_nd_, size_per_pg_hp_;
    double alpha_m_;

    TBitV page_bits_;
    vector<double> weights_;

#ifdef USE_SNAPPY
    snappy::SnappyIn page_rdr_;
#endif // USE_SNAPPY
#ifdef USE_LZ4
    lz4::LZ4In page_rdr_;
#endif // USE_LZ4

    TExeSteadyTm tm_;
protected:
    TStr GetBitsFNm(const int page_id, const int hop) const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("bits/bits_%d_%d.z", page_id, hop);
    }
    void ReadPageAtHop(const int page_id, const int hop, char* buf);
    // pin all pages with same page_id
    void PinPage(const int page_id);
    // NOTE: node_bits start from hop 1
    double DecodeAtHop(const char* hop_bits);
    char* GetNodeBitsFstHop(const int node);
    void GetNodeBits(const int node, char* node_bits);
    void GetNodeBits(const int node, uint64* node_bits) {
        GetNodeBits(node, (char*)node_bits);
    }
public:
    BitsDecode(const Parms& pm);
    void SetWeights(const double weights[]) {
        weights_.resize(max_hops_ + 1);
        for (int hop=0; hop<=max_hops_; hop++)
            weights_[hop] = weights[hop];
    }
    void GetNodeNbrs(const int node, vector<double>& nbr_cnts);
    double GetGroupGC(const TIntV& group);

};



#endif /* __BITSDECODE_H__ */
