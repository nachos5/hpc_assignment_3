#include "./utils.h"

// in this case we only need one landcell (the harbor), the rest is water
void generateType(int *rank, int *outbuf, int *harbor_rank, int *harbor_coords)
{
  // same seed to make sure we get the same result for all processes
  srand(time(NULL));
  int harbor_x = rand() % COLS; // 0 - (COLS-1)
  int harbor_y = rand() % ROWS; // 0 - (ROWS-1)
  *harbor_rank = coords_to_rank(harbor_x, harbor_y);
  // the harbor
  if (*rank == *harbor_rank)
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
void visualizeGrid(int *rank, int *outbuf, int *fish_ranks, int *boat_ranks, int *storm_ranks)
{
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
      char strengur[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0};
      strengur[0] = '|';
      strengur[1] = recv_type_buff[i] + '0';
      // fish
      if (fish_ranks[0] == recv_rank_buff[i])
      {
        strengur[3] = 'f';
        strengur[4] = '1';
      }
      else if (fish_ranks[1] == recv_rank_buff[i])
      {
        strengur[3] = 'f';
        strengur[4] = '2';
      }
      // boats
      if (boat_ranks[0] == recv_rank_buff[i])
      {
        strengur[6] = 'b';
        strengur[7] = '1';
      }
      else if (boat_ranks[1] == recv_rank_buff[i])
      {
        strengur[6] = 'b';
        strengur[7] = '2';
      }

      // Waves
      for (int j = 0; j < 16; j++)
      {
        if (storm_ranks[i] == 1)
        {
          strengur[3] = 's';
          strengur[4] = 's';
        }
      }

      // print the type of this cell + if it has fish
      printf("%s", strengur);
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
  MPI_Barrier(MPI_COMM_WORLD);
}

void obj_print_coordinates(int *type, int *index, int *coords, int *fish_group_size)
{
  if (*type == 1)
  {
    printf("Fish group %d of size %d is at coordinates %d,%d \n", *index, fish_group_size[*index - 1], coords[0], coords[1]);
  }
  else if (*type == 2)
  {
    printf("Boat %d is at coordinates %d,%d \n", *index, coords[0], coords[1]);
  }
}

/**
 * Find maximum between two numbers.
 */
int max(int num1, int num2)
{
    return (num1 > num2 ) ? num1 : num2;
}

/**
 * Find minimum between two numbers.
 */
int min(int num1, int num2) 
{
    return (num1 > num2 ) ? num2 : num1;
}

int coords_to_rank(int x, int y)
{
  return (x * COLS) + y;
}

int* rank_to_coords(int rank)
{
  int x = rank / COLS;
  int y = rank % COLS;
  int *coords = malloc(sizeof(int) * 2);
  coords[0] = x;
  coords[1] = y;
  return coords;
}

int manhattan_distance(int *p1, int *p2)
{
  return abs(p1[0] - p1[1]) + abs(p2[0] - p2[1]);
}

int towards_harbor(int *nbrs, int *harbor_rank)
{
  int min_dist = 1000;
  int min_index = 1000;
  int* harbor_coords = rank_to_coords(*harbor_rank);
  // because our grid is periodic we need to check "both ways"
  int* harbor_coords_other_way = rank_to_coords(*harbor_rank + COLS);
  for (int i = 0; i < 4; i++)
  {
    int dist;
    int* curr_coords = rank_to_coords(nbrs[i]);
    dist = min(manhattan_distance(harbor_coords, curr_coords), manhattan_distance(harbor_coords_other_way, curr_coords));
    min_dist = min(min_dist, dist);
    if (min_dist == dist) {
      min_index = i;
    }
  }
  // we return the index neighbour
  return min_index;
}