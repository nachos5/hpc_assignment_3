#include "./main.h"

int main(int argc, char **argv)
{
  // Variables common to all processes
  int numtasks, rank, outbuf, i, j;

  // Cartetian variables
  int dims[2] = {ROWS, COLS}, periods[2] = {1, 1}, reorder = 1;
  int coords[2]; // Coords of each thread in the topology

  // Harbor
  int harbor_coords[2];
  int harbor_rank;
  int harbor_total_fish = 0;

  // Fish
  int fish_ranks[2];
  int prev_fish_ranks[2];

  int fish_group_size[2] = {10, 10};
  int boat_has_fish_group[2] = {0, 0}; // 0 = no fish group, 1-n = fish group nr. 1-n

  // Boat
  int boat_ranks[2];
  int prev_boat_ranks[2];

  // Storm
  int storm_ranks[SIZE];

  // Waves

  // Calculation variables
  int number_of_elevations = SIZE;
  double e_step = 1.0 / number_of_elevations;
  double amplitude = 2.0; // Height of wave (max height 2.0 = storm)

  double elevation[number_of_elevations]; // Elevation of a single sine wave

  // Calculate the elevation for a sine wave at each timesetp
  int e = 0;
  for (double x = 0.0; x < 1.0; x += e_step)
  {
    elevation[e] = amplitude * sin(x * 2 * PI);

    // Elevation at peek = storm
    if (elevation[e] == amplitude)
    {
      storm_ranks[e] = 1;
    }
    else
    {
      storm_ranks[e] = 0;
    }
    e++;
  }

  MPI_Comm cartcomm;

  // Starting with MPI program
  MPI_Init(&argc, &argv);

  // Initialize the cartian communicator
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);
  MPI_Comm_rank(cartcomm, &rank);

  // Let's receive the coordinates of this cell
  MPI_Cart_coords(cartcomm, rank, 2, coords);

  // MPI I/O
  MPI_File fh;
  MPI_File_open(MPI_COMM_WORLD, "mpi_io.out", MPI_MODE_CREATE | MPI_MODE_WRONLY,
                MPI_INFO_NULL, &fh);

  // Water or harbor cells
  generateTileType(&rank, &outbuf, &harbor_rank, harbor_coords);

  // Printing harbor coords
  if (coords[0] == harbor_coords[0] && coords[1] == harbor_coords[1])
  {
    char str[25];
    sprintf(str, "Harbor coordinates: %d,%d \n\0", harbor_coords[0], harbor_coords[1]);
    printf(str);
    mpi_print(&fh, str, 25);
  }

  // Initialize boats and fish groups
  init_obj_ranks(&rank, fish_ranks, &harbor_rank);
  init_obj_ranks(&rank, boat_ranks, &harbor_rank);

  // Initialize the wave (elevation level)
  int e_index = rank; // Index of each threads elevation level

  // Update loop for the simulation
  for (int it = 0; it < ITERATIONS; it++)
  {
    // Visualize the grid
    visualizeGrid(&rank, &outbuf, fish_ranks, boat_ranks, boat_has_fish_group, storm_ranks);

    // Each thread moves onto the next step in the elevation (following the sine wave and simulating an actual wave)
    if (e_index - 1 < 0)
    {
      e_index = SIZE-1;
    }
    else
    {
      e_index--;
    }

    // Check if the new elevation reaches storm level
    if (elevation[e_index] == 2.0)
    {
      storm_ranks[rank] = 1;
    }
    else
    {
      storm_ranks[rank] = 0;
    }

    // Each thread shares it's updated value of it's storm cell
    for (int i = 0; i < SIZE; i++)
    {
      MPI_Bcast(&storm_ranks[i], 1, MPI_INT, i, MPI_COMM_WORLD);
    }

    // Fish iteration
    iteration(&rank, &cartcomm, &harbor_rank, &outbuf, FISH, coords, prev_fish_ranks,
              fish_ranks, fish_group_size, boat_has_fish_group, storm_ranks, it, &fh);
    // Boat iteration
    iteration(&rank, &cartcomm, &harbor_rank, &outbuf, BOAT, coords, prev_boat_ranks,
              boat_ranks, fish_group_size, boat_has_fish_group, storm_ranks, it, &fh);

    // Thread 0 checks if the boats caught some fish and then broadcasts to other threads
    if (rank == 0)
    {
      collisions(fish_ranks, boat_ranks, boat_has_fish_group,
                 &harbor_rank, fish_group_size, &harbor_total_fish);
    }
    MPI_Bcast(boat_has_fish_group, 2, MPI_INT, 0, MPI_COMM_WORLD);
  }

  if (rank == 0)
  {
    printf("\n\nTOTAL FISH CAUGHT: %d", harbor_total_fish);
  }

  // Close file
  MPI_File_close(&fh);

  MPI_Finalize();

  return 0;
}

void init_obj_ranks(int *rank, int *obj_ranks, int *harbor_rank)
{
  // two random cells who are not the harbour generate object groups
  // first we need to generate two random numbers representing the rank
  obj_ranks[0] = rand() % SIZE;
  obj_ranks[1] = rand() % SIZE;
  // if the object group lands on the harbor we have to try again
  while (*harbor_rank == *rank && obj_ranks[0] == *rank)
  {
    obj_ranks[0] = rand() % SIZE;
  }
  while (*harbor_rank == *rank && obj_ranks[1] == *rank && obj_ranks[0] == obj_ranks[1])
  {
    obj_ranks[1] = rand() % SIZE;
  }
}

