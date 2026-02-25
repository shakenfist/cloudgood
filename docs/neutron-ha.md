---
issue_repo: https://github.com/shakenfist/cloudgood
issue_path: docs/neutron-ha.md
---

# Neutron HA routers

This chapter will explore how Neutron's HA (High Availability) routers
extend the legacy router model with keepalived-based active/passive
failover, eliminating the single point of failure while keeping the same
namespace-based architecture.

## Planned topics

- The `HaRouter` class and how it extends `RouterInfo`
- keepalived and VRRP inside router namespaces
- The HA network: a hidden admin network connecting router replicas
- State transitions: backup → master failover
- Gratuitous ARP flooding on failover
- The `ha_confs_path` and keepalived configuration management
- How iptables rules are synchronized between active and standby
- Conntrack state and the cost of failover
- When HA routers make sense vs. DVR

--8<-- "docs-include/abbreviations.md"
