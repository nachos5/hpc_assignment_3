#!/bin/bash
#SBATCH -J cart2d-example
#SBATCH -n 64 
#SBATCH --mail-user=gon2@hi.is
#SBATCH --mail-type=end
module load gnu openmpi
mpirun /home/gon2/2019-HPC-Course/assignment-3/cart2d_folder/cart2d

