# Logram

> A fast, read-only, terminal-based log viewer that actually understands your logs.

Logram  lets you **define the structure** of your log lines, then navigate, search, and filter by what the fields *mean* — not just what they look like.

## What Logram gives you

- **Structured parsing** — describe your log format once; every line becomes a set of typed, named fields.
- **Expressive filters** — combine field-level conditions with boolean operators instead of chaining `grep | grep -v`.
- **Vim-style navigation** — move through large files without losing your place.
- **Per-file profiles** — formats and filters are remembered the next time you open the same file.

## From zero to the line you wanted

A walkthrough of the full pipeline: build Logram, open a log, and land on the exact line you were after. Each step links to the page that covers it in depth.

### 1. Build and launch

Requirements and install options live in [Getting Started](getting-started.md).

```bash
git clone https://github.com/tGautot/Logram && cd Logram
mkdir build && cd build && cmake .. && make -j4
./bin/lgm path/to/your.log
```

And optionally:

```bash
mv ./bin/lgm /usr/bin
```

### 2. Move around

Navigation can be vim-style: `j`/`k` to step through lines, `gg` / `G` to jump to the begining/end of the file, `/<substring>` to search, `n` / `N` for next/previous match. But some other bindings are also available by default (arrows, scrollwheel, wasd, ...), the full list is in [Keybindings](keybindings.md).

### 3. Teach Logram your log's shape

Given a line like:

```
0322 085338 TRACE  :......connect: Attempting connection to 192.168.1.10
```

declare its format from the logram command line (press `:` to enter the command line mode):

```
:cfg set line_format={INT:Date} {INT:Time} {STR:Level} :{CHR:,.,1}{STR:Source}: {STR:Message}
```

Each line now has named fields (`Date`, `Time`, `Level`, `Source`, `Message`) that filters can target. Field types, delimiters, and more examples are in [Format Strings](format-strings.md).

### 4. Filter down to what matters

With named fields, you can ask real questions. To keep only errors during a given time period using 3 commands:

```
:f Level EQUAL ERROR
:f Time GREATER 085339
:f Time SMALLER 085540
```

Or equivalently with a single command:

```
:f Level EQUAL ERROR AND Time GREATER 085339 AND Time SMALLER 085540
```

Filters combine with `AND`, `OR`, `XOR`, `NOR`; comparators include `EQUAL`, `CONTAINS`, `BEGINS_WITH`, `GREATER` and others. `:fclear` drops the active filter. The full grammar and operator list is in [Filtering](filtering.md).

While the format set for a file is remembered automatically, the filter is not. But it can be saved by manually running the following command. With it, on the next opening of the log file, the current filter will be automatically applied.

```bash
:cfg set filter
```

### 5. Searching for specific strings

Filtering narrows the view; you can the use the vim-style search `/pattern` (or `:?<patthen>`) plus `n`/`N` pin down the specific row inside it. Only the lines that are accepted by the current filters are checked for occurences.

### 6. Configuration and profile 

The multiple commands we saw that start with `:cfg ...` all change the configuration of the current `profile`. You can save this profile under a human readable name like this

```
:cfg saveas my-profile
```

By default, opening a new file (at a path on which logram was never used before) creates a new profile for it. But if you already created a profile which matches the need of this new file (because it has the same line format) you can load that profile using:

```
:cfg load my-profile
```

### 7. Quit

`q` anytime, or, in command mode, `:q`.

## Reference

- [Getting Started](getting-started.md) — build, install, and run Logram
- [Format Strings](format-strings.md) — describe your log's structure
- [Filtering](filtering.md) — field-level filters and boolean combinations
- [Keybindings](keybindings.md) — every key Logram responds to
