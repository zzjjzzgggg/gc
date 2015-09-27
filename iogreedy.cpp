#include "iogreedy.h"

void IOGreedy::GetRewardAll() {
    double n0, n1, rwd, max_rwd = 0;
#ifdef USE_SNAPPY
    snappy::SnappyOut wtr;
#endif // USE_SNAPPY
#ifdef USE_LZ4
    lz4::LZ4Out wtr;
#endif // USE_LZ4
        wtr.SetFileName(GetNdRwdFNm().CStr());
    for(int nd = 0; nd <= max_nid_; nd++) {
        n0 = 1;
        rwd = weights_[0];
        char* hop_bits = GetNodeBitsFstHop(nd);
        for(int hop=1; hop<=max_hops_; hop++) {
            n1 = DecodeAtHop(hop_bits);
            rwd += (n1 - n0) * weights_[hop];
            n0 = n1;
            hop_bits += bytes_per_pg_hp_;
        }
        wtr.Save(nd);
        wtr.Save(rwd);
        if(rwd > max_rwd) max_rwd = rwd;
    }
    wtr.SetFileName(GetMaxRwdFNm().CStr());
    wtr.Save(max_rwd);
    printf("max reward: %.6f\n", max_rwd);
}

int IOGreedy::PinInterPage(const int page_id) {
    int n=0;
    char* bits_ptr = (char*)(page_bits_.BegI());
    page_rdr_.SetFileName(GetBitsFNm(page_id).CStr());
    while(!page_rdr_.Eof()) {
        page_rdr_.Load(page_nds_[n]);
        page_rdr_.Load(page_tms_[n]);
        page_rdr_.Load(page_rwds_[n]);
        page_rdr_.Read(bits_ptr, bytes_per_nd_);
        bits_ptr += bytes_per_nd_;
        n++;
    }
    return n;
}

double IOGreedy::GetRewardGain(const int nd, const uint64* nd_bits) {
    if(group_.IsKey(nd)) return 0;
    for (size_t i =0; i<size_per_nd_; i++)
        tmp_bits_[i] = group_bits_[i] | nd_bits[i];
    double n0 = group_.Len() + 1, n1;
    double gc = n0 * weights_[0];
    char* hop_bits = (char*)(tmp_bits_.BegI());
    for (int hop=1; hop<=max_hops_; hop++) {
        n1 = DecodeAtHop(hop_bits);
        assert(n1 >= n0);
        gc += (n1 - n0) * weights_[hop];
        n0 = n1;
        hop_bits += bytes_per_hp_;
    }
    return gc - gc_;
}

void IOGreedy::SaveNodeData(const int node, const int tm,
                            const double rwd, const char* nd_bits) {
    if(rwd < 1e-6) return; // too small, discard it
    int inter = GetIntervalID(rwd);
    if(inter_front_cnt_[inter] == 0) {
        inter_wtrs_[inter].SetFileName(GetBitsFNm(pid_).CStr());
        inter_pids_[inter].push_back(pid_);
        pid_ ++;
    } else if (inter_wtrs_[inter].IsClosed()) { // append to file
        int pid = inter_pids_[inter].back();
        inter_wtrs_[inter].SetFileName(GetBitsFNm(pid).CStr(), true);
    }
    inter_wtrs_[inter].Save(node);
    inter_wtrs_[inter].Save(tm);
    inter_wtrs_[inter].Save(rwd);
    inter_wtrs_[inter].Write(nd_bits, bytes_per_nd_);
    inter_front_cnt_[inter] =
        (inter_front_cnt_[inter] + 1) % page_sz_;
}

void IOGreedy::AddNode(const int node, const double rwd,
                       const uint64* node_bits) {
    group_.AddKey(node);
    gc_ += rwd;
    for (size_t i=0; i<size_per_nd_; i++)
        group_bits_[i] |= node_bits[i];
    printf("  %4d %9d %9.2f %9.2f\r",
           group_.Len(), node, rwd, gc_);
    fflush(stdout);
}

