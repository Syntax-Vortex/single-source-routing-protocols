#ifndef CASE2_H
#define CASE2_H

int runCase2();

// Used by the simulator. p[0..5] = AB, AC, BD, BE, CD, CE.
// sender: 0=A, 1=B, 2=C. Receiver numbers: 0=B, 1=C, 2=D, 3=E.
int case2SenderLinks(int sender, double p[], int receiver[3],
                     double probability[3]);

// remainingOut (16 values) and nextSenderOut (16 values) are optional copies
// of the forwarding table, indexed by 8*b + 4*c + 2*d + e.
double calculateRouting(double p[], bool needD, bool needE, bool showTable,
                        double remainingOut[] = 0, int nextSenderOut[] = 0);

#endif

