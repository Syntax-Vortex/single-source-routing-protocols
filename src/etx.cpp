#include <iostream>
#include <iomanip>
#include "etx.h"
using namespace std;

double linkETX(double p) {
    if (p == 0) {
        return INF;
    }
    return 1.0 / p;
}

double bothETX(double p, double q) {
    if (p == 0 || q == 0) {
        return INF;
    }

    double firstOnly = p * (1 - q);
    double secondOnly = (1 - p) * q;
    double both = p * q;

    // x = 1 + P(neither)*x + P(first only)/q + P(second only)/p
    // Move P(neither)*x to the left and divide.
    return (1 + firstOnly / q + secondOnly / p)
           / (firstOnly + secondOnly + both);
}

void printETX(double value) {
    if (value == INF) {
        cout << "unreachable";
    } else {
        cout << value;
    }
}


// Print one row of the routing table.
void printRow(const char source[], const char target[], double etx) {
    cout << setw(10) << left << source << setw(14) << target;
    printETX(etx);
    cout << "\n";
}

