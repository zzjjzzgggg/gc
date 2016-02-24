#ifndef __BITSTRCAL_H__
#define __BITSTRCAL_H__

#include "stdafx.h"
#include "utils.h"

#ifdef USE_SNAPPY
#include "snappy/snappyio.h"
#endif // USE_SNAPPY

#ifdef USE_LZ4
#include "lz4/lz4io.h"
#endif // USE_LZ4


class BitStrCal {
private:
    TStr gf_nm_;
    int blk_sz_, max_hops_, approxes_, bkt_bits_, more_bits_,
        page_sz_, num_bkts_, blk_st_nd_, blk_ed_nd_, cache_st_nd_,
        cache_ed_nd_, max_nid_, num_processors_, max_page_id_;
    size_t bits_, bytes_per_bkt_, bytes_per_hp_, size_per_hp_;
    TBitV src_bits_, cache_bits_;

    TExeSteadyTm tm_;

    mutex mu_;
    condition_variable use_page_cond_, buf_page_cond_;
    int buf_size_, cur_pid_=-1;
    vector<int> pid_table_;
    vector<bool> pid_ready_;

private:
    TStr GetGBlkFNm(const int blk_id) const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("blks/gblk_%d.z", blk_id);
    }
    TStr GetBitsFNm(const int page_id, const int hop) const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("bits/bits_%d_%d.z", page_id, hop);
    }
    void ReadPageAtHop(const int page_id, const int hop, char*& buf);
    void InitSrcBits(const int blk_id, const int hop);
    void PinPageAtHop(const int page_id, const int hop);
    uint64* FetchNdBitsAtHop(const int node, const int hop);
    void ClearCache();
    void UpdateSrcBits(const int hop, const int src_nd,
                       const int dst_nd);
    void UpdateBlockAtHop(const int blk_id, const int hop);
    void SaveSrcBitsAtHop(const int hop);
    void WriteParms(const bool after_split_graph);

    void SynUpdateBlockAtHop(const int blk_id, const int hop);
    void TraverseGraphBlock(const TStr& graph_blk_fnm,
                            const int hop);
    void SynPinPageAtHop(const int hop, const int idx);
    int NextPageToLoad() const {
        int page_id = cur_pid_;
        // try at most buf_size_ times
        for (int i=0; i<buf_size_; i++) {
            int j=0;
            while (j<buf_size_) {
                if(pid_table_[j]==page_id) break;
                j++;
            }
            if (j==buf_size_) break;
            page_id ++;
        }
        return page_id;
    }
public:
    BitStrCal(const Parms& pm);

    void CheckGraph();

    // Split a graph into blocks. REQUIRE: edgelist is sorted by
    // destination node.
    void SplitGraph();
    void InitBits();
    void GenBits();
};



#endif /* __BITSTRCAL_H__ */
