/**
 * DCException.h
 *
 * Exception class with custom messages
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef DCEXCEPTION_H__
#define DCEXCEPTION_H__

#include <exception>
#include <string>

class DCException : public std::exception
{
public:
  DCException(const std::string& str) : ss(str) {}
  virtual ~DCException() throw() {}
  const char* what() const throw() { return ss.c_str(); }

private:
  std::string ss;
};

#endif /** DCEXCEPTION_H__ */

