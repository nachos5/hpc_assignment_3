#!/bin/bash
# script to remove slurm output files -> compile -> submit slurm job
rm -f slurm*
mpicc main.c -o main
sbatch submit.sh