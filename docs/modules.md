# Modules

Everything you do in Logram — moving the cursor, searching for text, managing filters, quitting with `:q` — is handled by a module. Modules are small, self-contained units that plug into Logram's terminal interface and define how the application reacts to your input. There is no hardcoded behaviour: if a keystroke does something, it's because a module told Logram what to do with it.

This design means that if you want to tailor Logram to your workflow or to the peculiarities of your logs, you can write your own module. Want a shortcut that jumps to the next error? A command that extracts timestamps into a summary? You write a module for that.

## Where to find existing modules

All module source files live under:

```
src/frontends/term/src/modules/
```

The module class declarations are in `src/frontends/term/include/terminal_modules.hpp`, and the base class that every module inherits from is defined in `src/frontends/term/include/logram_terminal_module.hpp`.

## A word of caution

Logram is in beta. The module interface, the terminal state structure, and the registration APIs are all work in progress. A module you write today will very likely break as the internals evolve. That's the trade-off for getting in early — you can shape Logram to your needs right now, but expect to update your code as things change.

Also, at the moment there is no dynamic loading mechanism. To add a module, you write it directly in the source tree alongside the existing ones and recompile the whole project. In practice this is fine — Logram compiles fast — but proper plugin support is planned for the future.

## How modules work

Every module has three responsibilities, each expressed as a method that Logram calls at startup. A module can engage with any combination of the three — it doesn't have to use all of them.

### Mapping input to actions

The first thing a module can do is tell Logram "when the user presses this key sequence, raise this action." An action is just a named event (internally, a numeric code) that represents an intention like "move up" or "start a search."

For example, the **Arrows** module maps the terminal escape sequences for the arrow keys to movement actions. Separately, the **WASD** module maps the `w`, `a`, `s`, `d` keys to those same actions, and the **Vim Motions** module maps `h`, `j`, `k`, `l`. All three modules produce the same movement actions — they just offer different key bindings for them. This is why input mapping and action handling are separate: multiple modules can map different keys to the same action without duplicating the logic that carries it out.

The **Text Search** module uses this mechanism too: it maps `/` to a "start search" action, and `n`/`N` to "search next" and "search previous."

### Reacting to actions

Once an action has been raised (because a key mapping matched), Logram walks through every module that registered an action callback and gives each one a chance to handle it.

The **Cursor Move** module is a good example. It doesn't map any keys itself — it leaves that to the Arrows, WASD, and Vim Motions modules. Instead, it registers a callback that watches for the movement actions and updates the cursor position accordingly: scrolling the view when the cursor reaches the edge of the screen, clamping to valid lines, and so on. This way, the movement logic exists in one place regardless of which keys triggered it.

The **Text Search** module also reacts to actions: when it sees the "start search" action, it switches Logram into command-entry mode so you can type a search pattern. When it sees "search next" or "search previous," it jumps to the next or previous match.

### Reacting to commands

When you type `:` in Logram, you enter command mode. The text you type (for instance `:q` or `:fset level == ERROR`) is dispatched to every module that registered a command callback. The first module that recognizes the command handles it.

The **Vim Quit** module is the simplest example: it listens for `:q` and exits the program. That's all it does — no key mappings, no action callbacks, just a single command.

The **Filter Management** module is at the other end of the spectrum. It handles an entire family of commands — `:fset`, `:f`, `:fadd`, `:fand`, `:for`, `:fxor`, `:fnor`, `:fout`, `:fclear` — to let you build and combine filters in different ways.

The **Vim Motions** module reacts to commands too: if you type `:<number>`, it jumps to that line — the same behaviour you'd expect from Vim.

## Putting it together

A module can be as simple as the **Vim Quit** module (one command, a handful of lines) or as involved as the **Filter Management** module (a family of commands, state tracking, filter composition). The three-part structure — map inputs, handle actions, handle commands — gives you a clean way to decide where your logic belongs:

- If you're adding a new key binding for an existing action, you only need input mapping.
- If you're adding new behaviour that responds to key presses, you need input mapping and an action callback.
- If you're adding a `:` command, you need a command callback.

Look at the existing modules in `src/frontends/term/src/modules/` for patterns to follow. The Arrows module and Vim Quit module are good starting points for understanding the basics. The Text Search module shows how all three parts fit together.

## Creating your own module

You can use the python script located at `scripts/new_module.py`, give it your module's name as argument, e.g. `python new_module.py WordJump`, which will make all the necessary updates so that you can focus on writing your module's logic.

In detail, that script will:
1. Create the module's source file (`word_jump_module.cpp`) at the right location (`src/frontends/term/src/modules`)
2. Update the right CMakeLists.txt so that file get compiled
3. Add your Module declaration to a header file.
4. Update Logram's main function in order to "register" the module

Once you've finished writing your module and you are ready to test it, just recompile Logram: 

```bash
mkdir build
cd build
cmake ..
make -j 8
```