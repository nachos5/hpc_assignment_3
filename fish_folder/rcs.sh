#!/bin/bash
# script to remove slurm output files -> compile -> submit slurm job
module load gnu openmpi
rm -f slurm*
mpicc main.c -o main
sbatch submit.sh