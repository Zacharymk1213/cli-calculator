# cli-calculator

[![cli-calculator](https://snapcraft.io/cli-calculator/badge.svg)](https://snapcraft.io/cli-calculator)
[![C++ CI Build & Draft Release](https://github.com/Benedek553/cli-calculator/actions/workflows/release.yml/badge.svg)](https://github.com/Benedek553/cli-calculator/actions/workflows/release.yml)
[![CodeQL](https://github.com/Benedek553/cli-calculator/actions/workflows/github-code-scanning/codeql/badge.svg)](https://github.com/Benedek553/cli-calculator/actions/workflows/github-code-scanning/codeql)
[![Lint](https://github.com/Benedek553/cli-calculator/actions/workflows/lint.yml/badge.svg)](https://github.com/Benedek553/cli-calculator/actions/workflows/lint.yml)
[![pages-build-deployment](https://github.com/Benedek553/cli-calculator/actions/workflows/pages/pages-build-deployment/badge.svg)](https://github.com/Benedek553/cli-calculator/actions/workflows/pages/pages-build-deployment)

A simple but powerful C++ project that provides two command-line tools for everyday and advanced mathematical tasks.

---

## Overview

This project consists of two executables:

* **`calculator`** – a full-featured interactive CLI calculator that:

  * evaluates expressions of arbitrary length (e.g. `2 x 4 : 16 + (3 - 1)`),
  * converts between number bases (binary, decimal, hexadecimal),
  * converts between common measurement units (length, mass, volume, temperature),
  * supports matrix addition, subtraction, and multiplication,
  * includes an integrated divisor finder,
  * performs prime factorization and stores the original number as a reusable variable,
  * prints its version or lists persisted variables directly from the CLI without starting the menu.

* **`divisors`** – a standalone utility that lists all positive divisors of an integer.

---

## Quickstart

[![Get it from the Snap Store](https://snapcraft.io/en/light/install.svg)](https://snapcraft.io/cli-calculator)

### Option 1: Install from the Snap Store

```bash
sudo snap install cli-calculator
```

Run the calculator:

```bash
cli-calculator
```
## Option 2: Install with APT

1. Download signing key:
```bash
curl -fsSL https://clicalculator.blob.core.windows.net/repo/clicalc.gpg \
| sudo gpg --dearmor -o /usr/share/keyrings/clicalc.gpg
```
2. Add the repo:
```bash
echo "deb [signed-by=/usr/share/keyrings/clicalc.gpg] \
https://clicalculator.blob.core.windows.net/repo stable main" \
| sudo tee /etc/apt/sources.list.d/clicalc.list

sudo apt update
```
3. Install package:
```bash
sudo apt install cli-calculator -y
```
### Option 3: Download the Latest Release (Manual Installation)

1. Download the latest prebuilt binary from the **Releases** page.
2. Make it executable:

```bash
chmod +x calculator
```

3. Move it into your system path:

```bash
sudo mv calculator /usr/local/bin/
```

4. Verify the installation:

```bash
calculator --help
```

---

## Building from Source

### Requirements

* Git
* CMake 3.10 or newer
* A C++17-compatible compiler (GCC, Clang, MSVC)
### Clone

```bash
git clone https://github.com/benedek553/cli-calculator.git
cd ./cli-calculator
```
### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run

```bash
# Start the calculator
./build/src/calculator

# Run the divisors tool
./build/src/divisors
```
### Install (optional) 
```bash
sudo cmake --install build
```
---

## Features

| Feature                  | Description |
| ------------------------ | ----------- |
| Expression evaluation    | Complex expressions with `+ - * / x :` operators, parentheses, `sin`, `cos`, `log`, and factorial (`!`). Supports floating-point operations and user-defined variables read from `vars.toml`. |
| Number-base conversion   | Accepts binary (`0b`), decimal, and hexadecimal (`0x`) inputs and converts between them; sign handling included. |
| Measurement conversions  | Converts between common units of length, mass, volume, and temperature directly from the Conversion Tools menu. |
| Equation solver          | Solves linear (`a * x + b = 0`) and quadratic (`a * x^2 + b * x + c = 0`) equations, including complex roots. |
| Matrix operations        | Adds, subtracts, or multiplies matrices of arbitrary size with guided prompts that validate dimensions before computing. |
| Divisor search           | Produces a sorted list of positive divisors for any integer (except 0). |
| Prime factorization      | Breaks positive integers into their prime powers, displays them in readable form (optionally starting with `-1` for negatives), and stores the factored integer in the `prime_factorization` variable. |
| Statistical analysis     | Accepts datasets directly from the menu to calculate min/max, mean, median, variance, standard deviation, percentiles, and modes. |
| Graph utility            | Imports values manually or from CSV, previews the series as ASCII, then writes a labeled PNG plot for sharing. |
| Variable inspection      | `--variables` lists persisted variable names and values without opening the interactive menu. |
| Variable persistence     | Menu option 7 lets you list/set/delete variables that are persisted in `vars.toml` and reused in subsequent evaluations. |

---

## Code Structure

* **`src/core/`** – shared logic

  * `expression.*`: tokenization and expression evaluation
  * `numeral_conversion.*`: signed base parsing and formatting
  * `equations.*`: linear and quadratic solvers
  * `input.*`: reusable console input helpers
  * `math_utils.*`: floating-point helpers
  * `divisors_lib.*`: shared divisor logic

* **`src/app/`** – application layer

  * `cli_actions.*`: one-shot CLI flag handling
  * `menu_handlers.*`: interactive menu workflows

* `src/main.cpp`: CLI bootstrap, flag dispatch, and menu entry point

* `src/tools/divisors.cpp`: standalone divisors executable

* `src/ansi_colors.hpp`: shared ANSI color utilities

---

## CLI Flags

* `--no-color`, `-nc`: disable ANSI colors
* `--eval <expr>`, `-e <expr>`: evaluate an expression and exit
* `--repl`: launch the lightweight REPL mode
* `--square-root <value>`, `-sqrt <value>`: compute a square root
* `--convert <from> <to> <value>`, `-c <from> <to> <value>`: convert between bases (`2`, `10`, `16`)
* `--prime-factorization <value>`, `-pf <value>`: display prime factors
* `--variables`, `--list-variables`: list persisted variables
* `--batch <file>`, `-b <file>`: execute commands from a batch file
* `--output <json|xml|yaml>`: structured output for one-shot CLI commands
* `--version`, `-v`: print version and exit

All numeric arguments may reference previously defined variables. Structured output is only available for one-shot CLI commands and is disabled in interactive and REPL modes.

---

## Variables

* Variables are stored in `vars.toml` and loaded on startup.
* Menu option **7) Variable manager** allows listing, creating, updating, and deleting variables.
* Variable names must start with a letter and may contain letters, digits, and underscores.

---

## Supported Functions

`sin`, `cos`, `tan`, `cot`, `asin`, `acos`, `atan`, `log`, `exp`, `sqrt`, `sinh`, and factorial (`!`).

Multiplication and division may also be written as `x` and `:` to match common Hungarian input conventions.

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

## REPL Tips

* Start with `--repl` or via **11) Launch REPL mode** in the menu.
* Use Up/Down arrows to navigate command history.
* `:history` lists past commands, `!<n>` reruns an entry.
* All CLI flags are available inside the REPL via `:command` or `--command`.
* Structured `--output` formats are intentionally disabled in REPL mode.

---

## Base Conversion Example

```bash
# Convert decimal 78 to hexadecimal
calculator --convert 10 16 78
# Result: 0x4E

# Convert binary to decimal
calculator -c 2 10 0b1011
# Result: 11

# Convert negative hexadecimal to binary
calculator --convert 16 2 -0x1A
# Result: -0b11010
```

---

## Contributing

Issues and pull requests are welcome. Contributions that improve correctness, usability, or documentation are especially appreciated.

