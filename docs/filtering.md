# Filtering

A **filter** hides every line of the log that doesn't match a condition you specify on its named fields. With a filter active, scrolling, jumping, and searching only see the kept lines — the underlying file is never modified.

Filters work on the named fields you defined in your [format string](format-strings.md). A line that fails to parse cannot be evaluated by a filter since it couldn't be matched to the defined fields, hence its visibility is only determined by the `hide_bad_fmt` (config value)[keybindings.md#Configuration].

## A single condition

The smallest filter is one comparison on one field:

```
<field_name> <COMPARATOR> <value>
```

For example, given a format with a `Level` field:

```
Level EQUAL ERROR
```

keeps lines whose `Level` parsed to exactly `ERROR`.

A few rules to keep in mind:

- The comparator **must be surrounded by spaces** on both sides. `Level=ERROR` is not valid; `Level EQ ERROR` is.
- The comparator name is matched case-sensitively (`EQUAL`, not `equal`).
- The value is parsed according to the field's declared type — `INT` fields call `strtol`, `DBL` fields call `strtod`, `CHR` takes the first character of the value, `STR` takes the whole thing.

## Comparators

All comparators apply to all four field types unless noted.

| Comparator | Aliases | Meaning |
|------------|---------|---------|
| `EQ` | `EQUAL` | field value equals the given value |
| `ST` | `SMALLER`, `SMALLER_THAN` | field value is strictly less than the given value |
| `SE` | `SMALLER_EQ`, `SMALLER_EQUAL`, `SMALLER_OR_EQUAL` | less than or equal |
| `GT` | `GREATER`, `GREATER_THAN` | strictly greater than |
| `GE` | `GREATER_EQ`, `GREATER_EQUAL` | greater than or equal |
| `CT` | `CONTAINS` | *(STR only)* the value appears as a substring |
| `BW` | `BEGINS_WITH`, `SW`, `STARTS_WITH` | *(STR only)* the field starts with the value |
| `EW` | `ENDS_WITH` | *(STR only)* the field ends with the value |

For `INT`, `DBL`, and `CHR` fields, ordering is numeric (`CHR` compares the byte value). For `STR` fields, ordering is lexicographic — `Level GT INFO` keeps strings that come after `INFO` alphabetically, not "more severe" ones.

### Case-insensitive matching

Append `_CI` to any comparator to make string comparison case-insensitive:

```
Message CT_CI timeout
```

matches `Timeout`, `TIMEOUT`, `tImEoUt`. The suffix is accepted on every comparator (e.g. `EQ_CI`, `BW_CI`) and is a no-op for non-`STR` fields.

## Combining conditions

Conditions can be combined with the boolean operators `AND`, `OR`, `XOR`, `NOR` — all infix, all space-padded:

```
Level EQ ERROR AND Source CT auth
Level EQ WARN OR Level EQ ERROR
Source CT auth XOR Source CT db
Level EQ DEBUG NOR Level EQ TRACE   # neither DEBUG nor TRACE
```

### Precedence and parentheses

There is **no operator precedence**. The parser splits the expression at the leftmost boolean operator and recurses on each side, so:

```
A AND B OR C    is parsed as    A AND (B OR C)
A OR B AND C    is parsed as    A OR (B AND C)
```

That is the opposite of what most languages do for `AND`/`OR`. Whenever you mix operators, **use parentheses to be explicit**:

```
(Level EQ ERROR AND Time GT 085339) OR Source EQ critical
```

Parentheses can be nested arbitrarily and are the only way to override the leftmost-split behaviour.

## Filtering by line number

There is one built-in field that exists regardless of your format string: `line_num`. It uses a special syntax — only `CT` (`CONTAINS`) is accepted, and the value is a `from,to` pair (inclusive on both ends):

```
line_num CT 100,200       # keep lines 100 through 200
```

You can combine it with other conditions like any other clause:

```
line_num CT 1000,2000 AND Level EQ ERROR
```

## Applying filters

Filters live in [command mode](keybindings.md#command-mode). The interesting verbs are all variations on a theme: do you want to *replace* the active filter, *combine* it with what's already there, or *clear* it?

### Replacing — `:fset`

```
:fset Level EQ ERROR
```

Drops whatever filter was active and installs this one as the only condition.

### Combining — `:f`, `:for`, `:fxor`, `:fnor`

These take the active filter and combine it with the new expression using the indicated boolean operator:

| Command | Behaviour |
|---------|-----------|
| `:f`, `:fadd`, `:fand` | combine with `AND` (intersection — both must hold) |
| `:for` | combine with `OR` (union — either may hold) |
| `:fxor` | combine with `XOR` (exactly one of the two) |
| `:fnor` | combine with `NOR` (neither holds) |

If no filter is active yet, the combinators behave like `:fset` and just install the new expression directly.

This is the natural workflow for narrowing down: set a broad filter, then `:f` your way down through extra conditions.

### Excluding — `:fout`

```
:fout Level EQ DEBUG
```

`:fout` is the "filter *out*" verb: it inverts the new expression, then ANDs it onto whatever is already active. The example above keeps everything that *isn't* DEBUG, on top of any pre-existing filter. Equivalent to manually negating with `NOR` against an always-true clause, but quicker to type.

### Clearing — `:fclear`

```
:fclear
```

Removes the active filter entirely. The view goes back to showing every line.

### Saving the active filter

The current filter can be persisted into the file's profile so it reapplies on the next open:

```
:cfg set filter
```

(Note: no `=` and no value — this command snapshots the *currently active* filter into the profile.) See [Keybindings → command mode](keybindings.md#command-mode) for the rest of the `:cfg` family.

## Grammar

For reference, the formal grammar accepted by the filter parser:

```
filter        ::= "(" filter ")"
                | filter bool_op filter
                | simple_filter
simple_filter ::= field_name comparator value
                | "line_num" "CT" int "," int
bool_op       ::= "AND" | "OR" | "XOR" | "NOR"
comparator    ::= base_cmp [ "_CI" ]
base_cmp      ::= "EQ"  | "EQUAL"
                | "ST"  | "SMALLER" | "SMALLER_THAN"
                | "SE"  | "SMALLER_EQ" | "SMALLER_EQUAL" | "SMALLER_OR_EQUAL"
                | "GT"  | "GREATER" | "GREATER_THAN"
                | "GE"  | "GREATER_EQ" | "GREATER_EQUAL"
                | "CT"  | "CONTAINS"
                | "BW"  | "BEGINS_WITH" | "SW" | "STARTS_WITH"
                | "EW"  | "ENDS_WITH"
```

Boolean operators and comparators are matched as whole tokens — they must be surrounded by spaces.
