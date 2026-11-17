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
- src/simulation.cpp: packet-level simulation of all four networks (see "Simulation" below).
- src/ran2.cpp, include/ran2.h: random number generator copied unchanged from ncsimulation8july.cpp.
- plot_results.py: draws the running-average plot from the simulation's CSV output.
- examples/: input and captured output for each case. The simulation reads its link probabilities from here.
- results/: output of `simulation --packets 10000 --seed 42 --verbose`. Start with results/report.txt.
- build.bat: compiles every case into build/etx_routing.exe and the simulation into build/simulation.exe.
- build.sh: the same for macOS/Linux (build/etx_routing and build/simulation).

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

## Simulation

`src/simulation.cpp` sends packets through all four networks, one transmission at a time.
It counts how many transmissions each packet needs and compares the average with the theoretical ETX that the code above calculates.

### Build and run

Windows (PowerShell):

```powershell
.\build.bat
.\build\simulation.exe --packets 10000 --seed 42
```

macOS / Linux:

```sh
./build.sh
./build/simulation --packets 10000 --seed 42
```

Options:

| Option | Meaning | Default |
|---|---|---|
| `--packets N` | packets simulated per network | 10000 |
| `--seed S` | seed for the random number generator, 1 to 2147483646 | 42 |
| `--verbose` | log the first packets of each network step by step | off |
| `--verbose-packets N` | how many packets `--verbose` logs | 10 |
| `--log-random` | write every generated random number to `random_numbers.csv` | off |
| `--examples DIR` | folder with case1.txt ... case4.txt (link probabilities) | examples |
| `--out DIR` | folder for report.txt and the CSV files | results |

The same seed and packet count always give the same output.

To draw the plot you need Python 3 with matplotlib. Install matplotlib in a
virtual environment (`.venv`, ignored by git) inside this folder. Homebrew Python
on macOS and many Linux distributions refuse a plain `pip install` outside one.
You only need to do this once.

macOS / Linux:

```sh
python3 -m venv .venv
.venv/bin/pip install matplotlib
.venv/bin/python plot_results.py   # reads and writes the results/ folder
```

Windows (PowerShell):

```powershell
python -m venv .venv
.venv\Scripts\pip install matplotlib
.venv\Scripts\python plot_results.py
```

### How it works

1. **Cases.** The case code supplies the outgoing links of every sender through
   `case2SenderLinks`, `case3SenderLinks` and `case4SenderLinks`. The ETX calculation
   uses the same functions. A sender with k receivers has 2^k cases, because each
   receiver either gets the packet or not. A case's probability is the product of
   p or (1 - p) for each link. The program checks that each sender's cases add up
   to 1 and stops with an error if they do not.
2. **Number line.** The cases are laid end to end on [0, 1). Each one gets an
   interval as long as its probability:

   ```text
   A -> {B,C} with AB = 0.8, AC = 0.7
   0        0.06           0.20                0.44                          1
   |--none--|----C only----|------B only-------|-------------both------------|
     case 0      case 1           case 2                  case 3
   ```

   A uniform random number r lands in an interval with probability equal to its
   length, so picking "the interval that contains r" picks each case with exactly
   its probability. The program prints this table for every sender (case,
   probability, interval start, interval end).
3. **Random numbers.** r comes from `randomgeneration()`, copied unchanged from
   ncsimulation8july.cpp. It is ran2 from *Numerical Recipes* and returns a float
   in (0, 1), so no normalisation is needed. The seed is handled the same way as
   in that file: it passes `seed`, then `seed + T`, `seed + 2T`, and so on, with
   T = time(NULL). The simulation uses `--seed` as both the start and T, so a run
   can be repeated. The running sum is kept in 32 bits, as it was when the
   original ran on Windows. With a 64-bit `long` (macOS/Linux) the sum would grow
   past 2147483563, which `randomgeneration()` cannot handle: its shuffle-table
   index goes negative and the program crashes.

   One random number is generated per transmission, and the generator runs as
   one stream through networks 1, 2, 3 and 4 in that order. With `--seed 42`,
   the values passed are 42, 84, 126, ..., and the first numbers returned are:

   | draw | value passed | random number | network 1 case (A -> B, C) |
   |---|---|---|---|
   | 1 | 42 | 0.65541637 | case 2 (B=1 C=0) |
   | 2 | 84 | 0.20099519 | case 0 (B=0 C=0) |
   | 3 | 126 | 0.89362240 | case 3 (B=1 C=1) |

   `--verbose` shows each number as it is drawn, and `--log-random` saves all
   of them to `random_numbers.csv`.
