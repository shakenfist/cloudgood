This document is intended to provide some style pointers in an attempt
to keep the overall document consistent. Honestly, they're also here
because I will forget these things as well and it gives me a quick
reference point to cut and paste from.

This site is rendered with mkdocs and served using github static HTML
hosting. Yes I know there's no mkdocs configuration in this repository,
that's because the content is on shakenfist.com, and that is driven by
the mkdocs.yml file in `shakenfist/shakenfist/`. There is a github
actions workflow which copies this content across once an hour and
publishes it if there have been any changes merged here.

## Admonitions

Because we use mkdocs, the relevant documentation is at https://squidfunk.github.io/mkdocs-material/reference/admonitions/.

???+ info

    This is an informational note. More detail which is an aside from
    the primary direction of the content should use one of these.

!!! note

    As mentioned in the [technology primer](technology-primter.md), some amount
    of this documentation is lifted from an unreleased project. One day when
    I release that thing someone should remind me to update this section to
    explain how it is also sometimes quite cool to execute raw code on modern
    CPUs. Or at least I think so.

???+ tip "REST API calls"

    This is a tip. Something small which not everyone would think is
    obvious, like "remember to run this command as root".

??? example "Python API client: get all visible instances"

    ```python
    import json
    from shakenfist_client import apiclient

    sf_client = apiclient.Client()
    instances = sf_client.get_instances()
    print(json.dumps(instances, indent=4, sort_keys=True))
    ```

!!! quote

    You might think that machine instructions are the basic steps that a computer performs. However, instructions usually require multiple steps inside the processor. One way of expressing these multiple steps is through microcode, a technique dating back to 1951. To execute a machine instruction, the computer internally executes several simpler micro-instructions, specified by the microcode. In other words, microcode forms another layer between the machine instructions and the hardware. The main advantage of microcode is that it turns the processor's control logic into a programming task instead of a difficult logic design task.

    https://www.righto.com/2023/07/undocumented-8086-instructions.html


There are many other types I have not yet used: abstract; success;
question; warning; failure; danger; and bug. As I decide what to use
these for they should be moved to the paragraphs above here with a worked
example.

## Annotations

Annotations are "pop up footnotes". The relevant documentation is at
https://squidfunk.github.io/mkdocs-material/reference/annotations/

An example:

Virtualization of compute is a very old technology (1). IBM had something which looked a bit like modern virtualization on its mainframe platforms in the 1960s...
{ .annotate }

1. https://en.wikipedia.org/wiki/Timeline_of_virtualization_technologies has a useful timeline if you're that way inclined.

The numbering of the annotations need only be unique within the paragraph the annotation exists in.

## Abbreviations

Abbreviations enable hover tooltips for technical terms. The definitions
are invisible in the rendered output -- readers just see tooltips when
hovering over defined terms like CPU or VM. The relevant documentation is at
https://squidfunk.github.io/mkdocs-material/reference/tooltips/#adding-abbreviations

### Using the shared abbreviations file

A shared abbreviations file exists with common virtualization and systems
terms. Include it at the very end of your markdown file:

```markdown
--8<-- "docs-include/abbreviations.md"
```

This single line gives you tooltips for terms like CPU, VM, KVM, EPT, TLB,
and many others. The file lives in `shakenfist/shakenfist/docs-include/`
and is available during the documentation build.

### Adding document-specific abbreviations

You can also define abbreviations directly in a document. Place them
anywhere (typically at the bottom, before the shared include):

*[API]: Application Programming Interface
*[VM]: Virtual Machine
*[CPU]: Central Processing Unit

The syntax is `*[TERM]: Definition -- optional longer explanation.`

## Definition Lists

Definition lists are useful for glossaries or documenting parameters. The
relevant documentation is at
https://squidfunk.github.io/mkdocs-material/reference/lists/#using-definition-lists

`--network`
:   The network to attach the instance to. Can be specified multiple times
    for multiple network interfaces.

`--disk`
:   A disk specification for the instance. The format is
    `size@bus:base` where size is in gigabytes.

`--cpu`
:   The number of vCPUs to allocate to the instance.

## Footnotes

Footnotes are traditional numbered references at the bottom of the page.
The relevant documentation is at
https://squidfunk.github.io/mkdocs-material/reference/footnotes/

Here is some text with a footnote[^1] and another[^2].

[^1]: This is the first footnote content. It will appear at the bottom of
    the rendered page.
[^2]: Footnotes can contain multiple paragraphs and even code blocks if
    you indent them properly.

## Code Highlighting

Code blocks support syntax highlighting with line numbers and line
highlighting. The relevant documentation is at
https://squidfunk.github.io/mkdocs-material/reference/code-blocks/

```python title="example.py" linenums="1" hl_lines="2-3"
def hello_world():
    message = "Hello, World!"
    print(message)
    return message
```

The `title` adds a filename header, `linenums` enables line numbers
starting from the specified value, and `hl_lines` highlights specific
lines.

## Tabbed Content

Tabbed content is useful for showing alternatives like different operating
systems or programming languages. The relevant documentation is at
https://squidfunk.github.io/mkdocs-material/reference/content-tabs/

=== "Ubuntu/Debian"

    ```bash
    sudo apt install python3-pip
    pip install shakenfist-client
    ```

=== "Fedora/RHEL"

    ```bash
    sudo dnf install python3-pip
    pip install shakenfist-client
    ```

=== "From source"

    ```bash
    git clone https://github.com/shakenfist/client-python
    cd client-python
    pip install -e .
    ```

## Task Lists

Task lists render as checkboxes, useful for checklists or tracking progress.
The relevant documentation is at
https://squidfunk.github.io/mkdocs-material/reference/lists/#using-task-lists

- [x] Create the virtual network
- [x] Upload the base image
- [ ] Launch the instance
- [ ] Configure networking
- [ ] Install application

## Table of Contents Permalinks

With `toc: permalink: true` in the config, each heading gets a clickable
link icon that appears on hover. This makes it easy to link directly to
specific sections of a document. No special syntax is needed -- it applies
automatically to all headings.