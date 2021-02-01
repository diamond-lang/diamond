with import <nixpkgs> { };

mkShell {
    buildInputs = [
        meson
        ninja
        pkg-config
    ];
}
