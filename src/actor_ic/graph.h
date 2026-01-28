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

// void graph::load_graph(std::string &filename) {
//     int fd = open(filename.c_str(), O_RDONLY);
//     struct stat st;
//     fstat(fd, &st);
//     size_t size = (size_t)st.st_size;

//     void *addr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
//     if (addr == MAP_FAILED) {
//         T0_fprintf(stderr, "mmap(%s) failed: %s\n", filename.c_str(), std::strerror(errno));
//     }

//     close(fd);
//     madvise(addr, size, MADV_WILLNEED);
//     unsigned char *base = (unsigned char *)addr;
//     uint64_t *header = (uint64_t *)base;
//     num_vertices = header[0];
//     num_edges    = header[1];
//     row_offsets = header + 2;
//     uint64_t *row_end = row_offsets + (num_vertices + 1);
//     unsigned char *p_cols = (unsigned char *)row_end;
//     col_indices = (uint64_t *)p_cols;
//     uint64_t *col_end = col_indices + num_edges;
//     unsigned char *p_w = (unsigned char *)col_end;
//     weights = (float *)p_w;

//     mapped      = addr;
//     mapped_size = size;
// }

// void graph::unload_graph() {
//     if (mapped && mapped_size > 0) {
//         munmap(mapped, mapped_size);
//     }
//     mapped      = nullptr;
//     mapped_size = 0;
//     num_vertices = 0;
//     num_edges    = 0;
//     row_offsets  = nullptr;
//     col_indices  = nullptr;
//     weights      = nullptr;
// }


void graph::load_graph(std::string &filename) {
    FILE *fp = fopen(filename.c_str(), "rb");
    fread(&num_vertices, sizeof(int64_t), 1, fp);
    fread(&num_edges, sizeof(int64_t), 1, fp);

    row_offsets = (uint64_t*) malloc((num_vertices + 1) * sizeof(uint64_t));
    col_indices = (uint64_t*) malloc(num_edges * sizeof(uint64_t));
    weights = (float*) malloc(num_edges * sizeof(float));
    fread(row_offsets, sizeof(uint64_t), num_vertices + 1, fp);
    fread(col_indices, sizeof(uint64_t), num_edges, fp);
    fread(weights, sizeof(float), num_edges, fp);

    fclose(fp);
}