/**
 * ParserBase.cpp
 *
 * Base class for any type of file parser
 *
 * @author Dennis J. McWherter, Jr.
 */

#include "ParserBase.h"

bool ParserBase::isValidResult(const std::vector<double>& val) const
{
  return &val != &sentinel;
}
