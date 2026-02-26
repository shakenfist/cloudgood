---
issue_repo: https://github.com/shakenfist/cloudgood
issue_path: docs/memory.md
---

# What even is RAM?

Before a computer can execute a program, the program must be in memory.
Before data can be processed, it must be in memory. Every instruction
fetch, every variable read, every pixel rendered -- it all passes through
memory. And yet, for something so fundamental, most of us have a
surprisingly vague understanding of what memory actually *is*. We know
DRAM is slower than cache and faster than disk, and we know DDR5 is
newer than DDR4, but could we explain *why*?

This chapter traces memory from mercury delay lines to CXL, through a
story that is equal parts physics, engineering, economics, and human
labor. The unifying thread is that memory has always been the most
physically constrained component in computing. Every generation is a new
set of compromises between speed, density, cost, reliability, and
persistence -- and every compromise has consequences that ripple up
through the software stack, all the way to how cloud platforms manage
virtual machines.

## Before semiconductors

Modern RAM is built from transistors, but the earliest computers had no
transistors at all. The engineers who built the first stored-program
computers had to invent memory from whatever physics was available to
them, and the results were astonishingly creative.

### Mercury delay lines

The first practical computer memory stored data as *sound waves in
mercury*.

The concept came from World War II radar research, where delay lines
were used to filter out ground clutter. J. Presper Eckert, working at the
University of Pennsylvania's Moore School of Electrical Engineering,
experimented with mercury delay lines for radar in 1943 and conceived
of adapting them for data storage in 1944.

The mechanism is beautifully simple: a sealed metal tube is filled with
liquid mercury. A piezoelectric quartz crystal transducer at one end
converts electrical pulses into acoustic (ultrasonic) waves. These waves
travel through the mercury at approximately 1,450 meters per second.
When they reach the transducer at the far end, they're converted back
into electrical signals, amplified, reshaped, and fed back to the input
-- *recirculating* continuously. Data exists only as a train of pulses
endlessly circling through the tube.

Mercury was chosen because its acoustic impedance closely matches that
of quartz crystals, minimizing signal loss at the boundary. But it came
with brutal operational requirements. The speed of sound in mercury
varies with temperature, so the tubes had to be heated to exactly 40°C
(EDVAC required 50°C ± 0.25°C) and held there precisely. Servicing the
tubes was, as contemporary accounts noted, "hot and uncomfortable work."

EDSAC, built by Maurice Wilkes at Cambridge and first operational in May
1949, used 32 mercury delay lines holding 512 words of 35 bits -- about
18 kilobits total. UNIVAC I, the first commercially produced computer,
used 7 mercury tanks containing 126 channels, storing 1,000 words. Each
of the 7 tanks weighed nearly 800 pounds when filled with mercury.
Access was sequential -- you had to wait for the desired word to
circulate past the read point -- giving an average access time of 222
microseconds.

### Williams tubes

In July 1946, Freddie Williams at the Telecommunications Research
Establishment (a British radar research facility) began experimenting
with storing data as charge patterns on the face of a cathode ray
tube (1). By late 1946, he could store a single bit. He moved to the
University of Manchester, recruited Tom Kilburn, and by autumn 1947 they
had scaled the technique to 2,048 bits on a single tube -- and
demonstrated it could hold data for four hours.
{ .annotate }

1. Williams literally used a television screen to store data. The
   electron beam wrote charge dots on the phosphor surface, and a thin
   metal pickup plate in front of the screen detected the capacitive
   changes caused by the stored charges. Reading was destructive (it
   overwrote the charge), so data had to be continuously refreshed.
   Sound familiar? This is exactly the problem DRAM still has.

Williams and Kilburn built the Manchester "Baby" (formally the Small-
Scale Experimental Machine, or SSEM) specifically to prove their memory
worked in a real computer. On June 21, 1948, Tom Kilburn's program ran
successfully: 17 instructions that found the highest proper factor of
2^18^ by trying every integer downward using repeated subtraction. It
ran for 52 minutes, performed 3.5 million operations, and produced the
correct answer: 131,072. This was the first program to run on a
stored-program computer.

Williams tubes had a crucial advantage over delay lines: **random
access**. Any bit could be read in about 10 microseconds, without
waiting for data to circulate. But they were fragile and unreliable. The
charge patterns leaked away in about 0.2 seconds, requiring constant
refresh, and accessing one bit disturbed its neighbors through secondary
electron splash. In the IBM 701, Williams tube memory had an average
time-to-failure of about 15 minutes.

### Core memory: woven by hand

