#!/usr/bin/env python3

import os
import sys

configs = [
    { 'target': target, 'lang_std': lang_std, 'compiler': compiler }
    for compiler in ['gcc', 'clang']
    for lang_std in ['c11', 'c99']
    for target in ['debug', 'release']
]

beginning = f'''\
# Generated using {os.path.basename(sys.argv[0])} - do not edit manually
#
# Build the project for all platforms
#
.SUFFIXES:

TEST_FILTERS =
JOBS = 1
'''

ending = '''\
.PHONY: clean
clean:
	rm -rf build\
'''

def all_target(configs):
    all = ' '.join(
        f'build-{c['target']}-{c['lang_std']}-{c['compiler']}'
        for c in configs
    )
    return '\n'.join((
        f'.PHONY: all',
        f'all: {all}',
        '',
    ))

def dyn_target_str(compile_target, lang_std, compiler):
    target_name = f'build-{compile_target}-{lang_std}-{compiler}'
    return '\n'.join((
        f'.PHONY: {target_name}',
        f'{target_name}:',
        f'	make -f Makefile.{compile_target} -j $(JOBS) \\',
        f'		TEST_FILTERS=$(TEST_FILTERS) \\',
        f'		BUILD_DIR=build/{compile_target}-{lang_std}-{compiler} \\',
        f'		LANG_STD={lang_std} CC={compiler}',
        '',
    ))

if __name__ == '__main__':
    print(beginning)
    print(all_target(configs))
    for c in configs:
        print(dyn_target_str(c['target'], c['lang_std'], c['compiler']))
    print(ending)
