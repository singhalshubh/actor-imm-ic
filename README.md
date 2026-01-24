# Influence Maximization (IM-IC model) Problem
This repository consists of Influence Maximization Kernels adopted on HClib Actor runtime for Independent Cascade Model. 

## References
[[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769) Kempe, D., Kleinberg, J., & Tardos, É. (2003, August). Maximizing the
           spread of influence through a social network. In Proceedings of the
           ninth ACM SIGKDD international conference on Knowledge discovery and
           data mining (pp. 137-146). ACM.

[[Marco'19]](https://ieeexplore.ieee.org/document/8890991) M. Minutoli, M. Halappanavar, A. Kalyanaraman, A. Sathanur, R. Mcclure and J. McDermott (2019). Fast and Scalable Implementations of Influence Maximization Algorithms. 2019 IEEE International Conference on Cluster Computing (CLUSTER). pp. 1-12. doi: 10.1109/CLUSTER.2019.8890991.

[[ripples]](https://doi.org/10.5281/zenodo.3942705) Marco Minutoli. (2021). pnnl/ripples: (v2.2). Zenodo. https://doi.org/10.5281/zenodo.3942705.

[[SNAP]](http://snap.stanford.edu/data) Jure Leskovec and Andrej Krevl. (2014, June). SNAP Datasets: Stanford Large Network Dataset Collection. http://snap.stanford.edu/data

[[HClib]](https://hclib-actor.com) Sri Raj Paul, Akihiro Hayashi, Kun Chen, Youssef Elmougy, Vivek Sarkar,
A Fine-grained Asynchronous Bulk Synchronous parallelism model for PGAS applications,
Journal of Computational Science,
Volume 69,
2023,
102014,
ISSN 1877-7503,
https://doi.org/10.1016/j.jocs.2023.102014.


# Directory Structure
```tree
├── README.md
├── scripts
│   ├── convert_to_csrbin.cpp
│   ├── ripples-conan.sh
│   ├── ripples-setup.sh
│   └── setup.sh
└── src
    ├── actor_ic
    │   ├── Makefile
    │   ├── graph.h
    │   ├── main
    │   ├── main.cpp
    │   ├── sampling.h
    │   ├── select.h
    │   └── utils.h
    └── test
        ├── Makefile
        ├── sampling.h
        ├── select.h
        ├── test_sample
        └── test_sample.cpp

4 directories, 17 files
```

# Prerequisite
## HClib run-time system
Please follow the instructions for Perlmutter for installing and loading all the dependencies of IMM Actor (this repo).
Download this repository from the link provided and rename the folder to imm_hclib
```bash
cd imm_hclib/scripts/
source setup.sh
```

## Dataset 
### General Instructions
We recommend using scratch space for your dataset, although it is not a mandatory requisite. We support dataset in edge list format in space-separated format. Dataset should contain vertice labels in the range of [1..N].
```bash
salloc --nodes 1 --qos interactive --time 0:30:00 --constraint cpu
srun -n 1 --cpu-bind none $HOME/ripples/build/Release/tools/dump-graph -i /<path-to-dataset>/<filename> -d IC --normalize -o /<path-to-dataset>/<filename>-IC.txt
``` 

> dump-graph.cc normalizes your dataset from random vertice labels to ordered [1..N] vertex labels. IC model is used to adjust/generate weights based on IMM [[Marco'19]] (https://ieeexplore.ieee.org/document/8890991) strategy same as [[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769). Follow the instructions to build dump-graph.cc executable, available at [[ripples]](https://doi.org/10.5281/zenodo.4673587) in ``tools/``.

Then, run the program `convert_to_csrbin.cpp` in scripts/ to convert the file into .csrbin format. The inputs include a filename (-f), output filename (-o) and whether the graph is undirected or not (-u means yes).  

# Build and Run workflow
## Build
```bash
cd actor-imm-ic/src/actor_ic
make
```
You will see `main` executable.
> Make sure your terminal session has followed the pre-requisite, in case you run into any library [NOT FOUND] errors.

## Run 
Run the executables, `main`.
```bash
salloc --nodes <> --qos regular --time 0:30:00 --constraint cpu
srun -n <> -c 1 ./main -f /<path-to-dataset>/<filename> -k <> -e <> -o <>
```
`salloc` and `srun` options for running:
- `nodes` for the number of nodes you desire to run our program on.
- `n` for the total number of cores
> e.g. on Frontier, `n = 64 * nodes`
  
mandatory flags for running `main`:
- `f` for input file name
- `k` for the number of influencers
- `e` for value of epsilon
- `o` for output file name, which stores IDs of vertices that were selected as *influencers*