Magnetic core memory dominated computing for approximately twenty years
(1955-1975) and remains one of the most remarkable feats of
manufacturing in computing history -- because it was assembled entirely
by hand.

Core memory consists of tiny toroidal (doughnut-shaped) rings of ferrite
ceramic, about 1mm in diameter, arranged in grids. Three or four fine
wires are threaded through each core: X and Y address wires for
selection, a sense wire for reading, and an inhibit wire for
controlling writes. The ferrite material has a square hysteresis loop,
meaning it snaps between two stable magnetic states -- clockwise or
counterclockwise magnetization -- representing 0 and 1.

The key innovation was Jay Forrester's *coincident-current* system,
patented in 1951 (1). Each X and Y wire carries only half the current
needed to flip a core. Only the core at the intersection of an energized
X line and an energized Y line receives enough total current to change
state. This elegant addressing scheme meant that a small number of wires
could control an enormous grid of cores.
{ .annotate }

1. The invention of core memory is genuinely contested. An Wang at
   Harvard filed a patent in 1949 for the *write-after-read* cycle
   that solved destructive reads, but his design was a serial shift
   register using two cores per bit. Jay Forrester at MIT filed in
   1951 for the coincident-current system that made large-scale
   random-access core memory practical. Frederick Viehe filed even
   earlier, in 1947. IBM paid Wang $500,000 for his patent. In 1964,
   IBM paid MIT $13 million for Forrester's -- the largest patent
   settlement to that date.

Reading was destructive: to read a core, you tried to write a 0 to it.
If it was already 0, nothing happened on the sense wire. If it was 1, it
flipped to 0, inducing a detectable voltage pulse. Since reading always
left the core in the 0 state, the original value had to be rewritten
immediately after every read -- Wang's write-after-read cycle.

Core memory had a remarkable property that neither Williams tubes nor
mercury delay lines possessed: it was **non-volatile**. Once magnetized,
a core held its state indefinitely without power. This was dramatically
demonstrated when the Space Shuttle Challenger's IBM AP-101B flight
computers were recovered from the ocean floor in 1986 -- their core
memory contents were intact.

But the most remarkable thing about core memory was how it was made. The
tiny ferrite cores had to be threaded with fine wires in precise
patterns, a process that directly resembled textile weaving. For the
Apollo Guidance Computer's core rope memory (read-only memory encoding
the flight software), Raytheon employed women, many recruited from the
local textile industry and from the Waltham Watch Company, who sat at
long desks passing wire-threaded needles back and forth through matrices
of cores. A wire passing *through* a core encoded a 1; a wire passing
*around* a core encoded a 0.

Half a mile of wire through 512 cores made one module. One module stored
65,000 bits. The process took months. The workers were nicknamed "Little
Old Ladies" (the memory was called "LOL memory"), and mysterious bugs in
the programs were called "FLTs" -- Funny Little Things. Margaret
Hamilton, who led the AGC's software development, was known as the "Rope
Mother" (1) -- responsible for getting the programs right before they
were woven into copper, because errors in read-only core rope were
extraordinarily costly to fix.
{ .annotate }

1. The title "Rope Mother" is confirmed by the Smithsonian National
   Air and Space Museum. It referred to Hamilton's role overseeing
   the software that went into the ropes, not to physically weaving
   them.

### The transition to semiconductors

The era of hand-woven memory ended with Robert Dennard's invention at
IBM in 1966: a single-transistor memory cell that stored one bit as
charge on a single capacitor. IBM filed the patent in 1967 and it was
granted on June 4, 1968. Dennard's insight -- that one transistor and
one capacitor could do what core memory needed an entire ferrite ring
and multiple wires for -- would become the basis for all DRAM.

The Intel 1103, introduced in 1970, was the first commercially
successful DRAM chip (1): 1,024 bits on a single die, fabricated in a
10-micron process. By 1972, it had become the best-selling semiconductor
device in the world. Core memory, which had taken months of hand labor
per module, was being replaced by chips that rolled off fabrication lines
by the thousand. By the late 1970s, the transition was complete.
{ .annotate }

