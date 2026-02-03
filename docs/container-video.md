---
issue_repo: https://github.com/shakenfist/cloudgood
issue_path: docs/container-video.md
---

# Container video

How do you provide graphical output from a headless Docker container? This
document explores the mechanisms that make this possible, with particular focus
on Kasm Workspaces as a reference implementation.

## The fundamental challenge

Docker containers are typically headless -- they have no physical display
hardware, no GPU (unless explicitly passed through), and no input devices. Yet
users expect to run full desktop environments and graphical applications inside
them. The solution involves several layers of abstraction that together create
a "virtual video device" of sorts.

## The X Window System model

To understand container graphics, we first need to understand how traditional
Linux graphics work. The X Window System (X11) uses a client-server model:

- **X server**: Manages the display hardware, handles input devices, and
  maintains the framebuffer (the region of memory that holds the pixels shown
  on screen)
- **X clients**: Applications that want to draw graphics. They send drawing
  commands to the X server rather than manipulating the framebuffer directly

This separation is key -- X clients don't need direct hardware access. They
just need to communicate with an X server. This means we can substitute a
"virtual" X server that doesn't require real hardware.

## Xvfb: The virtual framebuffer

[Xvfb](https://www.x.org/releases/X11R7.6/doc/man/man1/Xvfb.1.xhtml) (X Virtual
Framebuffer) is an X server that performs all graphical operations in memory
without any actual display output. From the perspective of X client
applications, Xvfb behaves identically to a real X server.

Xvfb allocates a region of memory to serve as the framebuffer. This can be:

- **malloc()**: Simple heap allocation (the default)
- **Shared memory**: Using System V shared memory, allowing other processes to
  read the framebuffer directly
