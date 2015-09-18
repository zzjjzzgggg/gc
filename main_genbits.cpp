#include "stdafx.h"

#include "bitsgen.h"
#include "bitsdecode.h"


void SplitGraph(const Parms& pm) {
    BitStrCal BitGen(pm);
    printf("=======================\n  splitting the graph...\n");
    BitGen.SplitGraph();
}


void GenBitStrings(const Parms& pm) {
    TExeSteadyTm tm;
    BitStrCal BitGen(pm);
    printf("=======================\n  initializing bitstrings...\n");
    tm.Tick();
    BitGen.InitBits();
    printf("%s\n", tm.GetStr());

    printf("=======================\n  calculating bitstrings...\n");
    tm.Tick();
    BitGen.GenBits();
    printf("%s\n", tm.GetStr());
}


void Validate(const Parms& pm) {
    BitsDecode bitsdec(pm);
    vector<double> nbr_cnt(pm.max_hops_ + 1);

    PUNGraph Graph=TSnap::LoadEdgeList<PUNGraph>(pm.gf_nm_);
    TBreathFS<PUNGraph> bfs(Graph);
    for (int num = 0; num < 2; num ++) {
        int nid = Graph->GetRndNId();
        // int nid = 0;
        bfs.DoBfs(nid, true, false, -1, pm.max_hops_);
        TIntV HopPDF(pm.max_hops_+1);
        for(int id=bfs.NIdDistH.FFirstKeyId();
            bfs.NIdDistH.FNextKeyId(id);)
            HopPDF[bfs.NIdDistH[id]]++;

        printf("\n=========Ground Truth [%d]==========\n", nid);
        for(int hop=0,tmp=0; hop<=pm.max_hops_; hop++){
            tmp += HopPDF[hop];
            printf("%d --> %d\n", hop, tmp);
        }

        printf("=========Adaptive Counting [%d]==========\n", nid);
        bitsdec.GetNodeNbrs(nid, nbr_cnt);
        for (int h=0; h<=pm.max_hops_; h++) {
            printf("%d --> %.6f\n", h, nbr_cnt[h]);
        }
    }
}


int main(int argc, char *argv[]) {
    Parms pm;
    TExeSteadyTm tm;
    ArgsParser parser(argc, argv);
    const int jobid =
        parser.GetIntArg("-job", -1,
                         "0: split graph\n"
                         "\t\t      1: gen bits\n"
                         "\t\t      2: validate");
    string gf_nm = parser.GetStrArg("-g", "", "graph file name");
    pm.blk_sz_ = parser.GetIntArg("-bs", 10000, "graph block size");
    pm.page_sz_ = parser.GetIntArg("-ps", 1000, "page size");
    pm.max_hops_ = parser.GetIntArg("-H", 5, "max hops");
    pm.num_approx_ = parser.GetIntArg("-N", 8, "n approx");
    pm.bkt_bits_ = parser.GetIntArg("-m", 6, "bucket bits");
    pm.more_bits_ = parser.GetIntArg("-r", 6, "more bits");
    if(parser.IsEnd()) return 0;

    pm.gf_nm_ = gf_nm.c_str();

    switch(jobid) {
    case 0:
        pm.Echo();
        SplitGraph(pm);
        break;
    case 1:
        pm.GetGraphSplitParms();
        pm.Echo();
        GenBitStrings(pm);
        break;
    case 2:
        pm.GetBitsGenParms();
        pm.Echo();
        Validate(pm);
        break;
    default:
        break;
    }
    printf("Cost time %s (%.2fs).\n", tm.GetTmStr(), tm.GetSecs());
    return 0;
}
