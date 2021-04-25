project('diamond', 'cpp', default_options : ['cpp_std=c++17'])

inc = include_directories('src')

subdir('src')
subdir('test')

executable('diamond',
            sources : ['src/main.cpp', 'src/errors.cpp', 'src/utilities.cpp'],
            include_directories : inc,
            link_with : libdiamond,
            install : true)
