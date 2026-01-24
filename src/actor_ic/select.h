uint64_t adjustPull(convey_t *conv, std::vector<uint64_t> &replica_tags, bool &done) {
    uint64_t pkt;
    while(convey_pull(conv, &pkt, NULL) == convey_OK) {
        replica_tags[pkt/THREADS] -= 1;
    }
    return convey_advance(conv, done);
}

uint64_t select(std::vector<uint64_t> &tags, std::vector<std::set<uint64_t>> &rrsets, 
        uint64_t k, std::vector<uint64_t> &influencers) {
    
    influencers.clear();
    std::vector<char> bitset(rrsets.size(), 1);
    for(int i = 0; i < rrsets.size(); i++) {
        if(rrsets[i].size() == 1){
            bitset[i] = 0;
        }
    }
    std::vector<uint64_t> replica_tags(tags.size(), 0);
    for(uint64_t u = 0; u < tags.size(); u++) {
        replica_tags[u] = tags[u];
    }  

    uint64_t max_coverage = 0;
    convey_t *conv = convey_new(SIZE_MAX, 0, NULL, 0);
    if (conv == NULL) return(-1);

    for (int i = 0; i < k; i++) {
        uint64_t local_max_size = 0;
        uint64_t local_infl;
        for(uint64_t u = 0; u < replica_tags.size(); u++) {
            if(local_max_size < replica_tags[u]) {
                local_max_size = replica_tags[u];
                local_infl = u*THREADS+MYTHREAD;
            }
        }
        uint64_t global_max_size = lgp_reduce_max_l(local_max_size);
        if(global_max_size == 0) {
            break;
        }
        #ifdef DEBUG
            T0_fprintf(stderr, "size: %ld\n", global_max_size);
        #endif
        max_coverage += global_max_size;
        uint64_t infl = std::numeric_limits<int64_t>::max();
        if(global_max_size == local_max_size) {
            infl = local_infl;
        }
        uint64_t curr_influencer = lgp_reduce_min_l(infl);
        influencers.push_back(curr_influencer);
        #ifdef DEBUG
            T0_fprintf(stderr, "infl: %ld\n", curr_influencer);
        #endif
        if (curr_influencer%THREADS == MYTHREAD) {
            replica_tags[curr_influencer/THREADS] = 0;
        }

        bool done = false;
        uint64_t send_pkt;
        uint64_t index = 0;
        bool numpushed = true;
        std::set<uint64_t>::iterator v;
        if (convey_begin(conv, sizeof(uint64_t), 0) != convey_OK) return(-1);

        for(;index < bitset.size();) {
            if(bitset[index] == 1 && rrsets[index].find(curr_influencer) != rrsets[index].end()) {
                bitset[index] = 0;
                v = rrsets[index].begin();
                for(;v != rrsets[index].end();) {
                    if(*v == curr_influencer) {
                        v++;
                        continue;
                    }
                    send_pkt = *v;
                    uint64_t send_pe = send_pkt%THREADS;
                    if(send_pe != MYTHREAD) {
                        if(convey_push(conv, &send_pkt, send_pe) != convey_OK) {
                            numpushed = false;
                            adjustPull(conv, replica_tags, done);
                        }
                        else {
                            numpushed = true;
                            v++;
                        }
                    }
                    else {
                        replica_tags[send_pkt/THREADS] -= 1;
                        v++;
                    }
                }
                if(numpushed) {
                    index++;
                }
            }
            else {
                index++;
            }
        }
        done = true;
        while(adjustPull(conv, replica_tags, done));
        convey_reset(conv);
        #ifdef DEBUG
            fprintf(stderr, "\n");
            for(int i = 0; i < replica_tags.size(); i++) {
                fprintf(stderr, "u: %ld, size: %ld\n", i*THREADS+MYTHREAD, replica_tags[i]);
            }
            lgp_barrier();
        #endif
    }
    convey_free(conv);
    return max_coverage;
} 