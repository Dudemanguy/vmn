# vmn
**v**im **m**usic **n**avigator is a simple, barebones commandline music player. vmn aims to provide a highly customizable, configurable interface to handle massive music libraries with ease. One feature of vmn is that it makes use of mpv's client API for playback which gives you all of the power of mpv's options integrated into a nice interface for browsing and listening to your music.

## Installation
vmn only depends on ncurses (with the menu library), libconfig, mpv, and meson for building. After checking out the source the source, navigate to the directory. Then simply run.
```
mkdir build
meson build
ninja -C build
sudo ninja -C build install
```

## Configuration
By default, vmn will search XDG_MUSIC_DIR, `$USER/Music`, for audio files. It's very likely that you would prefer a different directory. To do so, specify the library directory in `$USER/.config/vmn/config` like so.
```
library = "path/to/music/directory"
```
vmn accepts two different kinds of configurations: one for the vmn program itself and another for the mpv client. All configuration values are specified in the same `$USER/.config/vmn/config` file. The syntax is the same however options for vmn are enclosed in quotes while the options for the mpv client follow standard [mpv.conf syntax](https://github.com/mpv-player/mpv/blob/master/etc/mpv.conf). Most mpv options should work without a problem.

## Usage
Current keybindings are the following.

* scroll up: `k` or `Up Arrow`
* scroll down: `j` or `Down Arrow`
* scroll page up: `CTRL+b` or `Page Up`
* scroll page down: `CTRL+f` or `Page Down`
* jump to beginning: `g` or `Home Key`
* jump to end: `G` or `End Key`
* queue track: `i` or `space`
* queue all tracks: `y`
* clear queue: `u`
* toggle visual selection mode: `v`
* playback with mpv: `Enter`
* quit: `q`

All tracks that are queued will be played by mpv upon hitting enter. If no tracks are selected, then the currently highlighted track will be played upon hitting enter.

## License
GPLv2 or later.
