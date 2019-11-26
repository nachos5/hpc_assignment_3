#include "./utils.h"

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
void visualizeGrid(int *rank, int *outbuf, int *fish_ranks)
{
  sleep(1);
  MPI_Barrier(MPI_COMM_WORLD);
  // only the root process gathers the data and prints it
  if (*rank == 0)
  {
    // the root process gathers the type + rank info
    int recv_type_buff[SIZE];
    MPI_Gather(outbuf, 1, MPI_INT, recv_type_buff, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int recv_rank_buff[SIZE];
    MPI_Gather(rank, 1, MPI_INT, recv_rank_buff, 1, MPI_INT, 0, MPI_COMM_WORLD);

    printf("\nGrid:\n");
    for (int i = 0; i < SIZE; i++)
    {
      // print the type of this cell + if it has fish
      if (fish_ranks[0] == recv_rank_buff[i]) {
        printf("%df1   ", recv_type_buff[i]); // f1 for fish group 1
      } else if (fish_ranks[1] == recv_rank_buff[i]) {
        printf("%df2   ", recv_type_buff[i]); // f2 for fish group 2
      } else {
        printf("%d     ", recv_type_buff[i]);
      }
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
    MPI_Gather(outbuf, 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(rank, 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
  }
}