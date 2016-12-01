/* ---------------------------------------------------*
 | Copyright 2016. Universidad Carlos III de Madrid.  |
 |Todos los derechos reservados / All rights reserved |
 *----------------------------------------------------*/
#ifndef HOTSPOT_H
#define HOTSPOT_H

#include <string>

struct Hotspot {
  std::string Name;
  int line;
  double percentageGCOV;
  double percentageTime;
};
#endif
