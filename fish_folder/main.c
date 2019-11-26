
#include "./utils.c"
#include "./fish.c"

int main(int argc, char **argv)
{
  // variables common to all processes
  int numtasks, rank, source, dest, outbuf, i, j, tag = 1;
  int inbuf[4] = {MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL,
                  MPI_PROC_NULL};
  int dims[2] = {ROWS, COLS}, periods[2] = {1, 1}, reorder = 1;
  int coords[2];

  int harbor_coords[2];
  int is_harbor = 0;
  int fish_ranks[2];
  int prev_fish_ranks[2];
  int has_fish;

  MPI_Comm cartcomm;

  MPI_Request reqs[8];
  MPI_Status stats[8];
  MPI_Request fish_req[1];
  MPI_Status fish_stat[1];

  // starting with MPI program
  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);

  MPI_Comm_rank(cartcomm, &rank);

  // let's receive the coordinates of this cell
  MPI_Cart_coords(cartcomm, rank, 2, coords);

  // water or harbor
  generateType(&rank, &outbuf, harbor_coords);

  // let's check if this cell is the harbor cell
  if (coords[0] == harbor_coords[0] && coords[1] == harbor_coords[1])
  {
    is_harbor = 1;
    printf("Harbor coordinates: %d,%d \n", harbor_coords[0], harbor_coords[1]);
  }

  // ************************** //
  // ** ocean initialization ** //
  // ************************** //

  // two random cells who are not the harbour generate fish groups
  // first we need to generate two random numbers representing the rank
  fish_ranks[0] = rand() % SIZE;
  fish_ranks[1] = rand() % SIZE;
  // if the fish group lands on the harbor we have to try again
  while (is_harbor == 1 && fish_ranks[0] == rank)
  {
    fish_ranks[0] = rand() % SIZE;
  }
  while (is_harbor == 1 && fish_ranks[1] == rank && fish_ranks[0] == fish_ranks[1])
  {
    fish_ranks[1] = rand() % SIZE;
  }

  // ************************ //
  // ****** ITERATIONS ****** //
  // ************************ //

  // fish iteration

  for (int ii = 0; ii < 10; ii++)
  {
    prev_fish_ranks[0] = fish_ranks[0];
    prev_fish_ranks[1] = fish_ranks[1];
    // two fish groups
    if (fish_ranks[0] == rank || fish_ranks[1] == rank)
    {
      int index;
      if (fish_ranks[0] == rank)
        index = 1;
      else
        index = 2;
      // fish structure
      Fish fish = fish_constructor(index, 10, coords);
      // shifting to gain information about our neighbours using the cart-communicator
      int nbrs[4];
      MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
      MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);
      // printf("Neighbours %d, %d, %d, %d \n", nbrs[UP], nbrs[DOWN], nbrs[LEFT], nbrs[RIGHT]);

      // let's choose a random neighbour (direction) for the fish group to swim to
      // we don't want to get same numbers across processes this time so we generate different seeds
      srand(time(NULL) + rank);
      int rand_neighbour = nbrs[rand() % 4];
      // update fish ranks
      if (fish_ranks[0] == rank)
      {
        // we don't want the fish groups stacking on each other
        while (rand_neighbour == fish_ranks[1]) {
          rand_neighbour = nbrs[rand() % 4];
        }
        fish_ranks[0] = rand_neighbour;
      }
      if (fish_ranks[1] == rank)
      {
        // we don't want the fish groups stacking on each other
        while (rand_neighbour == fish_ranks[0]) {
          rand_neighbour = nbrs[rand() % 4];
        }
        fish_ranks[1] = rand_neighbour;
      }
    }

    // lets broadcast the new fish ranks to all processors!
    MPI_Bcast(&fish_ranks[0], 1, MPI_INT, prev_fish_ranks[0], MPI_COMM_WORLD);
    MPI_Bcast(&fish_ranks[1], 1, MPI_INT, prev_fish_ranks[1], MPI_COMM_WORLD);

    // lets visualize the grid
    visualizeGrid(&rank, &outbuf, prev_fish_ranks);
  }

  MPI_Finalize();

  return 0;
}
