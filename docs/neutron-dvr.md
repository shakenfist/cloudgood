---
issue_repo: https://github.com/shakenfist/cloudgood
issue_path: docs/neutron-dvr.md
---

# Neutron DVR

This chapter will explore how Neutron's Distributed Virtual Routing (DVR)
moves routing onto compute nodes, eliminating the centralized network node
bottleneck for east-west traffic while introducing significant additional
complexity.

## Planned topics

- The motivation: why centralized routing doesn't scale
- DVR architecture overview: routing on compute nodes
- Additional namespaces: `snat-*` and `fip-*`
- How OVS flow rules direct traffic to local router instances
- East-west traffic: local routing without leaving the hypervisor
- North-south traffic: the SNAT node for traffic without floating IPs
- Floating IP ownership and the `fip-*` namespace
- DVR + HA: combining distribution with failover
- The `DvrLocalRouter`, `DvrEdgeRouter`, and `DvrEdgeHaRouter` classes
- Additional OVS flow complexity and debugging challenges
- When DVR is worth the complexity and when it isn't

--8<-- "docs-include/abbreviations.md"
