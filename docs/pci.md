---
issue_repo: https://github.com/shakenfist/cloudgood
issue_path: docs/pci.md
---

# More about PCI than you thought you wanted to know

We've touched on PCI (Peripheral Component Interconnect) in a few places
already -- it comes up in the discussion of
[memory mapped devices](memory-mapped-devices.md) where we talked about how
PCI devices use BARs to claim regions of the physical address space, and
again in [device passthrough](device-passthrough.md) where we discussed
VFIO and IOMMU isolation. This page digs deeper into some of the interesting
details of how PCI actually works at the hardware and OS level, starting with
a look at what the physical address space actually looks like on a modern x86
system -- because it turns out it's not just RAM all the way down.

## The x86 physical address space

Most of us carry around a mental model that looks something like "memory is a
big linear array of bytes, all backed by RAM". This is a useful simplification,
but it's quite wrong. The physical address space on an x86 system is a
patchwork of RAM, memory-mapped device registers, firmware ROM, reserved
regions, and outright holes. Understanding this layout is essential for
understanding where PCI BARs end up and why.

### The first megabyte: a museum of compatibility

The Intel 8088 in the original IBM PC (1981) had 20 address lines, giving it
a 1 MiB address space. IBM divided this into 640 KiB of "conventional memory"
for RAM and 384 KiB of "Upper Memory Area" (UMA) for ROMs and memory-mapped
I/O. Every x86 system since then has preserved this layout in its first
megabyte, even though modern processors have address spaces measured in
terabytes.

Here's what lives there:

| Address range | Size | Contents |
|---|---|---|
| `0x00000`--`0x003FF` | 1 KiB | **Real-mode Interrupt Vector Table (IVT)** -- 256 entries of 4 bytes each (segment:offset pairs). Used by the BIOS and real-mode code for interrupt dispatch. |
| `0x00400`--`0x004FF` | 256 bytes | **BIOS Data Area (BDA)** -- BIOS state information: serial/parallel port addresses, equipment list, memory size, keyboard buffer, video state, disk state, timer tick count. |
| `0x00500`--`0x07BFF` | ~30 KiB | **Free conventional memory** -- Available for use. The bootloader is traditionally loaded at `0x7C00`. |
| `0x07C00`--`0x07DFF` | 512 bytes | **Boot sector** -- Where the BIOS loads the MBR. |
| `0x07E00`--`0x9FBFF` | ~608 KiB | **Conventional memory (continued)** -- Free for OS and application use. The exact top depends on the EBDA size. |
| `0x9FC00`--`0x9FFFF` | 1 KiB+ | **Extended BIOS Data Area (EBDA)** -- Variable size, always immediately below `0xA0000`. Contains SMP configuration, ACPI data. |
| `0xA0000`--`0xBFFFF` | 128 KiB | **Video memory** -- we discussed this in detail in the [instance video](instance-video.md) chapter. Graphics framebuffer at `0xA0000`, monochrome text at `0xB0000`, colour text at `0xB8000`. |
| `0xC0000`--`0xC7FFF` | 32 KiB | **Video BIOS (Option ROM)** -- The VGA adapter's BIOS extension, identified by the signature bytes `0x55 0xAA`. |
| `0xC8000`--`0xDFFFF` | 96 KiB | **Other Option ROMs** -- BIOS scans this region on 2 KiB boundaries looking for the `0x55 0xAA` signature. PCI device expansion ROMs are copied here and executed during POST. |
| `0xE0000`--`0xEFFFF` | 64 KiB | **Extension System BIOS** -- Additional firmware code or option ROMs. |
| `0xF0000`--`0xFFFFF` | 64 KiB | **System BIOS ROM** -- The main firmware. Contains the real-mode entry point at `0xFFFF0` (the 16-bit reset vector). Modern BIOSes shadow the ROM contents to RAM here for performance. |

The 640 KiB boundary was an arbitrary choice by IBM. They could have reserved
as little as 128 KiB for the UMA, but at the time 640 KiB of usable RAM
seemed like more than enough. It wasn't, but by then the layout was set in
stone.

### From 1 MiB to TOLUD: extended memory

Everything above 1 MiB and below a boundary called **TOLUD** (Top of Low
Usable DRAM) is generally usable RAM. This is where your OS kernel, drivers,
and applications live.

On very old systems there was an additional hole here -- the "ISA memory hole"
from `0xF00000` to `0xFFFFFF` (15--16 MiB), reserved for ISA devices that
could only address 24 bits. Modern systems don't use it, though some chipsets
still support it as an option.

### The PCI memory hole: TOLUD to 4 GiB

