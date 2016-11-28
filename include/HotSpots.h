/* ---------------------------------------------------*
 | Copyright 2016. Universidad Carlos III de Madrid.  |
 |Todos los derechos reservados / All rights reserved |
 *----------------------------------------------------*/
#pragma once

#include <string>

struct Hotspot {
  std::string Name;
  int line;
  double percentageGCOV;
  double percentageTime;
};
