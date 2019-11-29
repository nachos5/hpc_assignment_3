#ifndef UTILS_H
#define UTILS_H

// directions
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
// types
#define WATER 0
#define LAND 1
// grid size definitions
#define SIZE 16
#define ROWS 4
#define COLS 4
#define FISH 1
#define BOAT 2
#define ITERATIONS 30

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

void generateTileType(int *rank, int *outbuf, int *harbor_rank, int *harbor_coords);
void visualizeGrid(int *rank, int *outbuf, int *fish_ranks, int *boat_ranks, int *boat_has_fish_group);
void obj_print_coordinates(int type, int *index, int *coords, int *fish_group_size);
int max(int num1, int num2);
int min(int num1, int num2);
int coords_to_rank(int x, int y);
int towards_harbor(int *nbrs, int *harbor_rank);
int* rank_to_coords(int rank);

#endif