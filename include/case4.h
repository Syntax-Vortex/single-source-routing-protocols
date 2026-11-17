#ifndef CASE4_H
#define CASE4_H

int runCase4();

// Used by the simulator. p[0..8] = AB, AC, AD, BE, BF, CF, CG, DF, DG.
// sender: 0=A, 1=B, 2=C, 3=D. Receiver numbers: 0=B, 1=C, 2=D, 3=E, 4=F, 5=G.
int case4SenderLinks(int sender, double p[], int receiver[3],
                     double probability[3]);

// remainingOut (64 values) and nextSenderOut (64 values) are optional copies
// of the forwarding table, indexed by 32*b + 16*c + 8*d + 4*e + 2*f + g.
double calculateCase4(double p[], bool needE, bool needF, bool needG,
                      bool showTable, double remainingOut[] = 0,
                      int nextSenderOut[] = 0);

#endif
