#ifndef CASE3_H
#define CASE3_H

int runCase3();

// Used by the simulator. p[0..6] = AB, AC, AD, BE, CE, CF, DF.
// sender: 0=A, 1=B, 2=C, 3=D. Receiver numbers: 0=B, 1=C, 2=D, 3=E, 4=F.
int case3SenderLinks(int sender, double p[], int receiver[3],
                     double probability[3]);

// remainingOut (32 values) and nextSenderOut (32 values) are optional copies
// of the forwarding table, indexed by 16*b + 8*c + 4*d + 2*e + f.
double calculateCase3(double p[], bool needE, bool needF, bool showTable,
                      double remainingOut[] = 0, int nextSenderOut[] = 0);

#endif

