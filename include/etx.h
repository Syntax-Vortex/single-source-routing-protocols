#ifndef ETX_H
#define ETX_H

// HUGE_VAL is the C++ math library's value for infinity.
// Here, infinity means the packet cannot reach the required destination.
#include <cmath>
const double INF = HUGE_VAL;

double linkETX(double p);
double bothETX(double p, double q);
void printETX(double value);

void printRow(const char source[], const char target[], double etx);

#endif

