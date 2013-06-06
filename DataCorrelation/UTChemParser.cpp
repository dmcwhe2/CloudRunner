/**
 * UTChemParser.cpp
 *
 * UTChem data file parser
 *
 * @author Dennis J. McWherter, Jr.
 */

#define _CRT_SECURE_NO_WARNINGS // Disable MSVC compiler warnings about secure methods

#include <cstring>
#include <iostream> // For debugging

#include "UTChemParser.h"

#define MAX_STRLEN 256

using namespace std;

/**
 * Read file
 *
 * Reads the parser's files
 *
 * @return true if successful, false otherwise
 */
bool UTChemParser::readFile()
{
  vector<string>::const_iterator it;
  bool success = true;

  for(it = files.begin() ; it != files.end() ; ++it) {
    ifstream file(it->c_str());

    if(!file.is_open())
      return false;

    success &= parseValues(file);

    file.close();
  }

  return success;
}

/**
 * Return values for a particular index
 *
 * @param key     The key of the value to be found
 * @param layer   The layer to inspect
 * @return A vector containing the corresponding values
 */
const vector<double>& UTChemParser::getValues(const string& key, int layer) const
{
  const std::vector<std::vector<double> >& container = values.at(key);

  if(container.empty())
    return sentinel;

  int checkLayer = (layer - 1 < 0) ? 0 : layer - 1;
  const std::vector<double>& check = container.at(checkLayer);

  // If cannot find the vector, then return the sentinel
  return (check.empty()) ? sentinel : check;
}

/**
 * Gets all values from a particular key and copies them
 * into a separate vector
 *
 * @param key   The key to find all valid values for
 * @return  A new vector containing all values. If none exist, then
 *          the vector is empty.
 */
std::vector<double> UTChemParser::getAllValues(const std::string& key) const
{
  std::vector<double> ret;
  const std::vector<std::vector<double> >& container = values.at(key);

  if(container.empty())
    return ret;

  for(size_t i = 0 ; i < container.size() ; ++i) {
    const std::vector<double>& vec = container.at(i);
    vector<double>::const_iterator it;
    for(it = vec.begin() ; it != vec.end() ; ++it) {
      ret.push_back(*it);
    }
  }

  return ret;
}

/**
 * Parse values and store them properly in the map/vector
 *
 * @param file    The open file stream to parse data
 * @return  True if successfully parsed, false otherwise.
 */
