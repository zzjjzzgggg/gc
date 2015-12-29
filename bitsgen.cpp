#include "bitsgen.h"

BitStrCal::BitStrCal(const Parms& pm) {
    gf_nm_ = pm.gf_nm_;
    blk_sz_ = pm.blk_sz_;
    max_nid_ = pm.max_nid_;
    max_hops_ = pm.max_hops_;
    approxes_ = pm.num_approx_;
    bkt_bits_ = pm.bkt_bits_;
    more_bits_ = pm.more_bits_;
    page_sz_ = pm.page_sz_;
    blk_st_nd_ = blk_ed_nd_ = cache_st_nd_ = cache_ed_nd_ =-1;
    assert(blk_sz_ % page_sz_ == 0);

    // bits per nd per hop
    bits_ = int(ceil(TMath::Log2(max_nid_ + 1))) - bkt_bits_ +
        more_bits_;
    num_bkts_ = TMath::Pow2(bkt_bits_);

    assert ((approxes_ * bits_) % 8 == 0);
    bytes_per_bkt_ = approxes_ * bits_ /8;

    size_per_hp_ =  num_bkts_ * bytes_per_bkt_ / sizeof(uint64);
    if (num_bkts_ * bytes_per_bkt_ % sizeof(uint64) != 0)
        size_per_hp_ ++;
    bytes_per_hp_ = size_per_hp_ * sizeof(uint64);

    src_bits_.Gen(size_per_hp_ * blk_sz_);
    cache_bits_.Gen(size_per_hp_ * page_sz_);
    src_bits_.PutAll(0);
    cache_bits_.PutAll(0);

    printf("# of buckets: %d\n", num_bkts_);
    printf("one hop: %s, page size: %s, block size: %s\n",
           PrettySize(bytes_per_hp_).CStr(),
           PrettySize(bytes_per_hp_ * page_sz_).CStr(),
           PrettySize(bytes_per_hp_ * blk_sz_).CStr());
}

void BitStrCal::CheckGraph() {
    int src, dst, dst_pre=-1;
    PSIn pin = TZipIn::New(gf_nm_);
    while(!pin->Eof()) {
        pin->Load(src);
        pin->Load(dst);
        assert(dst >= dst_pre);
        dst_pre = dst;
    }
}

// require edges are sorted by destination node
void BitStrCal::SplitGraph() {
    TStr dir = gf_nm_.GetFPath() + "blks";
    system(("rm -rf "+dir).CStr());
    system(("mkdir "+dir).CStr());

    max_nid_ = -1;
    int blk_id, src, dst;
    THash<TInt, lz4::LZ4Out*> blkid_to_wtr;
    PSIn pin = TZipIn::New(gf_nm_);
    while(!pin->Eof()) {
        pin->Load(src);
        pin->Load(dst);
        max_nid_ = TMath::Mx(max_nid_, src, dst);
        blk_id = src/blk_sz_;
        if (!blkid_to_wtr.IsKey(blk_id)) {
            lz4::LZ4Out* pwtr =
                new lz4::LZ4Out(GetGBlkFNm(blk_id).CStr());
            blkid_to_wtr.AddDat(blk_id, pwtr);
        }
        blkid_to_wtr(blk_id)->Save(src);
        blkid_to_wtr(blk_id)->Save(dst);
    }
    WriteParms(true);
    for(int i=0;i<blkid_to_wtr.Len(); i++)
        delete blkid_to_wtr[i];
}

void BitStrCal::InitBits() {
    TStr dir = gf_nm_.GetFPath() + "bits";
    system(("rm -rf "+dir).CStr());
    system(("mkdir "+dir).CStr());

    size_t bit, bkt_off = 0;
    uint rnd_num, bkt_mask = 0;
    for (int i=0; i<bkt_bits_; i++) bkt_mask = (bkt_mask<<1) | 1;
    TBitV bits(size_per_hp_);
    char* const bits_hdr = (char*) bits.BegI();
    int page_id = 0;
    TRnd rnd(time(NULL) ^ (getpid()<<16));
    for (int nd = 0; nd<=max_nid_; nd++) {
        if (nd%page_sz_ == 0)
            wtr_.SetFileName(GetBitsFNm(page_id++, 0).CStr());
        bits.PutAll(0);
        for (int approx=0; approx<approxes_; approx++) {
            rnd_num = rnd.GetUniDevUInt();
            bkt_off = (rnd_num & bkt_mask) * bytes_per_bkt_;
            rnd_num >>= bkt_bits_;
            for(bit=0; bit<bits_ && (1<<bit & rnd_num)==0; bit++);
            if (bit >= bits_)  bit = bits_ - 1;
            *(bits_hdr + bkt_off + (approxes_ *bit + approx)/8) |=
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
        if(hop%2==1) {
            for (int blk_id = 0; blk_id <= max_bid; blk_id++) {
                if(hop==1 || blk_id!=0) InitSrcBits(blk_id, hop);
                UpdateBlockAtHop(blk_id, hop);
                SaveSrcBitsAtHop(hop);
                PrintStatusBar(blk_id+1, max_bid+1, tm);
            }
        } else {
            for (int blk_id = max_bid; blk_id >= 0; blk_id--) {
                if(blk_id != max_bid) InitSrcBits(blk_id, hop);
                UpdateBlockAtHop(blk_id, hop);
                SaveSrcBitsAtHop(hop);
                PrintStatusBar(max_bid-blk_id+1, max_bid+1, tm);
            }
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
        ReadPageAtHop(page_id, hop-1, buf);
}

void BitStrCal::UpdateBlockAtHop(const int blk_id, const int hop) {
    TStr block_file_name = GetGBlkFNm(blk_id);
    if(!TFile::Exists(block_file_name)) return;
    int src_nd, dst_nd;

    lz4::LZ4In rdr(block_file_name.CStr());

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
                page_sz_, blk_sz_, max_hops_, approxes_,
                max_nid_, bkt_bits_, more_bits_);
    fclose(file_id);
}