- **Memory-mapped files**: The framebuffer is written to a file in
  [XWD format](https://en.wikipedia.org/wiki/Xwd), allowing snapshots to be
  taken with a simple file copy

When combined with a VNC server like
[x11vnc](https://wiki.archlinux.org/title/X11vnc), you can expose this virtual
display over the network. However, this two-process approach (Xvfb + x11vnc)
has overhead and latency implications.

## Xvnc: The integrated solution

[Xvnc](https://kasmweb.com/kasmvnc/docs/master/man/Xvnc.html) takes a different
approach by combining the X server and VNC server into a single process. As the
KasmVNC documentation puts it:

> Xvnc is the X VNC (Virtual Network Computing) server. It is based on a
> standard X server, but it has a "virtual" screen rather than a physical one.
> X applications display themselves on it as if it were a normal X display, but
> they can only be accessed via a VNC viewer. So Xvnc is really two servers in
> one. To the applications it is an X server, and to the remote VNC users it is
> a VNC server.

This integration means:

1. Applications draw to Xvnc using standard X11 protocols
2. Xvnc maintains the framebuffer in memory
3. The built-in VNC server reads the same framebuffer directly
4. Changes are encoded and streamed to VNC clients

Because both components share the same memory space, there's no inter-process
communication overhead for framebuffer access.

## KasmVNC: Modern web-based streaming

[KasmVNC](https://github.com/kasmtech/KasmVNC) extends Xvnc with modern
web-based streaming capabilities. Key features include:

**Encoding strategies:**

- WebP and JPEG compression with automatic selection based on CPU availability
- Dynamic quality adjustment based on screen change rates
- Full-screen video detection that switches to optimised video encoding
- Lossless QOI format for LAN connections
- Multi-threaded encoding for smoother frame rates

**Smart update detection:**

- Pixel comparison on the framebuffer to reduce unnecessary updates
- Configurable thresholds for when to enter "video mode" (high change rate)

**Browser-native delivery:**

- Uses WebAssembly, WebRTC, and WebGL on the client side
- No traditional VNC viewer required -- just a web browser
- Built-in web server for serving the client application

Importantly, KasmVNC explicitly disables Xvfb and Xwayland in its build
configuration, relying entirely on its integrated Xvnc-based approach.

## The complete Kasm stack

[Kasm Workspaces](https://kasm.com/docs/latest/guide/workspaces.html) assembles
these components into a complete solution:

| Component   | Purpose                                              |
|-------------|------------------------------------------------------|
| KasmVNC     | Integrated X server and VNC streaming                |
| Kclient     | NodeJS wrapper providing audio and file access       |
| NGINX       | Serves the web interface with appropriate headers    |
| PulseAudio  | Captures audio and streams it alongside video        |

The container image includes all these components. When a container starts:

1. KasmVNC starts as the X server (typically on display `:1`)
2. A window manager or desktop environment connects to that display
3. Applications run inside the container, drawing to KasmVNC
4. The web client connects via NGINX and receives the video stream

## Is there a "virtual video device"?

To answer the original question: not in the traditional sense. There's no
kernel-level virtual device like `/dev/video0` being created. Instead:

- **The X server is the abstraction layer**. Applications don't talk to video
  hardware directly -- they talk to the X server
- **The framebuffer is just memory**. Xvnc maintains a region of memory that
  holds the current display state
- **VNC is the transport**. The framebuffer contents are encoded and sent over
  the network

This is fundamentally different from, say, a virtual webcam device or a virtual
GPU. The abstraction happens at the display protocol level (X11), not at the
kernel device level.

## Comparison with SPICE/QXL

VNC-based solutions like KasmVNC operate at the framebuffer level -- they send
compressed bitmap updates representing changed regions of pixels. An
alternative approach, used by [SPICE](https://www.spice-space.org/) with the
QXL virtual GPU, operates at the drawing command level.

### How SPICE/QXL works

In a SPICE/QXL setup (typically used with QEMU/KVM virtual machines):

1. The guest OS has a QXL display driver installed
2. When applications make drawing calls (via X11 or Windows GDI), the QXL
   driver intercepts these commands
3. Commands are translated into platform-independent QXL primitives
4. These primitives are sent to the SPICE client
5. The client renders the commands locally, using its own GPU or CPU

This means SPICE can send "scroll this region by 50 pixels" or "fill this
rectangle with blue" rather than sending the resulting pixels.

### Practical differences

| Operation       | VNC (bitmap)                          | SPICE/QXL (commands)                |
|-----------------|---------------------------------------|-------------------------------------|
| Scrolling       | Sends entire scrolled region as pixels | Sends scroll command; client moves pixels locally |
| Solid fill      | Sends rectangle of identical pixels   | Sends "fill with colour" command    |
| Repeated images | Re-sends bitmap (unless cached)       | Client caches and reuses            |
| Text rendering  | Sends text as rasterised pixels       | Can send glyph data for client rendering |
| Window drag     | Continuous bitmap updates             | Window position updates             |

The SPICE documentation notes that even basic QXL acceleration "provides a
better user experience than VNC because scrolling is accelerated and doesn't
result in a huge bitmap getting sent across the wire."

### Why containers use VNC instead of SPICE

SPICE/QXL requires:

- A **virtual GPU device** (QXL) that the hypervisor presents to the guest
- A **kernel driver** in the guest OS that talks to this virtual GPU
- The **QEMU/KVM virtualisation layer** to provide the QXL device emulation

Docker containers don't have this infrastructure. They share the host kernel
directly -- there's no hypervisor layer to inject a virtual GPU device, and no
separate guest kernel to load a QXL driver into.

The X11 client-server model provides the natural interception point for
containers. Applications already communicate with the X server via a socket
protocol, so we can substitute a virtual X server (Xvnc) without any kernel
modifications or special drivers. It's a userspace-only solution.

### Could containers use a command-based approach?

In theory, one could intercept X11 drawing commands before they're rasterised
and forward them to a remote client. Some older projects like
[NX](https://en.wikipedia.org/wiki/NX_technology) took this approach,
compressing and optimising the X11 protocol itself.

However, modern applications increasingly bypass X11's drawing primitives:

- **Client-side rendering**: Toolkits like GTK3/4 and Qt render to pixmaps
  locally, then send the finished bitmap to the X server
- **Direct rendering**: OpenGL/Vulkan applications render via DRI, bypassing
  X11 drawing commands entirely
- **Wayland**: The successor to X11 is explicitly designed around
  client-side rendering with buffer sharing

This trend toward client-side rendering means there are fewer high-level
drawing commands to intercept, making framebuffer-based approaches like VNC
increasingly practical even compared to command-based protocols.

### Xspice: SPICE without a hypervisor

[Xspice](https://www.spice-space.org/xspice.html) is the SPICE equivalent of
Xvnc -- an integrated X server and SPICE server in a single process. It
provides command-based SPICE rendering without requiring a virtual machine.

**How it works:**

Xspice reuses the QXL X driver code from QEMU, but adapts it for direct
execution rather than hardware emulation:

- I/O port access becomes direct function calls
- Hardware interrupts become function calls
- The spice-server library is linked directly into the driver
- Memory is managed as a single slot covering the whole address space

This means X clients draw to Xspice using standard X11 protocols, the QXL
driver translates those drawing commands into SPICE primitives, and the
integrated spice-server streams them to SPICE clients -- all without a
hypervisor layer.

**Limitations:**

- ~256MB memory footprint
- Some rendering operations (lines, circles) are slower than expected
- Server-side cursors only (cursor channel underutilised)
- Known bug with server reset when last client disconnects (workaround:
  `-noreset` flag)
- Less actively maintained than KasmVNC

**Why Kasm chose VNC over Xspice:**

1. **Browser delivery**: SPICE clients are desktop applications. There's no
   mature browser-based SPICE client comparable to KasmVNC's WebSocket/WebGL
   approach. Kasm's entire value proposition is browser-based access.

2. **Maintenance**: KasmVNC is actively developed with modern web technologies.
   Xspice development has been relatively quiet.

3. **Diminishing returns**: With modern toolkits doing client-side rendering,
   the command-based advantages of SPICE are less pronounced. A well-optimised
   framebuffer approach like KasmVNC may perform similarly in practice.

4. **Ecosystem**: VNC is ubiquitous and well-understood. The tooling, debugging,
   and operational knowledge around VNC is extensive.

That said, Xspice remains an interesting option for scenarios where you need
SPICE's features (like USB redirection or smartcard passthrough) in a container
environment, and can use a native SPICE client.

### xRDP: RDP for Linux

[xRDP](https://github.com/neutrinolabs/xrdp) is an open-source implementation
of Microsoft's Remote Desktop Protocol (RDP) server for Linux. It's interesting
because it can operate in two fundamentally different modes, spanning the
framebuffer-to-command spectrum.

**Xvnc backend (legacy):**

```
X apps → Xvnc (framebuffer) → VNC protocol → xRDP → RDP protocol → client
```

In this mode, xRDP acts as a protocol translator. Xvnc produces framebuffer
updates using the VNC protocol, and xRDP converts these to RDP format. This
involves double encoding and was the original approach.

**xorgxrdp backend (modern default):**

```
X apps → Xorg + xorgxrdp module → shared memory → xRDP → RDP protocol → client
```

The [xorgxrdp](https://github.com/neutrinolabs/xorgxrdp) module is an Xorg
driver that intercepts drawing commands directly and communicates with xRDP
via shared memory. This is architecturally similar to Xspice -- the
interception happens at the X server level, and RDP can send drawing commands
(window moves, font rendering, region updates) rather than raw pixels.

The xRDP developers explicitly moved to xorgxrdp because "due to the slow
performance of forwarding to a VNC server, the developers introduced X11rdp
mode" (xorgxrdp's predecessor).

**Where xRDP fits:**

| Solution | Interception point | Transport | What gets sent |
|----------|-------------------|-----------|----------------|
| Xvnc / KasmVNC | Framebuffer | VNC/WebSocket | Bitmap updates |
| xRDP + Xvnc | Framebuffer | VNC→RDP | Bitmap updates |
| xRDP + xorgxrdp | X server module | RDP | Drawing commands |
| Xspice | X server module | SPICE | Drawing commands |
| SPICE/QXL (VM) | Virtual GPU | SPICE | Drawing commands |

**Advantages of xRDP:**

- **Native Windows client**: Every Windows machine has an RDP client built in.
  No software installation required for end users.
- **Enterprise familiarity**: IT departments understand RDP; it fits existing
  access policies and tooling.
- **Protocol features**: RDP supports clipboard, audio, drive redirection,
  printing, and multi-monitor setups.
- **Adaptive compression**: RDP adjusts encoding based on network conditions.

**Why Kasm uses VNC instead:**

1. **Browser-first**: Kasm's value proposition is browser-based access. While
   web-based RDP clients exist (like Apache Guacamole), KasmVNC's integrated
   approach is more streamlined.

2. **Licensing clarity**: RDP is a Microsoft protocol. While xRDP is open
   source, the protocol itself has historically had licensing ambiguity that
   some organisations prefer to avoid.

3. **Control**: KasmVNC gives Kasm full control over the client experience,
   encoding strategies, and feature development.

xRDP remains an excellent choice for containers that need to serve Windows
users with native RDP clients, particularly in enterprise environments where
RDP is already the standard remote access protocol.

## GPU acceleration

For workloads that need actual GPU acceleration (3D graphics, video decode),
Kasm can mount in a GPU using NVIDIA Container Runtime:

```bash
docker run -e NVIDIA_VISIBLE_DEVICES=all --gpus all ...
```

In this case, the GPU provides hardware acceleration for rendering, but the
final framebuffer is still captured and streamed via KasmVNC. The GPU
accelerates the rendering; the VNC layer handles the display output.

## Further reading

- [KasmVNC GitHub repository](https://github.com/kasmtech/KasmVNC)
- [KasmVNC documentation](https://kasmweb.com/kasmvnc/docs/1.0.0/index.html)
- [Xvfb man page](https://www.x.org/archive/X11R7.7/doc/man/man1/Xvfb.1.xhtml)
- [Xvfb on Wikipedia](https://en.wikipedia.org/wiki/Xvfb)
- [SPICE for Newbies](https://www.spice-space.org/spice-for-newbies.html) --
  detailed explanation of the QXL command-based architecture
- [SPICE on the KVM wiki](https://www.linux-kvm.org/page/SPICE)
- [Xspice documentation](https://www.spice-space.org/xspice.html) -- integrated
  X server and SPICE server for non-VM environments
- [xRDP GitHub repository](https://github.com/neutrinolabs/xrdp) -- RDP server
  for Linux
- [xRDP on Wikipedia](https://en.wikipedia.org/wiki/Xrdp)
- [xorgxrdp GitHub repository](https://github.com/neutrinolabs/xorgxrdp) --
  Xorg driver for direct xRDP integration
- [Kerbside SPICE proxy documentation](https://shakenfist.com/components/kerbside/)
- [Running GUI apps in Docker](https://www.trickster.dev/post/running-gui-apps-within-docker-containers/)

--8<-- "docs-include/abbreviations.md"
