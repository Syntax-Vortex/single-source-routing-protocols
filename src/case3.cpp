#include <iostream>
#include <iomanip>
#include <cstring>
#include "case3.h"
#include "input.h"
#include "etx.h"
using namespace std;

// p[0..6] = AB, AC, AD, BE, CE, CF, DF.
// The outgoing links of one sender. Receiver numbers: 0=B, 1=C, 2=D, 3=E, 4=F.
// Unused receiver slots get probability 0.
// Returns how many receivers the sender really has.
int case3SenderLinks(int sender, double p[], int receiver[3],
                     double probability[3]) {
    for (int i = 0; i < 3; i++) {
        receiver[i] = 0;
        probability[i] = 0;
    }

    if (sender == 0) {
        receiver[0] = 0; probability[0] = p[0]; // A -> B
        receiver[1] = 1; probability[1] = p[1]; // A -> C
        receiver[2] = 2; probability[2] = p[2]; // A -> D
        return 3;
    } else if (sender == 1) {
        receiver[0] = 3; probability[0] = p[3]; // B -> E only
        return 1;
    } else if (sender == 2) {
        receiver[0] = 3; probability[0] = p[4]; // C -> E
        receiver[1] = 4; probability[1] = p[5]; // C -> F
        return 2;
    } else {
        receiver[0] = 4; probability[0] = p[6]; // D -> F only
        return 1;
    }
}

// The five flags b,c,d,e,f record who has the packet. A always has it.
// sender: 0=A, 1=B, 2=C, 3=D.
double senderCostCase3(double remaining[2][2][2][2][2],
                      int b, int c, int d, int e, int f,
                      int sender, double p[]) {
    if (sender == 1 && b == 0) return INF;
    if (sender == 2 && c == 0) return INF;
    if (sender == 3 && d == 0) return INF;

    int receiver[3];
    double probability[3];
    case3SenderLinks(sender, p, receiver, probability);

    double progressProbability = 0;
    double futureCost = 0;

    // A has three receivers: 2*2*2 = eight possible outcomes.
    // B/C/D use the same loops. Unused receiver slots have probability 0,
    // so any outcome claiming success in an unused slot is skipped.
    for (int first = 0; first <= 1; first++) {
        for (int second = 0; second <= 1; second++) {
            for (int third = 0; third <= 1; third++) {
                int received[3] = {first, second, third};
                int newHolders[5] = {b, c, d, e, f};
                double chance = 1;

                for (int i = 0; i < 3; i++) {
                    if (received[i] == 1) {
                        chance = chance * probability[i];
                        newHolders[receiver[i]] = 1;
                    } else {
                        chance = chance * (1 - probability[i]);
                    }
                }
                if (chance == 0) continue;

                int newB = newHolders[0];
                int newC = newHolders[1];
                int newD = newHolders[2];
                int newE = newHolders[3];
                int newF = newHolders[4];

                if (newB == b && newC == c && newD == d
                    && newE == e && newF == f) {
                    continue; // Retry cost is included by the division below.
                }

                double nextCost = remaining[newB][newC][newD][newE][newF];
                if (nextCost == INF) return INF;

                progressProbability = progressProbability + chance;
                futureCost = futureCost + chance * nextCost;
            }
        }
    }
    if (progressProbability == 0) return INF;

    // x = 1 + P(no change)*x + weighted future cost.
    return (1 + futureCost) / progressProbability;
}

