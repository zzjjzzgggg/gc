#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <functional>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <map>
#include <random>
#include <future>
#include <mutex>

#include "netsnap/snap/Snap.h"
#include "argsparser.h"

using namespace std;
typedef TVec<uint64> TBitV;

#define USE_LZ4

#define USE_EXP_WEIGHT

#define LCHLLC
