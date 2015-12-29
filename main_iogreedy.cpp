#include "stdafx.h"

#include "iogreedy.h"

void RunIOGreedy(const Parms& pm) {
    printf("Running io-greedy...\n");
    IOGreedy iogreedy(pm);

    iogreedy.Init();
    iogreedy.Iterations();
}

int main(int argc, char *argv[]) {
    Parms pm;
    ArgsParser parser(argc, argv);
    const int jobid = parser.GetIntArg("-job", -1,
                                       "0: run io-greedy");
    pm.gf_nm_ = parser.GetStrArg("-g", "", "graph").c_str();
    pm.GetBitsGenParms();
    pm.budget_ = parser.GetIntArg("-B", pm.budget_, "budget");
    pm.lambda_ = parser.GetFltArg("-L", pm.lambda_, "lambda");
    pm.max_hops_ = parser.GetIntArg("-H", pm.max_hops_, "max hops");
    pm.cache_capacity_ = parser.GetIntArg("-C", 4, "cached pages");
    if(parser.IsEnd()) return 0;
    TExeSteadyTm tm;

    pm.Echo();

    switch(jobid) {
    case 0:
        RunIOGreedy(pm);
        break;
    default:
        break;
    }

    printf("\nCost time %s (%.2fs).\n", tm.GetTmStr(), tm.GetSecs());
    return 0;
}
