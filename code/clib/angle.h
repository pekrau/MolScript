/* angle.h

   Angle constants

   clib v1.0

   Copyright (C) 1997 Per Kraulis
    29-Jan-1997  split out of MolScript
     3-Dec-1997  changed PI to ANGLE_PI to avoid clashes
*/

#ifndef ANGLE_H
#define ANGLE_H 1

#define ANGLE_PI 3.14159265358979323846

#define to_radians(d) ((d)*ANGLE_PI/180.0)
#define to_degrees(d) ((d)*180.0/ANGLE_PI)

#define SQRT2 1.4142136

#endif
