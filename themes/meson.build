subdir('Default')
subdir('Darker')
subdir('Compact')

themes = [
  'Gray',
  'System',
]

foreach theme : themes
  install_data(
    theme / 'gimp.css',
    install_dir: gimpdatadir / 'themes' / theme,
  )
endforeach
