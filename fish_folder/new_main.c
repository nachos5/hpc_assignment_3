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
#define SIZE 16
#define ROWS 4
#define COLS 4

typedef struct Harbor
{
  int rank;
  int x;
  int y;
} Harbor;

typedef struct Fish
{
  int rank;
  int x;
  int y;
} Fish;

typedef struct Boat
{
  int rank;
  int x;
  int y;
} Boat;


void initObjects(Harbor *harbor, Fish *fish, Boat *boat)
{
  harbor->x = rand() % COLS;
  harbor->y = rand() % ROWS;
  harbor->rank = harbor->x * COLS + harbor->y;

  do {
  fish[0].x = rand() % COLS;
  fish[0].y = rand() % ROWS;
  fish[0].rank = fish[0].x * COLS + fish[0].y;
  } while(fish[0].rank == harbor->rank);

  do {
    fish[1].x = rand() % COLS;
    fish[1].y = rand() % ROWS;
    fish[1].rank = fish[1].x * COLS + fish[1].y;
  } while(fish[1].rank == harbor->rank);


  do {
    boat[0].x = rand() % COLS;
    boat[0].y = rand() % ROWS;
    boat[0].rank = boat[0].x * COLS + boat[0].y;
  } while(boat[0].rank == harbor->rank);

  do {
    boat[1].x = rand() % COLS;
    boat[1].y = rand() % ROWS;
    boat[1].rank = boat[1].x * COLS + boat[1].y;
  } while(boat[1].rank == harbor->rank);
}

void updateObjects(int *nbrs, Harbor *harbor, Fish *fish, Boat *boat)
{
  do {
    fish[0].x = (fish[0].x + (rand() % 2) - 1) % COLS;
    fish[0].y = (fish[0].y + (rand() % 2) - 1) % ROWS;
    fish[0].rank = fish[0].x * COLS + fish[0].y;
  } while(fish[0].rank == harbor->rank);

  do {
    fish[1].x = (fish[1].x + (rand() % 2) - 1) % COLS;
    fish[1].y = (fish[1].y + (rand() % 2) - 1) % ROWS;
    fish[1].rank = fish[1].x * COLS + fish[1].y;
  } while(fish[0].rank == harbor->rank || fish[1].rank == fish[0].rank);

  boat[0].x = rand() % COLS;
  boat[0].y = rand() % ROWS;
  boat[0].rank = boat[0].x * COLS + boat[0].y;

  boat[1].x = rand() % COLS;
  boat[1].y = rand() % ROWS;
  boat[1].rank = boat[1].x * COLS + boat[1].y;
}

void updateAll(int *outbuf, int rank, Harbor *harbor, Fish *fish, Boat *boat)
{
  if (rank == harbor->rank)
  {
    *outbuf = 1;
  } 
  else if (rank == fish[0].rank || rank == fish[1].rank)
  {
    *outbuf = 2;
  } 
  else if (rank == boat[0].rank || rank == boat[1].rank)
  {
    *outbuf = 3;
  }
  else {
    *outbuf = 0;
  }
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

  // ADDED
  MPI_Datatype objectstruct;

  int blocklens[3];
  MPI_Aint indicies[3];
  MPI_Datatype oldtypes[3] = {MPI_INT, MPI_INT, MPI_INT};

  Harbor harbor;
  Fish fish[2];
  Boat boat[2];

  MPI_Comm cartcomm;

  MPI_Request reqs[8];
  MPI_Status stats[8];

  // starting with MPI program
  MPI_Init(&argc, &argv);

  // One value of each type
  blocklens[0] = 1;
  blocklens[1] = 1;
  blocklens[2] = 1;

  // Get addresses
  MPI_Get_address(&harbor.rank, &indicies[0]);
  MPI_Get_address(&harbor.x, &indicies[1]);
  MPI_Get_address(&harbor.y, &indicies[2]);

  // Relative addresses
  indicies[2] = indicies[2] - indicies[1];
  indicies[1] = indicies[1] - indicies[0];
  indicies[0] = 0;

  MPI_Type_create_struct(3, blocklens, indicies, oldtypes, &objectstruct);
  MPI_Type_commit(&objectstruct);

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);

  MPI_Comm_rank(cartcomm, &rank);

  // let's receive the coordinates of this cell
  MPI_Cart_coords(cartcomm, rank, 2, coords);  

  // Master thread initializes structs for all other threads
  if (rank == 0)
  {
    initObjects(&harbor, fish, boat);
    printf("Fish 1 rank %d, fish 2 rank %d \n", fish[0].rank, fish[1].rank);
  }

  /*
  printf("rank %d:     ", rank);
  printf("Harbor rank %d and coordinates: %d,%d \n", 
  harbor.rank, harbor.x, harbor.y);
  */

  MPI_Bcast(&harbor, 3, objectstruct, 0, MPI_COMM_WORLD);
  MPI_Bcast(&fish, 3, objectstruct, 0, MPI_COMM_WORLD);
  MPI_Bcast(&boat, 3, objectstruct, 0, MPI_COMM_WORLD);

  // Update grid for all threads
  updateAll(&outbuf, rank, &harbor, fish, boat);

  visualizeGrid(rank, outbuf);

  for (i = 0; i < 4; i++)
  {
    // UPDATE //
  
    // shifting to gain information about our neighbours
    MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
    MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);

    for (j = 0; j < 4; j++)
    {
      dest = nbrs[j];
      source = nbrs[j];

      // perform non-blocking communication
      MPI_Isend(&outbuf, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, &reqs[j]);
      MPI_Irecv(&inbuf[j], 1, MPI_INT, source, tag, MPI_COMM_WORLD, &reqs[j + 4]); // 4 as a kind of offset
    }

    /*
    printf("Rank %d is sending it's type: %d, to it's neighbours: %d %d %d %d. \n",
           rank, outbuf, nbrs[0], nbrs[1], nbrs[2], nbrs[3]);
    */

    // wait for non-blocking communication to be completed for output
    MPI_Waitall(8, reqs, stats);

    /*
    printf("Rank %d has received (u,d,l,r): %d %d %d %d \n", rank,
           inbuf[UP], inbuf[DOWN], inbuf[LEFT], inbuf[RIGHT]);
    */

    /* TODO  */


    // Master thread updates objects to a new position
    if (rank == 0)
    {
      updateObjects(nbrs, &harbor, fish, boat);
      printf("Fish 1 rank %d, fish 2 rank %d \n", fish[0].rank, fish[1].rank);
    }

    // Broadcast from master thread to all other threads
    MPI_Bcast(&harbor, 3, objectstruct, 0, MPI_COMM_WORLD);
    MPI_Bcast(&fish, 3, objectstruct, 0, MPI_COMM_WORLD);
    MPI_Bcast(&boat, 3, objectstruct, 0, MPI_COMM_WORLD);


    // Update grid for all threads
    updateAll(&outbuf, rank, &harbor, fish, boat);

   // VISUALIZE //

    visualizeGrid(rank, outbuf);
  }

  MPI_Finalize();

  return 0;
}
