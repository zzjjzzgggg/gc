#include "utils.h"

TStr PrettySize(size_t val) {
    if(val < Kilobytes)
        return TStr::Fmt("%zuB", val);
    else if(val < Megabytes)
        return TStr::Fmt("%.2fK", val/Kilobytes);
    else if(val < Gigabytes)
        return TStr::Fmt("%.2fM", val/Megabytes);
    else
        return TStr::Fmt("%.2fG", val/Gigabytes);
}

void PrintStatusBar(const int cur, const int total,
                    const TExeSteadyTm& tm) {
    const double gran = total/50.0;
    printf("[");
    for(int j=0; j<50; j++) {
        if (j<cur/gran) printf("*");
        else printf(" ");
    }
    printf("] %d/%d %s %.2fs\r",
           cur, total, tm.GetStr(), tm.GetSecs());
    fflush(stdout);
}
