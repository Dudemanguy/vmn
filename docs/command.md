# Command Mode #

**Note**: The command mode is currently in progress, but much is planned in the future.

Just like in vim, you can enter command mode (default `:`). Command allows you to interact directly with the libmpv backend in numerous ways. Interacting directly with vmn is also planned.

## Syntax ##

``<mpv/vmn> <exec> <exec_args>``

Multiple spaces between command arguments is allowed. However, it cannot begin with a space. Your input will be ignored if it does.

The first argument specifies whether the command should be sent to the libmpv backend or vmn. Currently only support for mpv commands are implemented.

### mpv ###
These are the supported commands for mpv. The same formatting and names from mpv's input config bindings are used. See this [document](https://github.com/mpv-player/mpv/blob/master/DOCS/man/input.rst) for details.

`cmd`\
Executes the `mpv_command` function (for using mpv's input commands).

`set`\
Executes the `mpv_set_option_string` function (for setting mpv properties).

The additional arguments are used as arguments for their respective function.

## Examples ##

`mpv cmd add volume 10` \
Raises volume by 10.

`mpv set osc no` \
Turns the osc off.
