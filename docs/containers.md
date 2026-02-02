---
issue_repo: https://github.com/shakenfist/cloudgood
issue_path: docs/containers.md
---

# Containers (incomplete)

This chapter will explore containers and how they relate to virtual machines.

## Planned topics (not in order)

More detail on processes
◦ Threads
◦ chroot
◦ Solaris Jails
◦ LXC
◦ Docker
◦ Podman and rootless containers
◦ Kubernetes and other orchestrators

- What is a container, really? (namespaces, cgroups, pivot_root)
- The isolation/performance trade-off compared to VMs
- Container runtimes (runc, crun, containerd, CRI-O)
- Why containers aren't as isolated as VMs
- The PID 1 problem in containers (referencing more-fundamentals.md)
- gVisor, Kata Containers, and other attempts to bridge the gap
- When to use containers vs VMs vs microVMs

--8<-- "docs-include/abbreviations.md"
