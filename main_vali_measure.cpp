#include "stdafx.h"

#include "bitsgen.h"
#include "bitsdecode.h"


void SingleNDNbrCntApprox(const Parms& pm) {
    BitsDecode bitsdec(pm);
    vector<double> nbr_cnt(pm.max_hops_ + 1);

    PUNGraph Graph=TSnap::LoadBinaryEdgeList<PUNGraph>(pm.gf_nm_);
    TBreathFS<PUNGraph> bfs(Graph);
    for (int num = 0; num < 2; num ++) {
        int nid = Graph->GetRndNId();
        // int nid = 0;
        bfs.DoBfs(nid, true, false, -1, pm.max_hops_);

        nbr_cnt.assign(nbr_cnt.size(), 0);
        for(int id=bfs.NIdDistH.FFirstKeyId();
            bfs.NIdDistH.FNextKeyId(id);) {
            nbr_cnt[bfs.NIdDistH[id]]++;
        }

        printf("\n=========Ground Truth [%d]==========\n", nid);
        for(int hop=0,tmp=0; hop<=pm.max_hops_; hop++){
            tmp += nbr_cnt[hop];
            printf("%d --> %d\n", hop, tmp);
        }

        printf("========= prob. counting [%d]==========\n", nid);
        nbr_cnt.assign(nbr_cnt.size(), 0);
        bitsdec.GetNodeNbrs(nid, nbr_cnt);
        for (int h=0; h<=pm.max_hops_; h++) {
            printf("%d --> %.6f\n", h, nbr_cnt[h]);
        }
    }
}

/**
 * generate `num` groups of size `size`, each
 * group stores as a row of the output file,
 * ground truth GC values using Exponential weighting and
 * inverse weighting are appended at the end of each row.
 */
void GenGroundTruth(const Parms& pm, const int gp_size,
                    const int gp_num) {
    PUNGraph Graph=TSnap::LoadBinaryEdgeList<PUNGraph>(pm.gf_nm_);
    TBreathFS<PUNGraph> bfs(Graph);
    TIntV nodes, group;
    TFltIntPrV gc_gids;
    vector<double> nbr_cnt(pm.max_hops_ + 1);
    Graph->GetNIdV(nodes);
    TStr out_fnm = pm.gf_nm_.GetFPath() +
        TStr::Fmt("groundtruth_%d.gz", gp_size);
    PSOut pout = TZipOut::New(out_fnm);
    for (int gid=0; gid<gp_num; gid++) {
        TRandom::Choose(nodes, group, gp_size);
        for(int i=0; i<gp_size; i++) pout->Save(group[i]);
        // ground truth
        bfs.DoBfs(group, true, false, -1, pm.max_hops_);
        nbr_cnt.assign(nbr_cnt.size(), 0);
        for(int id=bfs.NIdDistH.FFirstKeyId();
            bfs.NIdDistH.FNextKeyId(id);)
            nbr_cnt[bfs.NIdDistH[id]]++;
        double real_val_exp = 0, real_val_inv = 0;
        for(int hop=0; hop<=pm.max_hops_; hop++) {
            real_val_exp += ExpWeights[hop]*nbr_cnt[hop];
            real_val_inv += InvWeights[hop]*nbr_cnt[hop];
        }
        pout->Save(real_val_exp);
        pout->Save(real_val_inv);
    }
}


void GCApprox(const Parms& pm, const int gp_size) {
    BitsDecode bitsdec(pm);

    PSIn gp_in =TZipIn::New(pm.gf_nm_.GetFPath() +
                            TStr::Fmt("groundtruth_%d.gz", gp_size));
    vector<int> group(gp_size);
    vector<double> nbr_cnts;
    int nid;
    double gc_real_exp, gc_real_inv, gc_approx_exp, gc_approx_inv;
    printf("Real_exp\tApprox_exp\tA/R\tReal_inv\tApprox_inv\tA/R\n");
    while(!gp_in->Eof()) {
        for (int i=0; i<gp_size; i++){
            gp_in->Load(nid);
            group[i] = nid;
        }
        gp_in->Load(gc_real_exp);
        gp_in->Load(gc_real_inv);
        bitsdec.GetGroupNbrs(group, nbr_cnts);
        gc_approx_exp = gc_approx_inv = 0;
        for(int hop=0; hop<=pm.max_hops_; hop++) {
            gc_approx_exp += nbr_cnts[hop] * ExpWeights[hop];
            gc_approx_inv += nbr_cnts[hop] * InvWeights[hop];
        }
        printf("%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n",
               gc_real_exp, gc_approx_exp, gc_approx_exp/gc_real_exp,
               gc_real_inv, gc_approx_inv, gc_approx_inv/gc_real_inv);
    }
}

void RewardGainEachNode(const Parms& pm) {
    printf("Calculating reward gain of each node...\n");
    BitsDecode decoder(pm);
    decoder.GetRewardAll();
}

int main(int argc, char *argv[]) {
    Parms pm;
    TExeSteadyTm tm;
    ArgsParser parser(argc, argv);
    const int jobid = parser.GetIntArg("-job", -1,
                                       "0: nbr cnt approx\n"
                                       "\t\t      1: gc approx\n"
                                       "\t\t      2: ground truth\n"
                                       "\t\t      3: node reward gain");
    pm.gf_nm_ =parser.GetStrArg("-g", "", "graph file name").c_str();
    pm.GetBitsGenParms();
    pm.max_hops_ = parser.GetIntArg("-H", pm.max_hops_, "max hops");
    pm.alpha_ = parser.GetFltArg("-a", pm.alpha_, "alpha");
    pm.cache_capacity_ = parser.GetIntArg("-C", pm.cache_capacity_,
                                          "cache capacity (pages)");
    const int gp_sz = parser.GetIntArg("-S", 10, "group size");
    const int gp_num = parser.GetIntArg("-R", 10, "num repeat");
    if(parser.IsEnd()) return 0;

    pm.Echo();

    switch(jobid) {
    case 0:
        SingleNDNbrCntApprox(pm);
        break;
    case 1:
        GCApprox(pm, gp_sz);
        break;
    case 2:
        GenGroundTruth(pm, gp_sz, gp_num);
        break;
    case 3:
        RewardGainEachNode(pm);
        break;
    default:
        break;
    }
    printf("Cost time %s (%.2fs).\n", tm.GetTmStr(), tm.GetSecs());
    return 0;
}
