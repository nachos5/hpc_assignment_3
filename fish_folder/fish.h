#ifndef FISH_H
#define FISH_H

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// lets define the structure a fish group
typedef struct
{
  int index;
  int size;
  int direction;
  // coordinates
  int x;
  int y;
} Fish;

// function declarations
Fish fish_constructor(int index, int size, int *coords);
void fish_print_coordinates(Fish *fish);
void swim();

#endif