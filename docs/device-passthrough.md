# Device Passthrough (incomplete)

This chapter will explore how to give virtual machines direct access to
physical hardware.

## Planned topics

- Why passthrough matters (performance, functionality)
- IOMMU and VFIO fundamentals
- PCI passthrough basics
- SR-IOV (Single Root I/O Virtualization)
  - Virtual Functions vs Physical Functions
  - Network card SR-IOV
  - NVMe SR-IOV
- GPU passthrough
  - Use cases (ML/AI, VDI, gaming)
  - NVIDIA vGPU vs passthrough
  - AMD and Intel GPU virtualization
- USB passthrough
- The trade-offs (loss of live migration, hardware binding)
- Mediated devices (mdev)

--8<-- "docs-include/abbreviations.md"
