#include <iostream>
#include "input.h"
using namespace std;

double readProbability(const char name[]) {
    double p;

    while (true) {
        cout << "Probability for " << name << " [0 to 1]: ";
        cin >> p;

        if (cin.eof() && cin.fail()) {
            return -1; // Tell main that input ended.
        }

        if (cin.fail()) {
            cin.clear();            // Clear the input error.
            cin.ignore(10000, '\n'); // Discard the invalid line.
            cout << "Please enter a number.\n";
            continue;
        }

        cin.ignore(10000, '\n');
        if (p >= 0 && p <= 1) {
            return p;
        }

        cout << "Enter a number between 0 and 1, such as 0.8.\n";
    }
}


