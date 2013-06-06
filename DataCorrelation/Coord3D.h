/**
 * Coord3D.h
 *
 * 3-D coordinate information
 *
 * @author Dennis J. McWherter, Jr.
 */

#ifndef COORD3D_H__
#define COORD3D_H__

#include <string>

class Coord3D
{
public:
  /**
   * Constructor
   */
  Coord3D()
    : xCoord(0), yCoord(0), zCoord(0)
  {
  }

  /**
   * Constructor
   *
   * @param x   x coordinate value
   * @param y   y coordinate value
   * @param z   z coordinate value
   */
  Coord3D(double x, double y, double z)
    : xCoord(x), yCoord(y), zCoord(z)
  {
  }

  /**
   * Copy constructor
   *
   * @param coord   Coord3D structure to copy
   */
  Coord3D(const Coord3D& coord)
    : xCoord(coord.xCoord), yCoord(coord.yCoord), zCoord(coord.zCoord)
  {
  }

  /**
   * Assignment operator
   *
   * @param coord   Coord to copy
   * @return  A Coord3D structure with the properties of the input
   */
  Coord3D& operator=(const Coord3D& coord)
  {
    if(this != &coord) {
      xCoord = coord.xCoord;
      yCoord = coord.yCoord;
      zCoord = coord.zCoord;
    }
    return *this;
  }

  /**
   * Destructor
   */
  virtual ~Coord3D(){}

  /** Simple get methods */
  virtual double getX() const { return xCoord; }
  virtual double getY() const { return yCoord; }
  virtual double getZ() const { return zCoord; }

  /** Simple set methods */
  virtual void setX(double x) { xCoord = x; }
  virtual void setY(double y) { yCoord = y; }
  virtual void setZ(double z) { zCoord = z; }

  /**
   * Report the distance between this coordinate and another 3D point
   *
   * @param x   x coordinate
   * @param y   y coordinate
   * @param z   z coordinate
   * @return  The Euclidean distance between this coordinate the one specified
   */
  virtual double getDistance(double x, double y, double z) const;

  /**
   * Report the distance between this coordinate and another
   *
   * @param coord   The coordinate to compare distance
   * @return  The Euclidean distance between the two coordinates
   */
  virtual double getDistance(const Coord3D& coord) const;

  /**
   * Get the coordinate tuple as a string in the format: (x, y, z)
   *
   * @return  The coordinate as a string tuple
   */
  virtual std::string toString() const;

private:
  double xCoord, yCoord, zCoord;
};

#endif /** COORD3D_H__ */
