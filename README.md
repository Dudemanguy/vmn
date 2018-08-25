# vmn
**Note**: This is very much a work in progress.

**v**im **m**usic **n**avigator is a simple, barebones commandline music player. vmn aims to provide a highly customizable, configurable interface to handle massive music libraries with ease. One feature of vmn is that it makes use of mpv's client API for playback which gives you all of the power of mpv's options integrated into a nice interface for browsing and listening to your music.

## Installation
vmn only depends on ncurses, the ncurses menu library (likely already included with your distro's ncurses package), libconfig, mpv, and meson for building. First, checkout the source.
```
git clone https://github.com/Dudemanguy911/vmn.git
```

Then navigate to that directory.
```
mkdir build
meson build
ninja -C build
sudo ninja -C build install
```

## Usage
vmn reads its configuration in the standard `$USER/.confg/vmn/config` directory. By default, vmn will search XDG_MUSIC_DIR, `$USER/Music`, if no library directory is specified. If a different library directory needs to be read then specify it in the config file like so.
```
library = "path/to/music/directory"
```

Then, simply run the `vmn` executable. Current keybindings are the following.

* scroll up: `Up Arrow` or `k`
* scroll down: `Down Arrow` or `j`
* scroll page up: `Page Up` or `CTRL+b`
* scroll page down: `Page Down` or `CTRL+f`
* jump to beginning: `Home Key` or `g`
* jump to end: `End Key` or `G`
* queue track: `i`
* queue all tracks: `y`
* clear queue: `u`
* playback with mpv: `Enter`
* quit: `q`

All tracks that are queued will be played by mpv upon hitting enter. If no tracks are selected, then the currently highlighted track will be played upon hitting enter.

## License
GPLv2 or later.
