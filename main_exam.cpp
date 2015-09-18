#include "stdafx.h"

#include "iogreedy.h"


void EvalApprox(const Parms& pm, const int group_size,
                const int num_rpt) {

    PUNGraph Graph = TSnap::LoadEdgeList<PUNGraph>(pm.gf_nm_);
    TBreathFS<PUNGraph> bfs(Graph);

    IOGreedy greedy(pm);
    greedy.SetWeights(ExpWeights);

    TIntV group, hop_pdf(pm.max_hops_+1), nodes;
    Graph->GetNIdV(nodes);

    TFltPrV Dat;
    for (int rpt=0; rpt<num_rpt; rpt++) {
        TRandom::Choose(nodes, group, group_size);
        double approx_val = greedy.GetGroupGC(group);
        bfs.DoBfs(group, true, false, -1, pm.max_hops_);
        hop_pdf.PutAll(0);
        for(int id=bfs.NIdDistH.FFirstKeyId();
            bfs.NIdDistH.FNextKeyId(id);)
            hop_pdf[bfs.NIdDistH[id]]++;
        double real_val = 0;
        for(int hop=0; hop<=pm.max_hops_; hop++)
            real_val += ExpWeights[hop]*hop_pdf[hop];
        printf("%.6f\t%.6f\t%.6f\n",
               real_val, approx_val, approx_val/real_val);
        Dat.Add(TFltPr(real_val, approx_val));
    }
    TStr fnm = pm.gf_nm_.GetFPath() +
        TStr::Fmt("cmp_%d_N%d.dat", group_size, pm.num_approx_);
    BIO::SaveFltPrV(Dat, fnm, "# real  approx");
}

void RewardGainEachNode(const Parms& pm) {
    printf("Calculating reward gain of each node...\n");
    IOGreedy calcnt(pm);
    calcnt.SetWeights(ExpWeights);
    calcnt.GetRewardAll();
}

void Greedy(const Parms& pm) {
    printf("Running io greedy...\n");
    IOGreedy greedy(pm);
    greedy.SetWeights(ExpWeights);

    greedy.Init();
    greedy.Iterations();
}

void MultiPass(const Parms& pm) {
    printf("Running multi-pass...\n");
    IOGreedy greedy(pm);
    greedy.SetWeights(ExpWeights);
    greedy.MultiPass();
}

int main(int argc, char *argv[]) {
    Parms pm;
    TExeSteadyTm tm;
    ArgsParser parser(argc, argv);
    const int jobid =
        parser.GetIntArg("-job", -1,
                         "0: evaluate estimates\n"
                         "\t\t      1: node reward gains\n"
                         "\t\t      2: io greedy\n"
                         "\t\t      3: multi pass");
    pm.gf_nm_ = parser.GetStrArg("-g", "", "graph").c_str();
    pm.GetBitsGenParms();
    pm.budget_ = parser.GetIntArg("-B", pm.budget_, "budget");
    pm.lambda_ = parser.GetFltArg("-L", pm.lambda_, "lambda");
    pm.max_hops_ = parser.GetIntArg("-H", pm.max_hops_, "max hops");
    const int group_size = parser.GetIntArg("-S", 10, "group size");
    const int num_rpt = parser.GetIntArg("-R", 100, "num repeat");
    if(parser.IsEnd()) return 0;

    pm.Echo();

    switch(jobid) {
    case 0:
        EvalApprox(pm, group_size, num_rpt);
        break;
    case 1:
        RewardGainEachNode(pm);
        break;
    case 2:
        Greedy(pm);
        break;
    case 3:
        MultiPass(pm);
        break;
    default:
        break;
    }

    printf("\nCost time %s (%.2fs).\n", tm.GetTmStr(), tm.GetSecs());
    return 0;
}
