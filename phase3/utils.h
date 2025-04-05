#ifndef UTILS_H
#define UTILS_H

#include <string>
using namespace std;

string convert_int2hexstr(long long num, int numBits);
long long convert_hexstr2int(const string &hexstr, int numBits);
unsigned int convert_binstr2int(const string &binstr);
string convert_PC2hex(unsigned int num);

#endif // UTILS_H
