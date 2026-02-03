---
issue_repo: https://github.com/shakenfist/cloudgood
issue_path: docs/more-fundamentals.md
---

# More Fundamentals (incomplete)

Now that we've covered some history, let's circle back and fill in some gaps in our understanding of how operating systems work. These concepts will be important as we dig deeper into virtualization.

## What is memory?

When we talk about memory in the context of computers, we're usually talking about RAM -- Random Access Memory. This is the fast, temporary storage that the CPU uses to hold the data and instructions it's currently working with. Unlike a hard disk or SSD, RAM loses its contents when power is removed.

Physically, RAM is just a large array of bytes, each with an address. On a modern 64-bit system, addresses are 64 bits wide, which means in theory you could address up to 16 exabytes of memory (1). In practice, current CPUs only use 48 or 57 bits of the address space, but that's still far more than any computer actually has installed.
{ .annotate }

1. That's 16 million terabytes, or about 16 billion gigabytes. We're not there yet.

### Physical vs virtual memory

Here's where things get interesting. Early computers gave programs direct access to physical memory -- if your program wanted to write to address 0x1000, it wrote to that exact location in the RAM chips. This caused problems:

- Programs could accidentally (or maliciously) overwrite each other's memory
- Programs had to know exactly where in memory they would be loaded
- You couldn't run a program that needed more memory than you physically had

Modern operating systems solve these problems with **virtual memory**. Each process gets its own private address space that looks like it has access to a huge, contiguous block of memory starting at address 0. The CPU and operating system work together to translate these "virtual" addresses into "physical" addresses in actual RAM.

