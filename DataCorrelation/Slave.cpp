/**
 * Slave.cpp
 *
 * MPI slave implementation
 *
 * @author Dennis J. McWherter, Jr.
 */
#define _CRT_SECURE_NO_WARNINGS // Disable MSVC compiler warnings about secure methods
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

#define USE_GETCWD // To include proper files from DCUtil
#include "AnalyzeData.h"
#include "Configuration.h"
#include "DCUtil.h"
#include "Slave.h"
#include "Status.h"
#include "UTChemParser.h"

#include "mpi.h"

using namespace std;

/**
 * Constructor
 *
 * @param config    The configuration settings passed to the slave
 */
Slave::Slave(Configuration& config)
  : config(config)
{
}

/**
 * Destructor
 */
Slave::~Slave()
{
}

/**
 * Run the simulation
 *
 * @return  The path to the output file to be analyzed
 */
void Slave::run()
{
  vector<string> filepath;
  MPI_Status status;

  debugMacro("Started slave");

  // Receive information from master on how to proceed
  MPI_Recv(&work, 1, MPI_DOUBLE, MASTER, 0, MPI_COMM_WORLD, &status);

  debugMacro("Received work");

  filepath = runSimulation();

  calculateAndTxResults(filepath);
  
}

/**
 * Run the simulation
 *
 * @return  The the paths to the output files to be analyzed
 */
vector<string> Slave::runSimulation()
{
  string cmd, temppath, filepath;
  vector<string> files(config.getOutput());

  // Make a copy of the simulation directory
  temppath = copySimDir();

  cout<< "Successfully copied sim directory!" << endl;

  // NOTE: This assumes the output file is generated in this directory!
  // can do a "cd" or otherwise to the output directory if necessary
  filepath = temppath;
  filepath += '/';

  // Prepend the paths to the file names
  for(unsigned i = 0 ; i < files.size() ; ++i) {
    string f(filepath);
    f.append(files[i]);
    files[i] = f;
  }

  // If we are not supposed to run the simulation, then
  // continue and just return the files
  if(!config.runSim())
    return files;

  // Update the input files according to work/rules
  const rules_container& rules = config.getRules();
  rules_container::const_iterator it;
  for(it = rules.begin() ; it != rules.end() ; ++it) {
    if(!updateInputFile(it->second, temppath)) {
      cout<< temppath << endl;
      cerr<< "Could not update input file: " << endl;
      return vector<string>();
    }
  }

  // TODO: In configuration, configure how to run the simulator for things other
  // than UTChem.
  // For UTChem, use the following steps:
  //   copy
  //   cd Simulation_<work>
  //   Run <exe>
#ifdef _WIN32 // Windows path
  cmd.append("cd .\\");
#else // Use linux path 
  cmd.append("cd ./");
#endif
  cmd.append(temppath);

#ifdef __linux__ // make sure simulator is executable
  cmd.append(" && chmod 0555 ");
  cmd.append(config.getExecutablePath());
#endif

  cmd.append(" && ");
  cmd.append(config.getExecutablePath());

  debugMacro("Running the following command for setup: " << cmd);
  debugMacro("Current working directory: "<< getcwd(NULL, 0));

  // Make sure we are actually running on different nodes
  // Uncomment this line and different node names should print
  //system("echo %computername%");

  // Execute the above command
  system(cmd.c_str());

  return files;
}

/**
 * Calculate and transmit the results back to the master
 *
 * @param files    Files to open and use for calculations
 */
