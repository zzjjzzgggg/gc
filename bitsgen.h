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
        cache_ed_nd_, max_nid_;
    size_t bits_, bytes_per_bkt_, bytes_per_hp_, size_per_hp_;
    TBitV src_bits_, cache_bits_;

#ifdef USE_SNAPPY
    snappy::SnappyOut wtr_;
    snappy::SnappyIn rdr_;
#endif // USE_SNAPPY

#ifdef USE_LZ4
    lz4::LZ4Out wtr_;
    lz4::LZ4In rdr_;
#endif // USE_LZ4

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
