// Packet-level simulation of opportunistic routing over the four networks.
//
// For every network this program:
//   1. takes each sender's reception cases and their probabilities from the
//      existing case code (caseNSenderLinks) and checks they sum to 1;
//   2. lays the cases out on the number line [0, 1) as consecutive intervals;
//   3. draws random numbers with randomgeneration() from
//      ncsimulation8july.cpp to pick a case for every broadcast;
//   4. forwards the packet with the forwarding table that the existing ETX
//      code already computes (calculateRouting / calculateCase3 / calculateCase4);
//   5. counts transmissions and compares the average with the theoretical ETX.
//
// Build:  see build.sh / build.bat
// Run:    ./build/simulation --packets 10000 --seed 42

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include "etx.h"
#include "case2.h"
#include "case3.h"
#include "case4.h"
#include "ran2.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

// ---------------------------------------------------------------------------
// Random numbers
// ---------------------------------------------------------------------------

// ncsimulation8july.cpp calls the generator like this:
//
//     static long seed = time(NULL);
//     r = randomgeneration(seed);
//     seed = seed + time(NULL);
//
// We keep exactly that pattern, but use the --seed value in place of
// time(NULL) so that a run can be repeated: the k-th call passes k * seed.
// The seed must be positive. With seed <= 0 from the start, randomgeneration()
// would re-initialise itself on every call and keep returning the same number.
//
// The running sum is kept in 32 bits, as in the original program. That program
// was written for Windows, where long is 32 bits: seed + time(NULL) wraps
// around and often becomes negative, and a negative value makes
// randomgeneration() re-initialise from it. With a 64-bit long (Linux/macOS)
// the sum would instead keep growing past 2147483563, which
// randomgeneration() cannot handle: its shuffle-table index goes negative and
// the program crashes. Wrapping explicitly at 32 bits reproduces the original
// Windows behaviour and gives the same random numbers on every platform.
static unsigned int g_seed = 1;     // unsigned: wrapping is well defined
static unsigned int g_seedStep = 1;

void setSeed(long seed) {
    g_seed = (unsigned int)seed;
    g_seedStep = (unsigned int)seed;
}

long seedAsSigned32(unsigned int value) {
    // Read the 32 bits as a signed number, like a 32-bit long would hold them.
    if (value <= 2147483647u) return (long)value;
    return (long)value - 4294967296LL;
}

// Returns the next random number. If seedUsed is given, it receives the value
// that was passed to randomgeneration() for this draw.
static long g_drawCount = 0;        // how many numbers have been generated

double drawRandom(long* seedUsed = 0) {
    long seedNow = seedAsSigned32(g_seed);
    float r = randomgeneration(seedNow);
    g_seed = g_seed + g_seedStep;   // seed = seed + time(NULL) in the original
    g_drawCount++;
    if (seedUsed != 0) *seedUsed = seedNow;
    return r;
}

// ---------------------------------------------------------------------------
// Network description
// ---------------------------------------------------------------------------

// One sender (A, B, C or D) and the cases of its broadcast.
struct Sender {
    string name;
    int count;                // number of receivers of this sender
    int receiver[3];          // receiver numbers (index into Network::flagNames)
    double probability[3];    // link success probabilities
    vector<double> caseProbability;
    vector<double> intervalStart;
    vector<double> intervalEnd;
};

struct Network {
    int id;
    string description;
    vector<string> linkNames;     // input order, as in the README
    vector<double> p;             // link probabilities
    vector<string> flagNames;     // nodes other than A, in table order
    vector<int> destinations;     // flag numbers of the final destinations
    vector<Sender> senders;       // sender 0 is always A
    vector<double> remaining;     // remaining ETX for every holder state
    vector<int> nextSender;       // chosen sender for every holder state
    double theoreticalETX;
};

// A "state" says which nodes (other than A) hold the packet.
// It is the same index the C arrays use: for case 3, remaining[b][c][d][e][f]
// is element 16*b + 8*c + 4*d + 2*e + f. So the first flag (B) is the most
// significant bit.
int flagBit(const Network& net, int flag) {
    return 1 << ((int)net.flagNames.size() - 1 - flag);
}

bool hasPacket(const Network& net, int state, int flag) {
    return (state & flagBit(net, flag)) != 0;
}

