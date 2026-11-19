#!/bin/bash
set -eu
if [[ "$1" == bruno ]]; then
    export PYTHONPATH=/src/tools:/src
    exec python3 -B -m unittest discover -s tests -v
fi
cd vita-prototype
flags=(-std=c11 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all)
for name in sprite sprite_animation demo_round round_scene; do
    case $name in
        sprite*) sources=(sprite.c);;
        demo_round) sources=(demo_round.c);;
        round_scene) sources=(demo_round.c round_scene.c);;
    esac
    cc "${flags[@]}" "${sources[@]}" "test_$name.c" -o /tmp/test
    /tmp/test /tmp/synthetic.rgba
done
flags+=(-idirafter "$VITASDK/arm-vita-eabi/include")
cc "${flags[@]}" -Dmain=smoke_main -c main.c -o /tmp/main.o
cc "${flags[@]}" test_loop.c sprite.c /tmp/main.o -o /tmp/loop
test "$(/tmp/loop)" = "$(printf 'Active loop time: 1200000 us\nActive loop time: 1200000 us')"
cmake -S . -B /tmp/build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/build -j2
for target in smoke round; do
    unzip -tq "/tmp/build/handkerchief_${target}.vpk"
done
python3 -B check_livearea.py
