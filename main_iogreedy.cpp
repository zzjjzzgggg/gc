#include "stdafx.h"

#include "iogreedy.h"
void RunMonotoneIOGreedy(const Parms& pm) {
    printf("Running monotone io-greedy...\n");
    IOGreedy iogreedy(pm);

    iogreedy.Init();
    iogreedy.MonotoneIterations();
}

void RunNonMonotoneIOGreedy(const Parms& pm) {
    printf("Running nonmonotone io-greedy...\n");
    IOGreedy iogreedy(pm);

    iogreedy.Init();
    iogreedy.NonMonotoneIterations();
}

int main(int argc, char *argv[]) {
    Parms pm;
    ArgsParser parser(argc, argv);
    const int jobid = parser.GetIntArg("-job", -1,
              "0: run monotone io-greedy\n"
              "\t\t      1: run nonmonotone io-greedy");
    pm.gf_nm_ = parser.GetStrArg("-g", "", "graph").c_str();
    pm.GetBitsGenParms();
    pm.budget_ = parser.GetIntArg("-B", pm.budget_, "budget");
    pm.lambda_ = parser.GetFltArg("-L", pm.lambda_, "lambda");
    pm.alpha_ = parser.GetFltArg("-a", pm.alpha_, "alpha");
    pm.max_hops_ = parser.GetIntArg("-H", pm.max_hops_, "max hops");
    pm.buf_capacity_ = parser.GetIntArg("-b", 1024, "buffered blocks ");
    pm.cache_capacity_ = pm.num_processors_ =
        parser.GetIntArg("-c", pm.num_processors_, "cores");
    if(parser.IsEnd()) return 0;
    TExeSteadyTm tm;

    pm.Echo();

    switch(jobid) {
    case 0:
        RunMonotoneIOGreedy(pm);
        break;
    case 1:
        RunNonMonotoneIOGreedy(pm);
        break;
    default:
        break;
    }
    printf("\nCost time %s (%.2fs).\n", tm.GetTmStr(), tm.GetSecs());
    return 0;
}
