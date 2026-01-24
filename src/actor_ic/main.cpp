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

#include "graph.h"
#include "utils.h"
#include "sampling.h"
#include "select.h"

int main (int argc, char* argv[]) {
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {
        std::string fileName;
        std::string outputfileName;
        double epsilon;
        uint64_t k;
        int opt;
        while ((opt = getopt(argc, argv, "e:k:f:o:")) != -1) {
            switch (opt) {
                case 'e':
                    epsilon = std::stod(optarg);
                    break;
                case 'k':
                    k = std::stoull(optarg);
                    break;
                case 'f':
                    fileName = optarg;
                    break;
                case 'o':
                    outputfileName = optarg;
                    break;
                default:
                    break;
            }
        }
        T0_fprintf(stderr, "Application: IMM-IC model, Number of influencers: %ld, epsilon = %f, file: %s\n\n", k, epsilon, fileName.c_str());
        graph *g = new graph;
        g->load_graph(fileName);
        if(k > g->num_vertices/2) exit(-1);
        std::vector<std::set<uint64_t>> rrsets;
        std::vector<uint64_t> tags(g->num_vertices/THREADS + 1, 0);
        std::vector<uint64_t> influencers;
        double sample_time = 0.0, select_time = 0.0;
        
        double tt = wall_seconds();
        trng::lcg64 generator;
        trng::uniform01_dist<float> val;
        generator.seed(0UL);
        generator.split(2, 1);
        generator.split(THREADS, MYTHREAD);
        double l = 1.0;
        l = l * (1 + 1 / std::log2(g->num_vertices));
        double epsilonPrime =  1.4142135623730951 * epsilon;
        double LB = 0;
        size_t thetaPrimePrevious = 0;

        for(int tracker = 1; tracker < std::log2(g->num_vertices); ++tracker) {
            ssize_t thetaPrime = ThetaPrime(tracker, epsilonPrime, l, k, g->num_vertices)/THREADS + 1;
            size_t delta = thetaPrime - thetaPrimePrevious;
            T0_fprintf(stderr, "[ESTIMATE] Sampling: %ld samples/pe\n", delta);
            double t1 = wall_seconds();
            sampling(delta, g, rrsets, tags);
            T0_fprintf(stderr, "Time taken in sampling: %8.3lf seconds\n", wall_seconds() - t1);
            sample_time += wall_seconds() - t1;
            thetaPrimePrevious += delta;
            
            T0_fprintf(stderr, "[ESTIMATE] Selecting for top %ld vertices\n", k);
            t1 = wall_seconds();
            double rr_covered = select(tags, rrsets, k, influencers);
            T0_fprintf(stderr, "Time taken in selecting: %8.3lf seconds\n", wall_seconds() - t1);
            select_time += wall_seconds() - t1;
            double f = double(rr_covered/(THREADS*thetaPrimePrevious));
            T0_fprintf(stderr, "Fraction of selection: %f\n", f);
            if (f >= std::pow(2, -(tracker))) {
                LB = (g->num_vertices * f) / (1 + epsilonPrime);
                break;
            }
        }
        T0_fprintf(stderr, "Final step\n");
        size_t thetaLocal = Theta(epsilon, l, k, LB, g->num_vertices)/THREADS + 1;
        if (thetaLocal > thetaPrimePrevious) {
            size_t final_delta = thetaLocal - thetaPrimePrevious;
            T0_fprintf(stderr, "[FINAL] Sampling: %ld samples/pe\n", final_delta);
            double t1 = wall_seconds();
            sampling(final_delta, g, rrsets, tags);
            T0_fprintf(stderr, "Time taken in sampling: %8.3lf seconds\n", wall_seconds() - t1);
            sample_time += wall_seconds() - t1;
            thetaPrimePrevious += final_delta;

            T0_fprintf(stderr, "[ESTIMATE] Selecting for top %ld vertices\n", k);
            t1 = wall_seconds();
            double rr_covered = select(tags, rrsets, k, influencers);
            T0_fprintf(stderr, "[FINAL] Time taken in selecting: %8.3lf seconds\n", wall_seconds() - t1);
            select_time += wall_seconds() - t1;
            double f = double(rr_covered/(THREADS*thetaPrimePrevious));
            T0_fprintf(stderr, "Fraction of selection: %f\n", f);
        }
        T0_fprintf(stderr, "Total Time: %8.3lf seconds\n", wall_seconds() - tt);
        T0_fprintf(stderr, "Sample Time: %8.3lf seconds\n", sample_time);
        T0_fprintf(stderr, "Select Time: %8.3lf seconds\n", select_time);
        lgp_barrier();

        uint64_t size_rrsets = 0;
        for(auto rrset: rrsets) {
            for(auto u: rrset) {
                size_rrsets += 8;
            }
        }
        uint64_t total_size_rrsets = lgp_reduce_add_l(size_rrsets);
        T0_fprintf(stderr, "RRsets: %ld bytes\n", total_size_rrsets);
        if(MYTHREAD == 0) {
            if(influencers.size() > 0) {
                std::ofstream fp;
                fp.open(outputfileName, std::ios::app);
                for(auto inf: influencers) {
                    fp << inf + 1 << "\n";
                }
                fp.close();
            }
        }
        lgp_barrier();
        g->unload_graph();
        delete g;
    });
    lgp_finalize();
    return EXIT_SUCCESS;
}