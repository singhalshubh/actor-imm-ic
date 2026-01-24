// g++ -O2 -std=c++11 convert_to_csrbin.cpp -o convert_to_csrbin

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>
#include <iostream>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cinttypes>

static void usage(const char *prog) {
    std::fprintf(stderr,
        "Usage: %s -f <input_txt> -o <output_csrbin> [-u]\n"
        "  input_txt:  text file with lines 'u v w' (1-based vertices)\n"
        "  output_csrbin: binary CSR file to create\n"
        "  -u: treat input as undirected (store both directions)\n",
        prog);
}

int main(int argc, char **argv) {
    std::string input_path;
    std::string output_path;
    bool undirected = false;   // -u flag

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            input_path = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "-u") {
            undirected = true;
        } else {
            std::fprintf(stderr, "Unknown or incomplete argument: %s\n", arg.c_str());
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (input_path.empty() || output_path.empty()) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fin = std::fopen(input_path.c_str(), "r");
    if (!fin) {
        std::perror(("fopen " + input_path).c_str());
        return EXIT_FAILURE;
    }

    std::printf("Reading input: %s\n", input_path.c_str());

    uint64_t num_edges_input = 0;   // edges in the text file
    uint64_t max_vertex = 0;        // after shifting to 0-based

    // First pass: find max vertex (1-based → 0-based) and count input edges
    while (true) {
        uint64_t u1, v1;
        float w;
        int rc = std::fscanf(fin, "%lu %lu %f", &u1, &v1, &w);
        if (rc == EOF) break;
        if (rc != 3) {
            std::fprintf(stderr, "Error: malformed line in input\n");
            std::fclose(fin);
            return EXIT_FAILURE;
        }

        // convert to 0-based
        if (u1 == 0 || v1 == 0) {
            std::fprintf(stderr, "Error: vertices must be >= 1 in input (got 0)\n");
            std::fclose(fin);
            return EXIT_FAILURE;
        }

        uint64_t u = u1 - 1;
        uint64_t v = v1 - 1;

        if (u > max_vertex) max_vertex = u;
        if (v > max_vertex) max_vertex = v;
        num_edges_input++;
    }

    if (num_edges_input == 0) {
        std::fprintf(stderr, "Error: empty input\n");
        std::fclose(fin);
        return EXIT_FAILURE;
    }

    uint64_t num_vertices = max_vertex + 1;
    std::printf("Detected %" PRIu64 " vertices, %" PRIu64 " input edges\n",
                num_vertices, num_edges_input);

    uint64_t *degree = (uint64_t *) std::calloc(num_vertices, sizeof(uint64_t));
    if (!degree) {
        std::perror("calloc degree");
        std::fclose(fin);
        return EXIT_FAILURE;
    }

    // Second pass: compute degrees for TRANSPOSE CSR (neighbors are in-neighbors)
    std::rewind(fin);
    while (true) {
        uint64_t u1, v1;
        float w;
        int rc = std::fscanf(fin, "%lu %lu %f", &u1, &v1, &w);
        if (rc == EOF) break;

        uint64_t u = u1 - 1;
        uint64_t v = v1 - 1;

        // For directed: edge u->v → in transpose we store v-row containing u
        // So degree[v]++
        degree[v]++;

        if (undirected && u != v) {
            // For undirected, also store u-row containing v
            degree[u]++;
        }
    }

    // Effective number of stored edges in CSR (after transpose + undirected expansion)
    uint64_t num_edges_effective = 0;
    for (uint64_t v = 0; v < num_vertices; v++) {
        num_edges_effective += degree[v];
    }

    uint64_t *row_offsets = (uint64_t *) std::malloc((num_vertices + 1) * sizeof(uint64_t));
    if (!row_offsets) {
        std::perror("malloc row_offsets");
        std::free(degree);
        std::fclose(fin);
        return EXIT_FAILURE;
    }

    row_offsets[0] = 0;
    for (uint64_t v = 0; v < num_vertices; v++) {
        row_offsets[v + 1] = row_offsets[v] + degree[v];
    }

    int fd = ::open(output_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::perror(("open " + output_path).c_str());
        std::free(degree);
        std::free(row_offsets);
        std::fclose(fin);
        return EXIT_FAILURE;
    }

    std::printf("Writing CSR to: %s\n", output_path.c_str());
    if (undirected)
        std::printf("Treating graph as undirected (storing edges in both directions)\n");

    const size_t header_bytes  = 2 * sizeof(uint64_t);
    const size_t rows_bytes    = (num_vertices + 1) * sizeof(uint64_t);
    const size_t cols_bytes    = num_edges_effective * sizeof(uint64_t);
    const size_t weights_bytes = num_edges_effective * sizeof(float);
    const size_t total_bytes   = header_bytes + rows_bytes + cols_bytes + weights_bytes;

    if (ftruncate(fd, (off_t)total_bytes) != 0) {
        std::perror("ftruncate");
        ::close(fd);
        std::free(degree);
        std::free(row_offsets);
        std::fclose(fin);
        return EXIT_FAILURE;
    }

    void *base = ::mmap(nullptr, total_bytes, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
        std::perror("mmap");
        ::close(fd);
        std::free(degree);
        std::free(row_offsets);
        std::fclose(fin);
        return EXIT_FAILURE;
    }
    ::close(fd);

    uint8_t *p = (uint8_t *) base;

    uint64_t *f_num_vertices = (uint64_t *) p;
    p += sizeof(uint64_t);
    uint64_t *f_num_edges = (uint64_t *) p;
    p += sizeof(uint64_t);

    uint64_t *f_row_offsets = (uint64_t *) p;
    p += rows_bytes;

    uint64_t *f_col_indices = (uint64_t *) p;
    p += cols_bytes;

    float *f_weights = (float *) p;

    *f_num_vertices = num_vertices;
    *f_num_edges    = num_edges_effective;  // actual #entries in CSR

    std::memcpy(f_row_offsets, row_offsets,
                (num_vertices + 1) * sizeof(uint64_t));

    uint64_t *cursor = (uint64_t *) std::calloc(num_vertices, sizeof(uint64_t));
    if (!cursor) {
        std::perror("calloc cursor");
        std::free(degree);
        std::free(row_offsets);
        std::fclose(fin);
        ::munmap(base, total_bytes);
        return EXIT_FAILURE;
    }

    // Third pass: fill CSR
    std::rewind(fin);
    while (true) {
        uint64_t u1, v1;
        float w;
        int rc = std::fscanf(fin, "%lu %lu %f", &u1, &v1, &w);
        if (rc == EOF) break;

        uint64_t u = u1 - 1;
        uint64_t v = v1 - 1;

        // edge u->v → stored as v-row containing u (TRANSPOSE)
        {
            uint64_t pos = f_row_offsets[v] + cursor[v]++;
            f_col_indices[pos] = u;
            f_weights[pos]     = w;
        }

        if (undirected && u != v) {
            // also store u-row containing v
            uint64_t pos2 = f_row_offsets[u] + cursor[u]++;
            f_col_indices[pos2] = v;
            f_weights[pos2]     = w;
        }
    }

    std::fclose(fin);
    std::free(degree);
    std::free(row_offsets);
    std::free(cursor);

    msync(base, total_bytes, MS_SYNC);
    munmap(base, total_bytes);

    std::printf("Done. Stored %" PRIu64 " vertices, %" PRIu64 " CSR edges.\n",
                num_vertices, num_edges_effective);
    return EXIT_SUCCESS;
}