1. A historical nuance: the Intel 1103 actually used a three-transistor
   (3T) cell designed by William Regitz at Honeywell, not Dennard's
   single-transistor design. Dennard's 1T1C cell came to mass production
   in the mid-1970s with 4-kilobit chips and became the basis for all
   subsequent DRAM.

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | Mercury delay lines | [Computer History Museum: Delay Lines](https://www.computerhistory.org/revolution/memory-storage/8/309) |
    | The Manchester Baby | [Science and Industry Museum: Baby and Modern Computing](https://www.scienceandindustrymuseum.org.uk/objects-and-stories/baby-and-modern-computing) |
    | Core memory history | [Computer History Museum: Magnetic Core Memory](https://www.computerhistory.org/revolution/memory-storage/8/253) |
    | Apollo core rope memory | [Ken Shirriff: Software Woven Into Wire](http://www.righto.com/2019/07/software-woven-into-wire-core-rope-and.html) |
    | The Rope Mother | [Smithsonian: The Rope Mother Margaret Hamilton](https://airandspace.si.edu/stories/editorial/rope-mother-margaret-hamilton) |
    | Core memory weavers | [Science News: Core Memory Weavers](https://www.sciencenews.org/article/core-memory-weavers-navajo-apollo-raytheon-computer-nasa) |
    | Robert Dennard's DRAM patent | [IBM: The Invention of DRAM](https://www.ibm.com/history/dram) |

## How modern RAM works

Today's computers use two kinds of RAM, built from fundamentally
different cell designs. Understanding the difference explains the entire
memory hierarchy -- why you have megabytes of cache but gigabytes of
main memory.

### SRAM: six transistors per bit

Static RAM uses six transistors per bit, arranged as two cross-coupled
CMOS inverters forming a bistable latch. Think of it as two light
switches wired to oppose each other: flip one on and it forces the other
off, creating a self-sustaining stable state. Two additional access
transistors connect the cell to the bit lines for reading and writing.

The "static" in SRAM means the cell holds its state as long as power is
applied -- no refresh needed. The cross-coupled inverters continuously
reinforce each other. This makes SRAM fast (access times of 0.5-2.5
nanoseconds) and simple to interface with, which is why it's used for
CPU caches:

| Cache level | Typical access time | Typical size |
|-------------|-------------------|--------------|
| L1 | ~1-1.5 ns | 32-128 KB per core |
| L2 | ~3-5 ns | 256 KB - 1 MB per core |
| L3 | ~10-20 ns | 8-64 MB shared |

The cost is density. Six transistors per bit means SRAM cells are
roughly six times larger than DRAM cells. This is why you can't just
make all memory from SRAM -- a 32 GB SRAM module would be physically
enormous and prohibitively expensive. The entire memory hierarchy exists
because of this 6:1 transistor ratio.

### DRAM: one transistor, one capacitor

Dynamic RAM stores each bit as charge on a single capacitor, accessed
through a single transistor. A charged capacitor represents a 1; a
discharged capacitor represents a 0. This 1T1C design gives DRAM its
extraordinary density advantage -- one transistor plus one capacitor per
bit versus six transistors.

But capacitors leak. The charge that represents your data is constantly
draining away, and if it drops below a threshold, the bit is lost. This
is why DRAM is "dynamic" -- it must be *refreshed*. Every 64
milliseconds, the memory controller must read and rewrite every row in
the chip. With a typical chip having 8,192 rows per bank, one row is
refreshed every 7.8 microseconds. This continuous refresh consumes power,
adds latency (the memory is unavailable during refresh), and is a
fundamental constraint that DRAM has carried since the 1970s.

Reading is also destructive, just as it was with Williams tubes and core
memory. Opening a word line connects all capacitors in that row to their
bit lines, and the sense amplifiers detect the tiny charge differential
and latch it. The sense amplifiers then write the data back, restoring
the charge. Every read is a destructive read followed by an automatic
rewrite.

Access times are typically 50-100 nanoseconds -- roughly 20-100 times
slower than SRAM. This gap, combined with the density advantage, is
what creates the memory hierarchy: fast but small SRAM for caches, dense
but slower DRAM for main memory.

!!! note

    There is a striking historical echo here. Williams tubes stored
    charge on a phosphor screen that leaked away in 0.2 seconds and
    required constant refresh. DRAM stores charge on a capacitor that
    leaks away in milliseconds and requires constant refresh. The
    fundamental problem -- storing information as electrical charge --
    hasn't changed in 75 years. We've just gotten dramatically better
    at managing it.

## The DDR evolution

The DDR (Double Data Rate) generations are often perceived as "just
speed bumps," but each step involved genuine architectural innovation,
not just higher clock rates.

### DDR1 (2000): the doubling trick

DDR1's key innovation was transferring data on *both* edges of the clock
signal -- rising and falling -- doubling the effective data rate without
increasing the actual clock frequency. It also introduced a 2-bit
prefetch buffer: the internal DRAM core fetched 2 bits per internal
cycle, which were then clocked out over the external bus. Operated at
2.5V.

### DDR2 (2003) and DDR3 (2007): deeper prefetch

DDR2 doubled the prefetch to 4 bits, DDR3 to 8 bits. The pattern: the
internal DRAM core clock didn't get much faster; instead, each
generation fetched more data per internal cycle and clocked it out over
the bus at a higher rate. The metaphor is not a faster conveyor belt,
but bigger boxes on the same belt.

DDR2 also introduced on-die termination (ODT), which reduced signal
reflections at higher speeds. DDR3 lowered the voltage from 1.8V to
1.5V, cutting power consumption significantly.

### DDR4 (2012): hitting the wall

DDR4 kept the prefetch at 8 bits -- the first generation not to double
it. Instead, it introduced **bank groups**: 4 groups of 4 banks (16 total
banks), where accesses to *different* bank groups could overlap with
shorter timing than accesses within the same group. This was a lateral
innovation -- when deeper prefetch hit diminishing returns, the industry
found a different way to increase effective bandwidth. Voltage dropped
again, to 1.2V.

### DDR5 (2020): a fundamental rethink

DDR5 made three changes that are architecturally significant, not just
incremental:

**Channel splitting**: Each DDR5 DIMM has two independent 32-bit
subchannels instead of one 64-bit channel. The memory controller can
issue independent commands to each subchannel, effectively turning one
DIMM into two narrower, independently-addressed memory banks. This
improves utilization significantly for mixed workloads.

**The PMIC moved onto the DIMM**: DDR5 modules include an on-board
Power Management IC that receives 12V from the motherboard and generates
its own regulated voltages. The DIMM became a more self-contained
subsystem, with cleaner power delivery than relying on the
motherboard's voltage regulators.

**On-die ECC**: Every DDR5 chip has error correction built in --
for every 128 bits of data, 8 additional ECC bits are stored on-die.
The chip internally corrects single-bit errors before data leaves the
package. This is completely invisible to the host system and does *not*
replace system-level ECC on server DIMMs.

This last point deserves emphasis. On-die ECC is a quiet admission by
the DRAM industry that cells have become so physically small and
unreliable at modern process nodes that error correction is no longer
optional -- even for consumer hardware. The physics of the capacitor is
becoming a first-order engineering constraint.

| Generation | JEDEC ratified | Prefetch | Voltage | Key innovation |
|------------|---------------|----------|---------|----------------|
| DDR1 | June 2000 | 2n | 2.5V | Double data rate |
| DDR2 | September 2003 | 4n | 1.8V | On-die termination |
| DDR3 | June 2007 | 8n | 1.5V | Lower power |
| DDR4 | September 2012 | 8n | 1.2V | Bank groups |
| DDR5 | July 2020 | 16n | 1.1V | Channel split, on-die ECC, on-DIMM PMIC |

## When memory fails

The conventional wisdom about memory errors -- that cosmic rays
occasionally flip bits, and ECC corrects them -- turns out to be
largely wrong. The real story is far more interesting. (For how ECC
and RAS features affect the boot process, including patrol scrubbing
and predictive failure analysis, see [Booting](booting.md#memory-ras-features).)

### ECC: how it works

Server memory uses ECC (Error Correcting Code) DIMMs with a 72-bit
word: 64 data bits plus 8 check bits. The error correction code is
SECDED (Single Error Correction, Double Error Detection), an extended
Hamming code.

On every write, the memory controller computes 8 check bits from the 64
data bits using XOR operations across specific bit positions and stores
all 72 bits. On every read, the controller recomputes the check bits and
compares them to the stored values. If they match, no error occurred. If
a single bit flipped anywhere in the 72-bit word, the syndrome (the XOR
of expected and actual check bits) points directly to the bad bit, and
it's corrected transparently. If two bits flipped, the error is
detected but can't be corrected -- the system typically raises a machine
check exception.

This works well for random, independent bit flips. But the assumption
that errors *are* random and independent turns out to be wrong.

### The Google study: demolishing the cosmic ray myth

In 2009, Bianca Schroeder (University of Toronto), Eduardo Pinheiro,
and Wolf-Dietrich Weber (Google) published "DRAM Errors in the Wild: A
Large-Scale Field Study" -- a landmark paper based on measurements from
Google's fleet over 2.5 years. The findings overturned decades of
received wisdom:

**Error rates were far higher than anyone expected.** Over 8% of DIMMs
experienced at least one correctable error per year. About one-third of
all machines had at least one correctable error per year. Previous
estimates, based on lab testing and manufacturer specifications, had been
hundreds to thousands of times too low.

**Hard errors dominated, not soft errors.** The prevailing assumption
was that transient "soft" errors -- cosmic rays and alpha particles
striking cells -- were the main threat. Google found the opposite: most
errors were permanent (hard) failures associated with specific physical
locations on the DIMM. A DIMM that experienced one error was far more
likely to experience additional errors. A follow-up study (Li et al.,
"Cosmic Rays Don't Strike Twice," ASPLOS 2012) confirmed that errors
were spatially correlated -- the same cells failed repeatedly --
inconsistent with the random nature of cosmic ray strikes.

**Temperature barely mattered in the field.** In the lab, temperature is
a major factor in DRAM reliability. In real data centers, with all other
variables controlled for, temperature had a "surprisingly small effect."
The real-world failure modes were different from the lab failure modes.

A 2015 study by Facebook (Meza, Wu, Kumar, and Mutlu, DSN 2015)
confirmed and extended these findings across Facebook's fleet: 9.62% of
servers experienced correctable errors over 12 months. They also found
that actively retiring memory pages with repeated errors could reduce
the uncorrectable error rate by up to 2.8x.

### Rowhammer: when physics becomes an exploit

If DRAM cells are unreliable, can that unreliability be *weaponized*?
The answer, disturbingly, is yes.

In 2014, Yoongu Kim and colleagues published "Flipping Bits in Memory
Without Accessing Them" (ISCA 2014), demonstrating that repeatedly
activating (opening and closing) the same DRAM row -- "hammering" it --
could induce bit flips in physically adjacent rows *without ever
accessing them*. Over 80% of tested modules from all three major vendors
were vulnerable.

The physical mechanism is electromagnetic coupling between rows packed
too tightly together. When a word line toggles rapidly, it creates
voltage fluctuations that couple into neighboring word lines through
parasitic capacitance, accelerating charge leakage in victim cells
beyond what refresh can compensate for. The effect gets worse as process
nodes shrink -- smaller capacitors, closer rows, tighter noise margins.

In March 2015, Google Project Zero turned this into a working exploit.
Mark Seaborn and Thomas Dullien demonstrated two attacks: a kernel
privilege escalation where a user-space process flipped a bit in a page
table entry to gain read-write access to all of physical memory, and a
Chrome NaCl sandbox escape. This was the first demonstration that a
hardware reliability issue could be weaponized as a security exploit,
fundamentally challenging the assumption that memory contents cannot
change unless explicitly written to.

DDR5's on-die ECC was partly intended to mitigate Rowhammer. It hasn't
been sufficient. In 2025, researchers at ETH Zurich published "Phoenix,"
demonstrating the first Rowhammer privilege escalation on DDR5. They
reverse-engineered the in-DRAM mitigations, developed a
self-correcting synchronization technique, and achieved a root shell on
a commodity DDR5 system in under two minutes. All 15 DDR5 DIMMs in
their test pool were vulnerable to bit flips.

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | Google DRAM study | [DRAM Errors in the Wild (SIGMETRICS 2009)](https://research.google/pubs/pub35162/) |
    | Facebook DRAM study | [Revisiting Memory Errors in Large-Scale Production Data Centers (DSN 2015)](https://users.ece.cmu.edu/~omutlu/pub/memory-errors-at-facebook_dsn15.pdf) |
    | Original Rowhammer paper | [Flipping Bits in Memory Without Accessing Them (ISCA 2014)](https://users.ece.cmu.edu/~yoonguk/papers/kim-isca14.pdf) |
    | Project Zero Rowhammer exploit | [Exploiting the DRAM Rowhammer Bug](https://googleprojectzero.blogspot.com/2015/03/exploiting-dram-rowhammer-bug-to-gain.html) |
    | Phoenix DDR5 attack | [ETH Zurich COMSEC: Phoenix](https://comsec.ethz.ch/research/dram/phoenix/) |

## NUMA: the hidden variable in cloud performance

So far we've discussed memory as if each CPU has equal access to all of
it. On a single-socket system, that's roughly true. On the multi-socket
servers that fill data centers, it is emphatically not.

### The problem

In a dual-socket server, each CPU has its own directly-attached memory.
CPU 0 can access its local memory in about 100 nanoseconds. But if it
needs data that lives in memory attached to CPU 1, the request must
travel through an interconnect (Intel's UPI, AMD's Infinity Fabric) to
the remote socket, adding significant latency. This is Non-Uniform
Memory Access -- NUMA.

The latency penalty varies by hardware, but it's substantial:
approximately 30-50% additional latency for remote memory access on
modern platforms. Under contention, the difference can be much worse.
Bandwidth is even more affected -- one production study measured local
memory bandwidth of ~46 GB/s dropping to ~6 GB/s for cross-socket
access, nearly an 8x penalty.

### Why it matters for cloud

When a hypervisor places a virtual machine, it must try to keep the VM's
vCPUs and memory on the same NUMA node. A VM that spans NUMA nodes --
some of its vCPUs on socket 0, some of its memory on socket 1 -- gets
silently degraded performance. The guest OS has no idea why it's slow.
There's no error, no log message, just everything taking longer than it
should.

This is one of the great hidden variables of cloud performance. Two
identical VMs on the same physical host, running the same workload, can
have wildly different performance if one landed within a single NUMA
node and the other straddled two. Studies have shown performance
degradation of up to 700% (8x slower) for poorly placed VMs in
multi-VM scenarios.

Modern hypervisors address this with NUMA-aware scheduling, which can
improve VM performance by 20-23% over naive placement. **vNUMA**
exposes the physical NUMA topology to the guest OS as virtual NUMA
nodes, allowing the guest's own scheduler to cooperate with the
hypervisor. And cloud providers design their instance types to align
with NUMA boundaries -- which is why you can't always get an arbitrary
combination of vCPUs and memory.

## Intel Optane: a $7 billion tragedy

The story of Intel Optane is a cautionary tale about what happens when a
genuinely novel technology meets an unwilling software ecosystem.

### The promise

On July 28, 2015, Intel and Micron announced 3D XPoint (pronounced
"cross point") as a fundamentally new kind of memory. It used
chalcogenide phase-change materials at the intersections of a cross-
point grid, with Ovonic Threshold Switches instead of transistors as
cell selectors. No transistors at all in the storage layer -- a
genuinely novel architecture.

The claims were extraordinary: 1,000x faster than NAND flash, 1,000x
greater endurance, 10x denser than conventional DRAM. A new tier in
the memory hierarchy, sitting between DRAM and storage, combining
persistence with near-DRAM speed.

### What actually shipped

Optane shipped in three forms:

**Optane SSDs** (from March 2017): PCIe NVMe drives using 3D XPoint
instead of NAND flash. These were genuinely excellent -- dramatically
lower latency than any NAND SSD, especially for random reads and writes.
The Intel DC P4800X became a favorite for latency-sensitive workloads.

**Optane Memory** (from April 2017): Small M.2 modules (16-32GB) used
as a caching layer in front of hard drives. A consumer product that
worked but addressed a shrinking market as SSDs became cheap.

**Optane Persistent Memory (PMem)** (from Q2 2019): This was the
ambitious product. 128GB, 256GB, and 512GB modules that plugged into
standard DIMM slots alongside regular DRAM. They could operate in two
modes: "Memory Mode" (as a transparent DRAM cache with PMem as capacity)
or "App Direct Mode" (as persistent memory that applications could
mmap directly, with data surviving power loss). A second generation
followed in Q2 2020.

### Why it failed

Three problems compounded:

**The software ecosystem wouldn't rewrite.** Using Optane PMem in App
Direct mode -- its most transformative capability -- required
applications to understand persistent memory semantics. Intel developed
PMDK (Persistent Memory Development Kit) with libraries for C, C++,
Java, and others, and worked with the Linux kernel on DAX (Direct
Access) file system support. But writing correct persistent-memory code
is fundamentally different from writing for volatile memory. You need
crash-consistent data structures, careful ordering of stores, and new
concurrency models. Most applications were not rewritten. Without the
software ecosystem, the hardware advantage was stranded.

**Economics:** 3D XPoint was neither as cheap as NAND nor as fast as
DRAM. For workloads that didn't specifically need its unique
persistence-with-speed properties, it was -- as critics put it --
"worse than both at their respective jobs." The density claims were
quietly revised downward from 10x to 4x relative to DRAM.

**Manufacturing:** Intel and Micron had jointly operated the 3D XPoint
fab through their IMFT joint venture, but in January 2019, Micron bought
out Intel's stake for $1.5 billion. In March 2021, Micron ceased all 3D
XPoint development. In June 2021, they sold the Lehi, Utah fab to Texas
Instruments. The sole production facility for the technology was gone.

Intel announced the shutdown of its Optane business during the Q2 2022
earnings call on July 28, 2022, taking a $559 million inventory
write-off. Objective Analysis estimated cumulative losses of
approximately $7 billion through 2020 alone. A planned third-generation
PMem (codenamed "Crow Pass") for DDR5 systems was cancelled before
shipping.

### The lesson

Optane proved that inventing a genuinely new memory technology is not
enough. The real moat in computing is the software ecosystem. Without
applications rewritten to exploit persistence, the hardware advantage
was commercially irrelevant. The technology worked; the market didn't
care.

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | 3D XPoint announcement | [Intel and Micron Produce Breakthrough Memory Technology (2015)](https://www.intc.com/news-events/press-releases/detail/324/intel-and-micron-produce-breakthrough-memory-technology) |
    | PMDK documentation | [Persistent Memory Development Kit](https://pmem.io/pmdk/) |
    | Optane financial analysis | [Blocks and Files: Intel Optane Losses](https://blocksandfiles.com/2021/11/01/intel-optane-losses-laid-bare-in-sec-report/) |

## Memory in the cloud

Everything discussed so far -- the physics of DRAM cells, the DDR
interface, ECC, NUMA topology -- converges in the cloud, where
hypervisors must manage memory across many virtual machines on shared
hardware. This creates a unique set of trade-offs that don't exist on
bare-metal systems.

### Ballooning: cooperative memory management

When a host is under memory pressure, it needs to reclaim memory from
idle VMs. But the hypervisor doesn't know which pages inside a VM are
important -- only the guest OS knows its own workload. The
*virtio-balloon* driver solves this through cooperation.

The hypervisor tells the balloon driver inside the guest to "inflate" --
allocate pages and report them to the host. The guest kernel decides
which pages to evict (it knows its workload best), and the host can then
reuse the underlying physical memory. When memory pressure eases, the
balloon "deflates," returning pages to the guest.

This is an elegant protocol: the host sets the policy (how much memory
to reclaim), and the guest makes the tactical decisions (which pages to
give up). But it requires a functioning guest OS with a balloon driver --
it doesn't work if the guest is hung or unresponsive.

### KSM: deduplication as a double-edged sword

Kernel Same-page Merging (KSM), added to Linux in kernel 2.6.32
(December 2009), scans physical memory for pages with identical content.
When it finds duplicates, it maps all the virtual pages to a single
physical page (marked copy-on-write) and frees the rest. In a
virtualization environment where many VMs run the same OS, this can
recover significant amounts of memory -- every VM has a copy of the same
kernel, the same libraries, the same idle-state data structures.

The security problem was identified almost immediately. In 2011, Suzaki
et al. ("Memory Deduplication as a Threat to the Guest OS," EuroSec
2011) demonstrated that an attacker in one VM could detect whether
specific memory content existed in a co-resident VM by measuring write
timing. Writing to a deduplicated (shared) page triggers a copy-on-write
fault, which takes measurably longer than writing to a private page.
This timing difference lets an attacker determine what software is
running on neighboring VMs, defeat address space layout randomization,
and in combination with other techniques, potentially exfiltrate
cryptographic keys.

This is a microcosm of a fundamental tension in cloud computing: the
same technique that lets a provider run more VMs (sharing memory for
efficiency) is the exact technique that enables cross-VM attacks
(detecting shared state for exploitation). Most public cloud providers
now disable KSM entirely.

### Huge pages: trading flexibility for performance

Standard x86-64 pages are 4KB. For a VM with 64GB of memory, that's
over 16 million page table entries. Every memory access requires a TLB
(Translation Lookaside Buffer) lookup to translate virtual to physical
addresses, and with 16 million entries, TLB misses are frequent.

Huge pages -- 2MB or 1GB instead of 4KB -- reduce the number of page
table entries dramatically. 64GB of memory needs only 32,768 entries
with 2MB pages, or just 64 with 1GB pages. TLB miss rates drop by
50-90%, and memory-intensive workloads see 10-30% performance
improvements.

The trade-off is flexibility. Ballooning operates at 4KB granularity
and conflicts with huge pages -- inflating a balloon can cause the
kernel to split huge pages, fragmenting memory. KSM only works on 4KB
pages. And allocating huge pages requires contiguous physical memory,
which may not be available on a fragmented host, causing latency spikes
as the kernel compacts memory.

Cloud operators must choose: memory efficiency tricks (ballooning, KSM)
or raw performance (huge pages). Most production deployments for
performance-sensitive workloads choose huge pages and accept the reduced
flexibility.

### Memory hotplug: harder than it sounds

Adding memory to a running VM sounds straightforward -- just allocate
more physical pages and map them into the guest. Removing memory is
fiendishly difficult.

The problem is unmovable kernel allocations. The guest kernel places
page tables, slab caches, and memory map arrays in memory, and these
structures *cannot be migrated*. Once kernel data lands in a memory
block, that block cannot be offlined. Linux provides ZONE_MOVABLE -- a
memory zone where only movable (user-space) allocations are placed,
making those blocks safe to remove -- but configuring it requires
careful balancing. Too much memory in ZONE_MOVABLE starves the kernel of
space for its own allocations; too little defeats the purpose.

This is one of those problems where the theory is clean (just move the
pages!) but the practice is full of corner cases that make reliable
memory hot-remove an ongoing engineering challenge.

## CXL: the future of memory?

Compute Express Link (CXL) is an open interconnect standard that may
finally deliver on Optane's broken promise of disaggregated memory
tiers -- but through a standard interface rather than a proprietary
memory technology.

CXL was founded in March 2019 by Intel, with co-founders including
Google, Meta, Microsoft, and others. The key capability for memory is
CXL.mem, which allows memory devices to be attached via PCIe and
accessed with standard CPU load/store instructions. CXL-attached memory
appears to the operating system as a CPU-less NUMA node -- addressable,
cache-coherent, but with higher latency than local DRAM.

The vision for CXL 2.0 and later is **memory pooling**: shared memory
devices connected through CXL switches, dynamically allocated to
whichever host needs them. Instead of every server having fixed DRAM
that sits mostly idle, a rack of servers could share a pool of CXL
memory, with the switch directing each host's accesses to its currently
assigned portion. Early claims suggest memory utilization could improve
from typical 50-60% to 85%+.

The latency overhead is real but manageable. Current CXL devices add
approximately 70-90 nanoseconds for small configurations, with some
tail latencies reaching 700ns or more. Meta's Transparent Page
Placement (TPP) system addresses this by automatically promoting hot
pages from CXL memory to local DRAM and demoting cold pages in the
other direction, achieving 99.5% of the performance of all-local-DRAM
configurations.

CXL also introduces new NUMA-like complexity. CXL memory is inherently
non-local, creating additional latency tiers: local DRAM (fastest),
remote DRAM on another socket (slower), CXL-attached memory (slower
still). The Linux kernel already supports this through memory tiering,
using ACPI tables to discover latency characteristics and automatically
placing pages in the appropriate tier.

CXL 4.0 was announced in November 2025, doubling bandwidth to 128 GT/s.
Mainstream production deployments are expected in 2026-2027. If CXL
delivers on its promise, it may represent the most significant change in
how servers use memory since the transition from core to DRAM -- not by
inventing a new memory technology, but by making it possible to share
the technology we already have.

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | CXL specification | [Compute Express Link Consortium](https://computeexpresslink.org/cxl-specification/) |
    | CXL memory performance analysis | [Dissecting CXL Memory Performance at Scale](https://arxiv.org/html/2409.14317v1) |
    | Meta's TPP for CXL memory tiering | [The Next Platform: Meta Hacks CXL Memory Tier Into Linux](https://www.nextplatform.com/2022/06/16/meta-platforms-hacks-cxl-memory-tier-into-linux/) |
    | Linux kernel memory tiering | [Using Linux Kernel Tiering with CXL Memory](https://stevescargall.com/blog/2024/05/using-linux-kernel-tiering-with-compute-express-link-cxl-memory/) |
    | KSM security analysis | [Memory Deduplication as a Threat to the Guest OS (EuroSec 2011)](https://dl.acm.org/doi/10.1145/1972551.1972552) |
    | Linux memory hotplug | [Memory Hot(Un)Plug -- Kernel Documentation](https://docs.kernel.org/admin-guide/mm/memory-hotplug.html) |

## The thread through it all

The history of computer memory is a history of the same fundamental
problems recurring at every scale. Mercury delay lines stored data as
recirculating waves that would dissipate without constant refresh.
Williams tubes stored charge on phosphor screens that leaked away in
fractions of a second. DRAM stores charge on capacitors that leak away
in milliseconds. At every level, from the 800-pound mercury tanks of
UNIVAC to the nanometer-scale capacitors on a DDR5 chip, the challenge
is the same: charge doesn't want to stay put, and we have to keep
putting it back.

Each generation solved the previous generation's problems but introduced
new ones. Core memory was non-volatile and eliminated refresh, but had
to be hand-woven. Semiconductor memory eliminated the weaving but
brought refresh back. Higher density brought Rowhammer. The cloud added
sharing, which brought side-channel attacks via KSM. CXL promises to
solve memory stranding but introduces new NUMA tiers.

The cloud inherits all of these compromises and adds its own: sharing
vs. isolation, overcommit vs. reliability, NUMA-aware placement vs.
scheduling flexibility. Understanding the physics beneath the
abstraction layers doesn't just satisfy curiosity -- it explains why
your cloud VM sometimes performs differently than you'd expect, why ECC
matters, and why the memory hierarchy looks the way it does.

--8<-- "docs-include/abbreviations.md"