bool allDestinationsServed(const Network& net, int state) {
    for (size_t i = 0; i < net.destinations.size(); i++) {
        if (!hasPacket(net, state, net.destinations[i])) return false;
    }
    return true;
}

string holdersText(const Network& net, int state) {
    string text = "{A";
    for (size_t i = 0; i < net.flagNames.size(); i++) {
        if (hasPacket(net, state, (int)i)) text += "," + net.flagNames[i];
    }
    return text + "}";
}

// Reads examples/caseN.txt: first the case number, then the probabilities.
bool readProbabilities(const string& folder, Network& net) {
    ostringstream path;
    path << folder << "/case" << net.id << ".txt";
    ifstream in(path.str().c_str());
    if (!in) {
        cerr << "Error: cannot open " << path.str() << "\n";
        return false;
    }
    int caseNumber = 0;
    in >> caseNumber;
    if (!in || caseNumber != net.id) {
        cerr << "Error: " << path.str() << " should start with " << net.id << "\n";
        return false;
    }
    net.p.assign(net.linkNames.size(), 0);
    for (size_t i = 0; i < net.linkNames.size(); i++) {
        in >> net.p[i];
        if (!in || net.p[i] < 0 || net.p[i] > 1) {
            cerr << "Error: " << path.str() << " needs a probability in [0, 1] for "
                 << net.linkNames[i] << "\n";
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// The number line (interval) mapping
// ---------------------------------------------------------------------------

// A broadcast by a sender with k receivers has 2^k cases: every receiver
// either gets the packet (1) or not (0). Case number n is written in binary
// with the first receiver as the most significant bit, so for A -> {B,C}:
//     case 0 = B0 C0, case 1 = B0 C1, case 2 = B1 C0, case 3 = B1 C1
// (the same order as the tables in the notes). Receptions are independent,
// so a case's probability is the product of p or (1 - p) for each link.
//
// The cases are then placed one after another on the number line [0, 1):
//
//     0        P0       P0+P1        P0+P1+P2        1
//     |---c0---|---c1----|-----c2-----|------c3------|
//
// Case n owns the interval [start_n, end_n) where start_n is the sum of the
// probabilities of all earlier cases and end_n = start_n + P(case n).
// Each interval is exactly as long as its case's probability, and together
// they cover [0, 1) with no gaps or overlaps. A uniform random number r in
// [0, 1) therefore lands in case n's interval with probability P(case n).
//
// (ncsimulation8july.cpp builds the same cumulative slots x[0], x[1], ... and
// tests x[i-1] < r <= x[i]. Whether the boundary point belongs to the left or
// the right interval makes no difference: r hits an exact boundary with
// probability 0.)
bool buildIntervals(Sender& s, const string& networkName) {
    int cases = 1 << s.count;
    s.caseProbability.assign(cases, 0);
    s.intervalStart.assign(cases, 0);
    s.intervalEnd.assign(cases, 0);

    double sum = 0;
    for (int n = 0; n < cases; n++) {
        double chance = 1;
        for (int i = 0; i < s.count; i++) {
            bool received = ((n >> (s.count - 1 - i)) & 1) == 1;
            if (received) chance = chance * s.probability[i];
            else chance = chance * (1 - s.probability[i]);
        }
        s.caseProbability[n] = chance;
        s.intervalStart[n] = sum;     // where this case begins on the line
        sum = sum + chance;
        s.intervalEnd[n] = sum;       // where it ends = start + its probability
    }

    // The cases must cover the whole line: their probabilities sum to 1.
    if (fabs(sum - 1) > 1e-9) {
        cerr << "Error: in network " << networkName << ", the cases of sender "
             << s.name << " have total probability " << setprecision(12) << sum
             << " instead of 1.\n";
        return false;
    }
    // Remove floating-point rounding so the last interval ends exactly at 1
    // and every r < 1 is guaranteed to fall in some interval.
    s.intervalEnd[cases - 1] = 1.0;
    return true;
}

// Find which interval the random number r lies in. That interval's case
// is what happens in this transmission. Cases with probability 0 have an
// empty interval (start == end) and can never be chosen.
int findCase(const Sender& s, double r) {
    int cases = (int)s.caseProbability.size();
    for (int n = 0; n < cases; n++) {
        if (r >= s.intervalStart[n] && r < s.intervalEnd[n]) return n;
    }
    return cases - 1; // not reached for 0 <= r < 1
}

string caseBits(const Network& net, const Sender& s, int n) {
    string text;
    for (int i = 0; i < s.count; i++) {
        bool received = ((n >> (s.count - 1 - i)) & 1) == 1;
        if (i > 0) text += " ";
        text += net.flagNames[s.receiver[i]] + (received ? "=1" : "=0");
    }
    return text;
}

string receiversText(const Network& net, const Sender& s, int n) {
    string text;
    for (int i = 0; i < s.count; i++) {
        if (((n >> (s.count - 1 - i)) & 1) == 1) {
            if (!text.empty()) text += ",";
            text += net.flagNames[s.receiver[i]];
        }
    }
    return text.empty() ? "nobody" : text;
}

// ---------------------------------------------------------------------------
// Building the four networks from the existing code
// ---------------------------------------------------------------------------

typedef int (*LinksFunction)(int, double[], int[3], double[3]);

// Case 1 has no relays, so the existing code has no forwarding table for it.
// A is the only sender. Its remaining ETX per state uses the existing
// bothETX()/linkETX() functions from etx.cpp.
int case1SenderLinks(int /*sender*/, double p[], int receiver[3],
                     double probability[3]) {
    receiver[0] = 0; probability[0] = p[0]; // A -> B
    receiver[1] = 1; probability[1] = p[1]; // A -> C
    receiver[2] = 0; probability[2] = 0;
    return 2;
}

bool buildNetwork(int id, const string& examplesFolder, Network& net) {
    net.id = id;
    LinksFunction links = 0;
    const char* senderNames[4] = {"A", "B", "C", "D"};
    int senderCount = 0;

    if (id == 1) {
        net.description = "A -> {B,C}: deliver to BOTH B and C";
        const char* l[] = {"A -> B", "A -> C"};
        net.linkNames.assign(l, l + 2);
        const char* f[] = {"B", "C"};
        net.flagNames.assign(f, f + 2);
        net.destinations.push_back(0);
        net.destinations.push_back(1);
        senderCount = 1;
        links = case1SenderLinks;
    } else if (id == 2) {
        net.description = "A -> {B,C}, B/C -> {D,E}: deliver to BOTH D and E";
        const char* l[] = {"A -> B", "A -> C", "B -> D", "B -> E", "C -> D", "C -> E"};
        net.linkNames.assign(l, l + 6);
        const char* f[] = {"B", "C", "D", "E"};
        net.flagNames.assign(f, f + 4);
        net.destinations.push_back(2);
        net.destinations.push_back(3);
        senderCount = 3;
        links = case2SenderLinks;
    } else if (id == 3) {
        net.description = "A -> {B,C,D}, B -> E, C -> {E,F}, D -> F: deliver to BOTH E and F";
        const char* l[] = {"A -> B", "A -> C", "A -> D", "B -> E", "C -> E", "C -> F", "D -> F"};
        net.linkNames.assign(l, l + 7);
        const char* f[] = {"B", "C", "D", "E", "F"};
        net.flagNames.assign(f, f + 5);
        net.destinations.push_back(3);
        net.destinations.push_back(4);
        senderCount = 4;
        links = case3SenderLinks;
    } else {
        net.description = "A -> {B,C,D}, B -> {E,F}, C/D -> {F,G}: deliver to ALL E, F and G";
        const char* l[] = {"A -> B", "A -> C", "A -> D", "B -> E", "B -> F",
                           "C -> F", "C -> G", "D -> F", "D -> G"};
        net.linkNames.assign(l, l + 9);
        const char* f[] = {"B", "C", "D", "E", "F", "G"};
        net.flagNames.assign(f, f + 6);
        net.destinations.push_back(3);
        net.destinations.push_back(4);
        net.destinations.push_back(5);
        senderCount = 4;
        links = case4SenderLinks;
    }

    if (!readProbabilities(examplesFolder, net)) return false;

    ostringstream name;
    name << id;
    for (int s = 0; s < senderCount; s++) {
        Sender sender;
        sender.name = senderNames[s];
        sender.count = links(s, &net.p[0], sender.receiver, sender.probability);
        if (!buildIntervals(sender, name.str())) return false;
        net.senders.push_back(sender);
    }

    // Forwarding table from the existing ETX code.
    int states = 1 << net.flagNames.size();
    net.remaining.assign(states, INF);
    net.nextSender.assign(states, -1);
    if (id == 1) {
        // index = 2*b + c
        net.remaining[0] = bothETX(net.p[0], net.p[1]); // nobody has it
        net.remaining[1] = linkETX(net.p[0]);           // only C has it
        net.remaining[2] = linkETX(net.p[1]);           // only B has it
        net.remaining[3] = 0;                           // done
        for (int st = 0; st < 3; st++) {
            if (net.remaining[st] != INF) net.nextSender[st] = 0;
        }
        net.theoreticalETX = net.remaining[0];
    } else if (id == 2) {
        net.theoreticalETX = calculateRouting(&net.p[0], true, true, false,
                                              &net.remaining[0], &net.nextSender[0]);
    } else if (id == 3) {
        net.theoreticalETX = calculateCase3(&net.p[0], true, true, false,
                                            &net.remaining[0], &net.nextSender[0]);
    } else {
        net.theoreticalETX = calculateCase4(&net.p[0], true, true, true, false,
                                            &net.remaining[0], &net.nextSender[0]);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

struct Result {
    long packets;
    double mean;
    double standardError;
    vector<double> perSender;      // average transmissions by each sender
    vector<double> perDestination; // average transmissions until it got the packet
    vector<long> runningPacket;    // packet numbers saved for the plot
    vector<double> runningAverage;
};

void printIntervalTables(const Network& net, ofstream& csv) {
    for (size_t s = 0; s < net.senders.size(); s++) {
        const Sender& sender = net.senders[s];
        cout << "\nSender " << sender.name << " broadcasts to ";
        for (int i = 0; i < sender.count; i++) {
            if (i > 0) cout << ",";
            cout << net.flagNames[sender.receiver[i]];
        }
        cout << " (" << sender.caseProbability.size() << " cases)\n";
        cout << "case  received        probability  interval start  interval end\n";
        double total = 0;
        for (size_t n = 0; n < sender.caseProbability.size(); n++) {
            string bits = caseBits(net, sender, (int)n);
            cout << setw(4) << n << "  " << setw(14) << left << bits << right
                 << setw(13) << sender.caseProbability[n]
                 << setw(16) << sender.intervalStart[n]
                 << setw(14) << sender.intervalEnd[n] << "\n";
            csv << net.id << "," << sender.name << "," << n << "," << bits << ","
                << setprecision(10) << sender.caseProbability[n] << ","
                << sender.intervalStart[n] << "," << sender.intervalEnd[n]
                << setprecision(6) << "\n";
            total = total + sender.caseProbability[n];
        }
        cout << "      sum of probabilities = " << total << "  (checked: equals 1)\n";
    }
}

// randomLog: if not null, every generated random number is written to it.
bool simulateNetwork(const Network& net, long packets, int verbosePackets,
                     Result& result, ofstream* randomLog) {
    int senderCount = (int)net.senders.size();
    int destinationCount = (int)net.destinations.size();
    vector<double> senderTotal(senderCount, 0);
    vector<double> destinationTotal(destinationCount, 0);
    double sum = 0;
    double sumSquares = 0;

    // Save every packet for the first 1000, then about 10000 points in all.
    long stride = packets / 10000;
    if (stride < 1) stride = 1;

    result.runningPacket.clear();
    result.runningAverage.clear();

    for (long packet = 1; packet <= packets; packet++) {
        bool verbose = packet <= verbosePackets;
        if (verbose) cout << "\nPacket " << packet << "\n";

        int state = 0;          // only A has the packet
        long transmissions = 0;
        vector<long> deliveredAt(destinationCount, 0);

        while (!allDestinationsServed(net, state)) {
            // The forwarding table names the holder whose broadcast gives the
            // lowest remaining ETX. Every holder keeps its copy; a relay that
            // is not chosen now may still be chosen in a later state.
            int s = net.nextSender[state];
            if (s < 0) {
                cerr << "Error: network " << net.id << " cannot deliver to every "
                     << "destination from holders " << holdersText(net, state)
                     << " (a needed link has probability 0).\n";
                return false;
            }
            const Sender& sender = net.senders[s];

            transmissions++;
            senderTotal[s] += 1;

            // Pick the case: draw r and find the interval that contains it.
            long seedUsed = 0;
            double r = drawRandom(&seedUsed);
            int n = findCase(sender, r);

            if (randomLog != 0) {
                // Receivers are separated by spaces so the CSV keeps its columns.
                string receivers = receiversText(net, sender, n);
                for (size_t k = 0; k < receivers.size(); k++) {
                    if (receivers[k] == ',') receivers[k] = ' ';
                }
                *randomLog << g_drawCount << "," << net.id << "," << packet << ","
                           << transmissions << "," << sender.name << "," << seedUsed
                           << "," << setprecision(8) << r << "," << n << ","
                           << caseBits(net, sender, n) << "," << receivers << "\n";
            }

            int newState = state;
            for (int i = 0; i < sender.count; i++) {
                if (((n >> (sender.count - 1 - i)) & 1) == 1) {
                    newState = newState | flagBit(net, sender.receiver[i]);
                }
            }
            for (int d = 0; d < destinationCount; d++) {
                int flag = net.destinations[d];
                if (!hasPacket(net, state, flag) && hasPacket(net, newState, flag)) {
                    deliveredAt[d] = transmissions;
                }
            }

            if (verbose) {
                cout << "  tx " << transmissions << ": holders " << holdersText(net, state)
                     << ", sender " << sender.name << " (remaining ETX "
                     << net.remaining[state] << ")\n";
                cout << "        random number #" << g_drawCount
                     << ": randomgeneration(" << seedUsed << ") = "
                     << setprecision(8) << r << setprecision(6) << "\n";
                cout << "        r = " << setprecision(8) << r << setprecision(6)
                     << " lies in [" << sender.intervalStart[n] << ", "
                     << sender.intervalEnd[n] << ") -> case " << n << " ("
                     << caseBits(net, sender, n) << ")\n";
                cout << "        received by: " << receiversText(net, sender, n);
                if (newState == state) {
                    cout << " (no new holder, " << sender.name << " must transmit again)\n";
                } else {
                    cout << " -> holders " << holdersText(net, newState) << "\n";
                }
                if (allDestinationsServed(net, newState)) {
                    cout << "        all destinations have the packet: done after "
                         << transmissions << " transmissions\n";
                } else {
                    int next = net.nextSender[newState];
                    cout << "        next forwarder: "
                         << (next < 0 ? string("none") : net.senders[next].name)
                         << " (lowest remaining ETX " << net.remaining[newState] << ")\n";
                }
            }
            state = newState;
        }

        sum += transmissions;
        sumSquares += (double)transmissions * transmissions;
        for (int d = 0; d < destinationCount; d++) destinationTotal[d] += deliveredAt[d];

        if (packet <= 1000 || packet % stride == 0 || packet == packets) {
            result.runningPacket.push_back(packet);
            result.runningAverage.push_back(sum / packet);
        }
    }

    result.packets = packets;
    result.mean = sum / packets;
    double variance = 0;
    if (packets > 1) {
        variance = (sumSquares - packets * result.mean * result.mean) / (packets - 1);
        if (variance < 0) variance = 0;
    }
    result.standardError = sqrt(variance / packets);
    result.perSender.assign(senderCount, 0);
    for (int s = 0; s < senderCount; s++) result.perSender[s] = senderTotal[s] / packets;
    result.perDestination.assign(destinationCount, 0);
    for (int d = 0; d < destinationCount; d++) {
        result.perDestination[d] = destinationTotal[d] / packets;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Command line and output
// ---------------------------------------------------------------------------

void printUsage() {
    cout << "Usage: simulation [--packets N] [--seed S] [--verbose] [--verbose-packets N]\n"
         << "                  [--log-random] [--examples DIR] [--out DIR]\n\n"
         << "  --packets N          packets per network (default 10000)\n"
         << "  --seed S             positive seed for randomgeneration() (default 42)\n"
         << "  --verbose            log the first packets of each network step by step\n"
         << "  --verbose-packets N  how many packets --verbose logs (default 10)\n"
         << "  --log-random         write every generated random number to\n"
         << "                       OUT/random_numbers.csv\n"
         << "  --examples DIR       folder with case1.txt..case4.txt (default examples)\n"
         << "  --out DIR            folder for CSV output (default results)\n";
}

bool parseLong(const char* text, long& value) {
    char* end = 0;
    value = strtol(text, &end, 10);
    return end != text && *end == '\0';
}

void makeFolder(const string& folder) {
#ifdef _WIN32
    _mkdir(folder.c_str());
#else
    mkdir(folder.c_str(), 0755);
#endif
}

int main(int argc, char* argv[]) {
    long packets = 10000;
    long seed = 42;
    bool verbose = false;
    bool logRandom = false;
    long verbosePackets = 10;
    string examplesFolder = "examples";
    string outFolder = "results";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        bool hasValue = i + 1 < argc;
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--log-random") {
            logRandom = true;
        } else if (arg == "--packets" && hasValue) {
            if (!parseLong(argv[++i], packets) || packets < 1) {
                cerr << "Error: --packets must be a positive whole number.\n";
                return 1;
            }
        } else if (arg == "--seed" && hasValue) {
            if (!parseLong(argv[++i], seed) || seed < 1 || seed > 2147483646L) {
                cerr << "Error: --seed must be a whole number from 1 to 2147483646.\n";
                return 1;
            }
        } else if (arg == "--verbose-packets" && hasValue) {
            if (!parseLong(argv[++i], verbosePackets) || verbosePackets < 0) {
                cerr << "Error: --verbose-packets must be 0 or more.\n";
                return 1;
            }
        } else if (arg == "--examples" && hasValue) {
            examplesFolder = argv[++i];
        } else if (arg == "--out" && hasValue) {
            outFolder = argv[++i];
        } else {
            cerr << "Error: unknown or incomplete option " << arg << "\n\n";
            printUsage();
            return 1;
        }
    }
    if (!verbose) verbosePackets = 0;

    cout << fixed << setprecision(6);

    // Read the probabilities and build every network (with its probability
    // check) before opening any output file, so a bad input never wipes out
    // earlier results.
    vector<Network> networks(4);
    vector<Result> results(4);
    for (int id = 1; id <= 4; id++) {
        if (!buildNetwork(id, examplesFolder, networks[id - 1])) return 1;
    }

    makeFolder(outFolder);
    ofstream intervalsCsv((outFolder + "/case_intervals.csv").c_str());
    ofstream summaryCsv((outFolder + "/summary.csv").c_str());
    ofstream nodeCsv((outFolder + "/per_node.csv").c_str());
    ofstream destinationCsv((outFolder + "/per_destination.csv").c_str());
    ofstream runningCsv((outFolder + "/running_average.csv").c_str());
    if (!intervalsCsv || !summaryCsv || !nodeCsv || !destinationCsv || !runningCsv) {
        cerr << "Error: cannot write CSV files in " << outFolder << "\n";
        return 1;
    }
    intervalsCsv << "network,sender,case,received,probability,interval_start,interval_end\n";
    summaryCsv << "network,destinations,packets,seed,theoretical_etx,simulated_mean,"
                  "standard_error,abs_difference,percent_difference\n";
    nodeCsv << "network,sender,avg_transmissions_per_packet\n";
    destinationCsv << "network,destination,avg_transmissions_until_received\n";
    runningCsv << "network,packet,running_average,theoretical_etx\n";
    summaryCsv << fixed << setprecision(6);
    nodeCsv << fixed << setprecision(6);
    destinationCsv << fixed << setprecision(6);
    runningCsv << fixed << setprecision(6);

    // Optional log of every random number: which value was passed to
    // randomgeneration(), the number it returned and the case it selected.
    ofstream randomCsv;
    if (logRandom) {
        randomCsv.open((outFolder + "/random_numbers.csv").c_str());
        if (!randomCsv) {
            cerr << "Error: cannot write " << outFolder << "/random_numbers.csv\n";
            return 1;
        }
        randomCsv << "draw,network,packet,transmission,sender,seed_passed,"
                     "random_number,case,received,receivers\n";
        randomCsv << fixed;
    }

    cout << "Opportunistic routing simulation: " << packets << " packets per network, seed "
         << seed << "\n";
    setSeed(seed);

    for (int id = 1; id <= 4; id++) {
        Network& net = networks[id - 1];

        cout << "\n==================================================================\n";
        cout << "NETWORK " << id << ": " << net.description << "\n";
        cout << "Link probabilities (" << examplesFolder << "/case" << id << ".txt): ";
        for (size_t i = 0; i < net.p.size(); i++) {
            cout << (i ? ", " : "") << net.linkNames[i] << " = " << setprecision(2)
                 << net.p[i] << setprecision(6);
        }
        cout << "\n\nCASE INTERVALS on [0, 1)";
        printIntervalTables(net, intervalsCsv);

        if (net.theoreticalETX == INF) {
            cerr << "Error: network " << id << " cannot reach every destination.\n";
            return 1;
        }

        Result& res = results[id - 1];
        if (!simulateNetwork(net, packets, (int)verbosePackets, res,
                             logRandom ? &randomCsv : 0)) return 1;

        double difference = res.mean - net.theoreticalETX;
        double percent = 100 * difference / net.theoreticalETX;
        cout << "\nRESULT\n";
        cout << "Theoretical ETX (existing code):  " << net.theoreticalETX << "\n";
        cout << "Simulated average transmissions:  " << res.mean << "  (95% CI +/- "
             << 1.96 * res.standardError << ")\n";
        cout << "Difference:                       " << fabs(difference) << "  ("
             << setprecision(3) << fabs(percent) << "%)" << setprecision(6) << "\n";
        double z = res.standardError > 0 ? fabs(difference) / res.standardError : 0;
        if (z <= 3) {
            cout << "Within " << setprecision(2) << z << setprecision(6)
                 << " standard errors of the ETX: consistent with convergence.\n";
        } else {
            cout << "WARNING: " << setprecision(2) << z << setprecision(6)
                 << " standard errors away from the ETX. Investigate.\n";
        }

        cout << "Average transmissions per packet by each sender:";
        for (size_t s = 0; s < net.senders.size(); s++) {
            cout << "  " << net.senders[s].name << " = " << res.perSender[s];
            nodeCsv << id << "," << net.senders[s].name << "," << res.perSender[s] << "\n";
        }
        cout << "\nAverage transmissions until each destination first has the packet:";
        for (size_t d = 0; d < net.destinations.size(); d++) {
            string name = net.flagNames[net.destinations[d]];
            cout << "  " << name << " = " << res.perDestination[d];
            destinationCsv << id << "," << name << "," << res.perDestination[d] << "\n";
        }
        cout << "\n";

        string destinationList;
        for (size_t d = 0; d < net.destinations.size(); d++) {
            destinationList += (d ? " " : "") + net.flagNames[net.destinations[d]];
        }
        summaryCsv << id << "," << destinationList << "," << packets << "," << seed << ","
                   << net.theoreticalETX << "," << res.mean << "," << res.standardError << ","
                   << fabs(difference) << "," << fabs(percent) << "\n";
        for (size_t k = 0; k < res.runningPacket.size(); k++) {
            runningCsv << id << "," << res.runningPacket[k] << "," << res.runningAverage[k]
                       << "," << net.theoreticalETX << "\n";
        }
    }

    cout << "\n==================================================================\n";
    cout << "SUMMARY (" << packets << " packets per network, seed " << seed << ")\n";
    cout << "Network  Destinations  Theoretical ETX  Simulated avg  Abs diff  % diff\n";
    for (int id = 1; id <= 4; id++) {
        const Network& net = networks[id - 1];
        const Result& res = results[id - 1];
        string destinationList;
        for (size_t d = 0; d < net.destinations.size(); d++) {
            destinationList += (d ? "," : "") + net.flagNames[net.destinations[d]];
        }
        double difference = fabs(res.mean - net.theoreticalETX);
        cout << setw(7) << id << "  " << setw(12) << left << destinationList << right
             << setw(17) << net.theoreticalETX << setw(15) << res.mean
             << setw(10) << difference << setw(7) << setprecision(3)
             << 100 * difference / net.theoreticalETX << "%" << setprecision(6) << "\n";
    }
    cout << "\nRandom numbers generated in total: " << g_drawCount
         << " (one per transmission)\n";
    cout << "\nCSV files written to " << outFolder << "/: summary.csv, per_node.csv, "
         << "per_destination.csv, case_intervals.csv, running_average.csv"
         << (logRandom ? ", random_numbers.csv" : "") << "\n";
#ifdef _WIN32
    cout << "Plot: .venv\\Scripts\\python plot_results.py --out " << outFolder
         << "   (README: \"To draw the plot\")\n";
#else
    cout << "Plot: .venv/bin/python plot_results.py --out " << outFolder
         << "   (README: \"To draw the plot\")\n";
#endif
    return 0;
}
