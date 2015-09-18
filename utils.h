#ifndef __UTILS_H__
#define __UTILS_H__

#include "stdafx.h"

class Parms {
public:
    int max_hops_=5, num_approx_=8, bkt_bits_=6, more_bits_=6, page_sz_=1000,
        blk_sz_=10000, max_nid_=1, budget_=500;
    double lambda_=0.5;
    TStr gf_nm_;
public:
    Parms(){}
    void Echo() const {
        printf(" max_hops: %d\n"
               " num_approx: %d\n"
               " bucket_bits: %d\n"
               " more_bits: %d\n"
               " page_size: %d\n"
               " block_size: %d\n"
               " max_nid: %d\n"
               " graph: %s\n"
               " lambda: %.2f\n"
               " budget: %d\n\n",
               max_hops_, num_approx_, bkt_bits_, more_bits_,
               page_sz_, blk_sz_,max_nid_, gf_nm_.CStr(),
               lambda_, budget_);
    }

    void GetGraphSplitParms(const TStr& dir = "blks") {
        TSsParser ss(gf_nm_.GetFPath() + dir + "/parms");
        ss.Next(); blk_sz_ = ss.GetInt(1);
        ss.Next(); max_nid_ = ss.GetInt(1);
    }

    void GetBitsGenParms(const TStr& dir = "bits") {
        if(gf_nm_.Empty()) return;
        TSsParser ss(gf_nm_.GetFPath() + dir + "/parms");
        ss.Next(); page_sz_ = ss.GetInt(1);
        ss.Next(); blk_sz_ = ss.GetInt(1);
        ss.Next(); max_hops_ = ss.GetInt(1);
        ss.Next(); num_approx_ = ss.GetInt(1);
        ss.Next(); max_nid_ = ss.GetInt(1);
        ss.Next(); bkt_bits_ = ss.GetInt(1);
        ss.Next(); more_bits_ = ss.GetInt(1);
    }
};

static const double Kilobytes = 1<<10, Megabytes = 1<<20, Gigabytes = 1<<30;

TStr PrettySize(size_t val);

void PrintStatusBar(const int cur, const int total,
                    const TExeSteadyTm& tm);

static const double AlphaTimesM[] = {
    0,
    0.44567926005415,
    1.2480639342271,
    2.8391255240079,
    6.0165231584811,
    12.369319965552,
    25.073991603109,
    50.482891762521,
    101.30047482549,
    202.93553337953,
    406.20559693552,
    812.74569741657,
    1625.8258887309,
    3251.9862249084,
    6504.3071471860,
    13008.949929672,
    26018.222470181,
    52036.684135280,
    104073.41696276,
    208139.24771523,
    416265.57100022,
    832478.53851627,
    1669443.2499579,
    3356902.8702907,
    6863377.8429508,
    11978069.823687,
    31333767.455026,
    52114301.457757,
    72080129.928986,
    68945006.880409,
    31538957.552704,
    3299942.4347441
};


// e^-x
static const double ExpWeights[] = {
    1,
    0.367879441,
    0.135335283,
    0.049787068,
    0.018315639,
    0.006737947,
    0.002478752,
    0.000911882,
    0.000335463
};


#endif /* __UTILS_H__ */
