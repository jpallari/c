#!/usr/bin/env python3

import os
import sys

configs = [
    {
        'target': target,
        'lang_std': lang_std,
        'cc': cc,
        'mt': lang_std == 'c11',
    }
    for cc in ['gcc', 'clang']
    for lang_std in ['c11', 'c99', 'c23']
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
        f'build-{c['target']}-{c['lang_std']}-{c['cc']}'
        for c in configs
    )
    return '\n'.join((
        f'.PHONY: all',
        f'all: {all}',
        '',
    ))

def dyn_target_str(config):
    compile_target = config['target']
    lang_std = config['lang_std']
    cc = config['cc']
    mt = config['mt']

    target_name = f'build-{compile_target}-{lang_std}-{cc}'
    return '\n'.join(line for line in (
        f'.PHONY: {target_name}',
        f'{target_name}:',
        f'	$(MAKE) -j $(JOBS) \\',
        f'		TEST_FILTERS=$(TEST_FILTERS) \\',
        f'		BUILD_DIR=build/{compile_target}-{lang_std}-{cc} \\',
        f'		ENABLE_RELEASE=1 \\' if compile_target == 'release' else '',
        f'		DISABLE_MT=1 \\' if not mt else '',
        f'		LANG_STD={lang_std} CC={cc}',
    ) if line) + '\n'

if __name__ == '__main__':
    print(beginning)
    print(all_target(configs))
    for config in configs:
        print(dyn_target_str(config))
    print(ending)
