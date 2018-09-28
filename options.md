# Options

vmn has two different kinds of configuration parameters: one for vmn itself and another for the mpv client. Options for vmn itself can be specified via command line or in the configuration file located at `$HOME/.config/vmn/config`. Any option specified via command line will take priority over any value specified in the configuration file. The syntax for arguments is simply `--foo=value`. In the configuration file, `value` needs to be enclosed in quotes like so: `foo="value"`. 

Options for the mpv client cannot be set as command line arguments and must be set in the configuration file. These options follow standard [mpv.conf syntax](https://github.com/mpv-player/mpv/blob/master/etc/mpv.conf). Most mpv options should work without a problem.

### All Options

``--library=<directory>``\
   Tells vmn which parent directory to search for audio files. If this option is omitted or the directory is invalid, this defaults to `$HOME/Music`.

``--mpv-cfg=<yes|no>``\
   Tells the mpv client whether or not it is allowed to accept configuration. This defaults to `yes`.

``--mpv-cfg-dir=<directory>``\
   Tells the mpv client which directory to search for a configuration file. It will look for the configuration file the same way mpv does. `config` and `mpv.conf` are both valid and the    latter takes priority over the former if both exists. By default, this directory is `$HOME/.config/vmn` which means that mpv configuration values specified in the vmn configuration      file are accepted.

``--view=<file-path|song-only>``\
   Controls which view to use for vmn. This defaults to `file-path` which creates a navigable menus that go up and down directories that contain valid audio files. The `song-only` view     outputs the complete paths to all valid audio files found in alphabetical order.
