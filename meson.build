project('clipit', 'c', version : '1.4.5', license: 'GPL-3+')
package = meson.project_name()
sources = [
        'src/daemon.c',
        'src/eggaccelerators.c',
        'src/history.c',
        'src/keybinder.c',
        'src/main.c',
        'src/manage.c',
        'src/preferences.c',
        'src/utils.c'
]

#meson configure -Doption=newvalue
if get_option('with-gtk3')
  gtk = 'gtk+-3.0'
  app_ind = 'ayatana-appindicator3-0.1'
else
  gtk = 'gtk+-2.0'
  app_ind = 'ayatana-appindicator-0.1'
endif

dependencies = [
    dependency('glib-2.0'),
    dependency('x11'),
    dependency(gtk),
    dependency(app_ind)
]

conf = configuration_data()

if get_option('enable-appindicator')
  conf.set('HAVE_APPINDICATOR', 1)
else
  conf.set('HAVE_APPINDICATOR', 0)
endif

conf.set('ENABLE_NLS', 1)
conf.set_quoted('GETTEXT_PACKAGE', package)
conf.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
conf.set('HAVE_DCGETTEXT', 1)
conf.set('HAVE_GETTEXT', 1)
conf.set('HAVE_INTTYPES_H', 1)
conf.set('HAVE_LC_MESSAGES', 1)
conf.set('HAVE_LOCALE_H', 1)
conf.set('HAVE_MEMORY_H', 1)
conf.set('HAVE_STDINT_H', 1)
conf.set('HAVE_STDLIB_H', 1)
conf.set('HAVE_STRINGS_H', 1)
conf.set('HAVE_STRING_H', 1)
conf.set('HAVE_SYS_STAT_H', 1)
conf.set('HAVE_SYS_TYPES_H', 1)
conf.set('HAVE_UNISTD_H', 1)
conf.set_quoted('PACKAGE', package)
conf.set_quoted('PACKAGE_BUGREPORT', 'oss@rspwn.com')
conf.set_quoted('PACKAGE_NAME', package)
conf.set_quoted('PACKAGE_STRING', package + ' ' + meson.project_version())
conf.set_quoted('PACKAGE_TARNAME', package)
conf.set_quoted('PACKAGE_URL', '')
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set('STDC_HEADERS', 'void')
conf.set('STDC_HEADERS', 1)
conf.set_quoted('VERSION', meson.project_version())

configure_file(output : 'config.h', configuration : conf)

i18n = import('i18n')
add_project_arguments('-DGETTEXT_PACKAGE="@0@"'.format(package), language:'c')
subdir('po')

# To be used later: migrate from intltools 
# remove _
# https://stackoverflow.com/questions/62504873/i18n-merge-file-cant-translate-desktop-file
#
#i18n.merge_file(
#  input: 'data/clipit.desktop.in',
#  output: 'clipit.desktop',
#  type: 'desktop',
#  po_dir: join_paths(meson.source_root(), 'po'),
#  install: true,
#  install_dir: '/usr/share/applications'
#)

#i18n.merge_file(
#  input: 'data/clipit-startup.desktop.in',
#  output: 'clipit-startup.desktop',
#  type: 'desktop',
#  po_dir: join_paths(meson.source_root(), 'po'),
#  install: true,
#  install_dir: '/etc/xdg/autostart'
#)

intltool_merge = find_program('intltool-merge')

custom_target('clipit-startup.desktop',
  input : 'data/clipit-startup.desktop.in',
  output : 'clipit-startup.desktop',
  command: [intltool_merge, '-d', '-u', join_paths(meson.source_root(), 'po'), '@INPUT@', '@OUTPUT@'],
  install : true,
  install_dir : '/etc/xdg/autostart')

custom_target('clipit.desktop',
  input : 'data/clipit.desktop.in',
  output : 'clipit.desktop',
  command: [intltool_merge, '-d', '-u', join_paths(meson.source_root(), 'po'), '@INPUT@', '@OUTPUT@'],
  install : true,
  install_dir : get_option('datadir') / 'applications')


install_subdir('debian/icons', install_dir: get_option('datadir'))
install_data('data/clipit-trayicon.svg', install_dir: join_paths(get_option('datadir'), 'icons/hicolor/scalable/apps/'))
install_data('data/clipit-trayicon-offline.svg', install_dir: join_paths(get_option('datadir'), 'icons/hicolor/scalable/apps/'))
install_man('doc/clipit.1')
               
executable(package, sources, install : true, dependencies: dependencies)
