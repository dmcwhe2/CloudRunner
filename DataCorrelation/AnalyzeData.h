/**
 * AnalyzeData.h
 *
 * Analyze data
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef ANALYZEDATA_H__
#define ANALYZEDATA_H__

#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "Coord3D.h"

class ParserBase;

typedef std::pair<int, int> Edge;

class AnalyzeData
{
public: /** Static members */
  /**
   * Compute the norm between to vectors (presumably these are grids)
   *
   * NOTE: The vector sizes MUST be the same, or this will fail.
   *
   * @param x   The first set of values
   * @param y   The second set of values
   * @return  The 2-norm between the sets
   */
  static double computeNorm(const std::vector<double>& x, const std::vector<double>& y);

public: /** Public members */
  /**
   * Constructor
   *
   * @param parser    Parser object
   */
  AnalyzeData(ParserBase& parser);

  /**
   * Destructor
   */
  virtual ~AnalyzeData(){}

  /**
   * Retrieve simple average
   *
   * @param key   Key to analyze
   * @param n     Sample size (default=0 all values)
   * @return  The mean of the key for the given sample size
   */
  virtual double mean(const std::string& key, size_t n=0) const;

  /**
   * Retrieve simple variance
   *
   * @param key   Key to analyze
   * @param n     Sample size (default=0 means all values)
   * @return  Return the variance for the provided sample size
   */
  virtual double variance(const std::string& key, size_t n=0) const;

  /**
   * Retrieve simple standard deviation
   *
   * @param key   Key to analyze
   * @param n     Sample size (default=0 which means all values)
   * @return  The standard deviation of the key for the given sample size
   */
  virtual double stddev(const std::string& key, size_t n=0) const;

  /**
   * Compute the sum of a key
   *
   * @param key   Key to compute sum for
   * @param n     How many elements to sum (default=-1 which is all elements)
   * @return  Sum of the values for the key
   */
  virtual double sum(const std::string& key, size_t n=0) const;

  /**
   * Compute Pearson's Correlation Coefficient
   *
   * @param key1   First key to analyze
   * @param key2   Second key to analyze
   * @return  Result of pearon's coefficient between two keys
   */
  virtual double pearsons(const std::string& key1, const std::string& key2) const;

  /**
   * Compute Spearman's Coefficient
   *
   * @param key1  First key to analyze
   * @param key2  Second key to analyze
   * @return  Resulting Spearman's coefficient
   */
  virtual double spearmans(const std::string& key1, const std::string& key2) const;

  /**
   * Filter data between a given inclusive range (i.e. [lower, upper])
   *
   * @parma key     Key to filter on
   * @param lower   Lower threshold range
   * @param upper   Upper threshold range
   * @return  A vector containing a pair where each pair contains the value and its real-world coordinate
   */
  virtual std::vector<std::pair<double, Coord3D> > filter(const std::string& key, double lower, double upper) const;

  /**
   * Write out the GraphML file for graph connectivity
   *
   * @param key   Property for which to produce a connectivity graph
   * @param lower   Lower filter level
   * @param upper   Upper filter level
   * @param adtl    Optional parameter for specifying an extra identifier onto the filename (i.e. run number)
   * @return  The file name of the resultant GraphML file
   */
  virtual std::string getConnectivityGraph(const std::string& key, double lower=0.0, double upper=1.0, std::string addtl="") const;

private:
  /**
   * Sum the subset of values in a vector
   *
   * @param vec   The vector to sum up
   * @param n     Number of elements to sum (if n = 0, then sum everything)
   * @return  The sum of the first n elements in vec
   */
  virtual double sum(const std::vector<double>& vec, size_t n=0) const;

  /**
   * Write a VTK legacy file of a connected graph
   *
   * @param out     The open output file to write to
   * @param edges   The list of edges
   * @param edgeCount   Number of edges
   * @param nodes   The nodes and their coordinates
   */
  virtual void write_vtk(std::ostream& out, Edge* edges, unsigned edgeCount, const std::vector<std::pair<double, Coord3D> >& nodes) const;

  ParserBase& data;
};

#endif /** ANALYZEDATA_H__ */
