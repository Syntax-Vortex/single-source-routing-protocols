#include <iostream>
#include "case1.h"
#include "input.h"
#include "etx.h"
using namespace std;

int runCase1() {
    double ab = readProbability("A -> B");
    if (ab == -1) return 1;
    double ac = readProbability("A -> C");
    if (ac == -1) return 1;

    cout << "\nLINK ETX VALUES\n";
    printRow("A", "B", linkETX(ab));
    printRow("A", "C", linkETX(ac));

    cout << "\nFirst broadcast outcomes:\n";
    cout << "Neither: " << (1-ab)*(1-ac) << "\n";
    cout << "B only:  " << ab*(1-ac) << "\n";
    cout << "C only:  " << (1-ab)*ac << "\n";
    cout << "Both:    " << ab*ac << "\n";

    cout << "\nSource    Target        ETX\n";
    printRow("A", "B", linkETX(ab));
    printRow("A", "C", linkETX(ac));
    printRow("A", "B AND C", bothETX(ab, ac));

    cout << "\nFinal ETX from A to BOTH B and C = ";
    printETX(bothETX(ab, ac));
    cout << "\n";
    return 0;
}

