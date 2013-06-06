/**
 * ParserBase.h
 *
 * Base class for parser
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef PARSER_BASE_H__
#define PARSER_BASE_H__

#include <string>
#include <vector>

#include "Coord3D.h"

class ParserBase
{
public:
  /**
   * Constructor
   */
  ParserBase(const std::vector<std::string>& files)
    : files(files)
  {
  }

  /**
   * Destructor
   */
  virtual ~ParserBase(){}

  /**
   * Read file
   *
   * Reads the parser's file
   *
   * @return true if successful, false otherwise
   */
  virtual bool readFile() = 0;

  /**
   * Return values for a particular index
   *
   * @param key     The key of the value to be found
   * @param adtl    Additional information to identify the key
   * @return A vector containing the corresponding values
   */
  virtual const std::vector<double>& getValues(const std::string& key, int adtl) const = 0;

  /**
   * Gets all values from a particular key and copies them
   * into a separate vector
   *
   * @param key   The key to find all valid values for
   * @return  A new vector containing all values. If none exist, then
   *          the vector is empty.
   */
  virtual std::vector<double> getAllValues(const std::string& key) const = 0;

  /**
   * Get keys
   *
   * @return  A vector containing all of the keys parsed
   */
  virtual std::vector<std::string> getParsedKeys() const = 0;

  /**
   * Checks if two cells are connected by a given component
   *
   * @param key     Key of the vector to check
   * @param sCell   The start cell (index to vector)
   * @param eCell   The end cell (index to vector)
   * @return  True if the cells are connected, false otherwise.
   */
  virtual bool isConnected(const std::string& key, unsigned sCell, unsigned eCell) const = 0;

  /**
   * Translate a vector position of a cell into the cell's 3D-coordinate
   *
   * @param id    The id of the cell in the vector
   * @return  The corresponding 3D coordinate to the cell ID
   */
  virtual Coord3D getCoordinate(unsigned id) const = 0;

  /**
   * Check if result is valid
   *
   * @param val   The vector to check for validity
   * @return  True if valid, false otherwise (i.e. if val == sentinel).
   */
  virtual bool isValidResult(const std::vector<double>& val) const;

protected:
  // Sentinel node (equivalent to NULL if no vector exists)
  const std::vector<double> sentinel;
  std::vector<std::string> files;
};

#endif /** PARSER_BASE_H__ */
