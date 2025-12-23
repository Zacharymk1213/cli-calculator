[![cli-calculator](https://snapcraft.io/cli-calculator/badge.svg)](https://snapcraft.io/cli-calculator)
[![C++ CI Build & Draft Release](https://github.com/Benedek553/cli-calculator/actions/workflows/release.yml/badge.svg)](https://github.com/Benedek553/cli-calculator/actions/workflows/release.yml)
[![CodeQL](https://github.com/Benedek553/cli-calculator/actions/workflows/github-code-scanning/codeql/badge.svg)](https://github.com/Benedek553/cli-calculator/actions/workflows/github-code-scanning/codeql)
[![Lint](https://github.com/Benedek553/cli-calculator/actions/workflows/lint.yml/badge.svg)](https://github.com/Benedek553/cli-calculator/actions/workflows/lint.yml)
[![pages-build-deployment](https://github.com/Benedek553/cli-calculator/actions/workflows/pages/pages-build-deployment/badge.svg)](https://github.com/Benedek553/cli-calculator/actions/workflows/pages/pages-build-deployment)
# cli-calculator

Simple C++ project providing two command-line tools:

- `calculator`: a full CLI calculator that
  - can read expressions of arbitrary length (e.g. `2 x 4 : 16 + (3-1)`),
  - can convert between number bases (binary, decimal, hexadecimal),
  - can convert between common measurement units (length, mass, volume, temperature),
  - supports matrix addition, subtraction, and multiplication from the interactive menu,
  - includes an integrated divisors-finder,
  - offers a prime-factorization workflow that also stores the original number as a variable for later use,
  - prints its version without launching the menu, and can list persisted variables directly from the CLI.
- `divisors`: a standalone program that lists the positive divisors of an integer.
## Quickstart

[![Get it from the Snap Store](https://snapcraft.io/en/light/install.svg)](https://snapcraft.io/cli-calculator)

### Option 1: Install from the Snap Store

Install the snap package directly:

```bash
sudo snap install cli-calculator
```

Run the calculator:

```bash
cli-calculator
```
### Option 2: Download the Latest Release (Manual Installation)

1. Download the latest prebuilt binary from the **Releases** page.
2. Make it executable:

   ```bash
   chmod +x calculator
   ```
3. Move it to your system path:

   ```bash
   sudo mv calculator /usr/local/bin/
   ```
4. Verify installation:

   ```bash
   calculator --help
   ```

## Requirements

- CMake 3.10+
- Any C++17-compatible compiler (GCC, Clang, MSVC)
- zlib development headers and library (for PNG graph export)

## Build (out-of-source)

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Run

```bash
# Start the calculator
./build/src/calculator

# Run the divisors tool separately
./build/src/divisors
```

## Features (brief)

| Feature                  | Description |
| ------------------------ | ----------- |
| Expression evaluation    | Complex expressions with `+ - * / x :` operators, parentheses, `sin`, `cos`, `log`, and factorial (`!`). Supports floating-point operations and user-defined variables read from `vars.toml`. |
| Number-base conversion   | Accepts binary (`0b`), decimal, and hexadecimal (`0x`) inputs and converts between them; sign handling included. |
| Measurement conversions  | Converts between common units of length, mass, volume, and temperature directly from the Conversion Tools menu. |
| Equation solver          | Solves linear, quadratic, cubic (`a * x^3 + b * x^2 + c * x + d = 0`) equations, plus 2x2 linear systems, including complex roots. |
| Matrix operations        | Adds, subtracts, or multiplies matrices of arbitrary size with guided prompts that validate dimensions before computing. |
| Divisor search           | Produces a sorted list of positive divisors for any integer (except 0). |
| Prime factorization      | Breaks positive integers into their prime powers, displays them in readable form (optionally starting with `-1` for negatives), and stores the factored integer in the `prime_factorization` variable. |
| Statistical analysis     | Accepts datasets directly from the menu to calculate min/max, mean, median, variance, standard deviation, percentiles, and modes. |
| Graph utility            | Imports values manually or from CSV, previews the series as ASCII, then writes a labeled PNG plot for sharing. |
| Variable inspection      | `--variables` lists persisted variable names and values without opening the interactive menu. |
| Variable persistence     | Menu option 7 lets you list/set/delete variables that are persisted in `vars.toml` and reused in subsequent evaluations. |

## Code structure

- `src/core/` shared logic:
  - `expression.*`: tokenizes and evaluates expressions (functions, factorial, operators).
  - `numeral_conversion.*`: parsing/formatting signed integers between bases with prefixes.
  - `equations.*`: prints solutions for linear, quadratic, cubic equations, plus 2x2 linear systems.
  - `input.*`: reusable console input helpers (prompts, validation).
  - `math_utils.*`: floating-point helpers reused across modules.
  - `divisors_lib.*`: shared divisor calculation used by both executables.
- `src/app/` application layer:
  - `calculator_app.*`: application lifecycle + decision logic between CLI flags and the interactive menu.
  - `cli_parser.*`: shared logic that turns raw `argv` tokens into structured actions before dispatch.
- `menu_handlers.*`: interactive menu flows for arithmetic, conversions, divisors, equations, matrix operations, prime factorization, statistics, graphing, and square roots.
- `src/main.cpp`: boots CLI colors, dispatches CLI flags, and runs the interactive menu.
- `src/tools/divisors.cpp`: standalone divisors CLI entry point.
- `src/ansi_colors.hpp`: shared ANSI color helpers (included by both apps).

## CLI flags

- `--no-color` / `-nc`: disable ANSI colors in all outputs.
- `--eval <expression>` / `-e <expression>`: evaluate and print the result, then exit.
- `--repl`: launch a lightweight read-eval-print loop for quick calculations (supports `:history`, `!<n>`, Up/Down-arrow history like Bash, and every CLI flag via `:command`/`--command` inside the session). You can also launch it from the interactive main menu via **11) Launch REPL mode**.
- `--square-root <value>` / `-sqrt <value>`: compute a single square root (fails for negative inputs).
- `--convert <from> <to> <value>` / `-c <from> <to> <value>`: convert an integer from one base to another and print the result. Accepted bases are `2`, `10` and `16`.
- `--unit-convert <category> <from> <to> <value>`: convert measurement units (categories: `length`, `mass`, `volume`, `temperature`; units accept names or symbols).
- `--prime-factorization <value>` / `-pf <value>`: display the prime factors of the given integer (falls back to `-1` for negatives).
- `--solve-linear <a> <b>`: solve a linear equation `a*x + b = 0`.
- `--solve-quadratic <a> <b> <c>`: solve a quadratic equation `a*x^2 + b*x + c = 0`.
- `--solve-cubic <a> <b> <c> <d>`: solve a cubic equation `a*x^3 + b*x^2 + c*x + d = 0`.
- `--solve-linear-system <a1> <b1> <c1> <a2> <b2> <c2>`: solve a 2x2 linear system.
- `--matrix-add <A> <B>`: add matrices (`;` between rows, `,` or spaces between columns).
- `--matrix-subtract <A> <B>`: subtract matrices (`;` between rows, `,` or spaces between columns).
- `--matrix-multiply <A> <B>`: multiply matrices (`;` between rows, `,` or spaces between columns).
- `--stats <values...>` / `--statistics <values...>`: compute summary statistics for a list of values.
- `--graph-values <output.png> <values...> [--height N]`: render values into a PNG graph (prints an ASCII preview to stdout).
- `--graph-csv <output.png> <csv-path> <column> [--height N] [--no-headers]`: render a column from a CSV file into a PNG graph.
- `--variables` / `--list-variables`: print every persisted variable and its value.
- `--set-variable <name> <value>`: create/update a variable in `vars.toml`.
- `--unset-variable <name>`: remove a persisted variable.
- `--report-bug`: open the issue template in your browser.
- `--batch <file.txt>` / `-b <file.txt>`: execute commands listed in a text file (one CLI invocation per line, comments starting with `#` are ignored). Batch files recognize helper directives such as `@set <variable>` (store the previous numeric result), `@input <variable>` (prompt for a value/expression and store it), `@include <file>` (process another batch file), `@if <expression>` / `@endif` (conditional execution, truthy = non-zero), and `@unset <variable>` (remove a stored variable).
- `--output <format>`: emit structured responses (`json`, `xml`, or `yaml`) for CLI flags so scripts can parse the calculator output more easily.
- `--version` / `-v`: display the calculator version and exit.

All numeric CLI arguments (square root inputs, divisor targets, conversion bases/values, prime-factorization targets) accept variable names that were previously defined either via the menu or in batch mode via `@set`, `@input`, or an included file. Structured output formats require a one-shot CLI flag (they are rejected if only `--output` is provided without an action).

## Variables

- `vars.toml` stores user-defined variable names and values. The application loads it on start and overwrites it whenever you make changes through the UI.
- Menu option **7) Variable manager** lists current entries, lets you create/update a variable (prompted for name/value), and remove entries you no longer need.
- Valid variable names must start with a letter and can contain letters, digits, or underscores. Expressions referencing undefined variables throw an error.

## Supported functions in expressions

`sin`, `cos`, `tan`, `cot`, `asin`, `acos`, `atan`, `log`, `exp`, `sqrt`, `sinh`, and factorial `!`. Expressions also accept `x` and `:` as multiplication and division.

## Helpful tips

- In the calculator menu, type `back` to return to the algebraic input mode.
- The `x` and `:` symbols are interpreted as multiplication and division, respectively, to match common Hungarian input conventions.
- The number-base menu accepts numbers prefixed with `0b` or `0x`.
- Trigonometric/logarithmic functions expect radians and must be written with parentheses (e.g. `sin(0.5)`).

## Graph utility workflow

1. Launch **10) Graph utility** from the main menu.
2. Choose whether you want to enter values manually or load them from a CSV file.
   - *Manual*: paste or type a whitespace- or comma-separated list of numbers. Enter `back` to cancel.
   - *CSV*: provide the file path, confirm whether the first row holds headers, then pick the column either by name or index. Empty or invalid entries are skipped with a short summary.
3. Pick the ASCII preview height (2–20). This uses the existing terminal renderer so you can sanity-check your dataset before exporting.
4. Provide an output filename (defaults to `graph.png`). If you omit `.png`, it is appended automatically.
5. The tool writes a 600×400+ pixel PNG that includes grid lines, labeled axes, and red data points connected by blue line segments to make the exported chart easier to interpret at a glance.

PNG exports rely on zlib; make sure the development package is available if you build from source.

### REPL shortcuts

- Start it either with the `--repl` flag or by selecting **11) Launch REPL mode** from the interactive main menu.
- Press the Up/Down arrows (on POSIX terminals) to navigate command history, similar to Bash.
- Use `:history` to print the stored commands and `!<n>` to rerun the nth entry.
- Every CLI flag is available inside the REPL via `:command` or `--command`, so `:convert`, `--divisors`, `:batch`, etc., behave the same as their one-shot counterparts.
- `:cli-help`/`--help` prints the full CLI help text, while `:help` focuses on REPL-only shortcuts.
- Structured `--output` formats are intentionally disabled during REPL sessions to keep interactions conversational.

## Base conversion (CLI)

- Description: Convert a single integer from one numeral system to another using the `--convert` (or `-c`) flag. The program accepts source and target bases as decimal integers (`2`, `10` or `16`) followed by the value to convert.
- Input formats: the value may be signed (`+` or `-`) and may include the usual `0b` (binary) or `0x` (hex) prefixes. The converter also accepts plain digits for decimal input.
- Output: the converted integer is printed with the conventional prefix for non-decimal bases (`0b` for binary, `0x` for hexadecimal). Decimal output is shown without a prefix.
- Exit codes: returns `0` on success; prints an error and returns non-zero on invalid bases, malformed input, or overflow.

Examples

```bash
# Convert decimal 78 to hexadecimal
./build/src/calculator --convert 10 16 78
# Result: 0x4E

# Convert a binary value to decimal (accepts 0b prefix)
./build/src/calculator -c 2 10 0b1011
# Result: 11

# Convert a negative hexadecimal value to binary
./build/src/calculator --convert 16 2 -0x1A
# Result: -0b11010
```

## Examples (new CLI flags)

```bash
# Convert between units (symbols or names accepted)
./build/src/calculator --unit-convert length m km 1234

# Solve equations directly
./build/src/calculator --solve-linear 2 -8
./build/src/calculator --solve-quadratic 1 -3 2
./build/src/calculator --solve-cubic 1 -6 11 -6
./build/src/calculator --solve-linear-system 1 1 3 2 -1 0

# Matrix operations (rows separated by ';')
./build/src/calculator --matrix-add "1 2; 3 4" "5 6; 7 8"
./build/src/calculator --matrix-multiply "1 2 3; 4 5 6" "7 8; 9 10; 11 12"

# Stats and graphing
./build/src/calculator --stats 1 2 3 4 5 5
./build/src/calculator --graph-values graph.png 1 3 2 5 --height 8
./build/src/calculator --graph-csv graph.png data.csv 2 --no-headers

# Variables without opening the menu
./build/src/calculator --set-variable tax_rate 0.27
./build/src/calculator --unset-variable old_value
```

## Contribution

Feel free to submit issues or pull requests!
