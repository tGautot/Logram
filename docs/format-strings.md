# Format Strings

A **format string** describes the shape of a single line in your log file. Logram uses it to split each line into typed, named fields that you can then [filter](filtering.md) on.

A format string is a sequence of two kinds of tokens:

- **Field placeholders** of the form `{TYPE:name,...}` — each one consumes part of the line and stores it as a typed value.
- **Literals** — every other character must appear verbatim in the log line at the same position.

A line parses successfully only if the *entire* line is consumed by the format. If parsing fails, the line is marked as malformed (highlighted in red, or hidden — see [Malformed lines](#malformed-lines)).

## Setting the format

From [command mode](keybindings.md#command-mode):

```
:cfg set line_format=<your format string>
```

The format is stored in the file's profile (`~/.logram`) and reapplied automatically the next time you open the same file.

## Field types

| Type | Syntax | Stores |
|------|--------|--------|
| Integer | `{INT:name}` | `int64_t` |
| Double | `{DBL:name}` | `double` |
| String | `{STR:...}` *(see below)* | `string` |
| Character | `{CHR:name,c,r}` | `char` |

The `name` is what you reference in filters. It is optional — `{INT:}` parses an integer you can never filter on. Names may not contain `,` or `}`.

### `{INT:name}` — integer

Reads as many consecutive digits (`0`–`9`) as possible. Fails if the next character is not a digit.

```
{INT:Year}     matches "2025"
```

### `{DBL:name}` — double

Reads digits, with at most one `.` allowed somewhere in the run. Fails if the next character is not a digit.
```
{DBL:Latency}  matches "12.34" or "42"
```

### `{STR:...}` — string

When you want to extract a chunk of text from a line, you almost always know one of two things about it: either **how long it is**, or **what comes right after it**. The `STR` field handles both.

#### When you know the length

If the field is fixed-width — a 5-character log level column, a 4-digit year — give the length explicitly:

```
{STR:Level,5}
```

reads exactly 5 characters into `Level`, regardless of their content. Padding spaces stay in the field.

#### When you know what comes after

Most of the time you don't know how long the field is, but you know what ends it: a colon, a space, a closing bracket. In that case, write the literal terminator immediately after the field's closing `}`, and Logram will read up to that character:

```
{STR:Source}: {STR:Message}
```

`Source` reads up to the next `:`. The `:` itself isn't consumed by the `STR` — it's then matched by the literal `:` that follows in the format. Without that literal, parsing would stall.

If the terminator you put after `}` is a space, that's interpreted specially: the field stops at *any* whitespace character, not just spaces. So `{STR:Word} ` works for tab-separated logs too.

If `{STR:name}` is the very last token in the format string, there's no terminator and the field reads to the end of the line.

#### Recap

| Form | Reads until |
|------|-------------|
| `{STR:name,N}` | exactly `N` characters consumed |
| `{STR:name}<char>` | the next `<char>` (not consumed) |
| `{STR:name} ` | any whitespace character (not consumed) |
| `{STR:name}` *(end of format)* | end of line |

### `{CHR:name,c,r}` — character

Matches a single specific character `c`. Fails if the next character is not `c`.

- `r = 0` — match exactly one occurrence.
- `r = 1` — match one occurrence, then greedily consume any further consecutive `c` characters.

```
{CHR:dot,.,0}    matches a single "."
{CHR:,.,1}       matches one or more consecutive "." (no name)
```

`{CHR:,c,1}` is the canonical way to swallow a *run* of a non-whitespace separator (e.g. dots, dashes, equals signs) when you don't care about the exact count.

At the moment it is not possible to match any (wildcard) character, it needs to be known in advance.

## Literals and whitespace

Any character outside a `{...}` placeholder is taken literally:

- A **non-space literal** (e.g. `:`, `[`, `-`) is parsed as a single character that must match exactly. Equivalent to `{CHR:,X,0}`.
- A **space** in the format string matches *any* run of whitespace (spaces, tabs, etc.) — except newlines. So `{INT:a} {INT:b}` accepts both `12 34` and `12   34`.

## Field names and filtering

Names are optional, but only **named** fields are visible to filters. Each name must be unique within the format string.

If you only need a value for parsing structure (not filtering), leave the name out: `{INT:}` is a valid throwaway.

## Worked examples

### Plain space-separated lines

```
0322 085338 TRACE  :......connect: Attempting connection
```

```
{INT:Date} {INT:Time} {STR:Level} :{CHR:,.,1}{STR:Source}: {STR:Message}
```

- `{INT:Date}` and `{INT:Time}` read the two numeric tokens.
- `{STR:Level}` is followed by a space → stops at any whitespace, swallowing the run of spaces before the colon.
- The literal `:` matches the colon.
- `{CHR:,.,1}` consumes the run of leading dots before `Source`.
- `{STR:Source}` is followed by a literal `:` → stops at the next colon.
- `{STR:Message}` is the last field → reads to end of line.

### Zookeeper-style timestamp

```
2015-07-29 17:41:41,536 - INFO  [MainThread:Server@42] - Connection accepted
```

```
{INT:Year}-{INT:Month}-{INT:Day} {INT:Hour}:{INT:Min}:{INT:Sec},{INT:Ms} - {STR:Level} [{STR:Thread}:{STR:Class}@{INT:Line}] - {STR:Message}
```

Each `-`, `:`, `,`, `[`, `@`, `]` is a literal that must match in the source.

NOTE: There are plans to add a `{DATE:...}` field which could handle complexe and long date formats at some point.

### Fixed-width prefix

```
WARN  2025-04-07 something happened
```

```
{STR:Level,5} {INT:Year}-{INT:Month}-{INT:Day} {STR:Message}
```

`{STR:Level,5}` reads exactly 5 characters; the trailing space in "WARN " stays in `Level`.

## Malformed lines

Lines that don't fully match the format are kept in the view but flagged. Their behaviour is controlled by the `hide_bad_fmt` config key:

```
:cfg set hide_bad_fmt=true     # hide malformed lines entirely
:cfg set hide_bad_fmt=false    # show them, highlighted (default)
```

Malformed lines never satisfy a [filter](filtering.md) on a named field, since the field has no value.

## Default format

When a file has no profile yet, Logram uses `{STR:,0}` — a zero-length, unnamed string field. It only matches the empty line, so on opening a fresh file every non-empty line shows up as malformed. This is intentional: it's an obvious cue that you need to set a real format before Logram can do anything useful with the file.
