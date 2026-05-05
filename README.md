# cpp-compiler

A compiler for WLP4, the tiny C-like language used in UWaterloo's CS241.
Reads WLP4 source, emits MIPS assembly. I built it across the term as the
course's assignment series, then later went back and stitched the pieces
into one project so it's a real compiler instead of seven directories
named `a4` through `a8`.

## The pipeline

Four stages, each its own program reading stdin and writing stdout:

```
.wlp4 → scanner → parser → typecheck → codegen → .asm
```

That single design choice probably saved me more debugging time than
anything else. Pipe through the first stage to see your tokens, pipe
through two to see the parse tree, and so on. Every intermediate is plain
text, so when something looks wrong you can usually find it with `diff`.

After the four stages I link against the CS241 runtime libraries
(`print.merl`, `alloc.merl`) and produce a `.mips` file that runs under
`mips.twoints`.

## How I built it

**Scanner.** DFA-based simplified maximal munch. I drew the state diagram
on paper before touching any code, which felt slow at the time but turned
out to be a much better use of an afternoon than trying to reason about
transitions in my head.

The fun bug I shipped: I was calling `stoi` on the entire whitespace-
delimited chunk rather than the extracted token, so `*(array+1)` would
tokenize correctly and then crash when I tried to validate `1` as a
number. The assignment tests all had whitespace around numbers, so it
didn't surface until I came back later and tried to integrate everything
end to end.

**Parser.** SLR(1), and easily the hardest stage to debug. The grammar
fits on one page, but the action/goto tables explode to thousands of
entries, and one bad transition silently puts the parser in the wrong
state until a reduction fails ten tokens downstream.

The thing that actually unlocked progress was deciding to embed the
precomputed SLR table directly in the source rather than generating it
at startup. The binary becomes 1200+ lines of mostly data, which looks
awful, but it meant I wasn't debugging the table generator and the parser
at the same time.

**Type checker.** Once the parse tree was solid this was almost relaxing.
Walk the tree, push and pop scopes, propagate types up. WLP4 only has two
types (`int` and `int*`), which keeps the rules small. The trap I kept
hitting was lvalues. `*(p+1) = x` is legal and `(p+1) = x` is not, so you
have to know which expression nodes are actually addressable.

**Code generator.** This is where I felt like I was writing a real
compiler. MIPS has 32 registers and no instructions for printing or heap
allocation, so you set up the calling convention and let the MERL linker
pull in `print.merl` and `alloc.merl`.

A few conventions I settled on:

- `$29` is the frame pointer, `$30` the stack pointer.
- `$4` permanently holds the constant `4`, so a push is
  `sub $30, $30, $4` and stack offsets are just immediates.

The thing that took the longest was procedure calls. I kept forgetting to
save `$31` before `jalr`, or popping callee-saved registers in the wrong
order, and getting results that looked like memory corruption. The fix
every time was sitting down and drawing the stack frame on paper before
writing any more code.

By the time codegen worked end to end, I had a lot of respect for the
people who write production compilers. WLP4 is about the smallest thing
you can call a language and the backend was still the part I rewrote
the most.

## Repo layout

```
src/         the four stages
runtime/     print.merl, alloc.merl
scripts/     end-to-end driver
examples/    sample WLP4 programs
tests/       pre-typed parse trees for codegen-only testing
```

## Build and run

```sh
make
./scripts/wlp4c examples/simple.wlp4
mips.twoints examples/simple.mips
```

The link and run steps need the CS241 toolchain (`cs241.linkasm`,
`cs241.linker`, `cs241.merl`, `mips.twoints`) on `PATH`. Without those
you can still build all four stages and inspect the generated `.asm`.

## Language

WLP4 supports `int`, `int*`, `NULL`, functions, `if`/`else`, `while`,
`return`, `println`, `new int[…]`, `delete []`, and pointer arithmetic
with `&` and `*`. The entry point is always `wain(int, int)` or
`wain(int*, int)`. No strings, no structs, no floats, no `for` loop.
That's the point: small enough for one person to write a full compiler
for it in a term.
