
#include "./utils.c"

void init_obj_ranks(int *rank, int *obj_ranks, int *harbor_rank)
{
  // two random cells who are not the harbour generate fish groups
  // first we need to generate two random numbers representing the rank
  obj_ranks[0] = rand() % SIZE;
  obj_ranks[1] = rand() % SIZE;
  // if the fish group lands on the harbor we have to try again
  while (*harbor_rank == *rank && obj_ranks[0] == *rank)
  {
    obj_ranks[0] = rand() % SIZE;
  }
  while (*harbor_rank == *rank && obj_ranks[1] == *rank && obj_ranks[0] == obj_ranks[1])
  {
    obj_ranks[1] = rand() % SIZE;
  }
}

void drop_fish(int *harbor_rank, int *nbrs, int *boat_has_fish_group, int index) {
  for (int i=0; i<4; i++) {
    if (nbrs[i] == *harbor_rank) {
      printf("DROP FISH!");
      boat_has_fish_group[index] = 0;
      break;
    }
  }
}

// type 1 is fish, 2 is boat
void iteration(int *rank, MPI_Comm *cartcomm, int *harbor_rank, int *outbuf, int type,
               int *coords, int *prev_obj_ranks, int *obj_ranks, int *fish_group_size,
               int *boat_has_fish_group)
{
  prev_obj_ranks[0] = obj_ranks[0];
  prev_obj_ranks[1] = obj_ranks[1];
  // two object groups
  if (obj_ranks[0] == *rank || obj_ranks[1] == *rank)
  {
    int index;
    if (obj_ranks[0] == *rank)
    {
      index = 1;
    }
    else
    {
      index = 2;
    }
    // ocean object structure
    obj_print_coordinates(&type, &index, coords, fish_group_size);
    // shifting to gain information about our neighbours using the cart-communicator
    int nbrs[4];
    MPI_Cart_shift(*cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
    MPI_Cart_shift(*cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);

    // let's choose a random neighbour (direction) for the fish group to swim to
    // we don't want to get same numbers across processes this time so we generate different seeds
    srand(time(NULL) + *rank);
    int choose_neighbour = nbrs[rand() % 4];
    // update fish ranks
    if (obj_ranks[0] == *rank)
    {
      drop_fish(harbor_rank, nbrs, boat_has_fish_group, 0);
      // if this is a boat and has fish it goes towards the harbor
      if (type == 2 && boat_has_fish_group[0] > 0)
      {
        choose_neighbour = nbrs[towards_harbor(nbrs, harbor_rank)];
      }
      else
      {
        // we don't want the object groups stacking on each other (+ not on harbor)
        while (choose_neighbour == obj_ranks[1] || choose_neighbour == *harbor_rank)
        {
          choose_neighbour = nbrs[rand() % 4];
        }
      }
      obj_ranks[0] = choose_neighbour;
    }
    if (obj_ranks[1] == *rank)
    {
      // if this is a boat and has fish it goes towards the harbor
      if (type == 2 && boat_has_fish_group[1] > 0)
      {
        choose_neighbour = nbrs[towards_harbor(nbrs, harbor_rank)];
      }
      else
      {
        // we don't want the object groups stacking on each other (+ not on harbor)
        while (choose_neighbour == obj_ranks[0] || choose_neighbour == *harbor_rank)
        {
          choose_neighbour = nbrs[rand() % 4];
        }
      }
      obj_ranks[1] = choose_neighbour;
    }
  }

  // lets broadcast the new fish ranks to all processors (previous ranks broadcast the new ones)
  MPI_Bcast(&obj_ranks[0], 1, MPI_INT, prev_obj_ranks[0], MPI_COMM_WORLD);
  MPI_Bcast(&obj_ranks[1], 1, MPI_INT, prev_obj_ranks[1], MPI_COMM_WORLD);
}

void collisions(int *fish_ranks, int *boat_ranks, int *boat_has_fish_group)
{

  if (boat_ranks[0] == fish_ranks[0] && boat_has_fish_group[0] == 0)
  {
    boat_has_fish_group[0] = 1;
    // we just need one process to print
    printf("Boat 1 caught fish group 1! Net is full. \n");
  }
  else if (boat_ranks[0] == fish_ranks[1] && boat_has_fish_group[0] == 0)
  {
    boat_has_fish_group[0] = 2;
    printf("Boat 1 caught fish group 2! Net is full. \n");
  }

  if (boat_ranks[1] == fish_ranks[0] && boat_has_fish_group[1] == 0)
  {
    boat_has_fish_group[1] = 1;
    printf("Boat 2 caught fish group 1! Net is full. \n");
  }
  else if (boat_ranks[1] == fish_ranks[1] && boat_has_fish_group[1] == 0)
  {
    boat_has_fish_group[1] = 2;
    printf("Boat 2 caught fish group 2! Net is full. \n");
  }
}

int main(int argc, char **argv)
{
  // variables common to all processes
  int numtasks, rank, source, dest, outbuf, i, j, tag = 1;
  int inbuf[4] = {MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL,
                  MPI_PROC_NULL};
  int dims[2] = {ROWS, COLS}, periods[2] = {1, 1}, reorder = 1;
  int coords[2];

  int harbor_coords[2];
  int harbor_rank;

  int fish_ranks[2];
  int prev_fish_ranks[2];

  int boat_ranks[2];
  int prev_boat_ranks[2];

  int fish_group_size[2] = {10, 10};
  // 0 means no fish group, 1 means group 1 etc.
  int boat_has_fish_group[2] = {0, 0};

  MPI_Comm cartcomm;

  MPI_Request reqs[8];
  MPI_Status stats[8];

  // starting with MPI program
  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);

  MPI_Comm_rank(cartcomm, &rank);

  // let's receive the coordinates of this cell
  MPI_Cart_coords(cartcomm, rank, 2, coords);

  // water or harbor
  generateType(&rank, &outbuf, &harbor_rank, harbor_coords);

  // printing harbor coords
  if (coords[0] == harbor_coords[0] && coords[1] == harbor_coords[1])
  {
    printf("Harbor coordinates: %d,%d \n", harbor_coords[0], harbor_coords[1]);
  }

  init_obj_ranks(&rank, fish_ranks, &harbor_rank);
  init_obj_ranks(&rank, boat_ranks, &harbor_rank);

  for (int it = 0; it < 10; it++)
  {
    // lets visualize the grid
    visualizeGrid(&rank, &outbuf, fish_ranks, boat_ranks);
    // fish iteration
    iteration(&rank, &cartcomm, &harbor_rank, &outbuf, 1, coords, prev_fish_ranks,
              fish_ranks, fish_group_size, boat_has_fish_group);
    // boat iteration
    iteration(&rank, &cartcomm, &harbor_rank, &outbuf, 2, coords, prev_boat_ranks,
              boat_ranks, fish_group_size, boat_has_fish_group);
    // process 0 checks if the boats caught some fish and then broadcasts to other processes
    if (rank == 0)
    {
      collisions(fish_ranks, boat_ranks, boat_has_fish_group);
    }
    MPI_Bcast(boat_has_fish_group, 2, MPI_INT, 0, MPI_COMM_WORLD);
    // finally we check if a boat has fish and if it can drop it at the harbor this iteration
    //drop_fish(boat_has_fish_group);
  }

  MPI_Finalize();

  return 0;
}
