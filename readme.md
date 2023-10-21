<!-- -*- mode: markdown; coding: utf-8 -*- -->

# Keymap Laboratory Using Corne Keyboard

This site contains that the trials in order to create special keymaps
to minimize moving the hands using Corne Keyboard, which is the cute
42 keys vertically staggered keyboard.

See: [Foostan's Corne keyboard](https://github.com/foostan/crkbd/)

My goals are:

- Can generate all keycodes defined on US 104 keyboard,
- Japanese input using SKK can be easily done, and
- Friendly to use with Emacs.

## How keymap generation works

I'm using [QMK Firmware](https://docs.qmk.fm/),
and the firmware with special keymap is created
according to the instructions on that web pages.
Visit the page
[Building QMK with GitHub Userspace](https://docs.qmk.fm/#/newbs_building_firmware_workflow?id=building-qmk-with-github-userspace)
for more detailed description.

## How to see my keymap

You can use the site [QMK Configurator](https://config.qmk.fm/#/crkbd/rev1/LAYOUT_split_3x6_3)
with my *.json file.

## Current keyboard layouts

### Default Map (Layer 0)

![Layout 0](docs/L0.png "Layer 0")

### Alternative Default Map (Layer 1)

![Layout 1](docs/L1.png "Layer 1")

### Supplemental Map (Layer 2)

![Layout 2](docs/L2.png "Layer 2")

### Shifted Supplemental Map (Layer 3)

![Layout 3](docs/L3.png "Layer 3")
