#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

// our imports
#include "./fish.c"

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define WATER 0
#define LAND 1

// grid size definitions
#define SIZE 16
#define ROWS 4
#define COLS 4

// in this case we only need one landcell (the harbor), the rest is water
void generateType(int *rank, int *outbuf, int *harbor_coords)
{
  // same seed to make sure we get the same result for all processes
  srand(time(NULL));
  int harbor_x = rand() % COLS; // 0 - (COLS-1)
  int harbor_y = rand() % ROWS; // 0 - (ROWS-1)
  int harbor_index = (harbor_x * COLS) + harbor_y;
  // the harbor
  if (*rank == harbor_index)
  {
    *outbuf = 1;
    // printf("%d,%d \n", harbor_x, harbor_y);
    // printf("%d", harbor_index);
    // water
  }
  else
  {
    *outbuf = 0;
  }

  // let's store the havbor coordinates for all processes
  harbor_coords[0] = harbor_x;
  harbor_coords[1] = harbor_y;
}

// function to visualize the grid, the root process gathers the type of all cells (processes)
// and then prints the content in the right order
void visualizeGrid(int rank, int outbuf)
{
  sleep(1);
  MPI_Barrier(MPI_COMM_WORLD);
  // only the root process gathers the data and prints it
  if (rank == 0)
  {
    int recv_buff[SIZE];
    MPI_Gather(&outbuf, 1, MPI_INT, recv_buff, 1, MPI_INT, 0, MPI_COMM_WORLD);
    printf("\nGrid:\n");
    for (int i = 0; i < SIZE; i++)
    {
      // print the type of this cell
      printf("%d ", recv_buff[i]);
      // we check if we have to go to the next row
      if ((i + 1) % COLS == 0)
      {
        printf("\n");
      }
    }
    printf("\n");
  }
  else
  {
    MPI_Gather(&outbuf, 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
  }
}

int main(int argc, char **argv)
{
  int numtasks, rank, source, dest, outbuf, i, j, tag = 1;
  int inbuf[4] = {MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL,
                  MPI_PROC_NULL};
  int dims[2] = {ROWS, COLS}, periods[2] = {1, 1}, reorder = 1;
  int coords[2];

  int harbor_coords[2];
  int is_harbor = 0;
  int fish_rank_1, fish_rank_2;
  int has_fish;

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
  generateType(&rank, &outbuf, harbor_coords);

  // let's check if this cell is the harbor cell
  if (coords[0] == harbor_coords[0] && coords[1] == harbor_coords[1])
  {
    is_harbor = 1;
    printf("Harbor coordinates: %d,%d \n", harbor_coords[0], harbor_coords[1]);
  }

  // initObjects(coords, &rank, &is_harbor);

  // ************************** //
  // ***** initialization ***** //
  // ************************** //

  // two random cells who are not the harbour generate fish groups
  // first we need to generate two random numbers representing the rank
  fish_rank_1 = rand() % SIZE;
  fish_rank_2 = rand() % SIZE;
  // if the fish group lands on the harbor we have to try again
  while (is_harbor == 1 && fish_rank_1 == rank)
  {
    fish_rank_1 = rand() % SIZE;
  }
  while (is_harbor == 1 && fish_rank_2 == rank && fish_rank_1 == fish_rank_2)
  {
    fish_rank_2 = rand() % SIZE;
  }

  // ************************ //
  // ****** ITERATIONS ****** //
  // ************************ //

  // lets visualize the grid
  visualizeGrid(rank, outbuf);

  // two fish groups
  if (fish_rank_1 == rank || fish_rank_2 == rank)
  {
    Fish fish = fish_constructor(10, -1, coords);
    printf("FISH %d,%d \n", fish.x, fish.y);

    // shifting to gain information about our neighbours
    int nbrs[4];
    MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
    MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);
    printf("Neighbours %d, %d, %d, %d \n", nbrs[UP], nbrs[DOWN], nbrs[LEFT], nbrs[RIGHT]);

    // let's choose a random neighbour (direction) for the fish group to swim to
    int rand_neighbour = nbrs[rand() % 4];
    // swim!
  }

  // does this cell have a fish group?
  // has_fish = (fish.x == coords[0]) && (fish.y == coords[1]);
  // printf("has fish: %d \n", has_fish);

  // printf("%d", fish->x);

  // iteration(&cartcomm, &has_fish);

  // for (i = 0; i < 4; i++)
  // {
  //   dest = nbrs[i];
  //   source = nbrs[i];

  //   // perform non-blocking communication
  //   MPI_Isend(&outbuf, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &reqs[i]);
  //   MPI_Irecv(&inbuf[i], 1, MPI_INT, source, tag, MPI_COMM_WORLD, &reqs[i + 4]); // 4 as a kind of offset
  // }

  // printf("Rank %d is sending it's type: %d, to it's neighbours: %d %d %d %d. \n",
  //        rank, outbuf, nbrs[0], nbrs[1], nbrs[2], nbrs[3]);

  // wait for non-blocking communication to be completed for output
  // MPI_Waitall(8, reqs, stats);

  // printf("Rank %d has received (u,d,l,r): %d %d %d %d \n", rank,
  //        inbuf[UP], inbuf[DOWN], inbuf[LEFT], inbuf[RIGHT]);

  visualizeGrid(rank, outbuf);

  MPI_Finalize();

  return 0;
}
