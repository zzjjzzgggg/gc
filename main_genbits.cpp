#include "stdafx.h"

#include "bitsgen.h"
#include "bitsdecode.h"

void CheckGraph(const Parms& pm) {
    BitStrCal BitGen(pm);
    BitGen.CheckGraph();
    printf("Passed.\n");
}

void SplitGraph(const Parms& pm) {
    BitStrCal BitGen(pm);
    printf("=====================\n  splitting the graph...\n");
    BitGen.SplitGraph();
}

void GenBitStrings(const Parms& pm) {
    TExeSteadyTm tm;
    BitStrCal BitGen(pm);
    printf("=====================\n  initializing bitstrings...\n");
    tm.Tick();
    BitGen.InitBits();
    printf("%s\n", tm.GetStr());

    printf("=====================\n  calculating bitstrings...\n");
    tm.Tick();
    BitGen.GenBits();
    printf("%s\n", tm.GetStr());
}

void GCApprox(const Parms& pm, const int gp_size) {
    BitsDecode bitsdec(pm);

    PSIn gp_in =TZipIn::New(pm.gf_nm_.GetFPath() +
                            TStr::Fmt("groundtruth_%d.gz", gp_size));
    vector<int> group(gp_size);
    vector<double> nbr_cnts;
    int nid;
    double gc_real_exp, gc_real_inv, gc_approx_exp, gc_approx_inv;

    TFltPrV dat_exp, dat_inv;
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


int main(int argc, char *argv[]) {
    Parms pm;
    TExeSteadyTm tm;
    ArgsParser parser(argc, argv);
    const int jobid = parser.GetIntArg("-job", -1,
                                       "0: split graph\n"
                                       "\t\t      1: gen bits\n"
                                       "\t\t      2: check graph");
    pm.gf_nm_ = parser.GetStrArg("-g","", "graph file name").c_str();
    pm.blk_sz_ = parser.GetIntArg("-bs", 10000, "graph block size");
    pm.page_sz_ = parser.GetIntArg("-ps", 1000, "page size");
    pm.max_hops_ = parser.GetIntArg("-H", 5, "max hops");
    pm.num_approx_ = parser.GetIntArg("-N", 8, "n approxes");
    pm.bkt_bits_ = parser.GetIntArg("-m", 6, "bucket bits");
    pm.more_bits_ = parser.GetIntArg("-r", 6, "more bits");
    if(parser.IsEnd()) return 0;

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
        CheckGraph(pm);
        break;
    default:
        break;
    }
    printf("Cost time %s (%.2fs).\n", tm.GetTmStr(), tm.GetSecs());
    return 0;
}
