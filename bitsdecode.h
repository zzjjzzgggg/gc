#ifndef __BITSDECODE_H__
#define __BITSDECODE_H__

#include "stdafx.h"
#include "utils.h"
#include "lz4/lz4io.h"


class BitsDecode {
protected:
    TStr gf_nm_;
    int max_hops_, approxes_, bkt_bits_, more_bits_, page_sz_,
        num_bkts_, max_nid_, cache_capacity_, num_processors_;
    size_t bits_, bytes_per_bkt_,
        bytes_per_hp_, size_per_hp_,
        bytes_per_pg_hp_, size_per_pg_hp_,
        bytes_per_nd_, size_per_nd_,
        bytes_per_pg_, size_per_pg_;
    double alpha_m_, alpha_;

    TBitV cache_bits_;
    // cache structure illustration:
    //    +------+
    // h1:|      |<--u
    //    |      |<--v
    //    |      |
    //    +------+
    // h2:|      |<--u
    //    |      |<--v
    //    |      |
    //    +------+
    // h3:|      |<--u
    //    |      |<--v
    //    |      |
    //    +------+

    vector<double> weights_;

    // cache
    typedef TLst<TIntPr>::PLstNd CacheNd;
    typedef THash<TInt, CacheNd> TIntCacheNdH;
    TLst<TIntPr> cache_pr_lst_; // (PageID, RoomID)
    TIntCacheNdH cache_pid_nd_map_; // PageID -> (PageID, RoomID)

    // node group
    double gc_ = 0;
    TIntSet group_;
    TBitV group_bits_;

    TExeSteadyTm tm_;
protected:
    // pin bits of hop 1 to max_hop to one cache unit.
    // cache_unit should be in range [0, cache_capacity - 1].
    void PinPageToCacheUnit(const int page_id,
                            const int cache_unit);
    void NodeRewardTask(const int pid_from, const int pid_to,
                        const int processor_id);
    inline TStr GetNdRwdFNm(const int core_id) const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("nd_rwd%d_%g_%d.z",
                      max_hops_, alpha_, core_id);
    }
    inline TStr GetMxRwdFNm(const int core_id) const {
        return gf_nm_.GetFPath() + TStr::Fmt("mx_rwd%d_%g_%d.z",
                                       max_hops_, alpha_, core_id);
    }
    TStr GetBitsFNm(const int page_id, const int hop) const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("bits/bits_%d_%d.z", page_id, hop);
    }
    void ReadPageAtHop(const int page_id, const int hop, char* buf);
    // pin pages with page_id from hop 1 to max_hops
    void PinPage(const int page_id);
    // NOTE: node_bits start from hop 1
    double DecodeAtHop(const char* hop_bits) const;
    char* GetNodeBitsFstHop(const int node);
    void GetNodeBits(const int node, char* node_bits);
    void GetNodeBits(const int node, uint64* node_bits) {
        GetNodeBits(node, (char*)node_bits);
    }
    void GetNodeBitsFromCacheUnit(const int core_id, const int node,
                                  uint64* node_bits);
    // NOTE: node_bits start from hop 1
    double GetRewardGain(const int node, const uint64* nd_bits);
    double GetReward(const int node, const int cache_unit);
    double GetReward(const uint64* nd_bits, const int group_size,
                     const uint64* group_bits) const;
    void AddNode(const int nd, const double rwd,
                 const uint64* nd_bits, const bool echo=true,
                 FILE* fp=NULL);
    void Reset() {
        group_bits_.PutAll(0);
        group_.Clr(false);
        gc_ = 0;
        tm_.Tick();
    }
public:
    BitsDecode(const Parms& pm);
    void SetWeights(const double weights[]) {
        weights_.resize(max_hops_ + 1);
        for (int hop=0; hop<=max_hops_; hop++)
            weights_[hop] = weights[hop];
    }
    void GetNodeNbrs(const int node, vector<double>& nbr_cnts);
    double GetNodeNbrsHop1(const int node);
    void GetGroupNbrs(const vector<int>& group,
                      vector<double>& nbr_cnts);
    double GetGroupGC(const TIntV& group);
    double GetGroupGC(const vector<int>& group);
    void GetRewardAll();

    void Test();
};



#endif /* __BITSDECODE_H__ */
