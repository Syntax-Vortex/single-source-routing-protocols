#include <iostream>
#include <iomanip>
#include "case4.h"
#include "input.h"
#include "etx.h"
using namespace std;

// p[0..8] = AB, AC, AD, BE, BF, CF, CG, DF, DG.
// Flags b,c,d,e,f,g say who already holds the packet. A always has it.
// sender: 0=A, 1=B, 2=C, 3=D.
double senderCostCase4(double remaining[2][2][2][2][2][2],
                      int b, int c, int d, int e, int f, int g,
                      int sender, double p[]) {
    if (sender == 1 && b == 0) return INF;
    if (sender == 2 && c == 0) return INF;
    if (sender == 3 && d == 0) return INF;

    // Receiver numbers: 0=B, 1=C, 2=D, 3=E, 4=F, 5=G.
    int receiver[3] = {0, 0, 0};
    double probability[3] = {0, 0, 0};

    if (sender == 0) {
        receiver[0] = 0; probability[0] = p[0]; // A -> B
        receiver[1] = 1; probability[1] = p[1]; // A -> C
        receiver[2] = 2; probability[2] = p[2]; // A -> D
    } else if (sender == 1) {
        receiver[0] = 3; probability[0] = p[3]; // B -> E
        receiver[1] = 4; probability[1] = p[4]; // B -> F
    } else if (sender == 2) {
        receiver[0] = 4; probability[0] = p[5]; // C -> F
        receiver[1] = 5; probability[1] = p[6]; // C -> G
    } else {
        receiver[0] = 4; probability[0] = p[7]; // D -> F
        receiver[1] = 5; probability[1] = p[8]; // D -> G
    }

    double progressProbability = 0;
    double futureCost = 0;

    // A has eight reception outcomes; the relays have four each.
    // The unused third receiver slot for a relay has probability zero.
    for (int first = 0; first <= 1; first++) {
        for (int second = 0; second <= 1; second++) {
            for (int third = 0; third <= 1; third++) {
                int received[3] = {first, second, third};
                int newHolders[6] = {b, c, d, e, f, g};
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
                int newG = newHolders[5];

                if (newB == b && newC == c && newD == d
                    && newE == e && newF == f && newG == g) {
                    continue; // The final division includes unchanged-state retries.
                }

                double nextCost = remaining[newB][newC][newD][newE][newF][newG];
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

double calculateCase4(double p[], bool needE, bool needF, bool needG,
                      bool showTable) {
    // Six yes/no flags give 2*2*2*2*2*2 = 64 configurations.
    double remaining[2][2][2][2][2][2];
    int nextSender[2][2][2][2][2][2];

    // Changed configurations come first because receptions only add copies.
    for (int b = 1; b >= 0; b--) {
        for (int c = 1; c >= 0; c--) {
            for (int d = 1; d >= 0; d--) {
                for (int e = 1; e >= 0; e--) {
                    for (int f = 1; f >= 0; f--) {
                        for (int g = 1; g >= 0; g--) {
                            remaining[b][c][d][e][f][g] = INF;
                            nextSender[b][c][d][e][f][g] = -1;

                            bool eFinished = !needE || e == 1;
                            bool fFinished = !needF || f == 1;
                            bool gFinished = !needG || g == 1;
                            if (eFinished && fFinished && gFinished) {
                                remaining[b][c][d][e][f][g] = 0;
                                continue;
                            }

                            for (int sender = 0; sender <= 3; sender++) {
                                double cost = senderCostCase4(remaining, b, c, d,
                                                              e, f, g, sender, p);
                                if (cost < remaining[b][c][d][e][f][g]) {
                                    remaining[b][c][d][e][f][g] = cost;
                                    nextSender[b][c][d][e][f][g] = sender;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (showTable) {
        char names[4] = {'A', 'B', 'C', 'D'};
        cout << "\nForwarding table: deliver the same packet to ALL E, F and G\n";
        cout << "1 = already has packet, 0 = does not have packet. A always has it.\n";
        cout << "Some configurations may not occur for particular probabilities.\n";
        cout << "B C D E F G     Next sender     Remaining ETX\n";
        for (int b = 0; b <= 1; b++) {
            for (int c = 0; c <= 1; c++) {
                for (int d = 0; d <= 1; d++) {
                    for (int e = 0; e <= 1; e++) {
                        for (int f = 0; f <= 1; f++) {
                            for (int g = 0; g <= 1; g++) {
                                // E only receives from B; G from C/D; F from B/C/D.
                                if (e == 1 && b == 0) continue;
                                if (g == 1 && c == 0 && d == 0) continue;
                                if (f == 1 && b == 0 && c == 0 && d == 0) continue;

                                cout << b << " " << c << " " << d << " " << e << " "
                                     << f << " " << g << "     ";
                                int sender = nextSender[b][c][d][e][f][g];
                                if (remaining[b][c][d][e][f][g] == 0) {
                                    cout << setw(16) << left << "DONE";
                                } else if (sender == -1) {
                                    cout << setw(16) << left << "unreachable";
                                } else {
                                    cout << names[sender] << "               ";
                                }
                                printETX(remaining[b][c][d][e][f][g]);
                                cout << "\n";
                            }
                        }
                    }
                }
            }
        }
    }
    return remaining[0][0][0][0][0][0];
}

int runCase4() {
    cout << "\nCASE 4\nA -> {B,C,D}, B -> {E,F}, C -> {F,G}, D -> {F,G}\n";
    double p[9];
    const char links[9][7] = {"A -> B", "A -> C", "A -> D", "B -> E", "B -> F",
                             "C -> F", "C -> G", "D -> F", "D -> G"};
    for (int i = 0; i < 9; i++) {
        p[i] = readProbability(links[i]);
        if (p[i] == -1) return 1;
    }

    cout << "\nLINK ETX VALUES\n";
    for (int i = 0; i < 9; i++) {
        cout << links[i] << ": ";
        printETX(linkETX(p[i]));
        cout << "\n";
    }

    cout << "\nRELAY TABLES (relay already has the packet)\n";
    cout << "Source    Target        ETX\n";
    printRow("B", "E", linkETX(p[3]));
    printRow("B", "F", linkETX(p[4]));
    printRow("B", "G", INF);
    printRow("B", "E AND F", bothETX(p[3], p[4]));
    printRow("B", "E,F,G", INF);
    printRow("C", "E", INF);
    printRow("C", "F", linkETX(p[5]));
    printRow("C", "G", linkETX(p[6]));
    printRow("C", "F AND G", bothETX(p[5], p[6]));
    printRow("C", "E,F,G", INF);
    printRow("D", "E", INF);
    printRow("D", "F", linkETX(p[7]));
    printRow("D", "G", linkETX(p[8]));
    printRow("D", "F AND G", bothETX(p[7], p[8]));
    printRow("D", "E,F,G", INF);

    cout << "\nFINAL SOURCE TABLE\nSource    Target        ETX\n";
    printRow("A", "E only", calculateCase4(p, true, false, false, false));
    printRow("A", "F only", calculateCase4(p, false, true, false, false));
    printRow("A", "G only", calculateCase4(p, false, false, true, false));
    double finalETX = calculateCase4(p, true, true, true, false);
    printRow("A", "E,F,G", finalETX);
    cout << "Individual destination ETXs are separate goals; do not add them.\n";

    calculateCase4(p, true, true, true, true);
    cout << "\nFinal ETX from A to ALL E, F and G = ";
    printETX(finalETX);
    cout << "\n";
    return 0;
}

