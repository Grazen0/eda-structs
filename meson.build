project(
  'eda-structs',
  'cpp',
  version : '0.1',
  meson_version : '>= 1.3.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
  ],
)

dependencies = [
]

lib_args = ['-DBUILDING_EDA_STRUCTS']

sources = [
  'src/binary_heap.cpp',
  'src/binomial_heap.cpp',
  'src/fibonacci_heap.cpp',
  'src/persistent_stack.cpp',
  'src/persistent_seg_tree.cpp',
  'src/quick_heap.cpp',
]

inc_dirs = include_directories('include')

lib = library(
  'eda_structs',
  sources,
  install : true,
  include_directories : inc_dirs,
  cpp_shared_args : lib_args,
  gnu_symbol_visibility : 'hidden',
  dependencies : dependencies,
)

# Make this library usable as a Meson subproject.
eda_structs_dep = declare_dependency(
  include_directories : inc_dirs,
  dependencies : dependencies,
  link_with : lib,
)
meson.override_dependency('eda-structs', eda_structs_dep)

catch2_dep = dependency(
  'catch2-with-main',
  fallback : ['catch2', 'catch2_with_main_dep'],
  default_options: ['tests=false'],
)

test_dependencies = [
  catch2_dep,
  eda_structs_dep,
]

test_exe = executable(
  'eda_structs_test',
  'test/binary_heap_test.cpp',
  'test/binomial_heap_test.cpp',
  'test/fibonacci_heap_test.cpp',
  'test/persistent_stack_test.cpp',
  'test/persistent_seg_tree_test.cpp',
  'test/quick_heap_test.cpp',
  dependencies : dependencies + test_dependencies,
  link_with : lib,
)
test('eda_structs', test_exe)

benchmark('eda_structs', test_exe, args : ['[!benchmark]'])
