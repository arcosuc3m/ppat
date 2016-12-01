#ifndef STDEXTEND_H
#define STDEXTEND_H

#include <string>
/**
 *  @brief split a string of characters 'str' into several string
 *
 *  @param str string to be splited
 *  @param delimiter delimiter of each substring
 *  @return a vector with all the substrings
 */
std::vector<std::string> split(std::string str, char delimiter);
#endif
