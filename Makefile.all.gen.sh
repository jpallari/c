#!/bin/sh

COMPILERS="
gcc
clang
"

LANG_STDS="
c11
c99
"

COMPILE_TARGETS="
debug
release
"

iterate_combos() {
    _IF=$IF
    IF=$'\n'
    for target_cc in $COMPILERS; do
        if [ -z "${target_cc:-}" ]; then
            continue
        fi
        for target_lang_std in $LANG_STDS; do
            if [ -z "${target_lang_std:-}" ]; then
                continue
            fi
            for compile_target in $COMPILE_TARGETS; do
                if [ -z "${compile_target:-}" ]; then
                    continue
                fi
                echo "$compile_target" "$target_lang_std" "$target_cc" 
            done
        done
    done
    IF=$_IF
}

beginning() {
    cat <<EOF
# Generated using "$(basename $0)" - do not edit manually
#
# Build the project for all platforms
#
.SUFFIXES:

TEST_FILTERS =
JOBS = 1

EOF
}

target_name() {
    printf 'build-%s-%s-%s\n' "$1" "$2" "$3"
}

print_space() {
    printf ' '
}

target_all() {
    echo '.PHONY: all'
    printf 'all: '
    iterate_combos | while read -r compile_target target_lang_std target_cc; do
        target_name "${compile_target}" "${target_lang_std}" "${target_cc}" 
    done | paste -sd ' '
    echo ''
}

dyn_target() {
    _compile_target=$1
    _target_lang_std=$2
    _target_cc=$3
    _target_name=$(target_name "${_compile_target}" "${_target_lang_std}" "${_target_cc}")
    cat <<EOF
.PHONY: ${_target_name}
${_target_name}:
	make -f Makefile.${_compile_target} -j \$(JOBS) \\
		TEST_FILTERS=\$(TEST_FILTERS) \\
		BUILD_DIR=build/${_compile_target}-${_target_lang_std}-${_target_cc} \\
		LANG_STD=${_target_lang_std} CC=${_target_cc}
EOF
}

dyn_target_combos() {
    iterate_combos | while read -r compile_target target_lang_std target_cc; do
        dyn_target "${compile_target}" "${target_lang_std}" "${target_cc}"
        echo ''
    done
}

ending() {
    cat <<'EOF'
.PHONY: clean
clean:
	rm -rf build
EOF
}

beginning
target_all
dyn_target_combos
ending
