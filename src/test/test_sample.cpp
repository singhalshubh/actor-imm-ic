#include <math.h>
#include <shmem.h>
extern "C" {
#include <spmat.h>
}
#include <std_options.h>
#include <string>
#include <set>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <queue>
#include <sys/stat.h>
#include <sys/time.h>
#include "selector.h"
#include <endian.h>
#include <ctime> 
#include <cstdlib> 
#include "trng/lcg64.hpp"
#include "trng/uniform01_dist.hpp"
#include "trng/uniform_int_dist.hpp"
#include <random>
#include <cmath>
#include <cstddef>
#include <limits>
#include <chrono>
#include <utility>
#include <memory>
#include <sstream>
#include <fstream>

#include "../ic/graph.h"
#include "../ic/utils.h"
#include "sampling.h"
#include "select.h"
#include <papi.h>

int main (int argc, char* argv[]) {
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {
        std::string fileName;
        int opt;
        while ((opt = getopt(argc, argv, "e:k:f:o:")) != -1) {
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
        std::vector<std::set<uint64_t>> rrsets;
        std::vector<uint64_t> tags(g->num_vertices/THREADS + 1, 0);
        
        double tt = wall_seconds();
        size_t delta = 172;
        T0_fprintf(stderr, "[ESTIMATE] Sampling: %ld samples/pe\n", delta);
        
        int eventset = PAPI_NULL;
        long long val[2] = {0, 0};
        PAPI_library_init(PAPI_VER_CURRENT);
        PAPI_create_eventset(&eventset);
        PAPI_add_named_event(eventset, "PAPI_L3_TCM");
        PAPI_add_named_event(eventset, "PAPI_L3_TCA");
        PAPI_start(eventset);
        
        double t1 = wall_seconds();
        
        sampling(delta, g, rrsets, tags);
        PAPI_stop(eventset, val);

        long total_val[2];
        total_val[0] = lgp_reduce_add_l(val[0]);
        total_val[1] = lgp_reduce_add_l(val[1]);

        T0_fprintf(stderr, "PAPI l3 ratio: %lf\n", 100*((double) total_val[0] / (double) total_val[1]));
        // for(auto rrset: rrsets) {
        //     fprintf(stderr, "\nset: ");
        //     for(auto u: rrset) {
        //         fprintf(stderr, "%ld, ", u);
        //     }
        // }
        // fprintf(stderr, "\n");
        // for(int i = 0; i < tags.size(); i++) {
        //     fprintf(stderr, "u: %ld, size: %ld\n", i*THREADS+MYTHREAD, tags[i]);
        // }
        lgp_barrier();
        T0_fprintf(stderr, "Time taken in sampling: %8.3lf seconds\n", wall_seconds() - t1);
        uint64_t size_rrsets = 0;
        for(auto rrset: rrsets) {
            for(auto u: rrset) {
                size_rrsets += 8;
            }
        }
        uint64_t total_size_rrsets = lgp_reduce_add_l(size_rrsets);
        T0_fprintf(stderr, "RRsets: %ld bytes\n", total_size_rrsets);
        // std::vector<uint64_t> influencers;
        // int k = 2;
        // double rr_covered = select(tags, rrsets, k, influencers);
               
        lgp_barrier();
        g->unload_graph();
        delete g;
    });
    lgp_finalize();
    return EXIT_SUCCESS;
}