/**
 * main.cpp
 *
 * The entry point for data correlation program
 *
 * @author Dennis J. McWherter, Jr.
 */

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "Configuration.h"
#include "Master.h"
#include "Slave.h"
#include "Status.h"

#include "mpi.h"

using namespace std;

int main(int argc, char** argv)
{
  int rank;

  // Make sure we have an input file
  // This is a temporary purpose command line input
  // NOTE: Will probably be best to create a /very/
  // small and simple config file
  if(argc < 2) {
    cerr<< "Usage: "<< argv[0] << " <config_file>" << endl;
    return 1;
  }

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  cout<< "Initializing processor " << rank << "..." << endl;

  try{
    Configuration config(argv[1]);
    if(rank == MASTER) {
      Master master(config);
      master.run();
    } else {
      Slave slave(config);
      slave.run();
    }
  } catch(exception& e) {
    cerr<< "Error: "<< e.what() << endl;
  }

  MPI_Finalize();

  cout<< "Exiting ("<< rank <<")" << endl;

  return 0;
}
