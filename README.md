# vmn
**v**im **m**usic **n**avigator is a simple, barebones commandline music player. vmn aims to provide a highly customizable, configurable interface to handle massive music libraries with ease. One feature of vmn is that it makes use of mpv's client API for playback which gives you all of the power of mpv's options integrated into a nice interface for browsing and listening to your music.

## Installation
Make sure you have the following dependencies installed.

* ffmpeg
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
The default keybindings are the following.

* beginning: `g`
* end: `G`
* move backward: `h`
* move down: `j`
* move forward: `l`
* move up: `k`
* mpv kill: `Q`
* mute: `m`
* page down: `Ctrl+f`
* page up: `Ctrl+b`
* playnext: `>`
* playpause: `Space`
* playprev: `<`
* queue: `i`
* queue all: `y`
* queue clear: `u`
* quit: `Ctrl+[`
* search: `/`
* search next: `n`
* search prev: `N`
* start: `Enter`
* visual mode: `v`
* vmn quit: `q`
* vmn refresh: `a`
* voldown: `9`
* volup: `0`

To change keybindings, see the [options](https://github.com/Dudemanguy911/vmn/blob/master/options.md).

All items that are queued will be played by mpv upon hitting enter. If no items are selected, then the currently highlighted items will be played upon hitting enter. Items can be either directories or individual audio files. Playing back a directory will add all valid audio files to mpv for playback.

## License
GPLv2 or later.
