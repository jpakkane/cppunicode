project('cppunicode', 'cpp',
		      default_options: ['cpp_std=c++17', 'default_library=static'])
		      		       
icu_dep = dependency('icu-i18n')
thread_dep = dependency('threads')
executable('cppunicode', 'cppunicode.cpp',
           dependencies: [icu_dep, thread_dep])