void IOGreedy::Init() {
    double rwd;
    int cur_page = -1, node;
#ifdef USE_SNAPPY
    snappy::SnappyIn rdr;
#endif // USE_SNAPPY
#ifdef USE_LZ4
    lz4::LZ4In rdr;
#endif // USE_LZ4
    rdr.SetFileName(GetMaxRwdFNm().CStr());
    rdr.Load(max_rwd_);

    intervals_ =(int)ceil((log(0.001) - log(max_rwd_))/log(lambda_));
    inter_pids_.resize(intervals_);
    inter_wtrs_.resize(intervals_);
    inter_front_cnt_.resize(intervals_, 0);
    printf("max rwd: %.4f, intervals: %d\n", max_rwd_, intervals_);

    rdr.SetFileName(GetNdRwdFNm().CStr());
    while(!rdr.Eof()) {
        rdr.Load(node);
        rdr.Load(rwd);
        if(rwd < 1e-6) continue; // omit zero reward nodes
        if (node/page_sz_ != cur_page) {
            cur_page = node/page_sz_;
            PinPage(cur_page);
        }
        char* node_bits = (char*)(page_bits_.BegI()) +
            (node - cur_page * page_sz_) * bytes_per_hp_;
        char* dst_ptr = (char*)(tmp_bits_.BegI());
        for(int hop=1; hop <= max_hops_; hop++) {
            memcpy(dst_ptr, node_bits, bytes_per_hp_);
            node_bits += bytes_per_pg_hp_;
            dst_ptr += bytes_per_hp_;
        }
        SaveNodeData(node, 0, rwd, (char*)(tmp_bits_.BegI()));
    }
    for(auto& wtr: inter_wtrs_) wtr.Close();
}

void IOGreedy::Iterations() {
    int t = 0;
    FILE* fw = fopen(GetIOGreedyRstFNm().CStr(), "w");
    fprintf(fw, "# step\tnode\treward\tgc\ttime\n");
    while (group_.Len() < budget_) {
        int inter=0;
        while(inter<intervals_ && inter_pids_[inter].empty())
            inter++;
        if(inter == intervals_) break; // no data available
        int page_id = inter_pids_[inter].front();
        inter_pids_[inter].pop_front();
        if(inter_pids_[inter].empty()) inter_front_cnt_[inter]=0;
        int n = PinInterPage(page_id);
        for (int i=0; i<n; i++) {
            int nd = page_nds_[i], nd_t = page_tms_[i];
            double rwd = page_rwds_[i];
            uint64* node_bits = page_bits_.BegI() + i*size_per_nd_;
            if (nd_t < t)  rwd = GetRewardGain(nd, node_bits);
            int next_inter = GetIntervalID(rwd);
            if(next_inter >= inter) { // randomness may cause non-submodular
                AddNode(nd, rwd, node_bits);
                fprintf(fw, "%d\t%d\t%.6f\t%.6f\t%.2f\n",
                        ++t, nd, rwd, gc_, tm_.GetSecs());
                if (group_.Len() >= budget_) break;
            } else {
                SaveNodeData(nd, t, rwd, (char*)node_bits);
            }
        } // end for
        TFile::Del(GetBitsFNm(page_id)); // delete page file
        for(auto& wtr: inter_wtrs_) wtr.Close();
    } // end while
    fclose(fw);
    printf("\n");
}

void IOGreedy::MultiPass() {
    int max_nd, t=0;
    double max_rwd, rwd;
    TBitV node_bits(size_per_nd_);
    uint64* const node_bits_ptr = node_bits.BegI();
    FILE* fw = fopen(GetMultiPassRstFNm().CStr(), "w");
    fprintf(fw, "# step\tnode\treward\tgc\ttime\n");
    fprintf(fw, "0\t-1\t0\t0\t%.2f\n", tm_.GetSecs());
    while(group_.Len() < budget_) {
        max_rwd = 0;
        for(int nd=0; nd<=max_nid_; nd++) {
            if(group_.IsKey(nd)) continue;
            GetNodeBits(nd, node_bits_ptr);
            rwd = GetRewardGain(nd, node_bits_ptr);
            if (rwd > max_rwd) {
                max_rwd = rwd;
                max_nd = nd;
            }
        }
        GetNodeBits(max_nd, node_bits_ptr);
        AddNode(max_nd, max_rwd, node_bits_ptr);
        fprintf(fw, "%d\t%d\t%.6f\t%.6f\t%.2f\n",
                ++t, max_nd, max_rwd, gc_, tm_.GetSecs());
    }
    fclose(fw);
    printf("\n");
}
