#include "iogreedy.h"

// move will be called
IOGreedy::Block IOGreedy::PinBlock(const int id) {
    Block blk;
    // first check whether the block is in memory or on disk
    auto map_iter = id_blkiter_map_.find(id);
    if (map_iter != id_blkiter_map_.end()) { // in memory
        auto blk_iter = map_iter->second;
        blk = move(*blk_iter);
        id_blkiter_map_.erase(map_iter);
        blk_lst_.erase(blk_iter);
    } else { // read disk
        blk.id_ = id;
        lz4::LZ4In page_rdr(BlockFNm(id).CStr());
        while(!page_rdr.Eof())
            blk.Add(move(Node(page_rdr, size_per_nd_)));
        TFile::Del(BlockFNm(id)); // we nolonger need this block file
    }
    return blk;
}

IOGreedy::BlockIter IOGreedy::InsertBlockSorted(Block&& block) {
    auto iter = blk_lst_.begin();
    while (iter != blk_lst_.end() &&
           iter->interval_ > block.interval_) iter++;
    return blk_lst_.insert(iter, move(block));
}

void IOGreedy::WriteBlock(const Block& blk) {
    lz4::LZ4Out wtr(BlockFNm(blk.id_).CStr());
    blk.Save(wtr);
}

void IOGreedy::SaveNode(Node&& node) {
    int inter = node.interval_;
    // if the interval is non-empty, the tail block is in mem, and
    // tail block has space available, then we append the node to
    // the tail block.
    if(!inter_blkidlst_[inter].empty()) {
        int tail_blk_id = inter_blkidlst_[inter].back();
        auto iter = id_blkiter_map_.find(tail_blk_id);
        if(iter != id_blkiter_map_.end() &&
           iter->second->Len() < page_sz_) {
            iter->second->Add(move(node));
            return;
        }
    }
    // otherwise we need to create a new block in mem.
    // First, make sure we have space available.
    // blk_lst_ is sorted decendingly, so we search from the head
    // and delete the first full block.
    if (blk_lst_.size() >= buf_capacity_) {
        auto iter = blk_lst_.begin();
        while (iter != blk_lst_.end() &&
               blk_lst_.size() >= buf_capacity_) {
            if (iter->Len() == page_sz_) {
                WriteBlock(*iter);
                id_blkiter_map_.erase(iter->id_);
                blk_lst_.erase(iter++);
            } else {
                iter ++;
            }
        }
    }
    // now we must have free space, then create a new block
    Block blk(id_, inter, move(node));
    inter_blkidlst_[inter].push_back(id_);
    id_blkiter_map_[id_] = InsertBlockSorted(move(blk));
    id_ ++;
}

// organize original node bit-strings into intervals.
void IOGreedy::Init() {
    lz4::LZ4In rdr;
    double rwd;
    for (int core=0; core<num_processors_; core++){
        TStr fnm = GetMxRwdFNm(core);
        if (!TFile::Exists(fnm)) continue;
        rdr.SetFileName(fnm.CStr());
        rdr.Load(rwd);
        if(rwd > max_rwd_) max_rwd_ = rwd;
    }
    assert(max_rwd_ > 0);
    intervals_ =(int)ceil((log(0.01)-log(max_rwd_))/log(lambda_));
    printf("max rwd: %.4f, intervals: %d\n", max_rwd_, intervals_);
    assert((size_t)intervals_ <= buf_capacity_);
    inter_blkidlst_.resize(intervals_);

    // Todo: the following needs to be optimized
    for (int core=0; core<num_processors_; core++){
        TStr fnm = GetNdRwdFNm(core);
        if (!TFile::Exists(fnm)) continue;
        rdr.SetFileName(fnm.CStr());
        while (!rdr.Eof()) {
            Node node;
            rdr.Load(node.id_);
            rdr.Load(node.rwd_);
            if (node.rwd_ < 0.1) continue;
            node.Resize(size_per_nd_);
            GetNodeBits(node.id_, node.Ptr());
            node.interval_ = GetIntervalID(node.rwd_);
            SaveNode(move(node));
        }
    }
    printf("Initializtion cost time %s\n", tm_.GetStr());
}

