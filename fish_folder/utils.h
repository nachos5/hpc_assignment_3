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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

void generateType(int *rank, int *outbuf, int *harbor_coords);
void visualizeGrid(int *rank, int *outbuf, int *fish_ranks);

#endif