/**
 * Master.cpp
 *
 * Master implementation
 *
 * @author Dennis J. McWherter, Jr.
 */

#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#define USE_GETCWD // To include proper files from DCUtil
#include "AnalyzeData.h"
#include "Configuration.h"
#include "DCUtil.h"
#include "Master.h"
#include "Status.h"

#include "mpi.h"

using namespace std;

/**
 * Constructor
 *
 * @param config    Configuration settings to pass to the master
 */
Master::Master(const Configuration& config)
  : config(config), work(NULL)
{
}

/**
 * Destructor
 */
Master::~Master()
{
  delete [] work;
}

void Master::run()
{
  startTime = clock();

  int size = delegateWork();

  cout<< "Work sent by master" << endl << "Waiting for slaves' responses..." << endl;

  collectResults(size);

  cout<< "Results written to: "<< getcwd(NULL, 0) << "/results-" << config.getRunName() << ".csv" << endl;
}

/**
 * Delegate work to the slave nodes
 *
 * @return  The number of nodes in the environment
 */
int Master::delegateWork()
{
  int size = 0;

  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // Remember the work we assigned for reporting purposes
  work = new double[size];

  // Tell the worker threads what to modify/run
  work[0] = 0;
  for(int i = 1 ; i < size ; ++i) {
    // Calculate work to do (evenly divide the steps)
    if(i != 1) {
      switch(config.getSymmetry()) {
      case Configuration::POSITIVE:
        work[i - 1] = 1.f / (float)(i - 1);
        break;
      case Configuration::NEGATIVE:
        work[i - 1] = -1.f / (float)(i - 1);
        break;
      case Configuration::SYMMETRIC:
      default:
        work[i - 1] = (i % 2 == 0) ? 1.f / (float)(i - 1) : -1 * work[i - 2];
        break;
      }
    }
    MPI_Send(&work[i - 1], 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
  }

  return size;
}

/**
 * Collect results from the slaves
 *
 * @param size    The number of nodes in the environment
 */
void Master::collectResults(int size)
{
  unsigned count = 0, nSize = 0, stats = 0;
  MPI_Status status;
  vector<pair<int, pair<int, vector<double> > > > fullGrids; // Stores an int with the metadata (i.e. what stats to compute) and a vector of the values
  vector<string> gridNames; // Stores names of full grids with corresponding indices
  int totalNodes = 0;

  MPI_Comm_size(MPI_COMM_WORLD, &totalNodes);

  // NOTE: Currently doing synchronous sends/recv's
  // but can perform asyncrhonously if it is ever helpful
  string resName("results-");
  resName.append(config.getRunName());
  resName.append(".csv");
  ofstream out(resName.c_str());
  for(int i = 1 ; i < size ; ++i) {
    // Protocol step 1:
    // receive how many different parameters this process is giving us
    MPI_Recv(&count, 1, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &status);

    out<< "Varied Run," << ((work[i - 1] < 0) ? "" : "+") << (work[i - 1] * 100) << "%,id," << (i-1) << endl
      << "Parameter,Sum,Mean,Variance,\"Std. Dev.\",Pearson" << endl;

    if(status.MPI_TAG == FATAL_ERROR) {
      cout<< "Fatal error, could not compute results." << endl;
      continue;
    }

    // For each parameter, retrieve the information
    for(unsigned j = 0 ; j < count ; ++j) {
      // Protocol step 2:
      // receive the size of the name of this parameter
      MPI_Recv(&nSize, 1, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &status);

      // Protocol step 3:
      // receive the name from the client
      char* name = new char[nSize];
      MPI_Recv(name, nSize, MPI_BYTE, i, 1, MPI_COMM_WORLD, &status);

      // Protocol step 4:
      // receive the stats vector
      MPI_Recv(&stats, 1, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &status);

      // Protocol step 5:
      // Receive the values
      double val = 0.f;
      out<< name << ",";
      if(stats & Parameter::SUM) {
        MPI_Recv(&val, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &status);
        out<< val << ",";
      } else {
        out<< "NA,";
      }
      if(stats & Parameter::MEAN) {
        MPI_Recv(&val, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &status);
        out<< val << ",";
      } else {
        out<< "NA,";
      }
      if(stats & Parameter::VARIANCE) {
        MPI_Recv(&val, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &status);
        out<< val << ",";
      } else {
        out<< "NA,";
      }
      if(stats & Parameter::STDDEV) {
        MPI_Recv(&val, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &status);
        out<< val << ",";
      } else {
        out<< "NA,";
      }
      if(stats & Parameter::PEARSON) {
        MPI_Recv(&val, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &status);
        out<< val;
      } else {
        out<< "NA";
      }
      out << endl;
      if(stats & Parameter::NORM) {
        unsigned numElems = 0;

        // Receive the number of elements
        MPI_Recv(&numElems, 1, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &status);

        // Receive the values and store them for later
        vector<double> cellValues;
        cellValues.reserve(numElems);

        // Receive the elements
        double* dVals = new double[numElems];
        MPI_Recv(dVals, numElems, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &status);

        // Convert values to vector
        for(unsigned j = 0 ; j < numElems ; ++j) {
          double value = dVals[j];//0.f;
          cellValues.push_back(value);
        }

        delete [] dVals;

        fullGrids.push_back(pair<int, pair<int, vector<double> > >(stats, pair<int, vector<double> >(i-1, cellValues)));
        gridNames.push_back(name);
      }

      delete [] name;
    }

    out << endl << endl;

    cout<< "Process " << i << " complete." << endl;
  }

  // Compute stats on master which require knowledge of full grids (similarity measures)
  if(!fullGrids.empty()) {
    string similName("similarity-");
    similName.append(config.getRunName());
    similName.append(".csv");
    ofstream simil(similName.c_str());

    simil<< "Id,\"Perc Change\"" << endl;
    for(int i = 0 ; i < size - 1 ; ++i) {
      simil<< i << "," << (work[i] * 100) << "%" << endl;
    }
    simil << endl;

    simil<< "Parameter,Id1,Id2,Norm" << endl;

    unsigned i = 0, j = 0;
    vector<pair<int, pair<int, vector<double> > > >::iterator it, itt;
    for(it = fullGrids.begin(), i = 0 ; it != fullGrids.end() ; ++it, ++i) {
      // Compute norm statistics
      if(it->first & Parameter::NORM) {
        // Clear the NORM parameter from the current grid so it's not recomputed
        it->first &= ~Parameter::NORM;
        for(itt = fullGrids.begin(), j = 0 ; itt != fullGrids.end() ; ++itt, ++j) {
          if(itt == it || !(itt->first & Parameter::NORM) || gridNames[i].compare(gridNames[j]) != 0)
            continue; // Skip this computation to avoid re-computation
          // Output the data
          int id1 = it->second.first;// % totalNodes;
          int id2 = itt->second.first;// % totalNodes;
          simil<< gridNames[i] << "," << id1 << "," << id2 << ","
            << AnalyzeData::computeNorm(it->second.second, itt->second.second) << endl;
        }
      }
    }

    simil.close();
  }

  // Report runtime
  out<< endl << "Runtime:," << static_cast<double>((static_cast<double>((clock() - startTime)) / static_cast<double>(CLOCKS_PER_SEC))) << "s" << endl;

  out.close();
}
