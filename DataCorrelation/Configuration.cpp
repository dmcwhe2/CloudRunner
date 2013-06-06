/**
 * Configuration.cpp
 *
 * Configuration parser implementation
 *
 * @author Dennis J. McWherter, Jr.
 */

#include <cstdio>
#include <fstream>
#include <iostream>

#include "Configuration.h"
#include "DCException.h"
#include "DCUtil.h"

using namespace std;

/**
 * Constructor
 *
 * @param file    Path to configuration file
 */
Configuration::Configuration(const string& file)
  : filename(file), lineno(0), parserState(NONE), runSimulation(true), sym(SYMMETRIC)
{
  parse();
}

/** Private methods */

/**
 * Parse data from input file
 */
void Configuration::parse()
{
  string line;
  ifstream in(filename.c_str());
  //PARSESTATE state = NONE;

  if(!in.is_open())
    throw DCException("Could not open configuration file!");

  // Just a large state machine
  while(!in.eof()) {
    getline(in, line);
    lineno++;

    DCUtil::trim(line);

    if(line.empty() || line[0] == '#')
      continue; // Skip this line - it's a comment or empty line

    if(parserState == NONE) {
      if(DCUtil::startsWith(line, "main")) {
        parserState = MAIN;
        parseMain(line);
      } else if(DCUtil::startsWith(line, "rules")) {
        parserState = RULES;
        parseRules(line);
      } else if(DCUtil::startsWith(line, "analysis")) {
        parserState = ANALYSIS;
        parseAnalysis(line);
      } else {
        Configuration::throwException("Unexpected value", lineno);
      }
    } else if(parserState == MAIN) {
      if(parseMain(line))
        parserState = NONE;
    } else if(parserState == RULES) {
      if(parseRules(line))
        parserState = NONE;
    } else if(parserState == FILE) {
      if(parseFile(line))
        parserState = RULES;
    } else if(parserState == ANALYSIS) {
      if(parseAnalysis(line))
        parserState = NONE;
    } else if(parserState == PARAMETER) {
      if(parseParameter(line))
        parserState = ANALYSIS;
    }
  }

  in.close();

  // If parserState != NONE, we never finished block
  if(parserState == MAIN)
    throw DCException("Unclosed main{ ... } block");
  else if(parserState == RULES)
    throw DCException("Unclosed rules{ ... } block");
  else if(parserState == FILE)
    throw DCException("Unclosed file{ ... } block");
  else if(parserState == ANALYSIS)
    throw DCException("Unclosed analysis{ ... } block");
}

/**
 * Parse the main block
 *
 * @param line  The line which opened the block
 *
 * NOTE: Currently block parsing assumes a "nice" structure
 *    That is, the closing brace, }, is on its own line
 *    and that each property is on its own line
 */
bool Configuration::parseMain(const std::string& line)
{
  static bool open = false;

  if(!open) {
    open = Configuration::validOpen(line, "main");
  } else {
    if(DCUtil::startsWith(line, "}")) {
      open = false;
      return true;
    } else if(Configuration::isVarLine(line, "exe")) {
      // Extract
      exe = Configuration::extractValue(line);
    } else if(Configuration::isVarLine(line, "data")) {
      datadir = Configuration::extractValue(line);
    } else if(Configuration::isVarLine(line, "simulator")) {
      simulator = Configuration::extractValue(line);
    } else if(Configuration::isVarLine(line, "listKeys")) {
      string val(Configuration::extractValue(line));
      DCUtil::strToUpper(val);
      if(val.compare("FALSE") == 0 || val.compare("0") == 0)
        listKeyVals = false; // Don't list keys, just analyze
    } else if(Configuration::isVarLine(line, "runName")) {
      runName = Configuration::extractValue(line);
    } else if(Configuration::isVarLine(line, "runSim")) {
      string val(Configuration::extractValue(line));
      DCUtil::strToUpper(val);
      if(val.compare("FALSE") == 0 || val.compare("0") == 0)
        runSimulation = false; // Don't run simulation, just analyze
    } else if(Configuration::isVarLine(line, "output")) {
      string out(Configuration::extractValue(line));
      // Comma delimited
      size_t pos = 0, last = 0;
      while((pos = out.find_first_of(',', pos)) != string::npos) {
        string str(out.substr(last, (pos - last)));
        DCUtil::trim(str);
        output.push_back(str);
        pos++;
        last = pos;
      }
      // Push the last file from the list into the vector
      string str(out.substr(last));
      DCUtil::trim(str);
      output.push_back(str);
    } else if(Configuration::isVarLine(line, "symmetry")) {
      string val(Configuration::extractValue(line));
      if(DCUtil::startsWith(val, "symmetric")) {
        sym = SYMMETRIC;
      } else if(DCUtil::startsWith(val, "positive")) {
        sym = POSITIVE;
      } else if(DCUtil::startsWith(val, "negative")) {
        sym = NEGATIVE;
      }
    } else if(Configuration::isVarLine(line, "graph")) { // --- BEGIN GRAPHING SECTION WHICH MAY BE MOVED TO ITS OWN BLOCK (for flexibility)
      graph.valueToGraph = Configuration::extractValue(line);
    } else if(Configuration::isVarLine(line, "upperThresh")) {
      string val(Configuration::extractValue(line));
      graph.upperThresh = DCUtil::XToY<string, double>(val);
    } else if(Configuration::isVarLine(line, "lowerThresh")) {
      string val(Configuration::extractValue(line));
      graph.lowerThresh = DCUtil::XToY<string, double>(val);
    } else {
      Configuration::throwException("Unexpected value in main{...}", lineno);
    }
  }

  return false;
}

