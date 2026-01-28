D_PATH=~/scratch/ic_dataset/
NODES=16
# srun -N ${NODES} -n $((24*NODES)) ./opt-vec-sorted-bn/main -f ${D_PATH}/com-youtube.ungraph-IC.bin  -k 100 -e 0.13 -o inf.txt
# NODES=$((NODES*2))
# srun -N ${NODES} -n $((24*NODES)) ./opt-vec-sorted-bn/main -f ${D_PATH}/com-youtube.ungraph-IC.bin  -k 100 -e 0.13 -o inf.txt
# NODES=$((NODES*2))
srun -N ${NODES} -n $((24*NODES)) ./opt-vec-sorted-bn/main -f ${D_PATH}/com-youtube.ungraph-IC.bin  -k 100 -e 0.13 -o inf.txt
NODES=$((NODES*2))
srun -N ${NODES} -n $((24*NODES)) ./opt-vec-sorted-bn/main -f ${D_PATH}/com-youtube.ungraph-IC.bin  -k 100 -e 0.13 -o inf.txt
NODES=$((NODES*2))
srun -N ${NODES} -n $((24*NODES)) ./opt-vec-sorted-bn/main -f ${D_PATH}/com-youtube.ungraph-IC.bin  -k 100 -e 0.13 -o inf.txt
NODES=$((NODES*2))
srun -N ${NODES} -n $((24*NODES)) ./opt-vec-sorted-bn/main -f ${D_PATH}/com-youtube.ungraph-IC.bin  -k 100 -e 0.13 -o inf.txt
NODES=$((NODES*2))