4. **Forwarding.** The ETX code computes `remaining` and `nextSender` for every
   "who holds the packet" state. The simulation gets copies of both through
   optional arguments added to `calculateRouting`, `calculateCase3` and
   `calculateCase4`. Case 1 has no relays: A is the only sender, and its
   remaining ETX comes from `bothETX`/`linkETX`. On each step the simulation:
   - looks up the sender for the current state;
   - draws r and finds the case;
   - adds that case's receivers to the holders;
   - repeats until every destination of the network has the packet.

   If nobody new receives, the state does not change, so the same sender
   transmits again.

**Difference from a "best receiver forwards, the others drop" model.** In this
project, every node that receives the packet keeps its copy (see Assumptions).
The next sender is the holder whose broadcast gives the lowest remaining ETX
for the current set of holders, as in the notes' V_B, V_C and V_BC. A relay that
is not chosen now can still be chosen later; for example, in network 4, B
reaches E and then D finishes G. The theoretical ETX assumes exactly this
behaviour, so the simulation follows it too. If lower-priority receivers dropped
the packet, the simulated average would not converge to these ETX values.

Networks 1 to 3 have two destinations; network 4 has three (E, F, G). Each
network is simulated until all of its own destinations have the packet.

### Output

The output is a report that someone new to the project can follow from top to
bottom. Everything printed on screen is also saved to `results/report.txt`. The
committed copy comes from `./build/simulation --packets 10000 --seed 42 --verbose`.

1. **Introduction:** the purpose of the simulation, the method in five steps,
   and the settings used (packets, seed, input files).
2. **One section per network** (NETWORK 1 of 4 ... NETWORK 4 of 4):
   - links, source and destinations;
   - link success probabilities and their link ETX (1/p);
   - **STEP 1:** for every sender, a table of its reception cases (1 = received,
     0 = not), each case's probability and its interval on [0, 1), with the
     check that the probabilities sum to 1;
   - **STEP 2** (only with `--verbose`): the first packets, one table row per
     transmission. Each row shows the sender, which random number was generated
     (draw number, value passed to `randomgeneration()`, number returned), the
     interval it fell in and the case chosen, who received, and who holds the
     packet afterwards;
   - **STEP 2/3, result:**
     - the theoretical ETX next to the simulated average and its 95% confidence
       interval;
     - the difference, and a verdict: AGREES if within 3 standard errors,
       otherwise DOES NOT AGREE;
     - the average transmissions by each sender, which add up to the total;
     - for each destination, the average transmission number at which it first
       has the packet.
3. **Summary:**
   - one table comparing all four networks;
   - the total number of random numbers generated;
   - a conclusion;
   - the list of output files.

To keep a copy of the screen output under another name, you can still redirect it:

```sh
./build/simulation --packets 10000 --seed 42 --verbose > my_report.txt
```

These files are written to `results/`:

| File | Contents |
|---|---|
| report.txt | the full report described above, the same text as on screen |
| summary.csv | network, destinations, packets, seed, theoretical ETX, simulated mean, standard error, abs and % difference |
| per_node.csv | average transmissions per packet by each sender |
| per_destination.csv | average transmissions until each destination has the packet |
| case_intervals.csv | every sender's cases, probabilities and intervals |
| running_average.csv | running average after each packet (used for the plot; every packet up to 1000, then about 10000 points) |
| running_average.png | made by plot_results.py: running average per network with the theoretical ETX as a dashed line |
| random_numbers.csv | only with `--log-random`: one row per generated number with draw number, network, packet, transmission, sender, value passed to `randomgeneration()`, random number, case, received pattern and receivers (space-separated). About 6 MB for 10000 packets, so git ignores it |

### Results (10000 packets, seed 42, probabilities from examples/)

| Network | Destinations | Theoretical ETX | Simulated average | Abs diff | % diff |
|---|---|---|---|---|---|
| 1 | B, C | 2.666667 | 2.672000 | 0.005333 | 0.200% |
| 2 | D, E | 3.031289 | 3.037600 | 0.006311 | 0.208% |
| 3 | E, F | 2.901741 | 2.889000 | 0.012741 | 0.439% |
| 4 | E, F, G | 3.661859 | 3.675600 | 0.013741 | 0.375% |

All four are within 1.6 standard errors of the ETX. The leftover difference is
sampling noise, which shrinks like 1/sqrt(packets). With 1,000,000 packets, every
network was within 0.07% of its ETX for each of the seeds 7, 42, 99991, 123457
and 2147483646.