/**
 * Parse the rules block
 *
 * @param line  The line which opened the block
 */
bool Configuration::parseRules(const std::string& line)
{
  static bool open = false;

  if(!open) {
    open = Configuration::validOpen(line, "rules");
  } else {
    if(DCUtil::startsWith(line, "}")) {
      open = false;
      return true;
    } else if(DCUtil::startsWith(line, "file")) {
      parserState = FILE;
      parseFile(line);
    } else {
      Configuration::throwException("Unexpected value in rules{ ... }", lineno);
    }
  }

  return false;
}

/**
 * Parse a file block
 *
 * @param line    The line which opened the block
 * @return  True if closed, false otherwise
 */
bool Configuration::parseFile(const std::string& line)
{
  static bool open = false;
  static Rules r;

  // Get file name
  if(DCUtil::startsWith(line, "file")) {
    // Extract
    size_t start = line.find_first_of("\"");
    size_t end   = line.find_last_of("\"");
    if(start == string::npos || end == string::npos || start == end)
      Configuration::throwException("Invalid file{ ... } block defined", lineno);
    string filename(line.substr(start + 1, end - start - 1));
    r.name = filename;
  }

  if(!open) {
    open = Configuration::validOpen(line, "file");
  } else {
    if(DCUtil::startsWith(line, "}")) {
      open = false;
      // Add this rule to the structure
      rules[r.name].push_back(r);
      return true;
    } else if(Configuration::isVarLine(line, "start")) {
      r.start = DCUtil::XToY<string, int>(Configuration::extractValue(line));
    } else if(Configuration::isVarLine(line, "end")) {
      r.end = DCUtil::XToY<string, int>(Configuration::extractValue(line));
    } else if(DCUtil::startsWith(line, "modulo")) {
      r.modulo = DCUtil::XToY<string, int>(Configuration::extractValue(line));
    } else if(DCUtil::startsWith(line, "mval")) {
      r.mval = DCUtil::XToY<string, int>(Configuration::extractValue(line));
    } else if(DCUtil::startsWith(line, "format")) {
      r.format = Configuration::extractValue(line);
    } else if(DCUtil::startsWith(line, "outformat")) {
      r.outformat = Configuration::extractValue(line);
    } else if(DCUtil::startsWith(line, "transperc")) {
      r.transperc = DCUtil::XToY<string, float>(Configuration::extractValue(line));
    } else {
      Configuration::throwException("Unexpected value in file{ ... }", lineno);
    }
  }

  return false;
}

/**
 * Parse an analysis block
 *
 * @param line    The line which opened the block
 * @return  True if closed, false otherwise
 */
bool Configuration::parseAnalysis(const std::string& line)
{
  static bool open = false;

  if(!open) {
    open = Configuration::validOpen(line, "analysis");
  } else {
    if(DCUtil::startsWith(line, "}")) {
      open = false;
      return true;
    } else if(DCUtil::startsWith(line, "parameter")) {
      parserState = PARAMETER;
      parseParameter(line);
    } else {
      Configuration::throwException("Unexpected value in analysis{ ... }", lineno);
    }
  }

  return false;
}

/**
 * Parse a parameter block (must be within analysis block)
 *
 * @param line    The line which opened the block
 * @return  True if closed, false otherwise
 */
