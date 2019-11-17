#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define WATER 0
#define LAND 1

// grid size definitions
#define SIZE 64
#define ROWS 8
#define COLS 8

// function which determines whether this cell is land (1) or water (0)
// to increase the chances of a more realistic landscape, we will
// use one of the neighbours as a seed.
// To create the random index we use the rank as a seed.
void generateType(int *rank, int nbrs[], int *outbuf)
{
  srand(time(NULL) + *rank);
  int rand_index = rand() % 4; // 0,1,2,3
  srand(time(NULL) + nbrs[rand_index]);
  *outbuf = rand() % 2; // 0,1
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
  int nbrs[4];
  int dims[2] = {ROWS, COLS}, periods[2] = {1, 1}, reorder = 1;
  int coords[2];
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

  // shifting to gain information about our neighbours
  MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
  MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);

  generateType(&rank, nbrs, &outbuf);

  for (i = 0; i < 4; i++)
  {
    dest = nbrs[i];
    source = nbrs[i];

    // perform non-blocking communication
    MPI_Isend(&outbuf, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &reqs[i]);
    MPI_Irecv(&inbuf[i], 1, MPI_INT, source, tag, MPI_COMM_WORLD, &reqs[i + 4]); // 4 as a kind of offset
  }

  printf("Rank %d is sending it's type: %d, to it's neighbours: %d %d %d %d. \n",
         rank, outbuf, nbrs[0], nbrs[1], nbrs[2], nbrs[3]);

  // wait for non-blocking communication to be completed for output
  MPI_Waitall(8, reqs, stats);

  printf("Rank %d has received (u,d,l,r): %d %d %d %d \n", rank,
         inbuf[UP], inbuf[DOWN], inbuf[LEFT], inbuf[RIGHT]);

  visualizeGrid(rank, outbuf);

  MPI_Finalize();

  return 0;
}