This is the architecturally interesting part. On a system with 4 GiB or
more of physical RAM installed, you might expect the physical address space
from `0x00000000` to `0xFFFFFFFF` to be entirely populated by DRAM. It's not.
A substantial chunk -- typically somewhere between 512 MiB and 1.5 GiB -- is
carved out for PCI device MMIO.

The boundary is defined by a register in the memory controller called
**TOLUD** (Top of Low Usable DRAM). On Intel systems, this register used to
live in the northbridge; on modern processors it's in the CPU's integrated
"System Agent" (the part Intel used to call the "uncore"). Firmware sets
TOLUD during POST based on how much MMIO space the installed PCI devices
need. Typical values are around `0xC0000000` (3 GiB) to `0xE0000000` (3.5
GiB).

The region between TOLUD and `0xFEC00000` is where PCI/PCIe device BARs get
mapped. This is the MMIO window we've been talking about -- when firmware
enumerates PCI devices and assigns their BAR addresses, this is the address
space it allocates from. The PCIe Extended Configuration Access Mechanism
(ECAM) also lives here, typically consuming 256 MiB for its config space
mapping (the ACPI MCFG table tells the OS where to find it).

The memory controller uses TOLUD as a routing decision:

- Physical address **below TOLUD**: route to DRAM.
- Physical address **between TOLUD and 4 GiB**: route downstream to the
  PCH (Platform Controller Hub) for PCI/PCIe devices.
- Physical address **above 4 GiB**: check TOUUD (more on this below).

### The top of 32-bit space: fixed platform devices

The topmost ~20 MiB of the 32-bit address space is reserved for hardcoded
platform devices at addresses that are essentially unchangeable:

| Address | Device |
|---|---|
| `0xFEC00000` | **I/O APIC** -- the interrupt routing controller. The actual address is confirmed by the ACPI MADT table. |
| `0xFED00000` | **HPET** (High Precision Event Timer) -- high-resolution timer registers. |
| `0xFED40000` | **TPM** (Trusted Platform Module) -- when present. |
| `0xFEE00000` | **Local APIC** -- each CPU core's interrupt controller. Interestingly, every core maps its own local APIC at the same physical address, but the hardware routes each core's access to its own APIC transparently. |
| `0xFF000000`--`0xFFFFFFFF` | **Firmware ROM** -- the SPI flash containing BIOS/UEFI firmware, typically 16--32 MiB. The CPU's reset vector is at `0xFFFFFFF0` (4 GiB minus 16 bytes), which is where execution begins on power-on -- we discussed this in the [booting](booting.md) chapter. |

### Above 4 GiB: reclaimed RAM and high MMIO

If the PCI memory hole steals, say, 1 GiB of the 32-bit address space, then
on an 8 GiB system you'd only have access to 7 GiB of your RAM. To avoid
this, the memory controller implements **memory remap** (also called
"reclaim"): the DRAM that would have occupied the hole is remapped to
physical addresses starting at 4 GiB (or above whatever RAM naturally lives
there).

Two registers control this: **REMAPBASE** and **REMAPLIMIT** in the memory
controller. A companion register, **TOUUD** (Top of Upper Usable DRAM),
marks the top of all usable DRAM above 4 GiB, including the reclaimed
portion. In our 8 GiB example:

- 0 to 3 GiB: DRAM (below the hole)
- 3 GiB to 4 GiB: PCI MMIO hole (no RAM here)
- 4 GiB to 9 GiB: DRAM (7 GiB native + 1 GiB reclaimed from the hole)
- TOUUD = 9 GiB

Above TOUUD, 64-bit PCI BARs can be mapped. This is increasingly important
for devices with large memory requirements -- a modern GPU might have a 16
GiB framebuffer that simply wouldn't fit below 4 GiB.

### SMM: the invisible memory

There's one more region worth mentioning: **SMRAM**, used by System
Management Mode (SMM). SMM is a special CPU mode used by firmware for power
management and hardware error handling -- it's entirely invisible to the OS.

Modern systems use **TSEG** (Top Segment) for SMRAM, located just below
TOLUD. TSEG is typically 1--8 MiB, and when the CPU is not in SMM mode,
accesses to this region are blocked entirely. This provides security
isolation for firmware code that the OS can't touch (1).
{ .annotate }

1. SMM's invisibility is a double-edged sword -- it's also been the target of some creative firmware-level attacks, since malware running in SMM is invisible to the OS and even to hypervisors.

### Seeing it for yourself

On a Linux system, you can see the physical address space layout directly:

