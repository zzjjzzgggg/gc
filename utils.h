#ifndef __UTILS_H__
#define __UTILS_H__

#include "stdafx.h"

class Parms {
public:
    int max_hops_=5, num_approx_=8, bkt_bits_=6, more_bits_=6,
        page_sz_=1000, blk_sz_=10000, max_nid_=1, budget_=500,
        cache_capacity_=10, buf_capacity_, num_processors_;
    double lambda_=0.5, alpha_=0.5;
    TStr gf_nm_;
public:
    Parms(){
        num_processors_ = std::thread::hardware_concurrency();
        if (num_processors_ > 8) num_processors_ = 8;
    }
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
               " alpha: %.2f\n"
               " cache capacity: %d\n"
               " buf capacity: %d\n"
               " processors: %d\n"
               " budget: %d\n\n",
               max_hops_, num_approx_, bkt_bits_, more_bits_,
               page_sz_, blk_sz_,max_nid_, gf_nm_.CStr(),
               lambda_, alpha_, cache_capacity_, buf_capacity_,
               num_processors_,
               budget_);
    }

    void GetGraphSplitParms(const TStr& dir = "blks") {
        if(gf_nm_.Empty()) return;
        TStr parm_fnm = gf_nm_.GetFPath() + dir + "/parms";
        if(TFile::Exists(parm_fnm)) {
            TSsParser ss(parm_fnm);
            ss.Next(); blk_sz_ = ss.GetInt(1);
            ss.Next(); max_nid_ = ss.GetInt(1);
        }
    }

    void GetBitsGenParms(const TStr& dir = "bits") {
        if(gf_nm_.Empty()) return;
        TStr parm_fnm = gf_nm_.GetFPath() + dir + "/parms";
        if(TFile::Exists(parm_fnm)) {
            TSsParser ss(parm_fnm);
            ss.Next(); page_sz_ = ss.GetInt(1);
            ss.Next(); blk_sz_ = ss.GetInt(1);
            ss.Next(); max_hops_ = ss.GetInt(1);
            ss.Next(); num_approx_ = ss.GetInt(1);
            ss.Next(); max_nid_ = ss.GetInt(1);
            ss.Next(); bkt_bits_ = ss.GetInt(1);
            ss.Next(); more_bits_ = ss.GetInt(1);
        }
    }
};

static const double Kilobytes = 1<<10, Megabytes = 1<<20, Gigabytes = 1<<30;

TStr PrettySize(size_t val);

void PrintStatusBar(const int cur, const int total,
                    const TExeSteadyTm& tm);

static const double LLCAlphaTimesM[] = {
    0,
    0.44567926005415,
    1.2480639342271,
    2.8391255240079,
    6.0165231584811,
    12.369319965552,
    25.073991603109,
    50.482891762521
};

static const double HLLCAlpha[] = {
    0,
    0,
    0,
    0,
    0.673,
    0.697,
    0.709,
    0.729
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

// 1/x
static const double InvWeights[] = {
    1,
    1,
    0.5,
    0.333333333,
    0.25,
    0.2,
    0.133333333
};

void write_int(FILE* fp, const int i);
void write_uint64(FILE* fp, const uint64 i);
void read_int(FILE* fp, int* i);
void read_uint64(FILE* fp, uint64* i);

#endif /* __UTILS_H__ */
