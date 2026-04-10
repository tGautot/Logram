![Logram logo](docs/logram_logo_header.svg)

# Logram

`vim` and `grep` are powerful, but they treat your logs as plain text. Logram lets you **define the structure** of your log lines, then navigate, filter, and search based on what the fields *mean* — not just what they look like.

---

## The problem

You're staring at (tens of) thousands of log lines like this:

```
0322 085338 TRACE [127.0.0.1:25555] :......connect: Attempting connection to 192.168.1.10
0322 085339 INFO  [127.0.0.1:25555] :......connect: Connection established
0322 085340 WARN  [127.0.0.1:25555] :......retry: Timeout, retrying...
0322 085341 ERROR [127.0.0.1:25555] :......retry: Max retries exceeded
```

Truth is, you probably need only a few of these lines, the trick is finding them. With `vim` and `grep`, you write fragile regexes, which will inevitably yield false-positives, and then crawl your way from there. With Logram, you define the format of your logs once, and then you can quickly create complex filters honing in on only what you need.

---

## How it works

**1. Define your log format**

Tell Logram what your lines look like using a simple format string:

```
{INT:Date} {INT:Time} {STR:Level} :{STR:Source}: {STR:Message}
```

This extracts `Date`, `Time`, `Level`, `Source`, and `Message` as typed, named fields.

**2. Navigate your file**

Use vim-style keys (`hjkl`, `gg`, `G`, `:42`), arrows, scrollwheel, to move through the file at any scale. Logram uses memory-mapped file access — large files are no problem.

**3. Filter by field**

Apply filters like:
- `Level EQUAL ERROR`
- `Source BEGINS_WITH retry`
- `Time GREATER 085339`

Combine them with `AND`, `OR`, `XOR` logic. Only matching lines are shown. Checkout the [filtering documentation](https://logram.readthedocs.io/en/latest/filtering/) for more info.

**4. Search within results**

Use `/pattern` to find text within your filtered view. `n`/`N` jump to next/previous match.

---

## Features

- **Custom format definitions** — supports `INT`, `DBL`, `STR` fields (`DATE` support incoming)
- **Field-based filtering** — `EQUAL`, `CONTAINS`, `BEGINS_WITH`, `ENDS_WITH`, `GREATER`, `SMALLER`, and more
- **Boolean filter composition** — combine multiple filters with `AND`, `OR`, `XOR`, `NOR`
- **Vim keybindings** — `hjkl`, `gg`, `G`, `:q`, `/search`, `n`/`N`
- **Memory-mapped I/O** — handles large log files efficiently without loading them into RAM
- **Persistent per-file config** — format and settings are remembered per log file in `~/.logram`
- **ANSI color support** — customize colors for background, text, and selection

---

## Getting started

### Build

```bash
git clone <repo-url>
cd Logram
mkdir build && cd build
cmake ..
make
```

Requires: C++17, CMake 3.21+, a Unix/Linux system.

### Run

```bash
./build/bin/lgm path/to/your.log
```

On first open, Logram creates a profile for this file in `~/.logram`. Set your format string, and it will be remembered next time.

---

## Format string syntax

A format string is a sequence of **typed fields**:

| Type | Syntax | Matches |
|------|--------|---------|
| Integer | `{INT:Name}` | A sequence of digits |
| Float | `{DBL:Name}` | A decimal number |
| String | `{STR:Name}` | Any text up to the next delimiter |
| Character | `{CHR:Name,target,repeat}` | Occurence of a specific (repeating) char |

Field names are optional but required for filtering. For a more complete view on format string, please refer to the [documentation](https://logram.readthedocs.io/en/latest/format-strings/).


**Example** 
For a log file containing lines like this one:
```
0322 085339 INFO  [localhost:25555] :......connect: Connection established
```
You could create the following format
```
{INT:Date} {INT:Time} {STR:Level} [{STR:IP}:{INT:Port}] :{STR:Source}: {STR:Message}
```

You have thrown away all the whitespaces and formatting characters, and now you have 7 fields on which you can do filtering! The extracted values would be the following:

| Field | Value |
| ----- | ----- |
| Date  | 322   |
| Time  | 85339 |
| Level | "INFO"  |
| IP    | "localhost" |
| Port  | 25555 |
| Source | "......connect" |
| Message | "Connection established" |

Note: with a simple change to the format string you could even get rid of the dots in the `Source` Field! For this, refer to the [format string documentation](https://logram.readthedocs.io/en/latest/format-strings/)


---

## Keybindings

| Key | Action |
|-----|--------|
| `h` / `←` | Scroll left |
| `l` / `→` | Scroll right |
| `j` / `↓` | Move down |
| `k` / `↑` | Move up |
| `gg` | Jump to start of file |
| `G` | Jump to end of file |
| `:<number>` | Jump to line <number> |
| `/` | Enter search mode |
| `n` / `N` | Next / previous match |
| `:q` | Quit |

And many more! Checkout all the default [keybinds](https://logram.readthedocs.io/en/latest/keybindings/)!

---

## Why not just use `vim` or `grep`?

| | vim | grep / tail | Logram |
|---|-------------|--------|--------|
| Handles GB-scale files | Yes | Yes | Yes |
| Combine multiple filters | No | Awkward | Yes |
| Navigate large files interactively | Yes | No | Yes |
| Understands log structure | No | No | Yes |
| Filter by field value | No | No | Yes |

Logram is not a replacement for log aggregation platforms — it's a replacement for the ad-hoc vim/grep sessions you run when you're already on the machine and need answers fast.

---


## License

[MIT](LICENSE)

## Footnote: Modules

Logram's interactivity is done exclusively through modules. If you would like to add some capabilities to Logram that would fit your logs/workflows, you can do so by creating modules. Modules give you an easy way to "listen" to user input in the terminal, react to it and change the state of the program.  Note that since Logram is in beta, modules are bound to break. Please refer to the [usage guide](https://logram.readthedocs.io/en/latest/modules/) for more info.