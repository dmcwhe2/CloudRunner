/**
 * Slave.h
 *
 * MPI slave interface
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef SLAVE_H__
#define SLAVE_H__

#include <string>
#include <utility>
#include <vector>

#include "Configuration.h"

class Slave
{
public:
  /**
   * Constructor
   *
   * @param config    The configuration settings passed to the slave
   */
  Slave(Configuration& config);

  /**
   * Destructor
   */
  virtual ~Slave();

  void run();
private:
  /**
   * Data types to read in while processing user input lines
   */
  enum DATA_TYPE
  {
    FLOAT      = 0x01,
    INT        = 0x02,
    STRING_LIT = 0x03
  };

  // Typedef our arbitrary value container
  typedef std::vector<std::pair<Slave::DATA_TYPE, void*> > arbval_cont_t;

  /**
   * Run the simulation
   *
   * @return  The the paths to the output files to be analyzed
   */
  std::vector<std::string> runSimulation();

  /**
   * Copy the simulation directory
   *
   * @return  The path of the duplicated directory
   */
  std::string copySimDir();

  /**
   * Calculate and transmit the results back to the master
   *
   * @param files    Files to open and use for calculations
   */
  void calculateAndTxResults(const std::vector<std::string>& files);

  /**
   * Update the input file according to rules
   * for changing the simulation
   *
   * @param rules   Rules to use when updating the input
   * @param path    Path to the input file
   * @return  True on success, false otherwise
   */
  bool updateInputFile(const ruleset& rules, const std::string& path);

  /**
   * Process a rule and alter the line accordingly
   *
   * @param r         The rule to use in alteration
   * @param newFile   A vector containing altered lines (will be updated over each call)
   * @param line      The altered line
   */
  void processRule(const Rules& r, std::vector<std::string>& newFile, std::string& line);

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
  void preprocessInput(const Rules& r, const std::string& line, std::vector<std::string>& tokens, arbval_cont_t& newValues, int*& intVals, float*& floatVals);

  /** Variables */
  Configuration& config;
  double work;
};

#endif /** SLAVE_H__ */
