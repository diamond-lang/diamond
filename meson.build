project('diamond', 'cpp', default_options : ['cpp_std=c++17'])

inc = include_directories('src')

# Executable
executable('diamond',
            sources : ['src/main.cpp', 'src/parser.cpp', 'src/types.cpp'],
            include_directories : inc,
            install : true)
