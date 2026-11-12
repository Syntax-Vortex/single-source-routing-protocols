#include <iostream>
#include <iomanip>
#include <string>
#include "case1.h"
#include "case2.h"
#include "case3.h"
#include "case4.h"
using namespace std;

int main() {
    cout << fixed << setprecision(6);
    cout << "1. A -> {B,C}: deliver to BOTH B and C\n";
    cout << "2. A -> {B,C}, B/C -> {D,E}: deliver to BOTH D and E\n";
    cout << "3. A -> {B,C,D}, B -> E, C -> {E,F}, D -> F: deliver to BOTH E and F\n";

    cout << "4. A -> {B,C,D}, B -> {E,F}, C/D -> {F,G}: deliver to ALL E, F and G\n";

    string choice;
    while (true) {
        cout << "Choose 1, 2, 3 or 4: ";
        if (!getline(cin, choice)) return 1;
        if (choice == "1") return runCase1();
        if (choice == "2") return runCase2();
        if (choice == "3") return runCase3();
        if (choice == "4") return runCase4();
        cout << "Please enter 1, 2, 3 or 4.\n";
    }
}

