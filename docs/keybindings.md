# Keybindings & command mode

Logram has two input modes:

- **Action mode** is where you spend most of your time. Single keystrokes move the cursor, scroll, search, and so on. This is the default mode when Logram starts.
- **Command mode** is a one-line text prompt at the bottom of the screen. You enter it by typing `:`, type a command, and press Enter to run it. Anything that requires a string argument — setting a format, declaring a filter, jumping to a line, saving a profile — happens in command mode.

The status line at the bottom of the screen reflects which mode you're in.

---

## Action mode

### Navigation

Three movement sets are bound at once — pick whichever your fingers prefer; you can mix them freely.

**Vim style**

| Key | Action |
|-----|--------|
| `k` | move up |
| `j` | move down |
| `h` | move left |
| `l` | move right |
| `gg` | jump to the start of the file |
| `G` | jump to the end of the file |

**Arrow keys**

| Key | Action |
|-----|--------|
| `↑` | move up |
| `↓` | move down |
| `←` | move left |
| `→` | move right |

**WASD**

| Key | Action |
|-----|--------|
| `w` | move up |
| `s` | move down |
| `a` | move left |
| `d` | move right |

Horizontal movement scrolls the line view sideways once the cursor reaches the edge of the visible area.

### Search

| Key | Action |
|-----|--------|
| `/` | start a forward text search |
| `n` | jump to the next match |
| `N` | jump to the previous match |

`/` is a shortcut: it puts you in command mode pre-filled with `:?`, ready to type the pattern. Pressing Enter runs the search and centers the match. Subsequent `n`/`N` step through matches without re-typing the pattern.

### Quitting

| Key | Action |
|-----|--------|
| `q` | exit immediately |
| `:q` then Enter | exit (vim-style) |

`q` quits with no confirmation — handy, but easy to hit by accident.

### Misc

| Key | Action |
|-----|--------|
| `Enter` | dismiss the current error in the status line |
| `:` | enter command mode |

---

## Command mode

Once you press `:`, the bottom of the screen becomes an editable single-line buffer prefixed with the `:` you just typed. Use the editing keys below to write your command, then Enter to submit. If the command is recognized, it runs and you return to action mode. If it isn't, the status line shows an error and you return to action mode anyway.

### Editing keys

| Key | Action |
|-----|--------|
| `Enter` | submit and run the command |
| `Backspace` | delete the character before the cursor |
| `←` / `→` | move the cursor within the line |
| `Delete` | delete the character under the cursor |

There is no Escape-to-cancel yet. To bail out of a half-typed command, backspace down to just `:` and press Enter — Logram will report an "unrecognized command", which you can dismiss with another Enter.

### Available commands

Commands are grouped by purpose. The full details for filtering and format strings live on their dedicated pages.

#### Quitting

| Command | Effect |
|---------|--------|
| `:q` | exit Logram |

#### Jumping to a line

| Command | Effect |
|---------|--------|
| `:<N>` (e.g. `:1234`) | jump to line N in the current view |

The line number is interpreted relative to the *visible* (filtered) view, not the global file position.

#### Searching

| Command | Effect |
|---------|--------|
| `:?<pattern>` | search forward for the literal text `<pattern>` |

You normally use `/` instead of typing this directly. After running, `n` and `N` step through matches.

#### Filtering

See [Filtering → applying filters](filtering.md#applying-filters) for what each one does and how they compose.

| Command | One-liner |
|---------|-----------|
| `:fset <expr>` | replace the active filter |
| `:f` / `:fadd` / `:fand` `<expr>` | combine with `AND` |
| `:for <expr>` | combine with `OR` |
| `:fxor <expr>` | combine with `XOR` |
| `:fnor <expr>` | combine with `NOR` |
| `:fout <expr>` | invert and `AND` (exclude matching lines) |
| `:fclear` | drop the active filter |


#### Configuration

The Logram config lives in `~/.logram`. It's organized into named **profiles**: each open file is mapped to one profile, which stores its format string, active filter, color scheme, and a few display options. The file is humand readable, and you are free to manually edit, but will be written to by the program so some of your changes might be discraded. The `:cfg` command family lets you read and write profile state from inside Logram.

| Command | Effect |
|---------|--------|
| `:cfg set <key>=<value>` | set a config key in the current profile |
| `:cfg set filter` | snapshot the currently active filter into the profile |
| `:cfg saveas <name>` | clone the current profile under a new name and switch to it |
| `:cfg load <name>` | switch the current file to an existing profile |

The keys you'll most often touch:

| Key | Values | Meaning |
|-----|--------|---------|
| `line_format` | a [format string](format-strings.md) | how to parse each line |
| `hide_bad_fmt` | `true` / `false` | whether to hide lines that don't match the format |
| `line_num_mode` | `global` / `local` / `both` | whether the line-number gutter shows the position in the original file, the position in the filtered view, or both |
| `bg_col`, `txt_col` | ANSI color name (e.g. `default`, `black`, `red`, `bright_blue`) | main view background / text colors |
| `sl_bg_col`, `sl_txt_col` | same | status line background / text colors |