void Slave::calculateAndTxResults(const vector<string>& files)
{
  for(unsigned i = 0 ; i < files.size() ; ++i) {
    string filepath(files[i]);
    debugMacro("Reading: "<< filepath);
  }

  UTChemParser p(files);

  try {
    AnalyzeData d(p);

    // List parsed keys
    config.updateParams(p.getParsedKeys()); // Need these keys for any "all" values 
    if(config.listKeys()) {
      cout<< "Parsed keys: " << endl << "=====================" << endl;
      vector<string> keys(p.getParsedKeys());
      vector<string>::const_iterator keyIt;
      for(keyIt = keys.begin() ; keyIt != keys.end() ; ++keyIt) {
        cout<< *keyIt << endl;
      }
    }

    // Sample graphing with filtering
    // This is still done in a primitive way - the idea is to make this more
    // flexible/extensible later if time permits
    const GraphData& g = config.getGraphing();
    if(!g.valueToGraph.empty()) {
      string addtl("-");
      addtl.append(DCUtil::XToY<double, string>(work));
      debugMacro("Graphing: " << g.valueToGraph << " : " << g.lowerThresh << " : " << g.upperThresh);
      debugMacro(d.getConnectivityGraph(g.valueToGraph, g.lowerThresh, g.upperThresh, addtl));
    }

    const paramset& params = config.getParams();
    paramset::const_iterator it;

    // Protocol step 1:
    // send to master how many parameters will be sent over
    size_t val = params.size();
    MPI_Send(&val, 1, MPI_UNSIGNED, MASTER, 1, MPI_COMM_WORLD);
    
    // Now send each parameter
    for(it = params.begin() ; it != params.end() ; ++it) {
      // Make appropriate copies of the data to use with MPI_Send since
      // it does not take "const" args
      size_t nSize = it->name.size();
      unsigned stats = it->stats;
      char* name = new char[nSize + 1];
      strncpy(name, it->name.c_str(), nSize);
      name[nSize] = '\0'; // Make sure we terminate the string
      nSize += 1; // Make sure we send the null byte

      // Protocol step 2:
      // send to master the length of the name of this parameter
      MPI_Send(&nSize, 1, MPI_UNSIGNED, MASTER, 1, MPI_COMM_WORLD);

      // Protocol step 3:
      // send to master the name of this parameter
      MPI_Send(name, static_cast<int>(nSize), MPI_BYTE, MASTER, 1, MPI_COMM_WORLD);

      // Protocol step 4:
      // send to master the stats vector for this parameter
      //    NOTE: These parameters MUST be sent in the order from low
      //          to high as listed by bit position in bit-vector
      MPI_Send(&stats, 1, MPI_UNSIGNED, MASTER, 1, MPI_COMM_WORLD);

      // Protocol step 5:
      // send to master the result of the parameter calculations
      double calcResult = 0.f;
      // Count along the way, calculate, and send in order.
      if(stats & Parameter::SUM) {
        calcResult = d.sum(it->name);
        MPI_Send(&calcResult, 1, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD);
      }
      if(stats & Parameter::MEAN) {
        calcResult = d.mean(it->name);
        MPI_Send(&calcResult, 1, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD);
      }
      if(stats & Parameter::VARIANCE) {
        calcResult = d.variance(it->name);
        MPI_Send(&calcResult, 1, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD);
      }
      if(stats & Parameter::STDDEV) {
        calcResult = d.stddev(it->name);
        MPI_Send(&calcResult, 1, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD);
      }
      if(stats & Parameter::PEARSON) {
        calcResult = d.pearsons(it->name, it->pearson);
        MPI_Send(&calcResult, 1, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD);
      }
      if(stats & Parameter::NORM) {
        // Send the number of elements
        vector<double> gridVals(p.getAllValues(name));
        unsigned numVals = static_cast<unsigned>(gridVals.size());

        // Send the number of elements to receive
        MPI_Send(&numVals, 1, MPI_UNSIGNED, MASTER, 1, MPI_COMM_WORLD);

        double* dVals = new double[numVals];
        // Send each value
        for(unsigned i = 0 ; i < numVals ; ++i) {
          //MPI_Send(&gridVals[i], 1, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD);
          dVals[i] = gridVals[i];
        }

        MPI_Send(dVals, numVals, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD);

        delete [] dVals;
      }

      delete [] name;
    }
  } catch(exception& e) {
    cerr<< "Exception caught: "<< e.what() << endl;
    MPI_Abort(MPI_COMM_WORLD, FATAL_ERROR);
  }
}

/**
 * Copy the simulation directory
 *
 * @return  The path of the duplicated directory
 */
string Slave::copySimDir()
{
  string cmd, temppath;
  int rank;
  char workstr[32];

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Temporary directory to copy files to
  sprintf(workstr, "%f", work);
  temppath.append("Simulation_");
  temppath.append(workstr);
  temppath.append("-");
  temppath.append(DCUtil::XToY<int, string>(rank));

#if 0 // Old windows-specific copy method
  // Use Windows-specific methods since this is made for Windows Azure
  // xcopy /i /e /y /q <sim_path> Simulation_<work> (copy directory, even if empty, and overwrite existing in quiet mode)
  cmd.append("xcopy /i /e /y /q \"");
  cout<< config.getDataDir() << endl;
  cmd.append(config.getDataDir());
  cmd.append("\" .\\");
  cmd.append(temppath);

  // Execute
  system(cmd.c_str());
#endif

  // If we are not running the simulation, don't copy over the
  // directory - assume the simulation has already been done
  // and we are just going to be doing the analysis
  if(config.runSim()) {
    if(!DCUtil::copyDir(config.getDataDir(), temppath)) {
      cerr<< "Problem copying directory: " << config.getDataDir() << " to "<< temppath << endl;
    }
  }

  return temppath;
}

/**
 * Update the input file according to rules
 * for changing the simulation
 *
 * @param rules   Rules to use when updating the input
 * @param path    Path to the input file
 * @return  True on success, false otherwise
 */
