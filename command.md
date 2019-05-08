# Command Mode

**Note**: The command mode is currently in progress, but much is planned in the future.

Just like in vim, you can enter command mode (default `:`). Currently, command mode allows you to type in commands to send to the libmpv client. The proper syntax to use is `mpv cmd your-command-and-args`. The `your-command-and-args` section is formatted exactly like mpv's input config bindings. See this [document](https://github.com/mpv-player/mpv/blob/master/DOCS/man/input.rst) for details. For example, you would type `mpv cmd add volume 10` to increase the volume of the currently playing item by 10.

Checking for errors has not yet been implemented, so typing in an invalid command will silently fail. Multiple spaces between command arguments is allowed, but the command cannot being with a space. Your input will be ignored if it does. 
