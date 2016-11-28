/* ---------------------------------------------------*
 | Copyright 2016. Universidad Carlos III de Madrid.  |
 |Todos los derechos reservados / All rights reserved |
 *----------------------------------------------------*/
#pragma once

#include <string>
#include <map>

enum kind { L, R, LR, U };

struct DictionaryFunction {
  std::string Name;
  std::map<int, kind> arguments;
};
