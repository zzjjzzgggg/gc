#include "bitsgen.h"

BitStrCal::BitStrCal(const Parms& pm) {
    gf_nm_ = pm.gf_nm_;
    blk_sz_ = pm.blk_sz_;
    max_nid_ = pm.max_nid_;
    max_hops_ = pm.max_hops_;
    num_approx_ = pm.num_approx_;
    bkt_bits_ = pm.bkt_bits_;
    more_bits_ = pm.more_bits_;
    page_sz_ = pm.page_sz_;
    blk_st_nd_ = blk_ed_nd_ = cache_st_nd_ = cache_ed_nd_ =-1;

    // bits per nd per hop
    num_bits_ = int(ceil(TMath::Log2(max_nid_ + 1))) - bkt_bits_ +
        more_bits_;
    num_bkts_ = TMath::Pow2(bkt_bits_);
    bytes_per_bkt_ = num_approx_ * num_bits_ /8;
    bytes_per_hp_ = num_bkts_ * bytes_per_bkt_;
    size_per_hp_ = bytes_per_hp_ /sizeof(uint64);

    assert(blk_sz_ % page_sz_ == 0);
    assert(bytes_per_hp_ % sizeof(uint64) == 0);

    src_bits_.Gen(size_per_hp_ * blk_sz_);

    // cache parms
    cache_bits_.Gen(page_sz_ * size_per_hp_);

    if(!TFile::Exists(gf_nm_.GetFPath() + "blks"))
        TFile::MkDir(gf_nm_.GetFPath() + "blks");
    if(!TFile::Exists(gf_nm_.GetFPath() + "bits"))
        TFile::MkDir(gf_nm_.GetFPath() + "bits");

    printf("one hop: %s, page size: %s, block size: %s\n",
           PrettySize(bytes_per_hp_).CStr(),
           PrettySize(bytes_per_hp_ * page_sz_).CStr(),
           PrettySize(bytes_per_hp_ * blk_sz_).CStr());
}

void BitStrCal::SplitGraph() {
    FILE* blk_wtr = NULL;
    int blk_id = -1, cur_blk_id, num_blks = 0;
    TSsParser ss(gf_nm_); //true: silent
    while (ss.Next()) {
        const int src_nd = ss.GetInt(0), dst_nd = ss.GetInt(1);
        max_nid_ = TMath::Mx(max_nid_, src_nd, dst_nd);
        cur_blk_id = src_nd / blk_sz_;
        if (cur_blk_id != blk_id) { // a new block
            if (blk_wtr != NULL) {
                fclose(blk_wtr);
                SortAndCompressGraph(blk_id);
            }
            blk_id = cur_blk_id;
            blk_wtr = fopen("/tmp/graph_block", "wt");

            printf(" block %d...", blk_id); fflush(stdout);
        }
        fprintf(blk_wtr, "%d\t%d\n", src_nd, dst_nd);
    }
    if (blk_wtr != NULL) {
        fclose(blk_wtr);
        SortAndCompressGraph(blk_id);
    }
    printf("\n\nnum_blocks = %d, max_nid_ = %d\n",
           num_blks, max_nid_);
    WriteParms(true);
}

void BitStrCal::SortAndCompressGraph(const int blk_id) {
    system("sort -n -k2 /tmp/graph_block -o /tmp/graph_block_sorted");
    wtr_.SetFileName(GetGBlkFNm(blk_id).CStr());
    TSsParser ss("/tmp/graph_block_sorted");
    while (ss.Next()) {
        wtr_.Save(ss.GetInt(0));
        wtr_.Save(ss.GetInt(1));
    }
    wtr_.Close();
    system("rm /tmp/graph_block*");
}


void BitStrCal::InitBits() {
    size_t bit, bkt_off = 0;
    uint rnd_num, bkt_mask = 0;
    for (int i=0; i<bkt_bits_; i++) bkt_mask = (bkt_mask<<1) | 1;
    TBitV bits(size_per_hp_);
    char* const bits_hdr = (char*) bits.BegI();
    int page_id = 0;
    for (int nd = 0; nd<=max_nid_; nd++) {
        if (nd%page_sz_ == 0)
            wtr_.SetFileName(GetBitsFNm(page_id++, 0).CStr());
        bits.PutAll(0);
        for (int approx=0; approx<num_approx_; approx++) {
            rnd_num = TInt::Rnd.GetUniDevUInt();
            bkt_off = (rnd_num & bkt_mask) * bytes_per_bkt_;
            rnd_num >>= bkt_bits_;
            for(bit=0; bit<num_bits_ && (1<<bit & rnd_num)==0; bit++);
            if (bit >= num_bits_)  bit = num_bits_ - 1;
            *(bits_hdr + bkt_off + (num_approx_ *bit + approx)/8) |=
                            1<<(approx % 8);
        }
        wtr_.Write(bits_hdr, bytes_per_hp_);
    }
    wtr_.Close();
}

