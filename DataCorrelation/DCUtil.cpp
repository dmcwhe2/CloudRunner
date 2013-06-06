/**
 * DCUtil.cpp
 *
 * General utility function implementations
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#else // This is for opendir() and readdir() in *nix
#include <dirent.h>
#include <sys/stat.h>
#endif
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>

#include "DCUtil.h"

#define MAX_BUFFER (2048) // This is maximum size before writing to memory

// Make WIN32 _popen and _mkdir compatible with *nix popen and mkdir
#ifdef _WIN32
#define popen(x, y) _popen(x, y)
#define pclose(x) _pclose(x)
#define mkdir(x, y) _mkdir(x)
#endif

using namespace std;

/**
 * Trims the leading whitespace
 *
 * @param str   String to trim (will modify)
 */
void DCUtil::trim(string& str)
{
  string::iterator it;
  for(it = str.begin() ; it != str.end() ;) {
    if(*it == ' ' || *it == '\r' || *it == '\t' || *it == '\n' || *it == '\r')
      str.erase(it);
    else
      break;
  }
}

/**
 * Get a system variable
 *
 * @param var   Variable to retrieve
 * @return  The environment variable value
 */
string DCUtil::getEnvVar(const string& var)
{
  string cmd("echo "), result;
  cmd.append(var);
  char buffer[128];
  char* tmp = NULL;
  FILE* pipe = popen(cmd.c_str(), "r");

  if(pipe == NULL)
    return "ERROR";

  // Read the result
  while(!feof(pipe)) {
    if(fgets(buffer, sizeof(buffer), pipe) != NULL) {
      tmp = buffer;
      // New lines terminate strings for our purposes
      while(*tmp != '\0') {
        if(*tmp == '\n' || *tmp == '\r') {
          *tmp = '\0';
          break;
        }
        tmp++;
      }
      result.append(buffer);
    }
  }

  pclose(pipe);

  return result;
}

/**
 * Convert a string to all upper-case
 *
 * @param str   String to transform
 */
void DCUtil::strToUpper(string& str)
{
  for(unsigned i = 0 ; i < str.size() ; ++i) {
    if(str[i] >= 97 && str[i] <= 122)
      str[i] -= 32;
  }
}

/**
 * A function to check whether a line starts with a given key 
 *  (case insensitive)
 *
 * @param line    Line to check
 * @param key     Key to verify the line begins with
 * @return  True if the line starts with key, false otherwise
 */
bool DCUtil::startsWith(string line, string key)
 {
   if(line.size() < key.size())
     return false;

   DCUtil::trim(line);
   DCUtil::trim(key);

   DCUtil::strToUpper(line);
   DCUtil::strToUpper(key);

   return line.compare(0, key.size(), key) == 0;
 }

/**
 * Tokenize a string
 *
 * @param str     String to tokenize
 * @param delim   Delimiter to split on
 * @return  A vector containing the tokenized string
 */
vector<string> DCUtil::tokenize(const string& str, const char delim)
{
  vector<string> tokenized;
  stringstream ss(str);
  string line;

  while(getline(ss, line, delim))
    tokenized.push_back(line);

  return tokenized;
}

/**
 * Copy a file
 *
 * NOTE: Destination directory must already exist before
 *       file is created.
 *
 * @param src   Source file to copy
 * @param dest  Destination file (can be relative or full path)
 * @return  True if file copied successfully, false otherwise.
 */
bool DCUtil::copyFile(const string& src, const string& dest)
{
  char buffer[MAX_BUFFER];
  ifstream in(src.c_str(), ios::in | ios::binary);
  ofstream out(dest.c_str(), ios::out | ios::trunc | ios::binary);

  if(!in.is_open() || !out.is_open()) {
    in.close();
    out.close();
    return false;
  }

  while(!in.eof()) {
    in.read(buffer, MAX_BUFFER);
    streamsize bytesRead = in.gcount();
    out.write(buffer, bytesRead);
  }

  in.close();
  out.close();

  return true;
}

/**
 * Copy an entire directory
 *
 * @param src   Source directory
 * @param dest  Destination directory
 * @param rec   If true, then recursive copy (true by default)
 * @return  True if copy successful, false otherwise.
 */
bool DCUtil::copyDir(const string& src, const string& dest, bool rec)
{
  bool ret = true;
  vector<string> files(DCUtil::listFiles(src));
  vector<string>::iterator it;

  // Create destination directory if it does not already exist
  if(!DCUtil::isDirectory(dest))
    if(mkdir(dest.c_str(), 0755) != 0)
      return false;

  for(it = files.begin() ; it != files.end() ; ++it) {
    // Source path
    string srcpath(src);
    srcpath += '/';
    srcpath.append(*it);

    // Construct new path
    string next(dest);
    next += '/';
    next.append(*it);

    if(DCUtil::isDirectory(srcpath)) {
      if(rec) {
        ret &= copyDir(srcpath, next, rec);
      }
    } else {
      ret &= copyFile(srcpath, next);
    }
  }

  return ret;
}

/**
 * List all files in a directory (with full paths)
 *
 * @param dir   Directory to list files in
 * @return  A vector containing all the valid files within the directory
 */
vector<string> DCUtil::listFiles(string dir)
{
  vector<string> ret;

  // Sorry about the platform-dependent code, but that
  // just seems to be how listing files works -.-
#ifdef _WIN32
  dir.append("/*"); // Need this for windows
  WIN32_FIND_DATA data;
  HANDLE hFind = FindFirstFile(dir.c_str(), &data);

  if(hFind == INVALID_HANDLE_VALUE)
    return ret; // Bad directory

  while(FindNextFile(hFind, &data) != 0 && GetLastError() != ERROR_NO_MORE_FILES) {
    if(strncmp(data.cFileName, ".", strlen(".")) == 0 || strncmp(data.cFileName, "..", strlen("..")) == 0)
      continue; // Skip these directories
    ret.push_back(data.cFileName);
  }

  FindClose(hFind);
#else
  DIR* odir = opendir(dir.c_str());
  dirent* f = NULL;

  if(odir == NULL)
    return ret; // Could not open directory

  while((f = readdir(odir))) {
    if(strncmp(f->d_name, ".", strlen(".")) == 0 || strncmp(f->d_name, "..", strlen("..")) == 0)
      continue; // Skip these directories
    ret.push_back(f->d_name);
  }

  closedir(odir);
#endif

  return ret;
}

/**
 * Check if a file is a directory or not
 *
 * @param file    Directory to check
 * @return  True if directory, false otherwise
 */
bool DCUtil::isDirectory(const std::string& file)
{
  bool ret = false;

  // More platform-dependent code -.-
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(file.c_str());
  if(attr == INVALID_FILE_ATTRIBUTES)
    return false; // Bad path?
  ret = attr & FILE_ATTRIBUTE_DIRECTORY;
#else
  DIR* dir = opendir(file.c_str());
  ret = dir != NULL;
  closedir(dir);
#endif
  
  return ret;
}
