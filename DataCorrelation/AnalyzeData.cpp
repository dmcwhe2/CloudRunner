/**
 * AnalyzeData.cpp
 *
 * @author Dennis J. McWherter, Jr.
 */
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include <utility>
#include <vector>

#include "AnalyzeData.h"
#include "DCException.h"
#include "DCUtil.h"
#include "ParserBase.h"

// Use boost lib for GraphML writing
#include <boost/graph/graphml.hpp>

using namespace std;
using namespace boost;

// Use the typedef's used in the example at:
// http://www.boost.org/doc/libs/1_49_0/libs/graph/doc/write_graphml.html
typedef adjacency_list<vecS, vecS, undirectedS,
  property<vertex_color_t, string>,
  property<edge_weight_t, int> > Graph;

/** Static methods */
/**
 * Compute the norm between to vectors (presumably these are grids)
 *
 * NOTE: The vector sizes MUST be the same, or this will fail.
 *
 * @param x   The first set of values
 * @param y   The second set of values
 * @return  The 2-norm between the sets.
 */
double AnalyzeData::computeNorm(const vector<double>& x, const vector<double>& y)
{
  assert(x.size() == y.size()); // grep: Should this be a hard assert or should I simply return NaN?

  double ret = 0.f;
  double tmp = 0.f;
  
  // Norm eq:
  // sqrt(Sum for all i of (x_i - y_i)^2)
  for(unsigned i = 0 ; i < x.size() ; ++i) {
    tmp  = x[i] - y[i];
    tmp *= tmp;
    ret += tmp;
  }

  return sqrt(ret);
}

/** Public methods */

/**
 * Constructor
 *
 * @param parser    Parser object
 */
AnalyzeData::AnalyzeData(ParserBase& parser)
  : data(parser)
{
  if(!data.readFile())
    throw DCException("Could not successfully parse input file!");
}


/**
 * Retrieve simple average
 *
 * @param key   Key to analyze
 * @param n     Sample size (default=0 all values)
 * @return  The mean of the key for the given sample size
 */
double AnalyzeData::mean(const std::string& key, size_t n) const
{
  double ret = 0.f;

  vector<double> vals(data.getAllValues(key));
  size_t max = vals.size();

  n = (n < 1 || n > max) ? max : n;
  
  ret = sum(key, n);

  // Mean is (Sum of all elements) / Number of Elements [or sample of elements summed together]
  ret /= n;

  return ret;
}

/**
 * Retrieve simple variance
 *
 * @param key   Key to analyze
 * @param n     Sample size (default=0 means all values)
 * @return  Return the variance for the provided sample size
 */
double AnalyzeData::variance(const string& key, size_t n) const
{
  double ret = 0;
  double mval = mean(key, n);

  vector<double> vals(data.getAllValues(key));
  size_t max = vals.size();

  n = (n == 0 || n > max) ? max : n;

  // Expected value calculation for (x^2)
  for(size_t i = 0 ; i < n ; ++i)
    ret += (vals[i] * vals[i]);
  ret /= n;

  // At this point ret = E(x^2)
  // variance = E(x^2) - (E(x))^2
  ret -= (mval * mval);

  return ret;
}

/**
 * Retrieve simple standard deviation
 *
 * @param key   Key to analyze
 * @param n     Sample size (default=0 which means all values)
 * @return  The standard deviation of the key for the given sample size
 */
double AnalyzeData::stddev(const string& key, size_t n) const
{
  double ret = 0;
  double var = variance(key, n);

  // stddev = sqrt(variance)
  ret = sqrt(var);

  return ret;
}

/**
 * Compute the sum of a key
 *
 * @param key   Key to compute sum for
 * @param num   How many elements to sum (default=-1 which is all elements)
 * @return  Sum of the values for the key
 */
double AnalyzeData::sum(const string& key, size_t n) const
{
  vector<double> vals(data.getAllValues(key));
  return sum(vals, n);
}

/**
 * Compute Pearson's Correlation Coefficient
 *
 * @param key1   First key to analyze
 * @param key2   Second key to analyze
 * @return  Result of pearon's coefficient between two keys
 */
double AnalyzeData::pearsons(const string& key1, const string& key2) const
{
  vector<double> xv(data.getAllValues(key1)),
    xsq(data.getAllValues(key1)),
    ysq(data.getAllValues(key2));

  vector<double> tmp(data.getAllValues(key2));

  // It is possible to correlate two fields with different sample sizes
  // so let's compare against the smaller of the two.
  size_t n = (xv.size() < tmp.size()) ? xv.size() : tmp.size(); // Sample size

  assert(n != 0);

  vector<double> xy;
  for(size_t i = 0 ; i < n ; ++i)
    xy.push_back(xv[i]); // Get our sample size of x-vals

  // TODO: Should we pick randomly to correlate or just take the first n items of each?
  for(size_t i = 0 ; i < n ; ++i) {
    // Do all calculations - assume all are same size by
    // previous assertion
    xy[i]  *= tmp[i];
    ysq[i] *= ysq[i];
    xsq[i] *= xsq[i];
  }

  // Variables used in calculation
  double sX   = sum(key1, n);
  double sY   = sum(key2, n);
  double sXY  = sum(xy);
  double sXsq = sum(xsq);
  double sYsq = sum(ysq);

  double numer = ((n * sXY) - (sX * sY)); // Numerator
  double denom = sqrt((((n * sXsq) - (sX * sX)) * ((n * sYsq) - (sY * sY)))); // Denominator

  return (numer / denom);
}

