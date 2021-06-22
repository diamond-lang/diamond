libdiamond = static_library('diamond',
                      sources : ['parser.cpp', 'types.cpp', 'codegen.cpp', 'errors.cpp', 'utilities.cpp', 'semantic.cpp'],
                      include_directories : inc,
                      dependencies : [
                          dependency('llvm')
                      ],
                      install : false)
