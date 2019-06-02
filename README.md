# vmn
**v**im **m**usic **n**avigator is a simple, barebones commandline music player. vmn aims to provide a highly customizable, configurable interface to handle massive music libraries with ease. One feature of vmn is that it makes use of mpv's client API for playback which gives you all of the power of mpv's options integrated into a nice interface for browsing and listening to your music.

## Installation
Make sure you have the following dependencies installed.

* ffmpeg
* meson (0.47 or higher)
* mpv
* ncurses
* [scdoc](https://git.sr.ht/~sircmpwn/scdoc) (optional: for man pages)

After checking out the source, navigate to the directory, and then simply run.
```
mkdir build
meson build
ninja -C build
sudo ninja -C build install
```

## Releases
The release tags are super arbitrary and pretty much just personal bookkeeping for when I feel enough significant things have been accomplished. I would recommend just using the master branch.

## Configuration
See the [options](https://github.com/Dudemanguy911/vmn/blob/master/docs/options.md) page for details.

## Usage
The default keybindings are the following.

* beginning: `g`
* command: `:`
* end: `G`
* escape: `Ctrl+[`
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
* search: `/`
* search next: `n`
* search prev: `N`
* start: `Enter`
* visual mode: `v`
* vmn quit: `q`
* vmn refresh: `a`
* voldown: `9`
* volup: `0`

To change keybindings, see the [options](https://github.com/Dudemanguy911/vmn/blob/master/docs/options.md) page.

For more information on command mode, see the [command](https://github.com/Dudemanguy911/vmn/blob/master/docs/command.md) page.

All items that are queued will be played by mpv upon hitting enter. If no items are selected, then the currently highlighted items will be played upon hitting enter. Items can be a directory, a metadata tag, or individual audio files. Playing back a directory or metadata tag will add all valid audio files to mpv for playback.

## License
GPLv3