bool Slave::updateInputFile(const ruleset& rules, const string& path)
{
  string line, filename(path);
  vector<string> newFile; // This is the wasteful way to alter the lines
                          // Perhaps a continous write to disk would be better
                          // if the file does not fit in memory
                          // Another alternative is to have a buffer with a max size
                          // when max size is buffer, write out to disk and continue

  if(work == 0.f)
    return true; // No need to do work if we are an unaltered version

  ruleset::const_iterator it = rules.begin();
  if(it == rules.end())
    return false;

  filename += '/';
  filename.append(it->name);

  ifstream file(filename.c_str());

  if(!file.is_open())
    return false;

  unsigned i = 1;
  unsigned modulo = it->modulo;

  // Go through and update the file
  while(!file.eof()) {
    getline(file, line);

    // Try to find what to do
    unsigned mval = i % modulo;
    for(it = rules.begin() ; it != rules.end() ; ++it) {
      if(i >= it->start && i <= it->end && it->mval == mval) {
        break;
      }
    }
    const Rules& r = *it;

    // Check to see if we have any rule for this modulo
    // and whether or not the line is within range (only if we found a valid rule)
    if(it != rules.end() && i >= r.start && i <= r.end && r.mval == mval) {
      processRule(r, newFile, line);
    } else { // Otherwise, simply put the old line back
      newFile.push_back(line);
    }
    newFile[i - 1] += '\n'; // New line for every line
    i++;
  }

  file.close();

  // Write back out to the file
  ofstream outFile(filename.c_str(), ios::trunc);
  for(unsigned i = 0 ; i < newFile.size() ; ++i)
    outFile << newFile[i];
  outFile.close();

  return true;
}

/**
 * Process a rule and alter the line accordingly
 *
 * @param r         The rule to use in alteration
 * @param newFile   A vector containing altered lines (will be updated over each call)
 * @param line      The altered line
 */
void Slave::processRule(const Rules& r, vector<string>& newFile, string& line)
{
  char input[1024]; // May need to update this (i.e. use dynamic memory/resizing)
  int* intVals = NULL;
  float* floatVals = NULL;
  vector<string> tokens(DCUtil::tokenize(r.format, ' '));
  vector<string> outtoks(DCUtil::tokenize(r.outformat, ' '));
  arbval_cont_t newValues;
  string newLine;

  // Count the number/type of tokens we should be parsing
  preprocessInput(r, line, tokens, newValues, intVals, floatVals);

  assert(newValues.size() == tokens.size() && newValues.size() <= outtoks.size());

  for(unsigned j = 0 ; j < newValues.size() ; ++j) {
    newLine.append(" ");
    DATA_TYPE type = newValues[j].first;
    void* data = newValues[j].second;
    //debugMacro(((type == FLOAT) ? *((float*)data) : *((int*)data)) << " << value");
    switch(type) {
    case FLOAT:
      sprintf(input, outtoks[j].c_str(), *((float*)data));
      break;
    case INT:
      sprintf(input, outtoks[j].c_str(), *((int*)data));
      break;
    case STRING_LIT: // The token should be the string literal
    default:
      sprintf(input, outtoks[j].c_str());
      break;
    }
    newLine.append(input);
    line.append(newLine);
  }
  newFile.push_back(newLine);

  delete [] intVals;
  delete [] floatVals;
}

/**
 * Preprocess user input for alteration
 *
 * @param r           The rules to follow
 * @param line        The line to process
 * @param tokens      The tokens for input alteration
 * @param newVals     A vector containing the new values to replace in the input line
 * @param intVals     An (unallocated) pointer containing integer alteration data
 * @param floatVals   An (unallocated) pointer containing float alteration data
 *
 * NOTE: The caller is responsible for clearing the memory (if allocated) for any
 *       of the value arrays
 */
void Slave::preprocessInput(const Rules& r, const string& line, vector<string>& tokens, arbval_cont_t& newValues, int*& intVals, float*& floatVals)
{
  int read = 0;
  int floatCount = 0;
  int intCount = 0;
  int floatMax = 0;
  int intMax = 0;

  // Count the token types
  for(unsigned j = 0 ; j < tokens.size() ; ++j) {
    if(DCUtil::startsWith(tokens[j], "%f"))
      floatMax++;
    else if(DCUtil::startsWith(tokens[j], "%d"))
      intMax++;
  }

  // NOTE: This data must be freed by the caller
  if(intMax > 0)
    intVals = new int[intMax];
  if(floatMax > 0)
    floatVals = new float[floatMax];

  // NOTE: Need to find a test where we vary something other than floats/ints
  //    since it is conceivable that this current method will not work for those
  //    cases
  for(unsigned j = 0 ; j < tokens.size() ; ++j) {
    int in = 0;
    void* val = NULL;
    DATA_TYPE type;
    tokens[j].append("%n");
    if(DCUtil::startsWith(tokens[j], "%f")) { // float (%f)
      type = FLOAT;
      sscanf(line.c_str() + read, tokens[j].c_str(), &floatVals[floatCount], &in);
      floatVals[floatCount] += static_cast<float>(((r.transperc * .01 * floatVals[floatCount]) * work));
      val = &floatVals[floatCount];
      floatCount++;
    } else if(DCUtil::startsWith(tokens[j], "%d")) { // int (%d)
      type = INT;
      sscanf(line.c_str() + read, tokens[j].c_str(), &intVals[intCount], &in);
      intVals[intCount] += static_cast<int>(((r.transperc * .01 * static_cast<float>(intVals[intCount])) * work));
      val = &intVals[intCount];
      intCount++;
    } else { // Assume this is a string literal
      type = STRING_LIT;
      val = NULL; // For good measure
    }
    read += in;
    newValues.push_back(pair<DATA_TYPE, void*>(type, val));
  }
}
