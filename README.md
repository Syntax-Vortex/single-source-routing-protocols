# ETX routing project (beginner C++)

## Cases

1. A -> {B,C}. Deliver the same packet to BOTH B and C.
2. A -> {B,C}; B -> {D,E}; C -> {D,E}. Deliver to BOTH D and E.
3. A -> {B,C,D}; B -> E; C -> {E,F}; D -> F. Deliver to BOTH E and F.
4. A -> {B,C,D}; B -> {E,F}; C -> {F,G}; D -> {F,G}. Deliver to ALL E,F,G.

Case 4 has nine directed links. E can only receive from B, while G can only receive from C or D. No relay can deliver to all three destinations alone.

Case 3 has seven directed links. There is no B->F or D->E link.

## File layout

- src/main.cpp: menu and case selection.
- src/case1.cpp, include/case1.h: case 1 input, calculation and output.
- src/case2.cpp, include/case2.h: case 2 input, forwarding calculation and output.
- src/case3.cpp, include/case3.h: case 3 input, forwarding calculation and output.
- src/case4.cpp, include/case4.h: case 4 input, forwarding calculation and output.
- src/input.cpp, include/input.h: shared probability input.
- src/etx.cpp, include/etx.h: shared link ETX, two-receiver ETX and table formatting.
- examples/: input and captured output for each case.
- build.bat: compiles every case into build/etx_routing.exe.

The former routing.cpp/routing.h code is now inside case2.cpp.
No CMake or test framework is required.

## Build and run

Open this folder in VS Code. Run in its terminal:

```powershell
.\build.bat
.\build\etx_routing.exe
```

Choose 1, 2, 3 or 4. Enter probabilities as fractions from 0 to 1, such as 0.8.

Manual build:

```text
g++ -std=c++11 -Iinclude src/main.cpp src/input.cpp src/etx.cpp src/case1.cpp src/case2.cpp src/case3.cpp src/case4.cpp -o build/etx_routing.exe
```

## Probability input order

- Case 1: AB, AC.
- Case 2: AB, AC, BD, BE, CD, CE.
- Case 3: AB, AC, AD, BE, CE, CF, DF.
- Case 4: AB, AC, AD, BE, BF, CF, CG, DF, DG.

Example from PowerShell:

```powershell
Get-Content examples/case3.txt | & ./build/etx_routing.exe
```

## How ETX is calculated

A single link with probability p has ETX = 1/p.
For a broadcast to two receivers with probabilities p and q:

```text
x = 1 + (1-p)(1-q)x + p(1-q)/q + (1-p)q/p
```

The leading 1 counts the current broadcast. bothETX() rearranges this equation.

For the multi-hop cases, the forwarding table tracks who already holds the packet.
Case 2 uses four flags B,C,D,E (16 configurations).
Case 3 uses five flags B,C,D,E,F (32 configurations).
Case 4 uses six flags B,C,D,E,F,G (64 configurations).

For example, in case 3, remaining[1][0][1][0][0] means B and D have the packet, while C,E,F do not. A always holds a copy.

For each configuration, try every allowed sender and enumerate the reception outcomes.
A relay can only send after receiving the packet. Choose the smallest expected cost:

```text
sender cost = (1 + sum of outcome probability * next ETX for changed configurations)
              / probability that the configuration changes
```

This accounts for retries that leave the holders unchanged. The loops work backwards because a reception only changes a flag from 0 to 1.

Case 3's A broadcast has eight outcomes because it has three receivers.
B and D each have two outcomes because each has one receiver; C has four outcomes.
The code uses the same three outcome loops for every sender, assigning probability zero to unused receiver slots.

The joint ETX stops when every final destination has received the packet: two targets in cases 1-3, and all three E,F,G in case 4.
The E-only/F-only or D-only/E-only rows are separate goals: do not add them.

The local relay tables describe what that relay can deliver through its outgoing links alone. B cannot deliver case 3 to F, and D cannot deliver it to E. They can still cooperate to finish the joint goal.

Some printed holder configurations are hypothetical for probabilities equal to zero or one.
Exact ties prefer A, then B, then C, then D where applicable.

## Assumptions

- One broadcast counts as one transmission, regardless of how many receivers succeed.
- Attempts and receptions at different receivers are independent.
- Copies are retained and retries are unlimited.
- One node transmits at a time.
- Only the directed links specified for the chosen case exist.
- The calculation knows who has the packet; ACK and coordination costs are excluded.
- Final ETX counts total network transmissions, including all relays.
- Zero probability means an unavailable link. Impossible delivery prints unreachable.

## Hand-checkable examples

- Case 1, both probabilities 0.5: ETX = 2.666667.
- Case 2, all probabilities 0.5: ETX = 4.
- Case 3, all probabilities 1: ETX = 2 (A broadcasts, then C broadcasts).
- Case 3, all probabilities 0.5: ETX = 4.507937.
- Case 3, AB=AD=BE=DF=1, AC=CE=CF=0: ETX = 3 (A, B and D transmit).


## Case 4 examples

- All nine probabilities equal to 1: ETX = 3. A broadcasts once, then B and either C or D broadcast.
- If AB=0 or BE=0, E is unreachable and the joint ETX is unreachable, even if F and G remain reachable.
- A broadcast has eight reception outcomes. Each relay broadcast has four. The same retry equation is used for each possible sender.