This translation happens through a data structure called a **page table**. Memory is divided into fixed-size chunks called pages (typically 4KB on x86), and the page table maps each virtual page to a physical page (or indicates that the page isn't currently in RAM).

!!! info

    The page table translation happens in hardware, in a component called the MMU (Memory Management Unit). This is important because it means the translation is fast -- it happens on every single memory access, so it needs to be.

### Why this matters for virtualization

Virtual memory is fundamental to how virtualization works. When you run a virtual machine:

- The guest operating system thinks it has access to physical memory starting at address 0
- But that "physical" memory is actually the hypervisor's virtual memory
- Which is then mapped to actual physical RAM by the host's page tables

This gives us nested address translation -- guest virtual → guest physical → host virtual → host physical. Modern CPUs have hardware support (Intel EPT, AMD NPT) to make this efficient, but understanding the basic concept of address translation is essential.

## What is a process, really?

We mentioned earlier that a process is "an instance of a program running on top of a kernel". But what does a process actually contain?

A process is the kernel's way of tracking everything needed to run a program:

- **Address space**: The virtual memory mappings for this process -- what memory the process can access and where it's mapped
- **Program counter**: Where in the code the CPU is currently executing
- **Registers**: The current values in the CPU's registers
- **Open files**: File descriptors for files, sockets, pipes, and other I/O resources the process has open
- **Credentials**: The user and group IDs the process is running as
- **Resource limits**: How much CPU, memory, and other resources the process is allowed to use
- **Signal handlers**: How the process wants to handle various signals (like SIGTERM or SIGINT)

When the kernel switches from running one process to another (a "context switch"), it saves all of this state for the old process and restores the state for the new one.

### Threads

A thread is like a lightweight process. Multiple threads can exist within a single process, sharing the same address space and open files, but each with their own:

- Program counter
- Registers
- Stack

This makes threads cheaper to create and switch between than processes, and makes it easy for them to share data. However, that shared memory is also why multithreaded programming is notoriously difficult -- threads can interfere with each other in subtle ways.

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | Linux process management | The Linux kernel documentation has detailed information on [process management](https://docs.kernel.org/scheduler/index.html) |
    | Threads vs processes | [This Stack Overflow answer](https://stackoverflow.com/questions/200469/what-is-the-difference-between-a-process-and-a-thread) has a good explanation of the differences |

## PID 1 and init systems

Every process in Linux has a process ID (PID) -- a unique number that identifies it. When the kernel finishes its initialization, it starts exactly one user space process: the **init process**, which always gets PID 1. This process is special in several important ways.

### Why PID 1 is special

The init process has unique responsibilities that no other process has:

1. **It's the ancestor of all other processes**: Every other process on the system is either a direct child of init or a descendant of one of init's children. When you run `pstree` on a Linux system, PID 1 is at the root of the tree.

2. **It adopts orphaned processes**: When a process exits, its children don't disappear -- they become "orphans". The kernel automatically reparents these orphaned processes to PID 1. This is important for the next point.

3. **It must reap zombie processes**: When a process exits, it doesn't fully disappear until its parent calls `wait()` to collect its exit status. Until then, it exists as a "zombie" -- a process that has finished executing but still has an entry in the process table. PID 1 must handle this for all the orphans it adopts.

4. **It cannot be killed**: Sending SIGKILL to PID 1 does nothing -- technically, PID 1 ignores signals it hasn't explicitly set up handlers for, unlike other processes which receive default signal handling. If PID 1 exits for any reason, the kernel panics -- there's no recovering from losing the init process.

### Zombie processes

A zombie process (sometimes called a "defunct" process) occurs when:

1. A child process exits
2. The parent process hasn't yet called `wait()` to retrieve the child's exit status

The zombie takes up a slot in the process table but consumes no other resources -- it's not running, it has no memory allocated, it's just an entry waiting to be cleaned up. You can spot zombies in `ps` output by the "Z" state or "defunct" label.

In a well-designed system, zombies are transient -- they exist briefly between when a process exits and when its parent reaps it. However, if a parent process is buggy and never calls `wait()`, zombies can accumulate. This is mostly harmless in small numbers, but the process table has a finite size (typically 32,768 or 4,194,304 entries depending on configuration), and enough zombies can prevent new processes from being created.

This is why PID 1's zombie reaping responsibility is critical. When a process that has children exits, those children become orphans and are adopted by PID 1. If PID 1 doesn't properly reap these adopted children when they exit, they become zombies forever.

### System V init

The original Unix init system, often called "SysV init" or "System V init", is conceptually simple. It reads a configuration file (`/etc/inittab`) and runs shell scripts to start services. The system has "runlevels" -- numbered states (0-6) that define which services should be running:

- Runlevel 0: Halt (shutdown)
- Runlevel 1: Single-user mode (maintenance)
- Runlevel 2-4: Multi-user modes (varies by distribution)
- Runlevel 5: Multi-user with graphical login
- Runlevel 6: Reboot

Services are started by scripts in directories like `/etc/init.d/`, with symlinks in `/etc/rc3.d/` (for runlevel 3) controlling which services start. Scripts starting with "S" are started, scripts starting with "K" are killed, and the number after determines the order.

SysV init is sequential -- services start one after another, in order. This is simple and predictable, but slow. On a modern server with many services, waiting for each to start before beginning the next can add significant time to boot.

### systemd

systemd, first released in 2010, replaced SysV init on most major Linux distributions by around 2015. It's controversial (1), but understanding it is essential for working with modern Linux systems.
{ .annotate }

1. The controversy stems from systemd's scope -- it does far more than just init, handling logging, device management, network configuration, and more. Critics argue this violates the Unix philosophy of "do one thing well." Proponents argue that tight integration between these components enables features that would be difficult or impossible otherwise.

Key differences from SysV init:

- **Parallel startup**: systemd starts services in parallel where possible, based on declared dependencies. This dramatically speeds up boot times.

- **Declarative configuration**: Instead of shell scripts, services are defined in "unit files" with a simple INI-like syntax. This makes them easier to parse, analyze, and manage programmatically.

- **Socket and D-Bus activation**: Services can be started on-demand when something connects to their socket, rather than running all the time.

- **Cgroups integration**: systemd uses Linux control groups to track all processes belonging to a service, making it impossible for a service to "escape" supervision by forking.

- **Unified logging**: The journal (`journalctl`) captures all service output with metadata, making it easier to debug issues.

A simple systemd unit file looks like:

```ini
[Unit]
Description=My Example Service
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/my-service --config /etc/my-service.conf
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

### Why this matters for containers

Remember that containers often run a single application as PID 1. This creates a problem: that application probably wasn't designed to be an init system. It won't reap zombies, and it might not handle signals correctly.

This is why you'll often see containers use a minimal init system like `tini` or `dumb-init`. These are tiny programs designed to:

1. Run as PID 1
2. Spawn your actual application as a child process
3. Forward signals appropriately
4. Reap any zombie processes

For example, with `tini`:

```dockerfile
ENTRYPOINT ["/tini", "--", "/usr/bin/my-application"]
```

Now `tini` is PID 1, handles all the special PID 1 responsibilities, and your application can just be a normal process.

!!! info "Want to know more?"

    | Topic | Resource |
    |-------|----------|
    | The PID 1 problem in containers | [Krallin's tini README](https://github.com/krallin/tini) explains the zombie reaping problem well |
    | systemd documentation | The [systemd website](https://systemd.io/) has comprehensive documentation |
    | History of init systems | [This LWN article](https://lwn.net/Articles/578209/) covers the history and controversies |

## TODO

◦ This is not an operating systems course…
◦ Everything is a file philosophy and mknod (files themselves are an abstraction)
◦ File handles and file systems
◦ mmap
◦ The page cache
◦ Disk caching
◦ Sockets / UDS
◦ Process and thread scheduling
    ▪ Cooperative multithreading
    ▪ The other one

--8<-- "docs-include/abbreviations.md"
