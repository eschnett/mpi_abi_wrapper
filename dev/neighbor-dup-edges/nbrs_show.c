#include <stdio.h>
#include <mpi.h>
int main(int argc,char**argv){
  int rank,size,nbrs[6];
  MPI_Comm cart;
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  int dims[3]={1,1,size}, periods[3]={1,1,1};
  MPI_Cart_create(MPI_COMM_WORLD,3,dims,periods,0,&cart);
  for(int d=0;d<3;d++) MPI_Cart_shift(cart,d,1,&nbrs[2*d],&nbrs[2*d+1]);
  if(rank==0){
    printf("dims = {1, 1, %d}, periods = {1,1,1}, %d ranks\n",size,size);
    printf("rank 0 neighbour slots (MPI_Cart_shift per dimension):\n");
    for(int d=0;d<3;d++)
      printf("  dim %d: slot %d (negative dir) = rank %d   slot %d (positive dir) = rank %d\n",
             d,2*d,nbrs[2*d],2*d+1,nbrs[2*d+1]);
    printf("so the neighbour list is [%d %d %d %d %d %d]\n",
           nbrs[0],nbrs[1],nbrs[2],nbrs[3],nbrs[4],nbrs[5]);
  }
  MPI_Finalize();
  return 0;
}
