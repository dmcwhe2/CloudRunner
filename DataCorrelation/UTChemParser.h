/**
 * UTChemParser.h
 *
 * Parser for UTChem files
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef UTCHEMPARSER_H__
#define UTCHEMPARSER_H__

#include "ParserBase.h"

#include <fstream>
#include <map>

// NOTE: Currently only supporting .PERM files
//      this covers both permeability and porosity data
class UTChemParser : public ParserBase
{
public:
  /**
   * Constructor
   *
   * @param files  List of files to parse
   */
  UTChemParser(const std::vector<std::string>& files)
    : ParserBase(files), nx(1), ny(1), layers(1), timestr(NULL)
  {
  }

  /**
   * Destructor
   */
  virtual ~UTChemParser(){ delete [] timestr; }

  /**
   * Read file
   *
   * Reads the parser's file
   *
   * @return true if successful, false otherwise
   */
  virtual bool readFile();

  /**
   * Return values for a particular index
   *
   * @param key     The key of the value to be found
   * @param layer   The layer to inspect
   * @return A vector containing the corresponding values
   */
  virtual const std::vector<double>& getValues(const std::string& key, int layer) const;

  /**
   * Gets all values from a particular key and copies them
   * into a separate vector
   *
   * @param key   The key to find all valid values for
   * @return  A new vector containing all values. If none exist, then
   *          the vector is empty.
   */
  virtual std::vector<double> getAllValues(const std::string& key) const;

  /**
   * Get keys
   *
   * @return  A vector containing all of the keys parsed
   */
  virtual std::vector<std::string> getParsedKeys() const;

  /**
   * Checks if two cells are connected by a given component
   *
   * @param key     Key of the vector to check
   * @param sCell   The start cell (index to vector)
   * @param eCell   The end cell (index to vector)
   * @return  True if the cells are connected, false otherwise.
   */
  virtual bool isConnected(const std::string& key, unsigned sCell, unsigned eCell) const;

  /**
   * Translate a vector position of a cell into the cell's 3D-coordinate
   *
   * @param id    The id of the cell in the vector
   * @return  The corresponding 3D coordinate to the cell ID
   */
  virtual Coord3D getCoordinate(unsigned id) const;

private:

  /**
   * Parse values and store them properly in the map/vector
   *
   * @param file    The open file stream to parse data
   * @return  True if successfully parsed, false otherwise.
   */
  virtual bool parseValues(std::ifstream& file);

  // Map accessed as follows:
  // [property_name][layer-1][value]
  std::map<std::string, std::vector<std::vector<double> > > values;
  unsigned nx, ny, layers;

  // If we have time data
  char* timestr;
};

#endif /** UTCHEMPARSER_H__ */
