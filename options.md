# Options

vmn has two different kinds of configuration parameters: one for vmn itself and another for the mpv client. Options for vmn itself can be specified via command line or in the configuration file located at `$HOME/.config/vmn/config`. Any option specified via command line will take priority over any value specified in the configuration file. The syntax for arguments is simply `--foo=value`. In the configuration file, `value` needs to be enclosed in quotes like so: `foo="value"`. 

Options for the mpv client cannot be set as command line arguments and must be set in the configuration file. These options follow standard [mpv.conf syntax](https://github.com/mpv-player/mpv/blob/master/etc/mpv.conf). Most mpv options should work without a problem.

### Main Options

``--input-mode<yes|no>``\
   Show an interactive prompt for returning ASCII keycodes. Can be useful for setting keybindings. Defaults to `no`.

``--library=<directory>``\
   Select the parent directory to search for audio files. If the path is omitted or invalid, this defaults to `$HOME/Music`.

``--mpv-cfg=<yes|no>``\
   Tells the mpv client whether or not to accept configuration. This defaults to `yes`.

``--mpv-cfg-dir=<directory>``\
   Tells the mpv client which directory to search for a configuration file. It searches for a configuration file in the same manner as mpv (i.e. `config` and `mpv.conf` are both valid). By default, the path searched is `$HOME/.config/vmn` which means that mpv configuration values specified in the vmn configuration file are valid. This does nothing if `mpv-cfg` is set to `no`.

``--sort=<metadata,filename,track-number,random,none>``\
   A comma, separated list (spaces are not allowed) of sorting modes to be used with each metadata tag. The sort array must be the same length as the tag array or it will do nothing. Currently, only metadata sorting and no sorting work. Metadata sorting sorts alphabetically based on the tag names. By default, metadata sorting is used for everything except the *title* tag.

``--tags=<metadata,tags>``\
   A comman, separated list of metadata values (spaces are not allowed). vmn will read all music files and attempt to organize the curses interface based on the entered tag values. This only has an effect if the metadata view is used. By default, the organization (from let to right) is `artist`,`album`,`title`.

``--view=<file-path|metadata|song-only>``\
   Controls which view to use for vmn. This defaults to `metadata` which creates navigable menus that are built via the metadata in the valid audio files. The `file-path` view organizes itself based on the directory structure, and the `song-only` view outputs the complete paths of all valid audio files found in alphabetical order. Note that initially storing metadata from a large amount of files into cache can be a tad slow.

## Keybindings

Like the main options, keybindings in vmn are defined in the configuration file as `foo="value"`. However, these options cannot be set as command line arguments. Settings for keybindings are read as strings, but are internally used as ASCII characters. If more than a single character is specified for an option (i.e. `foo="key"`), then only the first character will be read and the rest will be ignored (so `foo` is set to `k`). Currently, the only modifier key supported is `Ctrl` which can be specified in the configuration by adding `Ctrl+`. For example, `foo="Ctrl+i"` sets `foo` to Ctrl and i. Note that the order does not matter, so `foo="i+Ctrl"` is also valid. If multiple "+'s" are given, vmn will use the first non-Ctrl character and ignore the rest. So `foo="Ctrl+i+j+k"` is read as just `Ctrl+i`. `foo="i+j+Ctrl+k"` also becomes `Ctrl+i`. Additionally, setting integer values for keybindings (`foo="12"`) is also supported since ncurses uses ASCII. ``input-mode`` can be used to find specific integer values of certain key/key combinations. Integers can also be combined with `Ctrl+` like normal characters. 


 Since vmn uses ncurses, the [macros](https://www.gnu.org/software/guile-ncurses/manual/html_node/Getting-characters-from-the-keyboard.html) for special keys are valid (i.e. `foo="KEY_RIGHT"` sets foo to the right arrow key). Function keys (f1, f2, f3, etc.) are also supported via `foo="f1"`. Note that the vast majority of those macros are for keys that are completely obsolete and nonexistent on current keyboards. Only the common keys are currently enabled in vmn, but support for one of the more esoteric keys can trivially be added upon request. Combining these macros with `Ctrl+` unfortunately does not work. If this is done, an error is given and vmn reverts back to the default settings. 
 ### Keybind commands

``beginning=<key>``\
  Jump to the first item in the menu.

``end=<key>``\
  Jump to the last item in the menu.

``move-backward=<key>``\
  Move back a menu and destroy the rightmost menu (not valid in *song-only* view).

``move-down=<key>``\
  Move down one item in the menu.

``move-forward=<key>``\
  Move forward into a new menu (not valid in *song-only* view).

``move-up=<key>``\
  Move up one item in the menu.

``mpv-kill=<key>``\
  Kill current mpv client.

``mute=<key>``\
  Toggle mute for the current mpv client.

``page-down=<key>``\
  Move down a full page in the menu.

``page-up=<key>``\
  Move up a full page in the menu.

``playnext=<key>``\
  Move to the next track in mpv's playlist.

``playpause=<key>``\
  Toggle play/pause for the current mpv client.

``playprev=<key>``\
  Move to the previous track in mpv's playlist.

``queue=<key>``\
  Queue item for playback.

``queue-all=<key>``\
  Queue all items in the menu for playback.

``queue-clear=<key>``\
  Empty queue and clear all items.

``quit=<key>``\
  Quit visual or search mode and return to normal mode.

``search=<key>``\
  Enter search mode.

``search-next=<key>``\
  Move to the next valid match given from search mode.

``search-prev=<key>``\
  Move to the previous valid pattern from search mode.

``start=<key>``\
  Launch mpv client and begin playback.

``visual=<key>``\
  Toggle visual selection mode.

``vmn-quit=<key>``\
  Exit vmn.

``vmn-refresh=<key>``\
  Deletes and reloads all ffmpeg metadata for the currently highlighted item.

``voldown=<key>``\
  Decrease volume.

``volup=<key>``\
  Increase volume.
