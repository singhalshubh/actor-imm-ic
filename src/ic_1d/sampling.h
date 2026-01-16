class bitvec {
public:
    std::vector<uint64_t> data;
    bitvec(uint64_t n) {
        data.resize((n + 63) / 64, 0ULL);
    }
    inline void set(uint64_t i) {
        data[i >> 6] |= (1ULL << (i & 63));
    }
    inline void reset() {
        for (size_t w = 0; w < data.size(); w++) {
            data[w] = 0ULL;
        }
    }
    inline bool test(uint64_t i) const {
        return (data[i >> 6] >> (i & 63)) & 1ULL;
    }
};

class SampleSelector: public hclib::Selector<1, uint64_t> {
    std::vector<uint64_t> &tags;
    void process(uint64_t v, int sender_rank) {
        tags[v/THREADS] += 1;
    }

public:
    SampleSelector(std::vector<uint64_t> &_tags): hclib::Selector<1, uint64_t>(true), tags(_tags) {
        mb[0].process = [this](uint64_t pkt, int sender_rank) { this->process(pkt, sender_rank); };
    }
};

void sampling(size_t delta, graph *g, std::vector<std::set<uint64_t>> &rrsets, std::vector<uint64_t> &tags) {
    SampleSelector ss(tags);
    hclib::finish([&] {
        trng::lcg64 generator;
        trng::uniform01_dist<float> val;
        trng::uniform_int_dist start(0, g->num_vertices);
        generator.seed(0UL + MYTHREAD);
        generator.split(2, 1);
        generator.split(THREADS, MYTHREAD);
        std::queue<uint64_t> frontier;
        bitvec visited(g->num_vertices);
        for(uint64_t pos = 0; pos < delta; pos++) {
            //T0_fprintf(stderr, "pos: %ld\n", pos); 
            rrsets.push_back(std::set<uint64_t>());
            uint64_t rrset_size = rrsets.size();
            
            visited.reset();
            uint64_t start_vertex = start(generator);
            frontier.push(start_vertex);
            visited.set(start_vertex);
            
            while(!frontier.empty()) {
                uint64_t u = frontier.front();
                frontier.pop();
                rrsets[rrset_size - 1].insert(u);
                ss.send(0, u, u%THREADS);
                uint64_t start = g->row_offsets[u];
                uint64_t end = g->row_offsets[u + 1];
                for (uint64_t i = start; i < end; i++) {
                    uint64_t v = g->col_indices[i];
                    float w = g->weights[i];
                    if(visited.test(v) == false && val(generator) <= w) {
                        frontier.push(v);
                        visited.set(v);
                    }
                }
            }
        }
        ss.done();
    });
}