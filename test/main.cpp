#include <math.h>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <sys/stat.h>
#include <sys/time.h>
#include <endian.h>
#include <ctime> 
#include <cstdlib> 
#include "trng/lcg64.hpp"
#include "trng/uniform01_dist.hpp"
#include "trng/uniform_int_dist.hpp"
#include <random>
#include <cmath>
#include <cstddef>
#include <chrono>
#include <limits>
#include <chrono>
#include <utility>
#include <memory>
#include <sstream>
#include <fstream>

#include "graph.h"
#include "sampling.h"

int main (int argc, char* argv[]) {
    const char *deps[] = { "system", "bale_actor" };
    std::string fileName;
    int opt;
    while ((opt = getopt(argc, argv, "f:")) != -1) {
        switch (opt) {
            case 'f':
                fileName = optarg;
                break;
            default:
                break;
        }
    }
    
    graph *g = new graph;
    g->load_graph(fileName);

    sampling(g);
    
    g->unload_graph();
    delete g;
    return EXIT_SUCCESS;
}