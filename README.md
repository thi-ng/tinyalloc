# thi.ng/talloc

Tiny replacement for `malloc`/`free` in linear memory situations.

## Building for WASM

```sh
emcc -Os -s WASM=1 -s SIDE_MODULE=1 -o talloc.wasm talloc.c
```

```sh
wasm2wast --generate-names malloc.wasm > malloc.wast
```
