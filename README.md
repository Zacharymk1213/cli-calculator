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
  - and includes an integrated divisors-finder.
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
| Equation solver          | Solves linear (`a * x + b = 0`) and quadratic (`a * x^2 + b * x + c = 0`) equations, including complex roots. |
| Divisor search           | Produces a sorted list of positive divisors for any integer (except 0). |
| Variable persistence     | Menu option 6 lets you list/set/delete variables that are persisted in `vars.toml` and reused in subsequent evaluations. |

## Code structure

- `src/core/` shared logic:
  - `expression.*`: tokenizes and evaluates expressions (functions, factorial, operators).
  - `numeral_conversion.*`: parsing/formatting signed integers between bases with prefixes.
  - `equations.*`: prints solutions for linear and quadratic equations.
  - `input.*`: reusable console input helpers (prompts, validation).
  - `math_utils.*`: floating-point helpers reused across modules.
  - `divisors_lib.*`: shared divisor calculation used by both executables.
- `src/app/` application layer:
  - `cli_actions.*`: one-shot flag handling for `--eval`, `--convert`, `--divisors`, `--square-root`.
  - `menu_handlers.*`: interactive menu flows for arithmetic, conversions, divisors, equations, square roots.
- `src/main.cpp`: boots CLI colors, dispatches CLI flags, and runs the interactive menu.
- `src/tools/divisors.cpp`: standalone divisors CLI entry point.
- `src/ansi_colors.hpp`: shared ANSI color helpers (included by both apps).

## CLI flags

- `--no-color` / `-nc`: disable ANSI colors in all outputs.
- `--eval <expression>` / `-e <expression>`: evaluate and print the result, then exit.
- `--square-root <value>` / `-sqrt <value>`: compute a single square root (fails for negative inputs).
 - `--convert <from> <to> <value>` / `-c <from> <to> <value>`: convert an integer from one base to another and print the result. Accepted bases are `2`, `10` and `16`.

## Variables

- `vars.toml` stores user-defined variable names and values. The application loads it on start and overwrites it whenever you make changes through the UI.
- Menu option **6) Variable manager** lists current entries, lets you create/update a variable (prompted for name/value), and remove entries you no longer need.
- Valid variable names must start with a letter and can contain letters, digits, or underscores. Expressions referencing undefined variables throw an error.

## Supported functions in expressions

`sin`, `cos`, `tan`, `cot`, `asin`, `acos`, `atan`, `log`, `exp`, `sqrt`, `sinh`, and factorial `!`. Expressions also accept `x` and `:` as multiplication and division.

## Helpful tips

- In the calculator menu, type `back` to return to the algebraic input mode.
- The `x` and `:` symbols are interpreted as multiplication and division, respectively, to match common Hungarian input conventions.
- The number-base menu accepts numbers prefixed with `0b` or `0x`.
- Trigonometric/logarithmic functions expect radians and must be written with parentheses (e.g. `sin(0.5)`).


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

## Contribution

Feel free to submit issues or pull requests!
