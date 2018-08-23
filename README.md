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
```

## Usage
vmn first needs to know what directory you keep your music files in. It will search the `$USER/.config/vmn/config`, so be sure to create those directories and file first. Then enter in directory that contains your music files like so.
```
library = "path/to/music/directory"
```

Then, simply run the `vmn` executable. Current keybindings are the following.

* scroll up: `Up Arrow` or `k`
* scroll down: `Down Arrow` or `j`
* scroll page up: `Page Up`
* scroll page down: `Page Down`
* jump to beginning: `Home Key` or `g`
* jump to end: `End Key` or `G`
* select track: `i`
* launch mpv: `Enter`

All tracks that are selected will be launched by mpv upon hitting enter.

## License
GPLv2 or later.
