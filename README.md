# cpp-compiler

A small compiler for **WLP4** (a C-like language) that targets **MIPS**
assembly. Built as a class project for UWaterloo's CS241 (Foundations of
Sequential Programs).

## Pipeline

```
.wlp4 → scanner → parser → typecheck → codegen → .asm → cs241 linker → .mips
```

Each of the four C++ stages is an independent stdin/stdout filter so they can
be debugged in isolation.

## Build

```sh
make
```

Produces `build/{scanner,parser,typecheck,codegen}`.

## Compile and run

```sh
./scripts/wlp4c examples/simple.wlp4   # writes examples/simple.mips
mips.twoints examples/simple.mips
```

Linking and execution require the CS241 toolchain on `PATH`
(`cs241.linkasm`, `cs241.linker`, `cs241.merl`, `mips.twoints`).

## Layout

| Path | Contents |
|------|----------|
| `src/` | the four compiler stages |
| `runtime/` | `print.merl`, `alloc.merl` (linked in for `println` / `new` / `delete`) |
| `scripts/wlp4c` | end-to-end driver |
| `examples/` | sample WLP4 programs |
| `tests/` | pre-typed inputs for codegen-only testing |

## Language scope

WLP4: `int`, `int*`, `NULL`, functions, `if`/`else`, `while`, `return`,
`println`, `new int[…]`, `delete []`, pointer arithmetic, `&` / `*`. Entry
point is always `wain(int, int)` or `wain(int*, int)`.
