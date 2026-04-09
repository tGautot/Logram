# Getting Started

## Requirements

- C++17 compiler
- CMake 3.21+
- Unix/Linux system

## Build

```bash
git clone <repo-url>
cd Logram
mkdir build && cd build
cmake ..
make -j4
```

The binary is placed at `build/bin/lgm`. The executable is fully self contained, you can make it available system-wide by moving to `/usr/bin`

## Run

```bash
lgm path/to/your.log
```

On first open, Logram creates a profile for this file in `~/.logram`. You can then set your format string — it will be remembered the next time you open the same file.

## First steps

1. Open a log file with `lgm`.
2. Set a format string to match your log lines (see [Format Strings](format-strings.md)).
3. Use `j`/`k` to scroll through lines, and `/` to search.
4. Add filters to narrow the view (see [Filtering](filtering.md)).
5. Press `:q` to quit.
