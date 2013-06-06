/**
 * Coord3D.cpp
 *
 * Coord3D convenience method implementations
 *
 * @author Dennis J. McWherter, Jr.
 */

#include <cmath>

#include "Coord3D.h"
#include "DCUtil.h"

using namespace std;

/**
 * Report the distance between this coordinate and another 3D point
 *
 * @param x   x coordinate
 * @param y   y coordinate
 * @param z   z coordinate
 * @return  The Euclidean distance between this coordinate the one specified
 */
double Coord3D::getDistance(double x, double y, double z) const
{
  double xDiff = xCoord - x;
  double yDiff = yCoord - y;
  double zDiff = zCoord - z;

  // Euclidean distance = sqrt((x1 - x2)^2 + (y1 - y2)^2 + (z1 - z2)^2)
  return sqrt(((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)));
}

/**
 * Report the distance between this coordinate and another
 *
 * @param coord   The coordinate to compare distance
 * @return  The Euclidean distance between the two coordinates
 */
double Coord3D::getDistance(const Coord3D& coord) const
{
  return getDistance(coord.xCoord, coord.yCoord, coord.zCoord);
}

/**
 * Get the coordinate tuple as a string in the format: (x, y, z)
 *
 * @return  The coordinate as a string tuple
 */
string Coord3D::toString() const
{
  string ret("(");
  ret.append(DCUtil::XToY<double, string>(xCoord));
  ret.append(", ");
  ret.append(DCUtil::XToY<double, string>(yCoord));
  ret.append(", ");
  ret.append(DCUtil::XToY<double, string>(zCoord));
  ret.append(")");
  return ret;
}
