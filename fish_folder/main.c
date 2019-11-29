
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
               int *boat_has_fish_group, int *e_index, double *elevation)
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
  
  /*
  // Waves
  if (type == 3)
  {
    if ((*e_index) + 1 >= 16)
    {
      (*e_index) = 0;
    }
    else {
      (*e_index)++;
    }

    if (elevation[(*e_index)] == 2.0)
    {
      //obj_ranks[0] = *rank;

    }
  }
  */

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

  int row_tasks, row_rank;

  int harbor_coords[2];
  int harbor_rank;

  int fish_ranks[2];
  int prev_fish_ranks[2];

  int boat_ranks[2];
  int prev_boat_ranks[2];

  int storm_ranks[SIZE];

  int fish_group_size[2] = {10, 10};
  // 0 means no fish group, 1 means group 1 etc.
  int boat_has_fish_group[2] = {0, 0};

  // ADDED Waves
  int number_of_elevations = 16;
  double e_step = 1.0/number_of_elevations;
  double amplitude = 2.0;

  int e = 0;
  double elevation[number_of_elevations];

  for(double x = 0.0; x < 1.0; x += e_step)
  {
    elevation[e] = amplitude * sin(x * 2*PI);

    if (elevation[e] == 2.0)
    {
      storm_ranks[e] = 1;
    }
    else
    {
      storm_ranks[e] = 0;
    }
    e++;
  }

  int e_index;

  MPI_Comm cartcomm;

  MPI_Request reqs[32];
  MPI_Status stats[32];
  
  
  int nbrs[4];

  int inbuffer[4][4] = {MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL, MPI_PROC_NULL};
  int outbuffer[4][4] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

  // starting with MPI program
  MPI_Init(&argc, &argv);


  // EEEEE
  MPI_Datatype cell;
  // <Land/Sea, harbor, boat, fish, storm>
  MPI_Type_vector(4, 1, 4, MPI_INT, &cell);
  MPI_Type_commit(&cell);

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);  
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);
  MPI_Comm_rank(cartcomm, &rank);


  MPI_Cart_coords(cartcomm, rank, 2, coords);


  /*
  // AAAAA //
  MPI_Comm row_comm;

  int color = rank / 4;

  MPI_Comm_split(MPI_COMM_WORLD, color, rank, &row_comm);


  MPI_Comm_size(row_comm, &row_tasks);
  MPI_Comm_rank(row_comm, &row_rank);

  // let's receive the coordinates of this cell
  MPI_Cart_coords(cartcomm, rank, 2, coords);

  // water or harbor
  //generateType(&rank, &outbuf, &harbor_rank, harbor_coords);

  // printing harbor coords
  if (coords[0] == harbor_coords[0] && coords[1] == harbor_coords[1])
  {
    printf("Harbor coordinates: %d,%d \n", harbor_coords[0], harbor_coords[1]);
  }

  init_obj_ranks(&rank, fish_ranks, &harbor_rank);
  init_obj_ranks(&rank, boat_ranks, &harbor_rank);

  e_index = row_rank;
  */


  MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
  MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);

  //printf("Rank %d is sending to it's neighbours: %d %d %d %d. \n", rank, nbrs[0], nbrs[1], nbrs[2], nbrs[3]);


  int k = 0;
  for (i = 0; i < 4; i++)
  {
    dest = nbrs[i];
    source = nbrs[i];

    for (int j = 0; j < 4; j++)
    {
      // perform non-blocking communication
      MPI_Isend(&outbuffer[i][j], 1, cell, dest, tag, MPI_COMM_WORLD, &reqs[k]);
      MPI_Irecv(&inbuffer[i][j], 1, cell, source, tag, MPI_COMM_WORLD, &reqs[k + 16]);
      k++;
    }
  }

  MPI_Waitall(32, reqs, stats);

  printf("Rank %d has received\n", rank);
  printf("check = %d\n", inbuffer[0][0]);
  for (int j = 0; j < 4; j++)
  {
    printf("(u,d,l,r): %d %d %d %d \n", inbuffer[UP][j], inbuffer[DOWN][j], inbuffer[LEFT][j], inbuffer[RIGHT][j]);
  }

  /*
  for (int it = 0; it < 10; it++)
  {

    
    // lets visualize the grid
    visualizeGrid(&rank, &outbuf, fish_ranks, boat_ranks, storm_ranks);


    int prev_e_index = e_index;
    if (e_index + 1 >= 16)
    {
      e_index = 0;
    }
    else {
      e_index++;
    }

    if (elevation[e_index] == 2.0)
    {
      storm_ranks[rank] = 1;
      MPI_Bcast(&storm_ranks[rank], 1, MPI_INT, rank, MPI_COMM_WORLD);
    }
    else if (elevation[prev_e_index] == 2.0)
    {
      storm_ranks[rank] = 0;
      MPI_Bcast(&storm_ranks[rank], 1, MPI_INT, rank, MPI_COMM_WORLD);
    }

    //MPI_Gather(&storm_ranks[rank], 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);

    //printf("rank %d\n", rank);
    //MPI_Send(&storm_ranks[rank], 1, MPI_INT, 0, tag, MPI_COMM_WORLD);

    //MPI_Recv(&storm_ranks[rank], 1, MPI_INT, a, tag, MPI_COMM_WORLD, stats)
    
    
    //MPI_Bcast(&storm_ranks, 16, MPI_INT, 0, MPI_COMM_WORLD);

    // fish iteration
    iteration(&rank, &cartcomm, &harbor_rank, &outbuf, 1, coords, prev_fish_ranks,
              fish_ranks, fish_group_size, boat_has_fish_group, &e_index, elevation);
    // boat iteration
    iteration(&rank, &cartcomm, &harbor_rank, &outbuf, 2, coords, prev_boat_ranks,
              boat_ranks, fish_group_size, boat_has_fish_group, &e_index, elevation);
    // process 0 checks if the boats caught some fish and then broadcasts to other processes
    if (rank == 0)
    {
      collisions(fish_ranks, boat_ranks, boat_has_fish_group);
    }
    MPI_Bcast(boat_has_fish_group, 2, MPI_INT, 0, MPI_COMM_WORLD);
    // finally we check if a boat has fish and if it can drop it at the harbor this iteration
    //drop_fish(boat_has_fish_group);
    
  }
  */
  MPI_Finalize();

  return 0;
}