void BitStrCal::GenBits() {
    TExeSteadyTm tm;
    WriteParms(false);
    const int max_bid = max_nid_ /blk_sz_;
    for (int hop=1; hop<=max_hops_; hop++) {
        printf("Handling hop %d... \n", hop);
        tm.Tick();
        for (int blk_id = 0; blk_id <= max_bid; blk_id++) {
            InitSrcBits(blk_id, hop-1);
            UpdateBlockAtHop(blk_id, hop);
            SaveSrcBitsAtHop(hop);

            PrintStatusBar(blk_id+1, max_bid+1, tm);
        }
        printf("\n");
        ClearCache();
    }
}

void BitStrCal::InitSrcBits(const int blk_id, const int hop) {
    char* buf = (char*)(src_bits_.BegI());
    blk_st_nd_ = blk_id * blk_sz_;
    blk_ed_nd_ = min(max_nid_, (blk_id + 1) * blk_sz_ -1);
    for (int page_id = blk_st_nd_ /page_sz_;
         page_id <= blk_ed_nd_ /page_sz_; page_id++)
        ReadPageAtHop(page_id, hop, buf);
}

void BitStrCal::UpdateBlockAtHop(const int blk_id, const int hop) {
    TStr block_file_name = GetGBlkFNm(blk_id);
    if(!TFile::Exists(block_file_name)) return;
    int src_nd, dst_nd;

#ifdef USE_SNAPPY
    snappy::SnappyIn rdr(block_file_name.CStr());
#endif // USE_SNAPPY
#ifdef USE_LZ4
    lz4::LZ4In rdr(block_file_name.CStr());
#endif // USE_LZ4

    while (!rdr.Eof()) {
        rdr.Load(src_nd);
        rdr.Load(dst_nd);
        UpdateSrcBits(hop, src_nd, dst_nd);
    }
}

void BitStrCal::UpdateSrcBits(const int hop, const int src_nd,
                              const int dst_nd) {
    uint64* wrt_ptr = src_bits_.BegI() +
        (src_nd - blk_st_nd_) * size_per_hp_;
    uint64* rdr_ptr = FetchNdBitsAtHop(dst_nd, hop-1);
    for (size_t ub=0; ub<size_per_hp_; ub++, wrt_ptr++, rdr_ptr++)
        *wrt_ptr |= *rdr_ptr;
}

uint64* BitStrCal::FetchNdBitsAtHop(const int nd, const int hop) {
    if (nd<cache_st_nd_ || nd>cache_ed_nd_)
        PinPageAtHop(nd/page_sz_, hop);
    return cache_bits_.BegI() + (nd - cache_st_nd_) * size_per_hp_;
}

// pin the page that contains the nd into memory
void BitStrCal::PinPageAtHop(const int page_id, const int hop) {
    char* bits_ptr = (char*)(cache_bits_.BegI());
    ReadPageAtHop(page_id, hop, bits_ptr);
    cache_st_nd_ = page_id * page_sz_;
    cache_ed_nd_ = min((page_id + 1) * page_sz_ -1, max_nid_);
}

// Important: use the reference of the pointer
void BitStrCal::ReadPageAtHop(const int page_id, const int hop,
                              char*& buf) {
    size_t num_read;
    rdr_.SetFileName(GetBitsFNm(page_id, hop).CStr());
    while(rdr_.ReadChunk(buf, num_read)) buf += num_read;
    rdr_.Close();
}

void BitStrCal::SaveSrcBitsAtHop(const int hop) {
    char* bits_ptr = (char*)src_bits_.BegI();
    for (int nd=blk_st_nd_; nd<=blk_ed_nd_; nd++) {
        if (nd % page_sz_ == 0)
            wtr_.SetFileName(GetBitsFNm(nd/page_sz_, hop).CStr());
        wtr_.Write(bits_ptr, bytes_per_hp_);
        bits_ptr += bytes_per_hp_;
    }
    wtr_.Close();
}

void BitStrCal::ClearCache() {
    cache_st_nd_ = cache_ed_nd_ = -1;
}

void BitStrCal::WriteParms(const bool after_split_graph) {
    const char* fnm = after_split_graph ? "blks/parms" : "bits/parms";
    FILE* file_id = fopen((gf_nm_.GetFPath() + fnm).CStr(), "wt");
    if(after_split_graph)
        fprintf(file_id, "BlockSize\t%d\nMaxNId\t%d\n",
                blk_sz_, max_nid_);
    else
        fprintf(file_id, "PageSize\t%d\n"
                "BlockSize\t%d\n"
                "MaxHops\t%d\n"
                "NApprox\t%d\n"
                "MaxNId\t%d\n"
                "BktBits\t%d\n"
                "MoreBits\t%d\n",
                page_sz_, blk_sz_, max_hops_, num_approx_,
                max_nid_, bkt_bits_, more_bits_);
    fclose(file_id);
}
