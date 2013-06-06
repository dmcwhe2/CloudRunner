/**
 * Master.h
 *
 * The master interface
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef MASTER_H__
#define MASTER_H__

#include <ctime>

class Configuration;

class Master
{
public:
  /**
   * Constructor
   *
   * @param config    Configuration settings to pass to the master
   */
  Master(const Configuration& config);

  /**
   * Destructor
   */
  virtual ~Master();

  /**
   * Start the master
   */
  void run();
private:

  /**
   * Delegate work to the slave nodes
   *
   * @return  The number of nodes in the environment
   */
  int delegateWork();

  /**
   * Collect results from the slaves
   *
   * @param size    The number of nodes in the environment
   */
  void collectResults(int size);

  const Configuration& config;
  double* work;
  clock_t startTime;
};

#endif /** MASTER_H__ */