bool UTChemParser::parseValues(ifstream& file)
{
  string line;
  int currentLayer;
  vector<double> vals;
  char buffer[MAX_STRLEN];
  char bufferTime[MAX_STRLEN];
  double nextVal;
  int advance = 0;
  int phaseno = 0; // Not always applicable
  
  memset(bufferTime, 0, MAX_STRLEN);

  for(int i = 0 ; i < 5 ; ++i)
    getline(file, line); // Skip first 4 lines

  if(3 != sscanf(line.c_str(), " NX = %d NY = %d NZ = %d", &nx, &ny, &layers))
    return false;

  int expected = nx * ny; // Expected number of elements to read before next script line

  getline(file, line);
  
  // Get time data
  if(line.find("TIME") != string::npos) {
    if(timestr == NULL) {
      timestr = new char[MAX_STRLEN];
    }
    if(1 != sscanf(line.c_str(), " TIME = %s", timestr)) {
      return false;
    }
    getline(file, line);
  }

  if(3 != sscanf(line.c_str(), "%[^0-9] %d %*[^0-9] %d", buffer, &phaseno, &currentLayer))
    if(2 != sscanf(line.c_str(), " %[^0-9] %d", buffer, &currentLayer))
      return false; // First time

  // Append time data if it exists
  if(timestr != NULL) {
    memset(bufferTime, 0, MAX_STRLEN);
    strncpy(bufferTime, buffer, strlen(buffer));
    strncat(bufferTime, "-", 1);
    strncat(bufferTime, timestr, MAX_STRLEN - strlen(buffer));
  }

  while(!file.eof()) {
    getline(file, line);

    // Time data
    if(line.find("TIME") != string::npos) {
      if(timestr == NULL) {
        timestr = new char[MAX_STRLEN];
      }
      if(1 != sscanf(line.c_str(), " TIME = %256s", timestr)) {
        return false;
      }
      getline(file, line);
    }
    const char* lptr = line.c_str();

    // Try to match either of the following:
    // (1) X-PERMEABILITY (MD) IN LAYER           12
    // (2) POROSITY IN LAYER           12
    // (3) VISCOSITY (MPA.S) OF PHASE            1  IN LAYER            1
    // (4) TOTAL FLUID CONC. OF COMP. NO.  1:WATER    IN LAYER   1
    // (5) SAT. OF PHASE            1  IN LAYER            1
    // (6) EFFECTIVE SALINITY (MEQ/ML) IN LAYER            1 (refer to other lines in .SALT - should match all)
    if(vals.size() == static_cast<size_t>(expected) && !line.empty()) {
      values[buffer].push_back(vals);
      if(strlen(bufferTime) > 0)
        values[bufferTime].push_back(vals);

      // Take the first word as the key then ignore everything up to the layer number
      if(2 == sscanf(lptr, " %256[^(] %*s IN LAYER %d", buffer, &currentLayer) // (6)
        || sscanf(lptr, " TOTAL FLUID CONC. OF COMP. NO. %*d:%256s IN LAYER %d", buffer, &currentLayer)) { // (4)
        size_t blen = strlen(buffer) - 1;
        while(blen >= 0) { // Trim trailing whitespace
          if(buffer[blen] != ' ' && buffer[blen] != '\t') {
            buffer[blen + 1] = '\0';
            break;
          }
          blen--;
        }
        phaseno = 0;
      } else if(3 != sscanf(lptr, " %256s %*[^0-9] %d %*[^0-9] %d", buffer, &phaseno, &currentLayer)) { // Try line (3), (5)
        if(2 != sscanf(lptr, " %256s %*[^0-9] %d", buffer, &currentLayer)) { // This tries for lines (1), (2)
          return false;
        }
      } else {
        // If we got here phaseno should be part of our buffer
        char intstr[20]; // Sufficiently large for an int? Sorry about the majik number :x
        sprintf(intstr, "_%d", phaseno);
        // Thus, for viscosity, this becomes "VISCOSITY_1" for reference (i.e. viscosity of phase 1)
        strncat(buffer, intstr, MAX_STRLEN - strlen(buffer));
      }

      // Append time data if it exists
      if(timestr != NULL) {
        memset(bufferTime, 0, MAX_STRLEN);
        strncpy(bufferTime, buffer, strlen(buffer));
        strncat(bufferTime, "-", 1);
        strncat(bufferTime, timestr, MAX_STRLEN - strlen(buffer));
      }

      vals.clear();
    } else { // Read values
      while(lptr != NULL && *lptr != '\0' && *lptr != '\n' && *lptr != '\r') {
        while(*lptr == ' ') lptr++; // eat whitespace
        if(1 != sscanf(lptr, "%lg%n", &nextVal, &advance))
          return false;
        lptr += advance;
        vals.push_back(nextVal);
      }
    }
  }

  if(!vals.empty())
    values[buffer].push_back(vals);

  return true;
}

/**
 * Get keys
 *
 * @return  A vector containing all of the keys parsed
 */
vector<string> UTChemParser::getParsedKeys() const
{
  vector<string> ret;

  map<string, vector<vector<double> > >::const_iterator it;

  for(it = values.begin() ; it != values.end() ; ++it) {
    ret.push_back(it->first);
  }

  return ret;
}

/**
 * Checks if two cells are connected by a given component
 *
 * @param key     Key of the vector to check
 * @param sCell   The start cell (index to vector)
 * @param eCell   The end cell (index to vector)
 * @return  True if the cells are connected, false otherwise.
 */
bool UTChemParser::isConnected(const std::string& key, unsigned sCell, unsigned eCell) const
{
  const vector<double>& vals = getAllValues(key);
  if(!isValidResult(vals)) // Not a valid key, so the answer is no - not connected
    return false;
  return true;
}

/**
 * Translate a vector position of a cell into the cell's 3D-coordinate
 *
 * @param id    The id of the cell in the vector
 * @return  The corresponding 3D coordinate to the cell ID
 */
Coord3D UTChemParser::getCoordinate(unsigned id) const
{
  unsigned R = nx;
  unsigned C = ny;

  // Equations (integer division):
  // k = id / RC
  // j = (id - kRC) / R
  // i = id - kRC - jR
  unsigned k = id / (R*C);
  unsigned j = (id - (k * R * C)) / R;
  unsigned i = id - (k * R * C) - (j * R);

  return Coord3D(i, j, k);
}