bool Configuration::parseParameter(const std::string& line)
{
  static bool open = false;
  static Parameter p;

  // Get parameter name
  if(DCUtil::startsWith(line, "parameter")) {
    // Extract
    size_t start = line.find_first_of("\"");
    size_t end   = line.find_last_of("\"");
    if(start == string::npos || end == string::npos || start == end)
      Configuration::throwException("Invalid parameter{ ... } block defined", lineno);
    string filename(line.substr(start + 1, end - start - 1));
    p.name = filename;
  }

  if(!open) {
    open = Configuration::validOpen(line, "parameter");
    p.stats   = 0; // Unless told otherwise, everything is disabled.
    p.pearson.clear();
  } else {
    if(DCUtil::startsWith(line, "}")) {
      open = false;
      params.push_back(p);
      return true;
    } else if(DCUtil::startsWith(line, "sum")) {
      if(DCUtil::XToY<string, int>(Configuration::extractValue(line)) > 0)
        p.stats |= Parameter::SUM;
    } else if(DCUtil::startsWith(line, "mean")) {
      if(DCUtil::XToY<string, int>(Configuration::extractValue(line)) > 0)
        p.stats |= Parameter::MEAN;
    } else if(DCUtil::startsWith(line, "variance")) {
      if(DCUtil::XToY<string, int>(Configuration::extractValue(line)) > 0)
        p.stats |= Parameter::VARIANCE;
    } else if(DCUtil::startsWith(line, "stddev")) {
      if(DCUtil::XToY<string, int>(Configuration::extractValue(line)) > 0)
        p.stats |= Parameter::STDDEV;
    } else if(DCUtil::startsWith(line, "pearson")) {
      p.pearson = Configuration::extractValue(line);
      if(!p.pearson.empty())
        p.stats |= Parameter::PEARSON;
    } else if(DCUtil::startsWith(line, "norm")) {
      if(DCUtil::XToY<string, int>(Configuration::extractValue(line)) > 0)
        p.stats |= Parameter::NORM;
    } else if(Configuration::isVarLine(line, "all")) {
      if(DCUtil::XToY<string, int>(Configuration::extractValue(line)) > 0) {
        p.stats |= Parameter::ALL_SIMILAR;
      }
    } else {
      Configuration::throwException("Unexpected value in parameter{ ... }", lineno);
    }
  }

  return false;
}

/**
 * Set the list of keys to update params for "all" values
 *
 * @param keys  Total list of keys
 */
void Configuration::updateParams(const vector<string>& keys)
{
  paramset::const_iterator it;
  vector<string>::const_iterator itt;
  paramset newParams;
  for(it = params.begin() ; it != params.end() ; ++it) {
    if(it->stats & Parameter::ALL_SIMILAR) {
      for(itt = keys.begin() ; itt != keys.end() ; ++itt) {
        if(it->name.compare(*itt) != 0 && DCUtil::startsWith(*itt, it->name)) {
          Parameter param;
          param.name    = *itt;
          param.pearson = it->pearson;
          param.stats   = it->stats;
          newParams.push_back(param);
        }
      }
    }
  }

  // Add this into our list
  for(it = newParams.begin() ; it != newParams.end() ; ++it)
    params.push_back(*it);
}

/** Helper methods (private) */

/**
 * Check if variable line
 *
 * @param line    The (trimmed) line to check
 * @param var     The variable to check
 * @return  True if variable line, otherwise false
 */
bool Configuration::isVarLine(string line, string var)
{
  if(line.size() <= var.size())
    return false;

  size_t pos = line.find_first_of("=");
  if(pos == string::npos)
    return false;

  string check(line.substr(var.size(), pos - var.size()));
  DCUtil::trim(check);

  if(!DCUtil::startsWith(line, var) || (!check.empty() && check.compare("=") != 0))
    return false;

  return true;
}

/**
 * Extract a value
 *
 * @param line  The variable line to extract a value from
 * @return  The value from the line
 */
string Configuration::extractValue(const string& line)
{
  size_t beg = line.find_first_of("=") + 1;
  string input(line.substr(beg, line.size() - beg));
  string ret, tmp, result;
  string open("");

  DCUtil::trim(input);

  if(input[0] != '$' && input[0] != '"')
    return "";

  // Iterate through the string and replace things as needed
  string::const_iterator it;
  
  for(it = input.begin() ; it != input.end() ; ++it) {
    if(!open.empty()) {
      if(open.compare("$") == 0) { // System environment variable
        if(*it == '$') {
          result = DCUtil::getEnvVar(tmp.c_str());
          tmp.clear();
          open.clear();
          ret.append(result);
        } else {
          tmp += *it;
        }
      } else if(open.compare("\"") == 0) { // String literal
        if(*it == '"')
          open.clear();
        else
          ret += *it;
      }
    } else {
      if(*it == '"' || *it == '$')
        open = *it;
    }
  }

  return ret;
}

/**
 * Check if this is a valid opening for a given key
 *
 * @param line  Line to check
 * @param key   The block to open
 * @return  True if valid, false otherwise
 */
bool Configuration::validOpen(const string& line, const string& key)
{
  string invalid("Invalid format in ");
  invalid.append(key);
  invalid.append(" { ... } block");

  // Check whether this is good or not
  if(line.compare(0, 1, "{") == 0)
    return true;
  else if(line.compare(0, key.length(), key) == 0 && line.find_first_of("{") != string::npos)
    return true;
  else if(!line.empty() && line.compare(0, key.length(), key) != 0 && line.compare(0, 1, "{") != 0)
    throw DCException(invalid.c_str());

  return false;
}

/**
 * Generic code to throw an Exception about unexpected
 * line
 *
 * @param lineno    The line number
 */
void Configuration::throwException(string err, int lineno)
{
  err.append(" on line: ");
  err.append(DCUtil::XToY<int, string>(lineno));
  throw DCException(err.c_str());
}

