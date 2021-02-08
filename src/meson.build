libdiamond = library('diamond',
                      sources : ['parser.cpp', 'types.cpp'],
                      include_directories : inc,
                      install : false)

executable('diamond',
            sources : ['main.cpp'],
            include_directories : inc,
            link_with : libdiamond,
            install : true)
