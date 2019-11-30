#ifndef MAIN_H
#define MAIN_H

#include "./utils.c"

int main(int argc, char **argv);
void init_obj_ranks(int *rank, int *obj_ranks, int *harbor_rank);
void updateObjRank(int type, int *index, int *harbor_rank, int *nbrs,
                   int *obj_ranks, int *choose_neighbour, int *boat_has_fish_group, int *storm_ranks);
void iteration(int *rank, MPI_Comm *cartcomm, int *harbor_rank, int *outbuf, int type,
               int *coords, int *prev_obj_ranks, int *obj_ranks, int *fish_group_size,
               int *boat_has_fish_group, int *storm_ranks, int iteration_index, MPI_File *fh);
void collisions(int *fish_ranks, int *boat_ranks, int *boat_has_fish_group,
                int *harbor_rank, int *fish_group_size, int *harbor_total_fish);
#endif