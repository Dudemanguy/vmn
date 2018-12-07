# Options

vmn has two different kinds of configuration parameters: one for vmn itself and another for the mpv client. Options for vmn itself can be specified via command line or in the configuration file located at `$HOME/.config/vmn/config`. Any option specified via command line will take priority over any value specified in the configuration file. The syntax for arguments is simply `--foo=value`. In the configuration file, `value` needs to be enclosed in quotes like so: `foo="value"`. 

Options for the mpv client cannot be set as command line arguments and must be set in the configuration file. These options follow standard [mpv.conf syntax](https://github.com/mpv-player/mpv/blob/master/etc/mpv.conf). Most mpv options should work without a problem.

### Main Options

``--input-mode<yes|no>``\
   Launches vmn in an interactive input mode. This returns the integer value of keys pressed on screen. This can be useful if you want to set a keybind but do not know the correct          keycode to use.

``--library=<directory>``\
   Tells vmn which parent directory to search for audio files. If this option is omitted or the directory is invalid, this defaults to `$HOME/Music`.

``--mpv-cfg=<yes|no>``\
   Tells the mpv client whether or not it is allowed to accept configuration. This defaults to `yes`.

``--mpv-cfg-dir=<directory>``\
   Tells the mpv client which directory to search for a configuration file. It will look for the configuration file the same way mpv does. `config` and `mpv.conf` are both valid and the    latter takes priority over the former if both exists. By default, this directory is `$HOME/.config/vmn` which means that mpv configuration values specified in the vmn configuration      file are accepted. This option does nothing if `mpv-cfg` is set to `no`.

``--sort=<metadata,filename,track-number,random,none>``\
   A comma, separated list (spaces are allowed) of sorting modes to be used with each metadata tag. The sort array must be the same length as the tag array or it will do nothing.           Currently, only metadata       sorting and no sorting work. Metadata sorting simply alphabetically sorts the menu based on the tag names. By default, metadata sorting is used for        everything except the "title" tag.

``--tags=<metadata,tags>``\
   A comma, separated list of metadata values (spaces are allowed). vmn will read all valid music files and attempt to organize the curses interface based on the entered tag values. This   only has an effect if the metadata view is used. By default, the organization (from left to right) is "artist, album, title."

``--view=<file-path|metadata|song-only>``\
   Controls which view to use for vmn. This defaults to `file-path` which creates a navigable menus that go up and down directories that contain valid audio files. The `metadata` view      uses ffmpeg to read metadata off of every file and organizes the menu based on it. The `song-only` view outputs the complete paths to all valid audio files found in alphabetical         order. Note that metadata view can be very slow off of a cold start.

## Keybindings

Like the main options, keybindings in vmn are defined in the configuration file as `foo="value"`. However, these options cannot be set as command line arguments. Settings for keybindings are read as strings, but are internally used as ASCII characters. If more than a single character is specified for an option (i.e. `foo="key"`), then only the first character will be read and the rest will be ignored (so `foo` is set to `k`). Currently, the only modifier key supported is `Ctrl` which can be specified in the configuration by adding `Ctrl+`. For example, `foo="Ctrl+i"` sets `foo` to Ctrl and i. Note that the order does not matter, so `foo="i+Ctrl"` is also valid. If multiple "+'s" are given, vmn will use the first non-Ctrl character and ignore the rest. So `foo="Ctrl+i+j+k"` is read as just `Ctrl+i`. `foo="i+j+Ctrl+k"` also becomes `Ctrl+i`. Additionally, setting integer values for keybindings (`foo="12"`) is also supported since ncurses uses ASCII. ``input-mode`` can be used to find specific integer values of certain key/key combinations. Integers can also be combined with `Ctrl+` like normal characters. 


 Since vmn uses ncurses, the [macros](https://www.gnu.org/software/guile-ncurses/manual/html_node/Getting-characters-from-the-keyboard.html) for special keys are valid (i.e. `foo="KEY_RIGHT"` sets foo to the right arrow key). Function keys (f1, f2, f3, etc.) are also supported via `foo="f1"`. Note that the vast majority of those macros are for keys that are completely obsolete and nonexistent on current keyboards. Only the common keys are currently enabled in vmn, but support for one of the more esoteric keys can trivially be added upon request. Combining these macros with `Ctrl+` unfortunately does not work. If this is done, an error is given and vmn reverts back to the default settings. 
 ### Keybind commands

``beginning=<key>``\
  Jump to the first item in the menu.

``end=<key>``\
  Jump to the last item in the menu.

``move-backward=<key>``\
  Move up in the directory and destroy the rightmost menu (only valid in `file-path` view).

``move-down=<key>``\
  Move down one item in the menu.

``move-forward=<key>``\
  Move down in the directory and into a new menu (only valid in `file-path` view).

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

``voldown=<key>``\
  Decrease volume.

``volup=<key>``\
  Increase volume.
