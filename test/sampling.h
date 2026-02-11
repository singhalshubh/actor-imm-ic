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

void sampling(graph *g) {
    trng::lcg64 generator;
    trng::uniform01_dist<float> val;
    trng::uniform_int_dist start(0, g->num_vertices);
    generator.seed(0UL);
    std::chrono::milliseconds bfs(0);
    std::chrono::milliseconds sort(0);
    double runs = 10;
    for(double i = 0; i < runs; i++) {
        std::vector<uint64_t> sample;
        std::queue<uint64_t> frontier;
        bitvec visited(g->num_vertices);    
        visited.reset();
        uint64_t start_vertex = start(generator);
        auto start = std::chrono::steady_clock::now();
        frontier.push(start_vertex);
        visited.set(start_vertex);
            
        while(!frontier.empty()) {
            uint64_t u = frontier.front();
            frontier.pop();
            sample.push_back(u);
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
        auto end = std::chrono::steady_clock::now();
        bfs += std::chrono::duration_cast<std::chrono::milliseconds>(end-start);

        start = std::chrono::steady_clock::now();
        std::stable_sort(sample.begin(), sample.end(), [](const uint64_t& a, const uint64_t& b){
            return a < b;
        });
        end = std::chrono::steady_clock::now();
        sort += std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
        sample.clear();
    }
    fprintf(stderr, "BFS: %lf sec, Sort: %lf sec\n", bfs.count()/(double)1000/runs, sort.count()/(double)10000/runs);
}