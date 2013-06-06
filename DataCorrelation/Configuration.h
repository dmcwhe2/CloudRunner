/**
 * Configuration.h
 *
 * Header for the configuration interface
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef CONFIGURATION_H__
#define CONFIGURATION_H__

#include <fstream>
#include <map>
#include <string>
#include <vector>

#define MAX_VALS 4 // Should directly correspond with the number of elements in the STATS enum

/**
 * Struct for rules
 */
struct Rules
{
  std::string name, format, outformat;
  unsigned start, end, modulo, mval;
  float transperc;
};

/**
 * Struct for parameters
 */
struct Parameter
{
  /**
   * Enum for bit vector (for stat enabling)
   */
  enum STATS
  {
    SUM         = 0x01,
    MEAN        = 0x02,
    VARIANCE    = 0x04,
    STDDEV      = 0x08,
    PEARSON     = 0x10,
    NORM        = 0x20,
    ALL_SIMILAR = 0x40
  };
  std::string name, pearson;
  unsigned stats;
};

/**
 * Struct for graphing data
 */
struct GraphData
{
  GraphData()
    : lowerThresh(0.0), upperThresh(1.0)
  {
  }
  std::string valueToGraph;
  double lowerThresh, upperThresh;
};

typedef std::vector<Parameter> paramset;
typedef std::vector<Rules> ruleset;
typedef std::map<std::string, ruleset > rules_container;

class Configuration
{
public:
  /**
   * Types of work symmetry
   */
  enum Symmetry
  {
    SYMMETRIC,
    POSITIVE,
    NEGATIVE
  };

  /**
   * Constructor
   *
   * @param file    Path to configuration file
   */
  Configuration(const std::string& file);

  /**
   * Destructor
   */
  virtual ~Configuration() {}

  /**
   * Get methods follow to retrieve the corresponding object
   */
  virtual std::string getExecutablePath() const { return exe; }
  virtual std::string getDataDir() const { return datadir; }
  virtual std::string getSimulator() const { return simulator; }
  virtual std::string getRunName() const { return runName; }
  virtual std::vector<std::string> getOutput() const { return output; }
  virtual bool runSim() const { return runSimulation; }
  virtual bool listKeys() const { return listKeyVals; }
  virtual Symmetry getSymmetry() const { return sym; }

  /**
   * Get the loaded ruleset
   *
   * @return  A const reference to the rules
   */
  const rules_container& getRules() const { return rules; }

  /**
   * Get the parameters
   *
   * @return  A const reference to the parameters
   */
  const paramset& getParams() const { return params; }

  /**
   * Get the graph information
   *
   * @return  A const reference to the graph information
   */
  const GraphData& getGraphing() const { return graph; }

  /**
   * Set the list of keys to update params for "all" values
   *
   * @param keys  Total list of keys
   */
  void updateParams(const std::vector<std::string>& keys);

private:
  /**
   * Parse data from input file
   */
  virtual void parse();

  /**
   * Parse the main block
   *
   * @param line  The line which opened the block
   * @return  True if closed, false otherwise
   */
  virtual bool parseMain(const std::string& line);

  /**
   * Parse the rules block
   *
   * @param line  The line which opened the block
   * @return  True if closed, false otherwise
   */
  virtual bool parseRules(const std::string& line);

  /**
   * Parse a file block
   *
   * @param line    The line which opened the block
   * @return  True if closed, false otherwise
   */
  virtual bool parseFile(const std::string& line);

  /**
   * Parse an analysis block
   *
   * @param line    The line which opened the block
   * @return  True if closed, false otherwise
   */
  virtual bool parseAnalysis(const std::string& line);

  /**
   * Parse a parameter block (must be within analysis block)
   *
   * @param line    The line which opened the block
   * @return  True if closed, false otherwise
   */
  virtual bool parseParameter(const std::string& line);

  /**
   * Check if variable line
   *
   * @param line    The (trimmed) line to check
   * @param var     The variable to check
   * @return  True if variable line, otherwise false
   */
  static bool isVarLine(std::string line, std::string var);

  /**
   * Extract a value
   *
   * @param line  The variable line to extract a value from
   * @return  The value from the line
   */
  static std::string extractValue(const std::string& line);

  /**
   * Check if this is a valid opening for a given key
   *
   * @param line  Line to check
   * @param key   The block to open
   * @return  True if valid, false otherwise
   */
  static bool validOpen(const std::string& line, const std::string& key);

  /**
   * Generic code to throw an exception about unexpected
   * line
   *
   * @param err       Error message to report
   * @param lineno    The line number
   */
  static void throwException(std::string err, int lineno);

  /**
   * Parse state enum
   */
  enum PARSESTATE {
    NONE,
    MAIN,
    RULES,
    FILE,
    ANALYSIS,
    PARAMETER
  };

  // Private variables related to state and parsing
  std::string filename;
  int lineno;
  PARSESTATE parserState;

  // Variables read in for use by user application
  /** Main vars */
  std::string exe, datadir, simulator, runName;
  std::vector<std::string> output;
  bool runSimulation, listKeyVals;

  /* Rules/files vars in structure */
  rules_container rules;

  /** Parameters to analyze */
  paramset params;

  /* Graph data */
  GraphData graph;

  /* Symmetry data */
  Symmetry sym;
};

#endif /** CONFIGURATION_H__ */
