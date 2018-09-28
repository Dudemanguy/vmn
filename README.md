# vmn
**v**im **m**usic **n**avigator is a simple, barebones commandline music player. vmn aims to provide a highly customizable, configurable interface to handle massive music libraries with ease. One feature of vmn is that it makes use of mpv's client API for playback which gives you all of the power of mpv's options integrated into a nice interface for browsing and listening to your music.

## Installation
Make sure you have the following dependencies installed.

* libconfig
* meson
* mpv
* ncurses

After checking out the source, navigate to the directory, and then simply run.
```
mkdir build
meson build
ninja -C build
sudo ninja -C build install
```

## Configuration
See the [options](https://github.com/Dudemanguy911/vmn/blob/master/options.md) page for details.

## Usage
Current keybindings are the following.

* scroll up: `k` or `Up Arrow`
* scroll down: `j` or `Down Arrow`
* scroll page up: `CTRL+b` or `Page Up`
* scroll page down: `CTRL+f` or `Page Down`
* jump to beginning: `g` or `Home Key`
* jump to end: `G` or `End Key`
* move down a directory: 'l' or `Right Arrow`
* move up a directory: 'h' or `Left Arrow`
* queue item: `i` or `space`
* queue all items: `y`
* clear queue: `u`
* toggle visual selection mode: `v`
* playback with mpv: `Enter`
* quit: `q`

All items that are queued will be played by mpv upon hitting enter. If no items are selected, then the currently highlighted items will be played upon hitting enter. Items can be either directories or individual audio files. Playing back a directory will add all valid audio files to mpv for playback.

## License
GPLv2 or later.
