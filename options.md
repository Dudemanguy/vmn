# Options

vmn has two different kinds of configuration parameters: one for vmn itself and another for the mpv client. Options for vmn itself can be specified via command line or in the configuration file located at `$HOME/.config/vmn/config`. Any option specified via command line will take priority over any value specified in the configuration file. The syntax for arguments is simply `--foo=value`. In the configuration file, `value` needs to be enclosed in quotes like so: `foo="value"`. 

Options for the mpv client cannot be set as command line arguments and must be set in the configuration file. These options follow standard [mpv.conf syntax](https://github.com/mpv-player/mpv/blob/master/etc/mpv.conf). Most mpv options should work without a problem.

### Main Options

``--library=<directory>``\
   Tells vmn which parent directory to search for audio files. If this option is omitted or the directory is invalid, this defaults to `$HOME/Music`.

``--mpv-cfg=<yes|no>``\
   Tells the mpv client whether or not it is allowed to accept configuration. This defaults to `yes`.

``--mpv-cfg-dir=<directory>``\
   Tells the mpv client which directory to search for a configuration file. It will look for the configuration file the same way mpv does. `config` and `mpv.conf` are both valid and the    latter takes priority over the former if both exists. By default, this directory is `$HOME/.config/vmn` which means that mpv configuration values specified in the vmn configuration      file are accepted. This option does nothing if `mpv-cfg` is set to `no`.

``--view=<file-path|song-only>``\
   Controls which view to use for vmn. This defaults to `file-path` which creates a navigable menus that go up and down directories that contain valid audio files. The `song-only` view     outputs the complete paths to all valid audio files found in alphabetical order.

## Keybindings

Like the main options, keybindings in vmn are defined in the configuration file as `foo="value"`. However, these options cannot be set as command line arguments. Settings for keybindings are read as strings, but are internally used as ASCII characters. If more than a single character is specified for an option (i.e. `foo="key"`), then only the first character will be read and the rest will be ignored (so `foo` is set to `k`). Currently, the only modifier key supported is `Ctrl` which can be specified in the configuration by adding `Ctrl+`. For example, `foo="Ctrl+i"` sets `foo` to Ctrl and i. Note that the order does not matter, so `foo="i+Ctrl"` is also valid. If multiple "+'s" are given, vmn will use the first non-Ctrl character and ignore the rest. So `foo="Ctrl+i+j+k"` is read as just `Ctrl+i`. `foo="i+j+Ctrl+k"` also becomes `Ctrl+i`.

 Since vmn uses ncurses, the [macros](https://www.gnu.org/software/guile-ncurses/manual/html_node/Getting-characters-from-the-keyboard.html) for special keys are valid (i.e. `foo="KEY_RIGHT"` sets foo to the right arrow key). Note that the vast majority of those macros are for keys that are completely obsolete and nonexistent on current keyboards. Only the common keys are currently enabled in vmn, but support for one of the more esoteric keys can trivially be added upon request. Combining these macros with `Ctrl+` unfortunately does not work. If this is done, an error is given and vmn reverts back to the default settings.

 ### Keybind commands

``move-up=<key>``\
  Move up one item in the menu.

``move-down=<key>``\
  Move down one item in the menu.

``move-forward=<key>``\
  Move down in the directory and into a new menu (only valid in `file-path` view).

``move-backward=<key>``\
  Move up in the directory and destroy the rightmost menu (only valid in `file-path` view).

``page-up=<key>``\
  Move up a full page in the menu.

``page-down=<key>``\
  Move down a full page in the menu.

``beginning=<key>``\
  Jump to the first item in the menu.

``end=<key>``\
  Jump to the last item in the menu.

``queue=<key>``\
  Queue item for playback.

``queue-all=<key>``\
  Queue all items in the menu for playback.

``queue-clear=<key>``\
  Empty queue and clear all items.

``visual=<key>``\
  Toggle visual selection mode.

``playback=<key>``\
  Begin playback.

``mpv-kill=<key>``\
  Kill current mpv client.

``vmn-quit=<key>``\
  Exit vmn.
