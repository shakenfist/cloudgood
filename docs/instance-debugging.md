---
issue_repo: https://github.com/shakenfist/cloudgood
issue_path: docs/instance-debugging.md
---

# Debugging guest instances

When a virtual machine misbehaves -- particularly when its kernel crashes --
debugging can be challenging. Unlike physical hardware where you might attach
a serial console or use JTAG, virtualization offers unique mechanisms for
observing and diagnosing guest problems. This document covers the paravirtualized
devices and techniques available for debugging guest instances under QEMU/KVM.

## The debugging challenge

When a guest kernel crashes, several things make debugging difficult:

**Limited visibility**: The hypervisor can see the guest's memory and CPU state,
but interpreting that raw data requires knowledge of kernel internals -- symbol
addresses, structure layouts, and memory management details that vary between
kernel versions.

**Console limitations**: Serial consoles capture kernel messages, but panics
often happen too fast for all messages to be transmitted. Network consoles
(netconsole) depend on the network stack still functioning.

**State preservation**: A crashed kernel can't write its own crash dump.
Something external must capture the state, either before the system resets
or by preserving data across the reboot.

Fortunately, QEMU provides several paravirtualized devices specifically designed
to address these challenges.

## pvpanic: crash notification

The simplest debugging aid is knowing *that* a crash happened. The pvpanic
device allows a guest kernel to signal the hypervisor when it panics.

### How it works

pvpanic is a minimal device -- either an ISA device using I/O port 0x505, or
a PCI device (vendor 0x1b36, device 0x0011). When the guest kernel's panic
handler runs, it writes a value to this port indicating the crash type. QEMU
receives this and emits a QMP event that management tools like libvirt can
act upon.

The device distinguishes between:

- **PANICKED**: The guest has crashed
- **CRASHLOADED**: The guest is about to load a crash kernel (kdump)

### Configuration

**QEMU command line**:

```bash
qemu-system-x86_64 ... -device pvpanic-pci
```

**libvirt XML**:

```xml
<devices>
  <panic model='pvpanic'/>
</devices>
```

**Guest requirements**: Linux kernels with `CONFIG_PVPANIC` enabled (standard
in most distributions). The driver is automatically loaded when the device
is detected.

### Limitations

pvpanic only provides notification -- it doesn't transmit any crash data.
You'll know the guest crashed, but not why. For actual debugging, you need
the mechanisms described below.

**kdump interaction**: If kdump is enabled, the crash kernel takes over before
the panic notifiers run. To ensure pvpanic fires, add `crash_kexec_post_notifiers`
to the guest kernel command line. This runs panic notifiers (including pvpanic)
before kexec loads the crash kernel.

### Use case

pvpanic is valuable for fleet monitoring. When managing hundreds of VMs, knowing
immediately that a guest crashed (versus became unresponsive due to load, or
lost network connectivity) helps categorize and prioritize incidents. It's a
lightweight addition with no performance impact during normal operation.

## vmcoreinfo and dump-guest-memory: full crash dumps

For actual post-mortem debugging, you need a memory dump that tools like
`crash` can analyze. QEMU's vmcoreinfo device and dump-guest-memory command
provide this capability.

### The vmcoreinfo device

The challenge with guest memory dumps is interpretation. A raw memory image
is just bytes -- to understand it, you need to know where kernel symbols
live, how structures are laid out, and what the page size is. This metadata
is called "vmcoreinfo" and normally lives in `/proc/vmcore` during a kdump.

The vmcoreinfo device creates a communication channel: the guest kernel
writes its vmcoreinfo data to this virtual device at boot, and QEMU stores
it. When QEMU later creates a memory dump, it includes this metadata as an
ELF note, making the dump analyzable.

### Configuration

**QEMU command line**:

```bash
qemu-system-x86_64 ... -device vmcoreinfo
```

**libvirt XML**:

```xml
<features>
  <vmcoreinfo state='on'/>
</features>
```

**Guest requirements**: Linux kernels automatically detect and use the
vmcoreinfo device when present. No special configuration needed.

### Capturing a dump

You can capture a guest memory dump at any time -- not just during a crash.
This is useful for debugging hung systems or capturing state for later analysis.

**Using libvirt**:

```bash
virsh dump --memory-only myvm /tmp/vmcore.img
```

The `--memory-only` flag is important -- it produces a dump that crash tools
can analyze, rather than a full VM state save.