double calculateCase3(double p[], bool needE, bool needF, bool showTable,
                      double remainingOut[], int nextSenderOut[]) {
    double remaining[2][2][2][2][2];
    int nextSender[2][2][2][2][2];

    // Work backwards so that every changed configuration is already solved.
    for (int b = 1; b >= 0; b--) {
        for (int c = 1; c >= 0; c--) {
            for (int d = 1; d >= 0; d--) {
                for (int e = 1; e >= 0; e--) {
                    for (int f = 1; f >= 0; f--) {
                        remaining[b][c][d][e][f] = INF;
                        nextSender[b][c][d][e][f] = -1;

                        bool eFinished = !needE || e == 1;
                        bool fFinished = !needF || f == 1;
                        if (eFinished && fFinished) {
                            remaining[b][c][d][e][f] = 0;
                            continue;
                        }

                        for (int sender = 0; sender <= 3; sender++) {
                            double cost = senderCostCase3(remaining, b, c, d, e, f,
                                                          sender, p);
                            if (cost < remaining[b][c][d][e][f]) {
                                remaining[b][c][d][e][f] = cost;
                                nextSender[b][c][d][e][f] = sender;
                            }
                        }
                    }
                }
            }
        }
    }

    if (showTable) {
        char names[4] = {'A', 'B', 'C', 'D'};
        cout << "\nForwarding table: deliver the same packet to BOTH E and F\n";
        cout << "1 = already has packet, 0 = does not have packet. A always has it.\n";
        cout << "Some configurations may not occur for particular probabilities.\n";
        cout << "B C D E F     Next sender     Remaining ETX\n";
        for (int b = 0; b <= 1; b++) {
            for (int c = 0; c <= 1; c++) {
                for (int d = 0; d <= 1; d++) {
                    for (int e = 0; e <= 1; e++) {
                        for (int f = 0; f <= 1; f++) {
                            // E can only receive from B/C; F from C/D.
                            if (e == 1 && b == 0 && c == 0) continue;
                            if (f == 1 && c == 0 && d == 0) continue;

                            cout << b << " " << c << " " << d << " " << e << " "
                                 << f << "     ";
                            int sender = nextSender[b][c][d][e][f];
                            if (remaining[b][c][d][e][f] == 0) {
                                cout << setw(16) << left << "DONE";
                            } else if (sender == -1) {
                                cout << setw(16) << left << "unreachable";
                            } else {
                                cout << names[sender] << "               ";
                            }
                            printETX(remaining[b][c][d][e][f]);
                            cout << "\n";
                        }
                    }
                }
            }
        }
    }
    // Optional copy of the tables for the simulator, in B,C,D,E,F order:
    // index = 16*b + 8*c + 4*d + 2*e + f.
    if (remainingOut != 0) memcpy(remainingOut, remaining, sizeof(remaining));
    if (nextSenderOut != 0) memcpy(nextSenderOut, nextSender, sizeof(nextSender));

    return remaining[0][0][0][0][0];
}

int runCase3() {
    cout << "\nCASE 3\nA -> {B,C,D}, B -> E, C -> {E,F}, D -> F\n";
    double p[7];
    const char links[7][7] = {"A -> B", "A -> C", "A -> D", "B -> E",
                             "C -> E", "C -> F", "D -> F"};
    for (int i = 0; i < 7; i++) {
        p[i] = readProbability(links[i]);
        if (p[i] == -1) return 1;
    }

    cout << "\nLINK ETX VALUES\n";
    for (int i = 0; i < 7; i++) {
        cout << links[i] << ": ";
        printETX(linkETX(p[i]));
        cout << "\n";
    }

    cout << "\nRELAY TABLES (relay already has the packet)\n";
    cout << "Source    Target        ETX\n";
    // B cannot reach F, and D cannot reach E: no such links exist.
    printRow("B", "E", linkETX(p[3]));
    printRow("B", "F", INF);
    printRow("B", "E AND F", INF);
    printRow("C", "E", linkETX(p[4]));
    printRow("C", "F", linkETX(p[5]));
    printRow("C", "E AND F", bothETX(p[4], p[5]));
    printRow("D", "E", INF);
    printRow("D", "F", linkETX(p[6]));
    printRow("D", "E AND F", INF);

    cout << "\nFINAL SOURCE TABLE\nSource    Target        ETX\n";
    printRow("A", "E only", calculateCase3(p, true, false, false));
    printRow("A", "F only", calculateCase3(p, false, true, false));
    double finalETX = calculateCase3(p, true, true, false);
    printRow("A", "E AND F", finalETX);
    cout << "E-only and F-only are separate goals; do not add their ETXs.\n";

    calculateCase3(p, true, true, true);
    cout << "\nFinal ETX from A to BOTH E and F = ";
    printETX(finalETX);
    cout << "\n";
    return 0;
}

