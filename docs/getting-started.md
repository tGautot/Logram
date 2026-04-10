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

The binary is placed at `Logram/build/bin/lgm`. Move it to `/usr/bin` to make it easily available system-wide.

You can run the test suite to make sure everything is ok:

```bash
cd bin # Necessary, tests only work when ran from Logram/build/bin at the moment
./run_tests
```



## Run

Logram's only argument is the path to the file you wish to inspect

```bash
lgm path/to/your.log
```

## First steps

1. Open a log file with `lgm`.
2. Set a format string to match your log lines (see [Format Strings](format-strings.md)).
3. Navigate through the file using the usual [keybindings](keybindings.md), and `/` to search.
4. Add filters to narrow the view (see [Filtering](filtering.md)).
5. Press `q` to quit.

See [the homepage](index.md) for a complete workflow example.