**Using QMP directly**:

```bash
# Connect to QEMU's QMP socket
nc -U /var/run/qemu-myvm.sock

# Send the dump command
{"execute": "qmp_capabilities"}
{"execute": "dump-guest-memory", "arguments": {"paging": false, "protocol": "file:/tmp/vmcore.img"}}
```

**From the QEMU monitor**:

```
(qemu) dump-guest-memory /tmp/vmcore.img
```

### Analyzing the dump

Once you have a dump, use the `crash` utility to analyze it. You'll need the
vmlinux file (with debug symbols) that matches the guest kernel:

```bash
crash /path/to/vmlinux /tmp/vmcore.img
```

Within crash, useful commands include:

```
crash> log          # View kernel log buffer
crash> bt           # Backtrace of current/crashed task
crash> bt -a        # Backtrace of all CPUs
crash> ps           # Process list at time of dump
crash> vm           # Virtual memory info
crash> files        # Open files
crash> net          # Network state
```

### Without vmcoreinfo

If you capture a dump without the vmcoreinfo device enabled, the dump may
still be partially usable -- crash will search for vmcoreinfo in the raw
memory. However, this is slower and less reliable. Always enable vmcoreinfo
if you might need to debug guest crashes.

## virtio-trace: real-time kernel tracing

Sometimes crashes are the culmination of a sequence of events, and you need
to see what led up to the failure. virtio-trace streams ftrace data from
guest to host with remarkably low overhead.

### How it works

Linux's ftrace subsystem maintains ring buffers of trace data. Normally you'd
read these from `/sys/kernel/debug/tracing/` within the guest. virtio-trace
provides an alternative path: a user-space agent in the guest splices pages
directly from ftrace's ring buffer to virtio-serial ports, and a reader on
the host receives them via FIFOs.

The key innovation is efficiency. Naive approaches (reading trace data,
copying to userspace, writing to virtio-serial) have 50%+ overhead.
virtio-trace uses splice() to move pages without copying, achieving only
0.43% overhead compared to native ftrace.

### Setup

virtio-trace setup is more involved than the other mechanisms.

**On the host**, create FIFOs for each vCPU:

```bash
mkdir -p /tmp/virtio-trace
for cpu in 0 1 2 3; do
    mkfifo /tmp/virtio-trace/trace-path-cpu${cpu}.in
    mkfifo /tmp/virtio-trace/trace-path-cpu${cpu}.out
done
```

**QEMU configuration** -- add virtio-serial ports:

```bash
qemu-system-x86_64 ... \
  -device virtio-serial \
  -chardev pipe,id=trace0,path=/tmp/virtio-trace/trace-path-cpu0 \
  -device virtserialport,chardev=trace0,name=trace-path-cpu0 \
  # ... repeat for each CPU
```

**In the guest**, configure ftrace and run the trace agent:

```bash
# Enable desired trace events
echo function > /sys/kernel/debug/tracing/current_tracer

# Run the trace agent (from kernel tools/virtio/virtio-trace/)
./trace-agent
```

**On the host**, read from the FIFOs:

```bash
cat /tmp/virtio-trace/trace-path-cpu0.out
```

### When to use virtio-trace

virtio-trace shines when you're chasing intermittent issues -- races, timing
bugs, or crashes that only happen under specific conditions. You can leave
tracing enabled continuously with minimal overhead, capturing the events
leading up to a failure.

For one-off debugging, the setup complexity may not be worth it. But for
development or tracking down elusive production issues, it's invaluable.

## virtio-vsock: structured host-guest communication

virtio-vsock isn't specifically a debugging tool, but it provides a useful
building block: reliable socket communication between host and guest that
doesn't depend on network configuration.

### How it works

vsock (Virtual Socket) is an address family (`AF_VSOCK`) that provides
socket communication between VMs and hosts. Each VM has a Context ID (CID),
and you can connect/listen just like TCP sockets, but without any network
stack involvement.

### Configuration

**QEMU command line**:

```bash
qemu-system-x86_64 ... -device vhost-vsock-pci,guest-cid=3
```

The CID must be unique per VM. CID 2 is reserved for the host.

**Guest requirements**: Linux 4.8+ with `CONFIG_VIRTIO_VSOCKETS` enabled.

### Debugging applications

You could build custom debugging tools on vsock:

