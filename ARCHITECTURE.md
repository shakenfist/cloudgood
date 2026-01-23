# Architecture

This repository contains educational documentation about cloud computing,
virtualization, and distributed systems. The content is written for
shakenfist.com and is synced into the main Shaken Fist documentation site.

## Repository Structure

```
cloudgood/
├── docs/                    # Markdown documentation files
│   ├── index.md             # Introduction and navigation
│   ├── order.yml            # Controls import order and titles
│   ├── technology-primer.md # Fast-paced technical primer
│   ├── fundamentals.md      # Core computing concepts
│   ├── virtualization-history.md  # History of virtualization
│   ├── more-fundamentals.md # Additional OS concepts
│   ├── containers.md        # Containers (placeholder)
│   ├── virtual-networking.md # Virtual networking (placeholder)
│   ├── storage.md           # Storage (placeholder)
│   ├── orchestration.md     # Orchestration (placeholder)
│   ├── device-passthrough.md # Device passthrough (placeholder)
│   └── advanced-hardware.md # RDMA, CXL, etc. (placeholder)
├── STYLEGUIDE.md            # Documentation style conventions
├── .pre-commit-config.yaml  # Pre-commit hook configuration
└── .markdownlint.yaml       # Markdownlint configuration
```

## Documentation Flow

The documents are designed to be read in a specific order:

1. **technology-primer.md** - Fast overview for those with technical background
2. **fundamentals.md** - Core concepts (computers, kernels, system calls)
3. **virtualization-history.md** - Historical context (UML through modern DPUs)
4. **more-fundamentals.md** - Deeper OS concepts (memory, processes, boot)

Future chapters (currently placeholders):

5. **containers.md** - Containers and their relationship to VMs
6. **virtual-networking.md** - SDN, OVS, overlays, eBPF
7. **storage.md** - Block, object, distributed storage
8. **orchestration.md** - OpenStack, Kubernetes, infrastructure management
9. **device-passthrough.md** - PCI, SR-IOV, GPU, USB passthrough
10. **advanced-hardware.md** - RDMA, CXL, emerging technologies

## Build Process

This repository does not contain an mkdocs configuration. The content is
imported into `shakenfist/shakenfist/docs/components/cloudgood/` by a GitHub
Actions workflow that runs hourly. The import is controlled by:

- **order.yml** - Defines which files to import and in what order
- **sync_component_docs.py** - The import script in shakenfist/actions/tools/

Files commented out in order.yml are considered incomplete and won't be
imported to the live site.

## Markdown Features

The documentation uses mkdocs-material extensions:

- **Admonitions** - `!!! note`, `!!! info`, `!!! quote`, etc.
- **Collapsible admonitions** - `???+ info` (expanded) or `??? info` (collapsed)
- **Annotations** - Pop-up footnotes using `{ .annotate }` and numbered lists
- **Tables** - Standard markdown tables, often in "Want to know more?" sections

See STYLEGUIDE.md for detailed examples.

## Linting

Markdown files are linted with markdownlint-cli2. The configuration in
.markdownlint.yaml disables rules that conflict with mkdocs-material syntax.

Run linting manually:
```bash
pre-commit run --all-files
```
