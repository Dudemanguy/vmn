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

scdoc = find_program('scdoc', required: get_option('man-pages'))
if scdoc.found()
  sh = find_program('sh')
  mandir = get_option('mandir')
  man_files = ['vmn.1.scd']
  foreach filename : man_files
		topic = filename.split('.')[-3].split('/')[-1]
		section = filename.split('.')[-2]
		output = '@0@.@1@'.format(topic, section)

		custom_target(
			output,
			input: filename,
			output: output,
			command: [
				sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc.path(), output)
			],
			install: true,
			install_dir: '@0@/man@1@'.format(mandir, section)
		)
  endforeach
endif

subdir('include')
inc =  include_directories('include')
vmn_sources = files('command.c', 'config.c', 'library.c', 'main.c', 'utils.c')
executable('vmn', vmn_sources, dependencies: [libavformat, libavutil, libconfig, menu, mpv, ncurses], include_directories: inc, install: true)