void updateObjRank(int type, int *index, int *harbor_rank, int *nbrs,
                   int *obj_ranks, int *choose_neighbour, int *boat_has_fish_group, int *storm_ranks)
{
  // if this is a boat and has fish it goes towards the harbor
  if (type == BOAT && boat_has_fish_group[*index] > 0)
  {
    *choose_neighbour = nbrs[towards_harbor(nbrs, harbor_rank, storm_ranks)];
  }
  else
  {
    int other_index;
    if (index == 0)
    {
      other_index = 1;
    }
    else
    {
      other_index = 0;
    }
    // we don't want the object groups stacking on each other (+ not on harbor, + not on a storm cell)
    while (*choose_neighbour == obj_ranks[other_index] || *choose_neighbour == *harbor_rank || storm_ranks[*choose_neighbour] == 1)
    {
      *choose_neighbour = nbrs[rand() % 4];
    }
  }
  obj_ranks[*index] = *choose_neighbour;
}

void iteration(int *rank, MPI_Comm *cartcomm, int *harbor_rank, int *outbuf, int type,
               int *coords, int *prev_obj_ranks, int *obj_ranks, int *fish_group_size,
               int *boat_has_fish_group, int *storm_ranks, int iteration_index, MPI_File *fh)
{
  int index;
  prev_obj_ranks[0] = obj_ranks[0];
  prev_obj_ranks[1] = obj_ranks[1];
  // two object groups
  if (obj_ranks[0] == *rank || obj_ranks[1] == *rank)
  {
    if (obj_ranks[0] == *rank)
    {
      index = 0;
    }
    else
    {
      index = 1;
    }
    // shifting to gain information about our neighbours using the cart-communicator
    int nbrs[4];
    MPI_Cart_shift(*cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
    MPI_Cart_shift(*cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);

    // let's choose a random neighbour (direction) for the object to go to
    // we don't want to get same numbers across processes this time so we generate different seeds
    srand(time(NULL) + *rank + iteration_index);
    int choose_neighbour = nbrs[rand() % 4];
    // update object rank
    updateObjRank(type, &index, harbor_rank, nbrs, obj_ranks, &choose_neighbour, boat_has_fish_group, storm_ranks);
    // print position
    if (!(type == FISH && boat_has_fish_group[index] > 0))
    {
      obj_print_coordinates(type, &index, coords, fish_group_size, fh);
    }
  }

  // lets broadcast the new object ranks to all processors (previous ranks broadcast the new ones)
  MPI_Bcast(&obj_ranks[0], 1, MPI_INT, prev_obj_ranks[0], MPI_COMM_WORLD);
  MPI_Bcast(&obj_ranks[1], 1, MPI_INT, prev_obj_ranks[1], MPI_COMM_WORLD);
}

void collisions(int *fish_ranks, int *boat_ranks, int *boat_has_fish_group,
                int *harbor_rank, int *fish_group_size, int *harbor_total_fish)
{
  // first we check if the boats caught fish
  if (boat_ranks[0] == fish_ranks[0] && boat_has_fish_group[0] == 0)
  {
    boat_has_fish_group[0] = 1;
    // we just need one process to print
    printf("\n\nBoat 1 caught fish group 1! Net is full.\n\n");
  }
  else if (boat_ranks[0] == fish_ranks[1] && boat_has_fish_group[0] == 0)
  {
    boat_has_fish_group[0] = 2;
    printf("\n\nBoat 1 caught fish group 2! Net is full.\n\n");
  }
  if (boat_ranks[1] == fish_ranks[0] && boat_has_fish_group[1] == 0)
  {
    boat_has_fish_group[1] = 1;
    printf("\n\nBoat 2 caught fish group 1! Net is full.\n\n");
  }
  else if (boat_ranks[1] == fish_ranks[1] && boat_has_fish_group[1] == 0)
  {
    boat_has_fish_group[1] = 2;
    printf("\n\nBoat 2 caught fish group 2! Net is full.\n\n");
  }
  // then we check whether the boats have fish and are at the harbor
  if (boat_has_fish_group[0] > 0 && boat_ranks[0] == *harbor_rank)
  {
    if (boat_has_fish_group[0] == 1)
    {
      printf("\n\nBoat 1 drops %d fishes at the harbor!\n\n", fish_group_size[0]);
      printf("Fish group 1 spawns again in the ocean!\n");
      *harbor_total_fish += fish_group_size[0];
    }
    else
    {
      printf("\n\nBoat 1 drops %d fishes at the harbor!\n\n", fish_group_size[1]);
      printf("Fish group 2 spawns again in the ocean!\n");
      *harbor_total_fish += fish_group_size[1];
    }
    boat_has_fish_group[0] = 0;
  }
  if (boat_has_fish_group[1] > 0 && boat_ranks[1] == *harbor_rank)
  {
    if (boat_has_fish_group[1] == 1)
    {
      printf("\n\nBoat 2 drops %d fishes at the harbor!\n\n", fish_group_size[0]);
      printf("Fish group 1 spawns again in the ocean!\n");
      *harbor_total_fish += fish_group_size[0];
    }
    else
    {
      printf("\n\nBoat 2 drops %d fishes at the harbor!\n\n", fish_group_size[1]);
      printf("Fish group 2 spawns again in the ocean!\n");
      *harbor_total_fish += fish_group_size[1];
    }
    boat_has_fish_group[1] = 0;
  }
}