```
$ sudo cat /proc/iomem
00000000-00000fff : Reserved
00001000-0009fbff : System RAM
0009fc00-0009ffff : Reserved
000a0000-000bffff : PCI Bus 0000:00
000c0000-000c8dff : Video ROM
000f0000-000fffff : Reserved
  000f0000-000fffff : System ROM
00100000-bffe9fff : System RAM
  01000000-01a031d0 : Kernel code
  01a031d1-02217cff : Kernel data
bffea000-bfffffff : Reserved
c0000000-febfffff : PCI Bus 0000:00
  e0000000-e03fffff : Reserved
  fe400000-fe7fffff : 0000:00:03.0
fec00000-fec003ff : IOAPIC 0
fed00000-fed003ff : HPET 0
fee00000-fee00fff : Local APIC
fffc0000-ffffffff : Reserved
100000000-2399fffff : System RAM
```

Everything we've discussed is visible here: conventional memory up to
`0x9FBFF`, the VGA/ROM region from `0xA0000` to `0xFFFFF`, extended memory
from 1 MiB to TOLUD (here around 3 GiB), the PCI MMIO window, the
fixed platform devices (IOAPIC, HPET, Local APIC), and reclaimed RAM
above 4 GiB.

### How the OS learns the layout: e820 and UEFI memory maps

The firmware communicates the physical address map to the OS through
standardized interfaces. On legacy BIOS systems, this is `INT 15h,
AX=E820h` -- a BIOS call that returns an array of address range descriptors,
each with a type:

| Type | Name | Meaning |
|---|---|---|
| 1 | AddressRangeMemory | Usable RAM -- OS can use freely |
| 2 | AddressRangeReserved | Reserved -- firmware, MMIO, or otherwise off-limits |
| 3 | AddressRangeACPI | ACPI tables -- OS can reclaim after parsing |
| 4 | AddressRangeNVS | ACPI NVS -- must be preserved across sleep |
| 5 | AddressRangeUnusable | Detected memory errors -- do not use |
| 6 | AddressRangeDisabled | Not enabled -- do not use |

UEFI systems provide an equivalent through `GetMemoryMap()`, with more
granular types that distinguish between boot services memory (reclaimable
after ExitBootServices), runtime services memory (must be preserved for UEFI
runtime calls), and conventional free memory.

Importantly, neither the e820 map nor the UEFI memory map describes the
first 1 MiB in detail -- the legacy layout (IVT, BDA, VGA, option ROMs) is
considered "assumed knowledge." The maps also don't report individual PCI
device MMIO regions -- they simply mark the PCI MMIO window as reserved.
The OS discovers PCI device BARs separately through PCI enumeration.

### A brief history of address space evolution

The layered complexity of the x86 physical address space makes more sense
when you see how each generation added to it while preserving backward
compatibility:

- **1981 (IBM PC, 8088)**: 20-bit / 1 MiB address space. The 640K/384K
  split is established.
- **1984 (IBM AT, 80286)**: 24-bit / 16 MiB. The A20 gate is introduced --
  a latch controlled through the keyboard controller (!!) that forces address
  line 20 low, because some real-mode software relied on the 8088's 1 MiB
  address wraparound behaviour.
- **1985 (80386)**: 32-bit / 4 GiB. The first megabyte's layout is
  preserved. The VGA memory hole at `0xA0000` becomes permanent.
- **1993 (Pentium, PCI 1.0)**: PCI devices need MMIO space. The region
  below 4 GiB is used, creating the PCI memory hole.
- **1995 (Pentium Pro, PAE)**: 36-bit / 64 GiB physical addressing. DRAM
  can now exceed 4 GiB, making the PCI hole a real problem (lost RAM).
  Memory remap/reclaim is invented.
- **2003 (AMD64)**: 40+ bit physical addressing. 64-bit BARs can map
  devices above 4 GiB, but the below-4G MMIO window persists for 32-bit
  device compatibility.
