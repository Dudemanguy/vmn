project('vmn', 'c')

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments('-Wno-format-security'), language: 'c')

libavformat = dependency('libavformat')
libavutil = dependency('libavutil')
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

scdoc = dependency('scdoc', native: true, required: get_option('man-pages'))
if scdoc.found()
    scdoc_prog = find_program(scdoc.get_pkgconfig_variable('scdoc'), native: true)
    sh = find_program('sh')
    mandir = get_option('mandir')
    man_files = ['docs/vmn.1.scd']
    foreach filename : man_files
        topic = filename.split('.')[-3].split('/')[-1]
        section = filename.split('.')[-2]
        output = '@0@.@1@'.format(topic, section)

        custom_target(
                output,
                input: filename,
                output: output,
                command: [
                    sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc_prog.path(), output)
                ],
                install: true,
                install_dir: '@0@/man@1@'.format(mandir, section)
        )
    endforeach
endif

subdir('include')
inc =  include_directories('include')
vmn_sources = files('src/command.c',
                    'src/config.c',
                    'src/library.c',
                    'src/main.c',
                    'src/render.c',
                    'src/utils.c')
executable('vmn', vmn_sources, dependencies: [libavformat, libavutil, menu, mpv, ncurses], include_directories: inc, install: true)
