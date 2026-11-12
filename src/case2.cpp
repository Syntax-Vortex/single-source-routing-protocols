#include <iostream>
#include <iomanip>
#include "etx.h"
#include "case2.h"
#include "input.h"
using namespace std;

// b,c,d,e are each 0 or 1: does that node already hold the packet?
// sender: 0 means A, 1 means B, 2 means C.
double senderCost(double remaining[2][2][2][2], int b, int c, int d, int e,
                  int sender, double p[]) {
    if (sender == 1 && b == 0) {
        return INF; // B cannot send a packet it does not have.
    }
    if (sender == 2 && c == 0) {
        return INF;
    }

    double firstProbability;
    double secondProbability;

    if (sender == 0) {
        firstProbability = p[0];  // A -> B
        secondProbability = p[1]; // A -> C
    } else if (sender == 1) {
        firstProbability = p[2];  // B -> D
        secondProbability = p[3]; // B -> E
    } else {
        firstProbability = p[4];  // C -> D
        secondProbability = p[5]; // C -> E
    }

    double progressProbability = 0;
    double futureCost = 0;

    // Two receivers, each either fails (0) or succeeds (1): four outcomes.
    for (int first = 0; first <= 1; first++) {
        for (int second = 0; second <= 1; second++) {
            double chance;

            if (first == 1) {
                chance = firstProbability;
            } else {
                chance = 1 - firstProbability;
            }
            if (second == 1) {
                chance = chance * secondProbability;
            } else {
                chance = chance * (1 - secondProbability);
            }

            if (chance == 0) {
                continue;
            }

            // Start with the old holders. Successful receptions add copies.
            int newB = b;
            int newC = c;
            int newD = d;
            int newE = e;

            if (sender == 0) {
                if (first == 1) newB = 1;
                if (second == 1) newC = 1;
            } else {
                if (first == 1) newD = 1;
                if (second == 1) newE = 1;
            }

            if (newB == b && newC == c && newD == d && newE == e) {
                // No new receiver: retry cost is handled by the division below.
                continue;
            }

            double nextCost = remaining[newB][newC][newD][newE];
            if (nextCost == INF) {
                return INF;
            }

            progressProbability = progressProbability + chance;
            futureCost = futureCost + chance * nextCost;
        }
    }

    if (progressProbability == 0) {
        return INF;
    }

    // Same idea as case 1:
    // x = 1 + P(no change)*x + weighted cost of changed states.
    return (1 + futureCost) / progressProbability;
}

double calculateRouting(double p[], bool needD, bool needE, bool showTable) {
    // No bit tricks: the four array positions directly represent B,C,D,E.
    double remaining[2][2][2][2];
    int nextSender[2][2][2][2];

    // Start with all flags at 1 and work backwards.
    // A new reception only changes 0 to 1, so its answer is already calculated.
    for (int b = 1; b >= 0; b--) {
        for (int c = 1; c >= 0; c--) {
            for (int d = 1; d >= 0; d--) {
                for (int e = 1; e >= 0; e--) {
                    remaining[b][c][d][e] = INF;
                    nextSender[b][c][d][e] = -1;

                    bool dFinished = !needD || d == 1;
                    bool eFinished = !needE || e == 1;

                    if (dFinished && eFinished) {
                        remaining[b][c][d][e] = 0;
                        continue;
                    }

                    for (int sender = 0; sender <= 2; sender++) {
                        double cost = senderCost(remaining, b, c, d, e, sender, p);
                        if (cost < remaining[b][c][d][e]) {
                            remaining[b][c][d][e] = cost;
                            nextSender[b][c][d][e] = sender;
                        }
                    }
                }
            }
        }
    }

    if (showTable) {
        char names[3] = {'A', 'B', 'C'};
        cout << "\nForwarding table: deliver the same packet to BOTH D and E\n";
        cout << "1 = already has packet, 0 = does not have packet. A always has it.\n";
        cout << "Some configurations may not occur for particular probabilities.\n";
        cout << "B C D E     Next sender     Remaining ETX\n";

        for (int b = 0; b <= 1; b++) {
            for (int c = 0; c <= 1; c++) {
                for (int d = 0; d <= 1; d++) {
                    for (int e = 0; e <= 1; e++) {
                        // A destination cannot have received from neither relay.
                        if (b == 0 && c == 0 && (d == 1 || e == 1)) {
                            continue;
                        }
                        cout << b << " " << c << " " << d << " " << e << "     ";
                        int sender = nextSender[b][c][d][e];
                        if (remaining[b][c][d][e] == 0) {
                            cout << setw(16) << left << "DONE";
                        } else if (sender == -1) {
                            cout << setw(16) << left << "unreachable";
                        } else {
                            cout << names[sender] << "               ";
                        }
                        printETX(remaining[b][c][d][e]);
                        cout << "\n";
                    }
                }
            }
        }
    }

    // Initially only A holds the packet.
    return remaining[0][0][0][0];
}


int runCase2() {
    double p[6];
    const char links[6][7] = {"A -> B", "A -> C", "B -> D",
                             "B -> E", "C -> D", "C -> E"};
    for (int i = 0; i < 6; i++) {
        p[i] = readProbability(links[i]);
        if (p[i] == -1) return 1;
    }
    cout << "\nLINK ETX VALUES\n";
    for (int i = 0; i < 6; i++) {
        cout << links[i] << ": ";
        printETX(linkETX(p[i]));
        cout << "\n";
    }
        cout << "\nRELAY TABLES (relay already has the packet)\n";
        cout << "Source    Target        ETX\n";
        printRow("B", "D", linkETX(p[2]));
        printRow("B", "E", linkETX(p[3]));
        printRow("B", "D AND E", bothETX(p[2], p[3]));
        printRow("C", "D", linkETX(p[4]));
        printRow("C", "E", linkETX(p[5]));
        printRow("C", "D AND E", bothETX(p[4], p[5]));

        cout << "\nFIXED ROUTE COMPARISON\n";
        cout << "Via B to D AND E: ";
        printETX(linkETX(p[0]) + bothETX(p[2], p[3]));
        cout << "\nVia C to D AND E: ";
        printETX(linkETX(p[1]) + bothETX(p[4], p[5]));
        cout << "\n";

        cout << "\nFINAL SOURCE TABLE\nSource    Target        ETX\n";
        printRow("A", "D only", calculateRouting(p, true, false, false));
        printRow("A", "E only", calculateRouting(p, false, true, false));

        double finalETX = calculateRouting(p, true, true, false);
        printRow("A", "D AND E", finalETX);

        calculateRouting(p, true, true, true);
        cout << "\nFinal ETX from A to BOTH D and E = ";
        printETX(finalETX);
        cout << "\n";
    return 0;
}
