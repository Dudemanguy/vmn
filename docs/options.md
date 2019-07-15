# Options

Options for vmn follow the `foo=value` syntax. It's essentially identical to mpv's syntax except that you do not enclose `value` in quotes (i.e. if there are spaces inbetween values). mpv options can also be set in the config file (i.e. `volume=50` is valid). Refer to the full list of [mpv's options](https://github.com/mpv-player/mpv/blob/master/DOCS/man/options.rst). The config file's location is at `$HOME/.config/vmn/config`. Options specified via command line will overwrite anything specified in the configuration file. For command line arguments, the syntax is simply `--foo=value`. Currently, mpv-specific options cannot be set as command line arguments.

### Main Options

``--headless=<yes|no>``\
   A convenience option for forcing all video output off. This is exactly the same as setting `force-window`, `video`, and `osc` all to `no` in your config. By default, this option defaults to `no`.

``--input-mode=<yes|no>``\
   Show an interactive prompt for returning ASCII keycodes. Can be useful for setting keybindings. Defaults to `no`.

``--library=<directory>``\
   Select the parent directory to search for audio files. If the path is omitted or invalid, this defaults to `$HOME/Music`.

``--sort=<metadata,filename,tracknumber,random,none>``\
	A comma, separated list (spaces are not allowed) of sorting modes to be used with each metadata tag. The sort array must be the same length as the tag array or it will do nothing. Metadata sorting sorts alphabetically based on the tag names. By default, metadata sorting is used for everything except the *title* tag (metadata sorting does not work on the *title* tag). By default, the *title* tag uses tracknumber sorting. Filename sorting only works on the *title* tag and sorts alphabetically based on the filename (note: this is imperfect at this time). Tracknumber sorting sorts based on both the value of the *disc* metadata tag and the *track* tag. It only works with the *title* tag. Using it on another tag will do nothing.

``--tags=<metadata,tags>``\
   A comma, separated list of metadata values (spaces are not allowed). vmn will read all music files and attempt to organize the curses interface based on the entered tag values. This only has an effect if the metadata view is used. By default, the organization (from let to right) is `artist`,`album`,`title`.

``--view=<file-path|metadata|song-only>``\
   Controls which view to use for vmn. This defaults to `metadata` which creates navigable menus that are built via the metadata in the valid audio files. The `file-path` view organizes itself based on the directory structure, and the `song-only` view outputs the complete paths of all valid audio files found in alphabetical order. Note that initially storing metadata from a large amount of files into cache can be a tad slow.

## Keybindings

Like the main options, keybindings in vmn are defined in the configuration file as `foo=value`. However, these options cannot be set as command line arguments. Settings for keybindings are read as strings, but are internally used as ASCII characters. If more than a single character is specified for an option (i.e. `foo=key`), then only the first character will be read and the rest will be ignored (so `foo` is set to `k`). Currently, the only modifier key supported is `Ctrl` which can be specified in the configuration by adding `Ctrl+`. For example, `foo=Ctrl+i` sets `foo` to Ctrl and i. Note that the order does not matter, so `foo=i+Ctrl` is also valid. If multiple "+'s" are given, vmn will use the first non-Ctrl character and ignore the rest. So `foo=Ctrl+i+j+k` is read as just `Ctrl+i`. `foo=i+j+Ctrl+k` also becomes `Ctrl+i`. Additionally, setting integer values for keybindings (`foo=12`) is also supported since ncurses uses ASCII. ``input-mode`` can be used to find specific integer values of certain key/key combinations. Integers can also be combined with `Ctrl+` like normal characters. 

 Since vmn uses ncurses, the [macros](https://www.gnu.org/software/guile-ncurses/manual/html_node/Getting-characters-from-the-keyboard.html) for special keys are valid (i.e. `foo=KEY_RIGHT` sets foo to the right arrow key). Function keys (f1, f2, f3, etc.) are also supported via `foo=f1`. Note that the vast majority of those macros are for keys that are completely obsolete and nonexistent on current keyboards. Only the common keys are currently enabled in vmn, but support for one of the more esoteric keys can trivially be added upon request. Combining these macros with `Ctrl+` unfortunately does not work. If this is done, an error is given and vmn reverts back to the default settings. 

 ### Keybind commands

``beginning=<key>``\
  Jump to the first item in the menu.

``command=<key>``\
  Enter command mode.

``end=<key>``\
  Jump to the last item in the menu.

``escape=<key>``\
  Quit visual or search mode and return to normal mode.

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