- A daemon that streams `/dev/kmsg` to the host
- Structured logging that survives network outages
- A remote shell that works even when networking is misconfigured

Example guest code to stream kernel messages:

```python
import socket

sock = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
sock.connect((2, 1234))  # CID 2 = host, port 1234

with open('/dev/kmsg', 'r') as kmsg:
    for line in kmsg:
        sock.send(line.encode())
```

Example host listener:

```python
import socket

sock = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
sock.bind((socket.VMADDR_CID_ANY, 1234))
sock.listen(1)

conn, addr = sock.accept()
print(f"Connection from CID {addr[0]}")
while True:
    data = conn.recv(4096)
    if not data:
        break
    print(data.decode(), end='')
```

## pstore/ramoops: crash logs that survive reboot

pstore (persistent store) reserves a region of RAM for crash data that
survives reboots. When the kernel panics, it writes the last kernel messages
to this region. After reboot, the data appears in `/sys/fs/pstore/`.

### How it works

ramoops is the RAM-backed pstore implementation. You reserve a memory region
at boot (via device tree, ACPI, or kernel parameters), and the kernel treats
it specially -- never allocating it for normal use. On panic, the kernel
writes to this region. Because RAM contents persist across warm reboots
(the memory isn't cleared), the data survives.

### Configuration

**Kernel command line**:

```
ramoops.mem_address=0x100000000 ramoops.mem_size=0x200000 ramoops.console_size=0x80000
```

Or via device tree / ACPI (more reliable, especially for VMs).

**Kernel config**: `CONFIG_PSTORE=y`, `CONFIG_PSTORE_RAM=y`,
`CONFIG_PSTORE_CONSOLE=y`

### Caveats for VMs

pstore's "survives reboot" guarantee depends on how the hypervisor handles
the memory region:

- **Warm reboot** (guest-initiated): Usually works -- QEMU doesn't clear RAM
- **Cold restart** (virsh destroy + start): RAM is reinitialized, pstore lost
- **Live migration**: The reserved region should be included, but verify

pstore is most useful when you expect the guest to reboot itself after a
panic (perhaps via `panic=30` kernel parameter) and want to examine the
crash afterwards.

## Practical recommendations

For general production use, enable these by default:

```xml
<devices>
  <panic model='pvpanic'/>
</devices>
<features>
  <vmcoreinfo state='on'/>
</features>
```

This gives you:

1. **Immediate notification** when a guest crashes (pvpanic)
2. **Full debugging capability** via dump-guest-memory when needed (vmcoreinfo)

For active debugging of a problematic guest:

1. Enable DRM/driver-specific debug logging in the guest
2. Use QEMU trace events for device-level visibility
3. Consider virtio-trace for continuous low-overhead tracing
4. Capture memory dumps for post-mortem analysis

For fleet monitoring:

1. Monitor QMP for pvpanic events
2. Automatically capture dumps when crashes are detected
3. Archive dumps with corresponding vmlinux files for later analysis

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | pvpanic overview | [An introduction to pvpanic](https://blogs.oracle.com/linux/post/an-introduction-to-pvpanic) (Oracle) |
    | QEMU pvpanic specification | [PVPANIC DEVICE](https://www.qemu.org/docs/master/specs/pvpanic.html) (QEMU docs) |
    | Kernel debugging in QEMU | [DebuggingKernelWithQEMU](https://wiki.ubuntu.com/DebuggingKernelWithQEMU) (Ubuntu Wiki) |
    | vmcoreinfo contents | [VMCOREINFO](https://www.kernel.org/doc/html/latest/admin-guide/kdump/vmcoreinfo.html) (Kernel docs) |
    | Guest crashdumping details | [Guest Windows debugging and crashdumping](https://daynix.github.io/2023/02/19/Guest-Windows-debugging-and-crashdumping-under-QEMU-KVM-dump-guest-memory-vmcoreinfo-and-virtio-win.html) (Daynix) |
    | virtio-trace | [Support virtio-trace](https://lwn.net/Articles/508063/) (LWN) |
    | virtio-trace setup | [virtio-trace README](https://www.kernel.org/doc/readme/tools-virtio-virtio-trace-README) (Kernel docs) |
    | vsock overview | [vsock(7) man page](https://man7.org/linux/man-pages/man7/vsock.7.html) |

--8<-- "docs-include/abbreviations.md"
