![Logram logo](docs/logram_logo_header.svg)

# Logram

`vim` and `grep` are powerful, but they treat your logs as plain text. Logram lets you **define the structure** of your log lines, then navigate, filter, and search based on what the fields *mean* — not just what they look like.

---

## The problem Logram solves

You're staring at thousands of log lines like this:

```
0322 085338 TRACE [127.0.0.1:25555] :......connect: Attempting connection to 192.168.1.10
0322 085339 INFO  [127.0.0.1:25555] :......connect: Connection established
0322 085340 WARN  [127.0.0.1:25555] :......retry: Timeout, retrying...
0322 085341 ERROR [127.0.0.1:25555] :......retry: Max retries exceeded
```

You probably need only a few of these lines — the trick is finding them. With `vim` and `grep`, you write fragile regexes that inevitably yield false positives, then crawl your way from there.

With Logram, you define the format of your logs once:

```
{INT:Date} {INT:Time} {STR:Level} :{STR:Source}: {STR:Message}
```

This extracts `Date`, `Time`, `Level`, `Source`, and `Message` as typed, named fields. From there you can filter by field — `Level EQUAL ERROR`, `Source BEGINS_WITH retry`, `Time GREATER 085339` — and combine conditions with `AND`, `OR`, `XOR` logic. Only matching lines are shown. Navigate with vim-style keys, search with `/pattern`, and get to the lines that matter.

| | vim | grep / tail | Logram |
|---|-------------|--------|--------|
| Handles GB-scale files | Yes | Yes | Yes |
| Combine multiple filters | No | Awkward | Yes |
| Navigate large files interactively | Yes | No | Yes |
| Understands log structure | No | No | Yes |
| Filter by field value | No | No | Yes |

Logram is not a replacement for log aggregation platforms — it's a replacement for the ad-hoc vim/grep sessions you run when you're already on the machine and need answers fast.

---

## Features

- **Custom format definitions** — supports `INT`, `DBL`, `STR`, `CHR` fields
- **Field-based filtering** — `EQUAL`, `CONTAINS`, `BEGINS_WITH`, `ENDS_WITH`, `GREATER`, `SMALLER`, and more
- **Boolean filter composition** — combine multiple filters with `AND`, `OR`, `XOR`, `NOR`
- **Vim keybindings** — `hjkl`, `gg`, `G`, `:q`, `/search`, `n`/`N` (plus arrow keys and WASD)
- **Memory-mapped I/O** — handles large log files efficiently without loading them into RAM
- **Persistent per-file config** — format, filters, and settings are remembered per log file in `~/.logram`
- **ANSI color support** — customize colors for background, text, and selection
- **Extensible via modules** — add custom interactive behaviors to fit your workflows (see [modules documentation](https://logram.readthedocs.io/en/latest/modules/))

---

## Getting started

### Requirements

- C++17 compiler
- CMake 3.21+
- Unix/Linux system

Logram has **no external dependencies** beyond the C standard library and the C++ STL. The only third-party library used is [Catch2](https://github.com/catchorg/Catch2) for the test suite, which is downloaded automatically via CMake's FetchContent — you don't need to install it.

### Build

```bash
git clone <repo-url>
cd Logram
mkdir build && cd build
cmake ..
make
```

### Run

```bash
./build/bin/lgm path/to/your.log
```

On first open, Logram creates a profile for this file in `~/.logram`. Set your [format string](https://logram.readthedocs.io/en/latest/format-strings/), and it will be remembered next time.

---

## Documentation

For a detailed guide on all of Logram's capabilities, refer to the [full documentation](https://logram.readthedocs.io/en/latest/):

- [Getting started](https://logram.readthedocs.io/en/latest/getting-started/) — build, install, first steps
- [Format strings](https://logram.readthedocs.io/en/latest/format-strings/) — how to define your log structure
- [Filtering](https://logram.readthedocs.io/en/latest/filtering/) — field-based filters, comparators, boolean composition
- [Keybindings](https://logram.readthedocs.io/en/latest/keybindings/) — all navigation, search, and command-mode bindings
- [Modules](https://logram.readthedocs.io/en/latest/modules/) — extending Logram with custom interactive behaviors

---

## License

[MIT](LICENSE)
