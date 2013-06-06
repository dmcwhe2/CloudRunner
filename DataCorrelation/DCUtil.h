/**
 * DCUtil.h
 *
 * General utility methods
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef DCUTIL_H__
#define DCUTIL_H__

#include <sstream>
#include <string>
#include <vector>

#define DEBUG // Comment this line to disable debug messages

#ifdef DEBUG
#define debugMacro(x) std::cout<< "***DEBUG*** (" << __FILE__ << ":" << __LINE__ << ") " << x << endl;
#else
#define debugMacro(x) // Do nothing
#endif

// getcwd() WIN32 hack
#ifdef USE_GETCWD
#ifdef _WIN32
#include <direct.h> // _getcwd()
#define getcwd(x, y) _getcwd(x, y)
#else
#include <cstdlib>
#include <unistd.h>
#endif
#endif

class DCUtil
{
public:
  /**
   * Trims the leading whitespace
   *
   * @param str   String to trim (will modify)
   */
  static void trim(std::string& str);

  /**
   * Get a system variable
   *
   * @param var   Variable to retrieve
   * @return  The environment variable value
   */
  static std::string getEnvVar(const std::string& var);

  /**
   * Convert an integer type to a string
   *
   * @param val   Value to convert
   * @return  The value in string form
   */
  template<class T, class R>
  static R XToY(const T& val)
  {
    R ret;
    std::stringstream ss;
    ss << val;
    ss >> ret;
    return ret;
  }

  /**
   * Resize an array to the new size
   *
   * NOTE: All contents will be copied over to the new array if being grown,
   *      However, when shrinking, the first 0,...,nlen-1 items are inserted only.
   *      This also assumes that the array element types have proper assignment operators
   *
   * @param arr     Array to resize
   * @param orig    Original length
   * @param nlen    New length
   */
  template<class T>
  static void resizeArray(T*& arr, unsigned orig, unsigned nlen)
  {
    T* ret = new T[nlen];
    unsigned limit = (orig > nlen) ? nlen : orig;
    for(unsigned i = 0 ; i < limit ; ++i)
      ret[i] = arr[i];
    delete [] arr;
    arr = ret;
  }

  /**
   * Convert a string to an integer
   *
   * @param str   String to convert
   * @return  The integer value of the string
   */
  static int strToInt(const std::string& str);

  /**
   * Convert a string to all upper-case
   *
   * @param str   String to transform
   */
  static void strToUpper(std::string& str);

  /**
   * A function to check whether a line starts with a given key 
   *  (case insensitive)
   *
   * @param line    Line to check
   * @param key     Key to verify the line begins with
   * @return  True if the line starts with key, false otherwise
   */
  static bool startsWith(std::string line, std::string key);

  /**
   * Tokenize a string
   *
   * @param str   String to tokenize
   * @param delim   Delimiter to split on
   * @return  A vector containing the tokenized string
   */
  static std::vector<std::string> tokenize(const std::string& str, const char delim);

  /**
   * Copy a file
   *
   * @param src   Source file to copy
   * @param dest  Destination file (can be relative or full path)
   * @return  True if file copied successfully, false otherwise.
   */
  static bool copyFile(const std::string& src, const std::string& dest);

  /**
   * Copy an entire directory
   *
   * @param src   Source directory
   * @param dest  Destination directory
   * @param rec   If true, then recursive copy (true by default)
   * @return  True if copy successful, false otherwise.
   */
  static bool copyDir(const std::string& src, const std::string& dest, bool rec=true);

  /**
   * List all files in a directory (with full paths)
   *
   * @param dir   Directory to list files in
   * @return  A vector containing all the valid files within the directory
   */
  static std::vector<std::string> listFiles(std::string dir);

  /**
   * Check if a file is a directory or not
   *
   * @param file    Directory to check
   * @return  True if directory, false otherwise
   */
  static bool isDirectory(const std::string& file);

private:
  // Simple container so we don't have random methods floating.
  // Just a C++ thing.
  DCUtil(){}
  virtual ~DCUtil(){}
};

#endif /** DCUTIL_H__ */
