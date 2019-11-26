#include "./fish.h"

// assigning fish group
Fish fish_constructor(int index, int size, int *coords)
{
  Fish* fish = (Fish*)malloc(sizeof(Fish));
  fish->index = index;
  fish->size = size;
  fish->x = coords[0];
  fish->y = coords[1];

  fish_print_coordinates(fish);
  return *fish;
}

void fish_print_coordinates(Fish *fish) {
  printf("Fish group %d is at coordinates %d,%d \n", fish->index, fish->x, fish->y);
}

// the fish groups swims
void swim() {

}
