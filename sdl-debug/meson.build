dependencies = [
    dependency('sdl3'),
    libchips_dep
]

sources = [
    'main.c'
]

libchips_sdl = executable(
    'libchips-sdl-debug',
    sources,
    dependencies: dependencies,
)

configure_file(
    input: 'data/tiles.png',
    output: 'tiles.png',
    copy: true
)
