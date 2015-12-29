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
        Node():tm_(0) {}
        Node(lz4::LZ4In& rdr, const size_t& sz) {
            bits_.resize(sz);
            rdr.Load(id_);
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
            wtr.Save(id_);
            wtr.Save(tm_);
            wtr.Save(rwd_);
            wtr.Write(Ptr(), bits_.size()*sizeof(uint64));
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

    int intervals_, id_=0, t_=0, budget_;
    double lambda_, max_rwd_ = 0;

    // buffer settings
    size_t buf_capacity_ = 2048;
    list<Block> blk_lst_;
    map<int, BlockIter> id_blkiter_map_;
    vector< list<int> > inter_blkidlst_;

    // mutex
    int stop_level_;
    std::mutex stop_mutex_;

    vector<Block> blocks_processing_;

private:
    TStr BlockFNm(const int id) const {
        return gf_nm_.GetFPath() +
            TStr::Fmt("greedy/bits_%d.z", id);
    }
    // pin a bit-string page (called a block in IOGreedy)
    // return the number of nodes in the page
    Block PinBlock(const int id);
    BlockIter InsertBlockSorted(Block&& block);
    void WriteBlock(const Block& blk);
    int GetIntervalID(const double rwd) const {
        if(rwd < 0.1) return intervals_ - 1;
        int inter=(int)floor((log(rwd)-log(max_rwd_))/log(lambda_));
        if(inter >= intervals_) inter = intervals_ - 1;
        // randomness may cause non-submodualr, this needs further
        // investigation.
        if(inter < 0) inter = 0;
        return inter;
    }
    void SaveNode(Node&& node);
    int UpdateBlock(const int blk_no);

    void HandleSuccessedBlock(Block& block,
                              const int suc_nd_idx,
                              FILE* fw);
    void HandleCompleteBlock(Block& block);
    void HandleInterruptedBlock(Block& block);
public:
    IOGreedy(const Parms& pm):
        BitsDecode(pm), budget_(pm.budget_), lambda_(pm.lambda_) {

        TStr dir = gf_nm_.GetFPath() + "greedy";
        if(TFile::Exists(dir)) system(("rm -rf " + dir).CStr());
        system(("mkdir " + dir).CStr());

        printf("bytes per nd: %lu\n", bytes_per_nd_);
        printf("size per nd: %lu\n", size_per_nd_);
        tm_.Tick();
    }

    // generate initial intervals and blocks
    void Init();
    void Iterations();

};


#endif /* __IOGREEDY_H__ */
