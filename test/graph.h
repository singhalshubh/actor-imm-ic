class graph {
public:
    uint64_t num_vertices = 0;
    uint64_t num_edges    = 0;
    uint64_t *row_offsets = nullptr;
    uint64_t *col_indices = nullptr;
    float    *weights     = nullptr;
    void  *mapped      = nullptr;
    size_t mapped_size = 0;

    void load_graph(std::string &filename);
    void unload_graph() {}
};

void graph::load_graph(std::string &filename) {
    FILE *fp = fopen(filename.c_str(), "rb");
    fread(&num_vertices, sizeof(int64_t), 1, fp);
    fread(&num_edges, sizeof(int64_t), 1, fp);

    fprintf(stderr, "|V| = %ld, |E| = %ld\n", num_vertices, num_edges);

    row_offsets = (uint64_t*) malloc((num_vertices + 1) * sizeof(uint64_t));
    col_indices = (uint64_t*) malloc(num_edges * sizeof(uint64_t));
    weights = (float*) malloc(num_edges * sizeof(float));
    fread(row_offsets, sizeof(uint64_t), num_vertices + 1, fp);
    fread(col_indices, sizeof(uint64_t), num_edges, fp);
    fread(weights, sizeof(float), num_edges, fp);

    fclose(fp);
}