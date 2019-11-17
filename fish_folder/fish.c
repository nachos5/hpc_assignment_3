#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// global index
int fish_index = 0;

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

// assigning fish group
Fish fish_constructor(int size, int direction, int *coords)
{
  Fish* fish = (Fish*)malloc(sizeof(Fish));
  fish->index = fish_index;
  fish_index++;
  fish->size = size;
  fish->direction = direction;
  fish->x = coords[0];
  fish->y = coords[1];

  printf("Created a fish group at coordinates %d,%d \n", fish->x, fish->y);
  return *fish;
}

void fish_print_coordinates(Fish fish) {
  printf("Fish group is at coordinates %d,%d \n", fish.x, fish.y);
}

// the fish groups swims
void swim() {

}
