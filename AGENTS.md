# Agent Instructions

This document provides guidance for AI assistants working with this repository.

## Repository Purpose

This is an educational documentation repository about cloud computing and
virtualization. The content is aimed at developers learning about:

- How computers and operating systems work
- Virtualization concepts and history
- Linux kernel fundamentals
- Container and VM technologies

## Key Files

- **docs/order.yml** - Controls which files are imported and their order.
  Commented lines (starting with #) are incomplete files not yet published.
- **STYLEGUIDE.md** - Documentation conventions for mkdocs-material syntax.
- **.markdownlint.yaml** - Linting rules; review before suggesting changes.

## Writing Style

- Use conversational but technically accurate tone
- Explain jargon when first introduced
- Use annotations `{ .annotate }` for asides and footnotes
- Use admonitions for supplementary information:
  - `!!! note` for important asides
  - `!!! info "Want to know more?"` for reference tables
  - `!!! quote` for external quotations with attribution
- Wrap lines at 80 characters for prose paragraphs
- Use single quotes for Python strings (per user preference)

## Annotations Syntax

Annotations require:
1. The `{ .annotate }` marker after the paragraph
2. A numbered list using periods (`1.`, `2.`) not colons
3. Numbers only need to be unique within that paragraph

Example:
```markdown
This is a fact (1) with an annotation.
{ .annotate }

1. This is the annotation text.
```

## Pre-commit Checks

Always run before proposing commits:
```bash
pre-commit run --all-files
```

This runs markdownlint-cli2 on all docs/*.md files.

## Common Tasks

### Adding a new document

1. Create the .md file in docs/
2. Add an entry to docs/order.yml (commented if incomplete)
3. Update docs/index.md to reference the new document
4. Run pre-commit to validate

### Marking a document as complete

1. Uncomment the entry in docs/order.yml
2. The sync workflow will import it on next run

### Fixing linting errors

Common issues and fixes:
- MD009: Trailing spaces - remove them
- MD012: Multiple blank lines - reduce to single blank line
- MD010: Hard tabs - replace with spaces
- MD047: Missing final newline - add newline at end of file

## MkDocs Extensions

This documentation uses Material for MkDocs with several extensions. Use them
consistently to improve readability. Full examples are in STYLEGUIDE.md.

### Abbreviations (hover tooltips)

Every document should include the shared abbreviations file at the very end:

```markdown
--8<-- "docs-include/abbreviations.md"
```

This provides tooltips for common terms (CPU, VM, KVM, EPT, TLB, etc.). The
definitions are invisible - readers just see tooltips on hover. Add
document-specific abbreviations before the include if needed.

### Definition Lists

Use for defining terms, parameters, or options. Preferred over bullet lists
when each item is a term with a description:

```markdown
`virtio-net`
:   Paravirtualized network card for efficient packet transmission.

`virtio-blk`
:   Paravirtualized block device for disk I/O.
```

### Tabbed Content

Use when showing alternatives (different OS commands, languages, or methods):

```markdown
=== "Ubuntu/Debian"

    ```bash
    sudo apt install package
    ```

=== "Fedora/RHEL"

    ```bash
    sudo dnf install package
    ```
```

### Code Blocks

Use `title` for filenames and `linenums` for longer examples:

````markdown
```python title="example.py" linenums="1" hl_lines="2-3"
def hello():
    message = "highlighted"
    print(message)
```
````

### Task Lists

Use for checklists or step tracking:

```markdown
- [x] Completed step
- [ ] Pending step
```

### When to Use What

| Content Type | Use This |
|--------------|----------|
| Term definitions | Definition list |
| OS/method alternatives | Tabbed content |
| Configuration options | Definition list |
| Step-by-step with tracking | Task list |
| Aside or footnote | Annotation `{ .annotate }` |
| Supplementary info | Admonition (`!!! info`) |
| External quote | Admonition (`!!! quote`) |