// return -1 if stopped; node index if successed in finding a node;
// -2 if completed this block.
int IOGreedy::UpdateBlock(const int blk_no) {
    Block& block = blocks_processing_[blk_no];
    for (int i=0; i<block.Len(); i++) {
        // std::unique_lock<std::mutex> lock_read(stop_mutex_);
        if ( block.interval_ >= stop_level_) return -1;
        // else lock_read.unlock();
        Node& node = block.nodes_[i];
        if (node.tm_ < t_) {
            node.rwd_ = GetRewardGain(node.id_, node.Ptr());
            node.tm_ = t_;
            node.interval_ = GetIntervalID(node.rwd_);
        }
        if(node.interval_ <= block.interval_) {
            // this process successed!
            std::unique_lock<std::mutex> lock(stop_mutex_);
            // in case some higher level process has successed
            if (block.interval_ >= stop_level_) return -1;
            stop_level_ = node.interval_;
            return i;
        }
    }
    return -2;
}

void IOGreedy::Iterations() {
    const TStr& rst_fnm = gf_nm_.GetFPath() +
        TStr::Fmt("iogreedy_%g_%d_%d.dat",
                  lambda_, max_hops_, budget_);
    FILE* fw = fopen(rst_fnm.CStr(), "w");
    fprintf(fw, "# step\tnode\treward\tgc\ttime\n");
    stop_level_ = intervals_; // "lowest" level
    vector< future<int> > futures;
    while (group_.Len() < budget_) {
        int inter=0;
        while(inter<intervals_ && inter_blkidlst_[inter].empty())
            inter++;
        // no data available
        if (inter == intervals_ && futures.empty()) {
            printf("no data available!\n");
            break;
        }
        // parallel processing in one interval
        while(!inter_blkidlst_[inter].empty() &&
              futures.size() < (size_t)num_processors_) {
            int head_blk_id = inter_blkidlst_[inter].front();
            inter_blkidlst_[inter].pop_front();
            Block block = PinBlock(head_blk_id);
            block.interval_ = inter;
            int blk_idx = blocks_processing_.size();
            blocks_processing_.push_back(move(block));
            futures.push_back(async(launch::async,
                                    &IOGreedy::UpdateBlock,
                                    this, blk_idx));
        }
        if (futures.size() < (size_t)num_processors_ &&
            inter != intervals_)
            continue; // keep on adding jobs
        int blk_no = 0;
        for(auto& f: futures) {
            int code = f.get();
            Block block = move(blocks_processing_[blk_no++]);
            if(code >=0 &&
               stop_level_ == block.nodes_[code].interval_){
                // found a successed node in position `code`
                HandleSuccessedBlock(block, code, fw);
                if (group_.Len() >= budget_) break;
            } else if(code == -2) {
                // block is updated completely, write to disk
                HandleCompleteBlock(block);
            } else {
                // 1. block is not fully updated due to other block
                // has successed;
                // 2. block is successed, but another 'higher' level
                // has successed also.
                HandleInterruptedBlock(block);
            }
        }
        futures.clear();
        blocks_processing_.clear();
        stop_level_ = intervals_;
    }
    fclose(fw);
    printf("\n");
}

void IOGreedy::HandleSuccessedBlock(Block& block,
                                    const int suc_nd_idx,
                                    FILE* fw) {
    Node& node = block.nodes_[suc_nd_idx];
    assert(node.tm_ == t_);
    AddNode(node.id_, node.rwd_, node.Ptr(), true, fw);
    if (group_.Len() >= budget_) return;
    t_ ++;
    // save the updated nodes
    for (int i=0; i<suc_nd_idx; i++)
        SaveNode(move(block.nodes_[i]));
    if (suc_nd_idx + 1 < block.Len()) {
        // insert the undated half block to block list
        Block half_block(block.id_, block.interval_);
        for (int i = suc_nd_idx + 1; i < block.Len(); i++)
            half_block.Add(move(block.nodes_[i]));
        inter_blkidlst_[block.interval_].push_front(half_block.id_);
        id_blkiter_map_[half_block.id_] =
            InsertBlockSorted(move(half_block));
    }
}

void IOGreedy::HandleCompleteBlock(Block& block) {
    for(Node& node:block.nodes_) SaveNode(move(node));
}

void IOGreedy::HandleInterruptedBlock(Block& block) {
    // insert the block to block list
    inter_blkidlst_[block.interval_].push_front(block.id_);
    id_blkiter_map_[block.id_] = InsertBlockSorted(move(block));
}
