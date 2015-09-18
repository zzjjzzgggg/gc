#ifndef __IOGREEDY_H__
#define __IOGREEDY_H__

#include "stdafx.h"
#include "utils.h"

#include "bitsdecode.h"

#ifdef USE_SNAPPY
#include "snappy/snappyio.h"
#endif // USE_SNAPPY

#ifdef USE_LZ4
#include "lz4/lz4io.h"
#endif // USE_LZ4


class IOGreedy: public BitsDecode {
private:
    int intervals_, budget_, pid_=0;
    double lambda_, max_rwd_, gc_=0;
    TBitV group_bits_, tmp_bits_;
    TIntSet group_;
    vector<int> inter_front_cnt_, page_nds_, page_tms_;
    vector<double> page_rwds_;
    vector<list<int>> inter_pids_;
#ifdef USE_SNAPPY
    vector<snappy::SnappyOut> inter_wtrs_;
#endif // USE_SNAPPY
#ifdef USE_LZ4
    vector<lz4::LZ4Out> inter_wtrs_;
#endif // USE_LZ4

private:
    TStr GetNdRwdFNm() const {
        return gf_nm_.GetFPath() + "greedy/node_reward";
    }
    TStr GetMaxRwdFNm() const {
        return gf_nm_.GetFPath() + "greedy/max_reward";
    }
    TStr GetBitsFNm(const int page_id) const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("greedy/bits_%d.z", page_id);
    }
    TStr GetIOGreedyRstFNm() const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("greedy_%g_%d_%d.dat", lambda_, max_hops_, budget_);
    }
    TStr GetMultiPassRstFNm() const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("multipass_%d.dat", budget_);
    }
    // return the number of nodes in the page
    int PinInterPage(const int page_id);
    // NOTE: node_bits start from hop 1
    double GetRewardGain(const int node, const uint64* nd_bits);
    int GetIntervalID(const double rwd) const {
        if(rwd < 1e-6) return intervals_ - 1;
        int inter = (int)floor((log(rwd)-log(max_rwd_))/log(lambda_));
        if(inter >= intervals_) inter = intervals_ - 1;
        // randomness may cause non-submodualr, this needs further
        // investigation.
        if(inter < 0) inter = 0;
        return inter;
    }
    void SaveNodeData(const int node, const int tm,
                      const double rwd, const char* node_bits);
    void AddNode(const int nd, const double rwd, const uint64* bits);
public:
    IOGreedy(const Parms& pm): BitsDecode(pm), budget_(pm.budget_),
                               lambda_(pm.lambda_) {
        group_bits_.Gen(size_per_nd_);
        group_bits_.PutAll(0);
        tmp_bits_.Gen(size_per_nd_);
        page_nds_.resize(page_sz_);
        page_tms_.resize(page_sz_);
        page_rwds_.resize(page_sz_);
        if(!TFile::Exists(gf_nm_.GetFPath() + "greedy"))
            TFile::MkDir(gf_nm_.GetFPath() + "greedy");
        tm_.Tick();
    }
    void GetRewardAll();

    void MultiPass();

    // generate initial intervals and blocks
    void Init();
    void Iterations();
};


#endif /* __IOGREEDY_H__ */