- **Today**: The legacy layout is fossilised. Even with UEFI, the first
  megabyte is still special, the VGA hole is still there (even without VGA
  hardware), and the PCI MMIO hole still sits below 4 GiB. The A20 gate
  line still exists as a CPU pin (A20M#), though nothing uses it.

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | x86 memory map | [OSDev Wiki: Memory Map (x86)](https://wiki.osdev.org/Memory_Map_(x86)) |
    | The PCI memory hole | [Wikipedia: PCI hole](https://en.wikipedia.org/wiki/PCI_hole) |
    | The A20 gate saga | [OS/2 Museum: The A20-Gate Fallout](https://www.os2museum.com/wp/the-a20-gate-fallout/) |
    | e820 memory map | [Wikipedia: E820](https://en.wikipedia.org/wiki/E820) |
    | TOLUD/TOUUD register details | [Intel 10th Gen Core Datasheet Vol. 2](https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/10th-gen-core-families-datasheet-vol-2-datasheet.pdf) (PDF) |
    | Physical address space in QEMU | [Gerd Hoffmann's blog](https://www.kraxel.org/blog/2023/12/qemu-phys-bits/) |
    | System address map initialization | [Infosec Institute: Part 1 (PCI)](https://www.infosecinstitute.com/resources/hacking/system-address-map-initialization-in-x86x64-architecture-part-1-pci-based-systems/), [Part 2 (PCIe)](https://www.infosecinstitute.com/resources/reverse-engineering/system-address-map-initialization-x86x64-architecture-part-2-pci-express-based-systems/) |

## PCI BAR userspace mapping via sysfs

Now that we understand where PCI BARs live in the physical address space
(the MMIO window between TOLUD and 4 GiB, or above TOUUD for 64-bit BARs),
let's look at how you actually access them from userspace.

Linux exposes PCI device resources through sysfs, which means you can map a
device's BARs directly into userspace with `mmap()`. This is how tools like
`lspci -x` and userspace drivers (including DPDK and SPDK) access device
registers without a kernel driver. However, there's a subtlety around
sub-page BAR alignment that's worth understanding.

### How mmap and PCI BARs interact

`mmap()` operates at **page granularity** -- on most systems (x86, ARM),
that's 4KB. It can only map memory starting at page-aligned physical
addresses (multiples of 0x1000).

PCI BARs are **naturally aligned to their own size**, not to page size. A
256-byte BAR only needs 256-byte alignment, a 1KB BAR needs 1KB alignment,
and so on. This means a small BAR can legitimately reside at a physical
address that is not page-aligned -- for example, `0xF7A0_0100`.

### The sub-page offset problem

When you open a sysfs resource file like
`/sys/bus/pci/devices/0000:01:00.0/resource0` and call `mmap()`, the kernel
maps a whole page. If the BAR's physical address is `0xF7A0_0100`:

1. The kernel maps the **containing page** starting at `0xF7A0_0000`.
2. Your mmap pointer points to the start of that page.
3. The actual BAR registers start at offset `0x100` into the mapped region.
4. You need to add that offset to your pointer before accessing registers.

The offset is `BAR_physical_address & 0xFFF` (the sub-page portion of the
address).

### Why BARs of 4KB or larger don't have this problem

A BAR of 4KB or larger is, by PCI spec, naturally aligned to at least 4KB --
which is already page-aligned. The mmap pointer lands directly at the start
of the BAR with zero offset.

### Example code

```c
int fd = open("/sys/bus/pci/devices/0000:01:00.0/resource0", O_RDWR);

// Get the BAR's physical address from the resource file
unsigned long bar_phys = /* parsed from 'resource' file */;
unsigned long page_offset = bar_phys & (sysconf(_SC_PAGESIZE) - 1);

// mmap the region
void *mapped = mmap(NULL, bar_size + page_offset, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);

// Actual BAR starts here
void *bar_base = (char *)mapped + page_offset;
```

### How small BARs are packed in physical address space

During PCI enumeration, firmware walks the bus and discovers each device's
BAR size requirements (by writing all-1s to the BAR and reading back the
mask). It then assigns physical addresses following one key rule: a BAR must
be naturally aligned to its own size.

Firmware typically uses a sorted allocation strategy -- it sorts BARs
largest-first and allocates them from a memory window assigned to the
relevant PCI bridge. This naturally packs things fairly tightly because
large BARs go first (grabbing big aligned chunks), and smaller BARs fill in
the gaps. As a result, multiple small BARs often end up sharing the same
4KB page.

This is practically important: if you mmap one small BAR's resource file,
you're mapping a whole page that might contain registers belonging to a
different BAR or even a different device. The kernel enforces the resource
file bounds, but it's worth being aware of.

#### Factors that work against perfect packing

- **Bridge windows** have their own alignment constraints -- a PCI-to-PCI
  bridge's memory window is typically aligned and sized to 1MB granularity,
  which can leave large gaps between buses.
- **Hotplug reservations** may leave preallocated holes in the address
  space.
- **BIOS quirks** -- some firmware over-aligns things or leaves gaps for
  compatibility reasons.
- **OS reassignment** -- Linux can optionally reassign BARs at boot
  (`pci=realloc`), and its allocator may make different packing decisions
  than firmware did.

Small BARs tend to cluster together in practice because the allocation
algorithm naturally produces that result, but it's a side effect of
alignment rules and allocation order rather than an explicit "pack into
minimum pages" goal.

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | Accessing PCI(e)'s memory space over sysfs | [Johannes 4GNU_Linux](https://www.youtube.com/watch?v=iZk1cDsIiiU) (YouTube) |
    | PCI BAR mapping walkthrough | [Low Level Learning](https://www.youtube.com/watch?v=iZk1cDsIiiU) (YouTube) |
    | BAR alignment rules | PCI Local Bus Specification |
    | Linux sysfs PCI interface | Linux kernel `Documentation/PCI/sysfs-pci.rst` |

--8<-- "docs-include/abbreviations.md"
