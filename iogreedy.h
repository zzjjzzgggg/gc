#ifndef __IOGREEDY_H__
#define __IOGREEDY_H__

#include "stdafx.h"
#include "utils.h"
#include "lz4/lz4io.h"
#include "bitsdecode.h"

class IOGreedy: public BitsDecode {
private:
    class Node {
    public:
        int id_, tm_, interval_;
        double rwd_;
        vector<uint64> bits_;
    public:
        Node():id_(-1),tm_(0) {}
        Node(lz4::LZ4In& rdr, const size_t& sz) {
            bits_.resize(sz);
            rdr.Load(id_); assert(id_>=0);
            rdr.Load(tm_);
            rdr.Load(rwd_);
            rdr.Read(Ptr(), sz*sizeof(uint64));
        }
        // move constructor
        Node(Node&& node): id_(node.id_), tm_(node.tm_),
                           interval_(node.interval_),
                           rwd_(node.rwd_) {
            bits_ = std::move(node.bits_);
            node.Clr();
        }
        // move assignment
        Node& operator=(Node&& nd) noexcept {
            id_ = nd.id_;
            tm_ = nd.tm_;
            rwd_ = nd.rwd_;
            bits_ = std::move(nd.bits_);
            nd.Clr();
            return *this;
        }
        void Clr() {
            id_ = interval_ = tm_ = -1;
            rwd_ = 0;
            bits_.clear();
        }
        void Resize(const size_t& sz) {
            bits_.resize(sz);
        }
        uint64* Ptr() const {
            return (uint64*)(&(*(bits_.begin())));
        }
        void Save(lz4::LZ4Out& wtr) const {
            if (id_ >= 0 && rwd_ > 0.01) {
                wtr.Save(id_);
                wtr.Save(tm_);
                wtr.Save(rwd_);
                wtr.Write(Ptr(), bits_.size() * sizeof(uint64));
            }
        }
    };
    class Block {
    public:
        int id_, interval_;
        vector<Node> nodes_;
    public:
        Block(): id_(-1), interval_(-1) {}
        Block(const int id, const int inter): id_(id),
                                              interval_(inter) {}
        // move constructor
        Block(const int id, const int inter, Node&& node):
            id_(id), interval_(inter){
            nodes_.push_back(std::move(node));
        }
        // move constructor
        Block(Block&& block): id_(block.id_),
                              interval_(block.interval_) {
            nodes_ = std::move(block.nodes_);
            block.Clr();
        }
        // move assignment
        Block& operator=(Block&& block) {
            id_ = block.id_;
            interval_ = block.interval_;
            nodes_ = std::move(block.nodes_);
            block.Clr();
            return *this;
        }
        int Len() const { return nodes_.size(); }
        void Clr() { id_ = interval_ = -1; nodes_.clear(); }
        void Add(Node&& node) { nodes_.push_back(std::move(node)); }
        void Save(lz4::LZ4Out& wtr) const {
            for (size_t i=0; i<nodes_.size(); i++) {
                nodes_[i].Save(wtr);
            }
        }
    };
    typedef list<Block>::iterator BlockIter;
    typedef list<Node>::iterator NodeIter;

    int intervals_, id_=0, t_=0, budget_, dummy_node_=-1;
    double lambda_, max_rwd_ = 0;

    // buffer settings
    size_t buf_capacity_;
    list<Block> blk_lst_;
    map<int, BlockIter> id_blkiter_map_;
    vector<list<int> > inter_blkidlst_;

    // mutex
    int stop_level_;
    std::mutex mutex_;

    vector<Block> blks_processing_;

    // random greedy
    list<Node> top_nodes_;
private:
    TStr BlockFNm(const int id) const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("greedy/bits_%d.z", id);
    }
    // pin a bit-string page (called a block in IOGreedy)
    // return the number of nodes in the page
    Block PinBlock(const int id);
    BlockIter InsertBlockSorted(Block&& block) {
        auto iter = blk_lst_.begin();
        while (iter != blk_lst_.end() &&
               iter->interval_ > block.interval_) iter++;
        return blk_lst_.insert(iter, move(block));
    }
    NodeIter InsertTopNodeSorted(Node&& node) {
        auto iter = top_nodes_.begin();
        while (iter != top_nodes_.end() &&
               iter->interval_ > node.interval_) iter++;
        return top_nodes_.insert(iter, move(node));
    }

    void WriteBlock(const Block& blk) {
        lz4::LZ4Out wtr(BlockFNm(blk.id_).CStr());
        blk.Save(wtr);
    }
    // make sure the return value is in range [0, intervals-1]
    int GetInterval(const double rwd) const {
        if (rwd < 0.01) return intervals_-1;
        int inter=(int)floor((log(rwd)-log(max_rwd_))/log(lambda_));
        if (inter >= intervals_) inter = intervals_-1;
        if (inter < 0) inter = 0;
        return inter;
    }
    void SaveNode(Node&& node);
    int MonotoneUpdateBlock(const int blk_no);
    int NonMonotoneUpdateBlock(const int blk_no);

    bool HandleSuccessedBlock(Block& block,
                              const int suc_nd_idx, FILE* fw);
    void HandleCompleteBlock(Block& block) {
        for(Node& node:block.nodes_)
            if(node.id_>=0) SaveNode(move(node));
    }
    void HandleInterruptedBlock(Block& block) {
        // insert the block to block list
        inter_blkidlst_[block.interval_].push_front(block.id_);
        id_blkiter_map_[block.id_] = InsertBlockSorted(move(block));
    }
    void SampleTk(FILE* fw);
    void InitTask(const int core);
public:
    IOGreedy(const Parms& pm):
        BitsDecode(pm), budget_(pm.budget_), lambda_(pm.lambda_) {
        TStr dir = gf_nm_.GetFPath() + "greedy";
        if(TFile::Exists(dir)) system(("rm -rf " + dir).CStr());
        system(("mkdir " + dir).CStr());

        buf_capacity_ = pm.buf_capacity_;

        printf("bytes per nd: %lu\n", bytes_per_nd_);
        printf("size per nd: %lu\n", size_per_nd_);
        printf("buffered blocks: %lu\n", buf_capacity_);
        tm_.Tick();
    }

    // generate initial intervals and blocks
    void Init();
    void MonotoneIterations();
    void NonMonotoneIterations();

    // debug
    void Test();
};


#endif /* __IOGREEDY_H__ */
