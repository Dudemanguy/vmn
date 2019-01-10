project('vmn', 'c')
libavformat = dependency('libavformat')
libavutil = dependency('libavutil')
libconfig = dependency('libconfig')
menu = dependency('menu')
mpv = dependency('mpv')
ncurses = dependency('ncurses')
datadir = get_option('datadir')
if (get_option('bash-completions'))
  bash_files = files('completions/bash/vmn')
  bash_install_dir = datadir + '/bash-completion/completions'
  install_data(bash_files, install_dir: bash_install_dir)
endif
if (get_option('zsh-completions'))
  zsh_files = files('completions/zsh/_vmn')
  zsh_install_dir = datadir + '/zsh/site-functions'
  install_data(zsh_files, install_dir: zsh_install_dir)
endif
subdir('include')
inc =  include_directories('include')
vmn_sources = files('main.c', 'config.c', 'library.c')
executable('vmn', vmn_sources, dependencies: [libavformat, libavutil, libconfig, menu, mpv, ncurses], include_directories: inc, install: true)