/**
 * Compute Spearman's Coefficient
 *
 * @param key1  First key to analyze
 * @param key2  Second key to analyze
 * @return  Resulting Spearman's coefficient
 */
double AnalyzeData::spearmans(const string& key1, const string& key2) const
{
  // TODO: Implement
  return 0.f;
}

/**
 * Filter data between a given inclusive range (i.e. [lower, upper])
 *
 * @param key     Key to filter on
 * @param lower   Lower threshold range
 * @param upper   Upper threshold range
 * @return  A vector containing a pair where each pair contains the value and its real-world coordinate
 */
vector<pair<double, Coord3D> > AnalyzeData::filter(const string& key, double lower, double upper) const
{
  vector<double> vals(data.getAllValues(key));
  vector<pair<double, Coord3D> > ret;

  // Generate our filtered list
  for(unsigned i = 0 ; i < vals.size() ; ++i) {
    if(vals[i] >= lower && vals[i] <= upper) {
      Coord3D coord(data.getCoordinate(i));
      ret.push_back(pair<double, Coord3D>(vals[i], coord));
    }
  }

  return ret;
}

/**
 * Write out the GraphML file for graph connectivity
 *
 * @param key     Property for which to produce a connectivity graph
 * @param lower   Lower filter level
 * @param upper   Upper filter level
 * @param adtl    Optional parameter for specifying an extra identifier onto the filename (i.e. run number)
 * @return  The file name of the resultant GraphML file
 */
string AnalyzeData::getConnectivityGraph(const string& key, double lower, double upper, string addtl) const
{
  string filename(key);
  filename.append("-ConnectivityGraph");
  filename.append(addtl);
  string filename2(filename);
  filename.append(".graphml");
  filename2.append(".vtk");
  unsigned edgeCount = 0;
  unsigned edgeMax   = 10;
  Edge* edges = new Edge[edgeMax];

  // First filter the data
  vector<pair<double, Coord3D> > vals(filter(key, lower, upper));

  // Compute connectivity
  for(unsigned i = 0 ; i < vals.size() ; ++i) {
    const pair<double, Coord3D>& iv = vals[i];
    for(unsigned j = 0 ; j < vals.size() ; ++j) {
      const pair<double, Coord3D>& jv = vals[j];
      if(i != j && iv.second.getDistance(jv.second) == 1) {
        // These nodes are touching (and the nodes are not the same) so add the edge
        edges[edgeCount++] = Edge(i, j);
        // Grow array as necessary
        if(edgeCount >= edgeMax) {
          DCUtil::resizeArray<Edge>(edges, edgeMax, edgeMax*2);
          edgeMax *= 2;
        }
      }
    }
  }

  // Create the graph object
  Graph g(edges, edges + edgeCount, vals.size());

  graph_traits<Graph>::vertex_iterator v, vend;
  for(tuples::tie(v, vend) = vertices(g) ; v != vend ; ++v) {
    put(vertex_color_t(), g, *v, vals[*v].second.toString());
  }

  dynamic_properties dp;
  dp.property("name", get(vertex_color_t(), g));

  // Then write out the graphML file
  ofstream out(filename.c_str());
  write_graphml(out, g, dp, true);
  out.close();

  // Write out legacy VTK file
  ofstream outvtk(filename2.c_str());
  write_vtk(outvtk, edges, edgeCount, vals);
  outvtk.close();

  // Free up that memory
  delete [] edges;

  return filename;
}

/** Private methods */

/**
 * Sum the subset of values in a vector
 *
 * @param vec   The vector to sum up
 * @param n     Number of elements to sum (if n = 0, then sum everything)
 * @return  The sum of the first n elements in vec
 */
double AnalyzeData::sum(const vector<double>& vec, size_t n) const
{
  double ret = 0.f;
  size_t max = vec.size();
  n = (n == 0 || n > max) ? max : n;
  for(size_t i = 0 ; i < n ; ++i)
    ret += vec[i];
  return ret;
}

/**
 * Write a VTK legacy file of a connected graph
 *
 * @param out         The open output file to write to
 * @param edges       The list of edges
 * @param edgeCount   Number of edges
 * @param nodes       The nodes and their coordinates
 */
void AnalyzeData::write_vtk(ostream& out, Edge* edges, unsigned edgeCount, const vector<pair<double, Coord3D> >& nodes) const
{
  if(edges == NULL)
    return; // This must have been an error?

  // Write out basic header information
  // Format described here: http://www.vtk.org/VTK/img/file-formats.pdf
  out<< "# vtk DataFile Version 2.0\n"
     << "Connectivity graph generated by DataCorrelation Tool\n"
     << "ASCII\n"
     << "DATASET POLYDATA\n"
     << "POINTS " << nodes.size() << " double\n";

  // Write out the "points" (nodes)
  for(unsigned i = 0 ; i < nodes.size() ; ++i) {
    const Coord3D& coord = nodes[i].second;
    out << coord.getX() << " " << coord.getY() << " " << coord.getZ() << "\n";
  }

  // Write out the lines (edges)
  out<< "LINES " << edgeCount << " " << (edgeCount * 3) << "\n";
  for(unsigned i = 0 ; i < edgeCount ; ++i) {
    out<< "2 " << edges[i].first << " " << edges[i].second << "\n";
  }
}
