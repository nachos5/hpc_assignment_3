#!/bin/bash
#SBATCH -J oceanman
#SBATCH -n 16
#SBATCH --mail-user=gon2@hi.is
#SBATCH --mail-type=end
module load gnu openmpi
mpirun /home/gon2/2019-HPC-Course/assignment-3/fish_folder/main

