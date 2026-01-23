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