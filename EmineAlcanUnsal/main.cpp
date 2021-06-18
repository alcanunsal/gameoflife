/*
 * Emine Alcan Unsal
 * 2016400105
 * Compiling
 * Working
 * alcanunsal@gmail.com
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <math.h>

int GRID_SIZE = 360;

using namespace std;


int main(int argc, char** argv) {

    if (argc != 4) {
        cout<<"unsuitable argument format. suitable format is \"mpirun -np [M] --oversubscribe ./game input.txt output.txt [T]\""<<endl;
        exit(1);
    }

    MPI_Status status;
    int tag, size, rank;
    double numIterations =atoi(argv[3]);
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // rank of current process

    int masterData[GRID_SIZE][GRID_SIZE];
    int newMasterData[GRID_SIZE][GRID_SIZE];
    int numWorkers = size-1;
    int workerDim = GRID_SIZE/(int)sqrt(numWorkers);
    int n = (int)sqrt(numWorkers);

    ifstream i_file;

    if (rank==0) {
        // inside master process
        i_file.open(argv[1]);

         // check if file is opened or not
        if (!i_file) {
            cout<<"couldnt open file"<<endl;
        }

        // read input and store master data in masterData 2D array.
        for (int o=0; o<GRID_SIZE; o++) {
            string line, token;
            getline(i_file, line);
            stringstream lines(line);
            for (int r=0; r<GRID_SIZE; r++) {
                lines >> token;
                int cell;
                if (token.compare("0")==0) {
                    cell = 0;
                } else if (token.compare("1")==0) {
                    cell = 1;
                } else {
                    cout<<"invalid input: "<<token<<" .....from i="<<o<<" ,j="<<r<<endl;
                    MPI_Finalize();
                    exit(1);
                }
                masterData[o][r]=cell;
            }
        }
        i_file.close();

        //send data to each subprocess (slave)
        for (int k=1; k<=numWorkers; k++) {
            int subgrid[workerDim][workerDim];
            for (int row=0; row<workerDim; row++) {
                for (int col=0; col<workerDim; col++) {
                    int ro=(int)((k-1)/sqrt(numWorkers));
                    int co=k-(sqrt(numWorkers)*ro)-1;
                    subgrid[row][col]=masterData[ro*workerDim+row][co*workerDim+col];
                }
            }
            MPI_Send(&subgrid, workerDim*workerDim, MPI_INT, k, 0, MPI_COMM_WORLD);
        }

        //receives data from each slave
        int sub_grid[workerDim][workerDim];
        for(int p=1; p<=numWorkers; p++) {
            MPI_Recv(&sub_grid, workerDim*workerDim, MPI_INT, p, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int aa=0; aa<workerDim; aa++) {
                for (int bb=0; bb<workerDim; bb++) {

                    int ro=(p-1)/sqrt(numWorkers);
                    int co=p-(sqrt(numWorkers)*ro)-1;
                    newMasterData[workerDim*ro+aa][workerDim*co+bb]=sub_grid[aa][bb];
                }
            }
        }

        //writes to output file (argv[3])
        ofstream o_file;
        string file = argv[2];
        o_file.open(file);
        if (!o_file) {
            cout<<"couldnt open file"<<endl;
        }
        for (int x=0; x<GRID_SIZE; x++) {
            for (int y=0; y<GRID_SIZE; y++) {
                o_file<<newMasterData[x][y]<<" ";
            }
            o_file<<"\n";
        }
        o_file.close();

    } else {

        int originalSlave[workerDim][workerDim];
        int newSlave[workerDim][workerDim];
        //receive data from master into originalSlave
        MPI_Recv(&originalSlave, workerDim*workerDim,MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // main iteration loop
        for (int iter = 0; iter < numIterations; iter++) {

            // data to get from neighbors
            int get_upper[workerDim];
            int get_lower[workerDim];
            int get_left[workerDim];
            int get_right[workerDim];
            int get_upperLeft, get_upperRight, get_lowerLeft, get_lowerRight;

            //edges to be sent
            int send_left[workerDim];
            int send_right[workerDim];
            int send_upper[workerDim];
            int send_lower[workerDim];

            //corners to be sent
            int send_upperLeft = originalSlave[0][0];
            int send_upperRight = originalSlave[0][workerDim - 1];
            int send_lowerLeft = originalSlave[workerDim - 1][0];
            int send_lowerRight = originalSlave[workerDim - 1][workerDim - 1];

            // prepere the data to be sended
            for (int z = 0; z < workerDim; z++) {
                send_left[z] = originalSlave[z][0];
                send_right[z] = originalSlave[z][workerDim - 1];
                send_upper[z] = originalSlave[0][z];
                send_lower[z] = originalSlave[workerDim - 1][z];
            }

            //do the communications
            //horizontal communication
            if (rank % 2 == 0) {
                //receive from right
                if (rank % n != 0) {
                    MPI_Recv(&get_right, workerDim, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&get_right, workerDim, MPI_INT, rank - n + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //receive from left
                if (rank % n != 1) {
                    MPI_Recv(&get_left, workerDim, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&get_left, workerDim, MPI_INT, rank + n - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //send to left
                if (rank % n != 1) {
                    MPI_Send(&send_left, workerDim, MPI_INT, rank - 1, 1, MPI_COMM_WORLD);
                } else {
                    MPI_Send(&send_left, workerDim, MPI_INT, rank - 1 + n, 1, MPI_COMM_WORLD);
                }
                //send to right
                if (rank % n != 0) {
                    MPI_Send(&send_right, workerDim, MPI_INT, rank + 1, 1, MPI_COMM_WORLD);
                } else {
                    MPI_Send(&send_right, workerDim, MPI_INT, rank + 1 - n, 1, MPI_COMM_WORLD);
                }
            } else {
                //send to left
                if (rank % n != 1) {
                    MPI_Send(&send_left, workerDim, MPI_INT, rank - 1, 1, MPI_COMM_WORLD);
                } else {
                    MPI_Send(&send_left, workerDim, MPI_INT, rank - 1 + n, 1, MPI_COMM_WORLD);
                }
                //send to right
                if (rank % n != 0) {
                    MPI_Send(&send_right, workerDim, MPI_INT, rank + 1, 1, MPI_COMM_WORLD);
                } else {
                    MPI_Send(&send_right, workerDim, MPI_INT, rank + 1 - n, 1, MPI_COMM_WORLD);
                }
                //receive from right
                if (rank % n != 0) {
                    MPI_Recv(&get_right, workerDim, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&get_right, workerDim, MPI_INT, rank - n + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //receive from left
                if (rank % n != 1) {
                    MPI_Recv(&get_left, workerDim, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&get_left, workerDim, MPI_INT, rank + n - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
            //up-to-down communication
            if ((int)((rank - 1) / n) % 2 == 0) {
                //send to up
                if (rank > n) {
                    MPI_Send(&send_upper, workerDim, MPI_INT, rank - n, 1, MPI_COMM_WORLD);
                } else {
                    MPI_Send(&send_upper, workerDim, MPI_INT, numWorkers + rank - n, 1, MPI_COMM_WORLD);
                }
                //send to down
                if (rank <= numWorkers - n) {
                    MPI_Send(&send_lower, workerDim, MPI_INT, rank + n, 1, MPI_COMM_WORLD);
                } else {
                    MPI_Send(&send_lower, workerDim, MPI_INT, rank % n, 1, MPI_COMM_WORLD);
                }
                //receive from down
                if (rank > numWorkers - n) {
                    int dest = rank % n;
                    if (rank % n == 0) {
                        dest=4;
                    }
                    MPI_Recv(&get_lower, workerDim, MPI_INT, dest, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&get_lower, workerDim, MPI_INT, rank + n, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //receive from up
                if (rank <= n) {
                    MPI_Recv(&get_upper, workerDim, MPI_INT, rank - n + numWorkers, 1, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&get_upper, workerDim, MPI_INT, rank - n, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }

            } else {
                //receive from down
                if (rank > numWorkers - n) {
                    MPI_Recv(&get_lower, workerDim, MPI_INT, rank-numWorkers+n, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&get_lower, workerDim, MPI_INT, rank + n, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //receive from up
                if (rank > n) {
                    MPI_Recv(&get_upper, workerDim, MPI_INT, rank - n, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&get_upper, workerDim, MPI_INT, rank - n + numWorkers, 1, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);
                }
                //send to up
                if (rank > n) {
                    MPI_Send(&send_upper, workerDim, MPI_INT, rank - n, 1, MPI_COMM_WORLD);
                } else {
                    MPI_Send(&send_upper, workerDim, MPI_INT, numWorkers + rank - n, 1, MPI_COMM_WORLD);
                }
                //send to down
                if (rank <= numWorkers - n) {
                    MPI_Send(&send_lower, workerDim, MPI_INT, rank + n, 1, MPI_COMM_WORLD);
                } else {
                    MPI_Send(&send_lower, workerDim, MPI_INT, rank-numWorkers+n, 1, MPI_COMM_WORLD);
                }

            }

            //diagonal communication
            if (rank % 2 == 0) {
                //receive lower right//receive from right-down
                if (rank % n == 0 || rank > numWorkers - n) {
                    if (rank % n == 0 && rank > numWorkers - n) {
                        MPI_Recv(&get_lowerRight, 1, MPI_INT, 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else if (rank % n == 0) {
                        MPI_Recv(&get_lowerRight, 1, MPI_INT, rank + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else {
                        MPI_Recv(&get_lowerRight, 1, MPI_INT, (rank % n) + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                } else {
                    MPI_Recv(&get_lowerRight, 1, MPI_INT, rank + n + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //send upper left//send to left-up
                if (rank <= n || rank % n == 1) {
                    if (rank == 1) {
                        MPI_Send(&send_upperLeft, 1, MPI_INT, numWorkers - 1 + 1, 2, MPI_COMM_WORLD);
                    } else if (rank <= n) {
                        MPI_Send(&send_upperLeft, 1, MPI_INT, numWorkers - n - 1 + rank, 2, MPI_COMM_WORLD);
                    } else {
                        MPI_Send(&send_upperLeft, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD);
                    }
                } else {
                    MPI_Send(&send_upperLeft, 1, MPI_INT, rank - n - 1, 2, MPI_COMM_WORLD);
                }
                //receive upper left//receive from left-up
                if (rank <= n || rank % n == 1) {
                    if (rank <= n && rank % n == 1) {
                        MPI_Recv(&get_upperLeft, 1, MPI_INT, numWorkers - 1 + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else if (rank <= n) {
                        MPI_Recv(&get_upperLeft, 1, MPI_INT, numWorkers + rank - n - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else {
                        MPI_Recv(&get_upperLeft, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                } else {
                    MPI_Recv(&get_upperLeft, 1, MPI_INT, rank - n - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //send lower right//send to right-down
                if (rank % n == 0 || rank > numWorkers - n) {
                    if (rank % n == 0 && rank > numWorkers - n) {
                        MPI_Send(&send_lowerRight, 1, MPI_INT, 1, 2, MPI_COMM_WORLD);
                    } else if (rank % n == 0) {
                        MPI_Send(&send_lowerRight, 1, MPI_INT, rank + 1, 2, MPI_COMM_WORLD);
                    } else {
                        MPI_Send(&send_lowerRight, 1, MPI_INT, (rank % n) + 1, 2, MPI_COMM_WORLD);
                    }
                } else {
                    MPI_Send(&send_lowerRight, 1, MPI_INT, rank + n + 1, 2, MPI_COMM_WORLD);
                }

                //receive lower left//receive from left-down
                if (rank % n == 1 || rank > numWorkers - n) {
                    if (rank % n == 1 && rank > numWorkers - n) {
                        MPI_Recv(&get_lowerLeft, 1, MPI_INT, n + 1 - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else if (rank % n == 1) {
                        MPI_Recv(&get_lowerLeft, 1, MPI_INT, rank + (2 * n) - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else {
                        MPI_Recv(&get_lowerLeft, 1, MPI_INT, rank-numWorkers+n-1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                } else {
                    MPI_Recv(&get_lowerLeft, 1, MPI_INT, rank + n - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //send upper right//send to right-up
                if (rank <= n || rank % n == 0) {
                    if (rank == n) {
                        MPI_Send(&send_upperRight, 1, MPI_INT, numWorkers - n + 1, 2, MPI_COMM_WORLD);
                    } else if (rank <= n) {
                        MPI_Send(&send_upperRight, 1, MPI_INT, numWorkers + rank - n + 1, 2, MPI_COMM_WORLD);
                    } else {
                        MPI_Send(&send_upperRight, 1, MPI_INT, rank - 2 * n + 1, 2, MPI_COMM_WORLD);
                    }
                } else {
                    MPI_Send(&send_upperRight, 1, MPI_INT, rank - n + 1, 2, MPI_COMM_WORLD);
                }
                //receive upper right//receive from right-up
                if (rank <= n || rank % n == 0) {
                    if (rank <= n && rank % n == 0) {
                        MPI_Recv(&get_upperRight, 1, MPI_INT, numWorkers - n + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else if (rank <= n) {
                        MPI_Recv(&get_upperRight, 1, MPI_INT, rank - n + 1 + numWorkers, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else {
                        MPI_Recv(&get_upperRight, 1, MPI_INT, rank - (2 * n) + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                } else {
                    MPI_Recv(&get_upperRight, 1, MPI_INT, rank - n + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //send lower left//send to left-down
                if (rank % n == 1 || rank > numWorkers - n) {
                    if (rank % n == 1 && rank > numWorkers - n) {
                        MPI_Send(&send_lowerLeft, 1, MPI_INT, n, 2, MPI_COMM_WORLD);
                    } else if (rank % n == 1) {
                        MPI_Send(&send_lowerLeft, 1, MPI_INT, rank + (2 * n) - 1, 2, MPI_COMM_WORLD);
                    } else {
                        MPI_Send(&send_lowerLeft, 1, MPI_INT, rank - numWorkers + n-1, 2, MPI_COMM_WORLD);
                    }
                } else {
                    MPI_Send(&send_lowerLeft, 1, MPI_INT, rank + n - 1, 2, MPI_COMM_WORLD);
                }
            } else {
                //send upper left//send to left-up
                if (rank <= n || rank % n == 1) {
                    if (rank == 1) {
                        MPI_Send(&send_upperLeft, 1, MPI_INT, numWorkers - 1 + 1, 2, MPI_COMM_WORLD);
                    } else if (rank <= n) {
                        MPI_Send(&send_upperLeft, 1, MPI_INT, numWorkers - n - 1 + rank, 2, MPI_COMM_WORLD);
                    } else {
                        MPI_Send(&send_upperLeft, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD);
                    }
                } else {
                    MPI_Send(&send_upperLeft, 1, MPI_INT, rank - n - 1, 2, MPI_COMM_WORLD);
                }
                //receive lower right//receive from right-down
                if (rank % n == 0 || rank > numWorkers - n) {
                    if (rank % n == 0 && rank > numWorkers - n) {
                        MPI_Recv(&get_lowerRight, 1, MPI_INT, 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else if (rank % n == 0) {
                        MPI_Recv(&get_lowerRight, 1, MPI_INT, rank + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else {
                        MPI_Recv(&get_lowerRight, 1, MPI_INT, (rank % n) + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                } else {
                    MPI_Recv(&get_lowerRight, 1, MPI_INT, rank + n + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //send lower right//send to right-down
                if (rank % n == 0 || rank > numWorkers - n) {
                    if (rank % n == 0 && rank > numWorkers - n) {
                        MPI_Send(&send_lowerRight, 1, MPI_INT, 1, 2, MPI_COMM_WORLD);
                    } else if (rank % n == 0) {
                        MPI_Send(&send_lowerRight, 1, MPI_INT, rank + 1, 2, MPI_COMM_WORLD);
                    } else {
                        MPI_Send(&send_lowerRight, 1, MPI_INT, (rank % n) + 1, 2, MPI_COMM_WORLD);
                    }
                } else {
                    MPI_Send(&send_lowerRight, 1, MPI_INT, rank + n + 1, 2, MPI_COMM_WORLD);
                }
                //receive upper left//receive from left-up
                if (rank <= n || rank % n == 1) {
                    if (rank <= n && rank % n == 1) {
                        MPI_Recv(&get_upperLeft, 1, MPI_INT, numWorkers - 1 + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else if (rank <= n) {
                        MPI_Recv(&get_upperLeft, 1, MPI_INT, numWorkers + rank - n - 1, 2, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                    } else {
                        MPI_Recv(&get_upperLeft, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    }
                } else {
                    MPI_Recv(&get_upperLeft, 1, MPI_INT, rank - n - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //send upper right//send to right-up
                if (rank <= n || rank % n == 0) {
                    if (rank == n) {
                        MPI_Send(&send_upperRight, 1, MPI_INT, numWorkers - n + 1, 2, MPI_COMM_WORLD);
                    } else if (rank <= n) {
                        MPI_Send(&send_upperRight, 1, MPI_INT, numWorkers + rank - n + 1, 2, MPI_COMM_WORLD);
                    } else {
                        MPI_Send(&send_upperRight, 1, MPI_INT, rank - 2 * n + 1, 2, MPI_COMM_WORLD);
                    }
                } else {
                    MPI_Send(&send_upperRight, 1, MPI_INT, rank - n + 1, 2, MPI_COMM_WORLD);
                }
                //receive lower left//receive from left-down
                if (rank % n == 1 || rank > numWorkers - n) {
                    if (rank % n == 1 && rank > numWorkers - n) {
                        MPI_Recv(&get_lowerLeft, 1, MPI_INT, n + 1 - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else if (rank % n == 1) {
                        MPI_Recv(&get_lowerLeft, 1, MPI_INT, rank + (2 * n) - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else {
                        MPI_Recv(&get_lowerLeft, 1, MPI_INT, rank-numWorkers+n-1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                } else {
                    MPI_Recv(&get_lowerLeft, 1, MPI_INT, rank + n - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                //send lower left//send to left-down
                if (rank % n == 1 || rank > numWorkers - n) {
                    if (rank % n == 1 && rank > numWorkers - n) {
                        MPI_Send(&send_lowerLeft, 1, MPI_INT, n, 2, MPI_COMM_WORLD);
                    } else if (rank % n == 1) {
                        MPI_Send(&send_lowerLeft, 1, MPI_INT, rank + (2 * n) - 1, 2, MPI_COMM_WORLD);
                    } else {
                        MPI_Send(&send_lowerLeft, 1, MPI_INT, rank - numWorkers + n-1, 2, MPI_COMM_WORLD);
                    }
                } else {
                    MPI_Send(&send_lowerLeft, 1, MPI_INT, rank + n - 1, 2, MPI_COMM_WORLD);
                }
                //receive upper right//receive from right-up
                if (rank <= n || rank % n == 0) {
                    if (rank <= n && rank % n == 0) {
                        MPI_Recv(&get_upperRight, 1, MPI_INT, numWorkers - n + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    } else if (rank <= n) {
                        MPI_Recv(&get_upperRight, 1, MPI_INT, rank - n + 1 + numWorkers, 2, MPI_COMM_WORLD,
                                 MPI_STATUS_IGNORE);
                    } else {
                        MPI_Recv(&get_upperRight, 1, MPI_INT, rank - (2 * n) + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                } else {
                    MPI_Recv(&get_upperRight, 1, MPI_INT, rank - n + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
            int sum_neighbors = 0;
            // make calculations for the next version of grid using neighbors
            for (int a = 0; a < workerDim; a++) {
                for (int b = 0; b < workerDim; b++) {
                    if (a == 0 && b == 0) {
                        //top left corner
                        sum_neighbors = originalSlave[1][1] + originalSlave[0][1] + originalSlave[1][0] +
                                get_upper[0]+ get_upper[1] + get_left[0] + get_left[1] + get_upperLeft;

                    } else if (a == 0 && b == workerDim - 1) {
                        //top right corner
                        sum_neighbors = originalSlave[0][workerDim - 2] + originalSlave[1][workerDim - 2] +
                                originalSlave[1][workerDim - 1]+ get_right[0] + get_right[1] + get_upper[workerDim - 1] +
                                get_upper[workerDim - 2] + get_upperRight;
                    } else if (a == workerDim - 1 && b == 0) {
                        //bottom left corner
                        sum_neighbors = originalSlave[workerDim - 1][1] + originalSlave[workerDim - 2][1] +
                                originalSlave[workerDim - 2][0]+ get_left[workerDim - 1] + get_left[workerDim - 2] + get_lower[0] +
                                get_lower[1] + get_lowerLeft;
                    } else if (a == workerDim - 1 && b == workerDim - 1) {
                        //bottom right corner
                        sum_neighbors = originalSlave[workerDim - 1][workerDim - 2] +originalSlave[workerDim - 2][workerDim - 2] +
                                originalSlave[workerDim - 2][workerDim - 1]+ get_lower[workerDim - 2] + get_lower[workerDim - 1] +
                                get_right[workerDim - 1] + get_right[workerDim - 2] +get_lowerRight;
                    } else if (a == 0) {
                        //top edge (except corners)
                        sum_neighbors = originalSlave[0][b - 1] + originalSlave[0][b + 1] +originalSlave[1][b - 1] +
                                originalSlave[1][b] + originalSlave[1][b + 1]+ get_upper[b - 1] + get_upper[b] + get_upper[b + 1];
                    } else if (a == workerDim - 1) {
                        //bottom edge (except corners)
                        sum_neighbors =originalSlave[workerDim - 1][b - 1] + originalSlave[workerDim - 1][b + 1] +
                                originalSlave[workerDim - 2][b - 1] + originalSlave[workerDim - 2][b + 1]
                                + originalSlave[workerDim - 2][b] + get_lower[b - 1] + get_lower[b] +get_lower[b + 1];
                    } else if (b == 0) {
                        //left edge (except corners)
                        sum_neighbors = originalSlave[a - 1][0] + originalSlave[a + 1][0] +originalSlave[a - 1][1] +
                                originalSlave[a + 1][1] + originalSlave[a][1]+ get_left[a - 1] + get_left[a] + get_left[a + 1];
                    } else if (b == workerDim - 1) {
                        //right edge (except corners)
                        sum_neighbors =originalSlave[a - 1][b] + originalSlave[a + 1][b] +originalSlave[a - 1][b - 1] +
                                originalSlave[a + 1][b - 1] + originalSlave[a][b - 1]+ get_right[a - 1] + get_right[a] + get_right[a + 1];
                    } else {
                        //middle
                        sum_neighbors =originalSlave[a - 1][b - 1] + originalSlave[a - 1][b] +originalSlave[a - 1][b + 1] +
                                originalSlave[a][b - 1]+ originalSlave[a][b + 1] + originalSlave[a + 1][b - 1] +originalSlave[a + 1][b] +
                                originalSlave[a + 1][b + 1];
                    }
                    if (originalSlave[a][b] == 1) {
                        if (sum_neighbors < 2) {
                            newSlave[a][b] = 0;
                        } else if (sum_neighbors > 3) {
                            newSlave[a][b] = 0;
                        } else {
                            newSlave[a][b] = 1;
                        }
                    } else if (originalSlave[a][b] == 0) {
                        if (sum_neighbors == 3) {
                            newSlave[a][b] = 1;
                        } else {
                            newSlave[a][b] = 0;
                        }
                    } else {
                        cout << ".......unexpected value in slave grid: " << originalSlave[a][b] << endl;
                        newSlave[a][b] = 8;
                    }
                }
            }

            for(int ro=0; ro<workerDim; ro++) {
                for (int co=0; co<workerDim; co++) {
                    originalSlave[ro][co]=newSlave[ro][co];
                }
            }
        }

        MPI_Send(&originalSlave, workerDim*workerDim, MPI_INT,0,99,MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}

