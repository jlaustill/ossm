# Contributing to OSSM

Thank you for your interest in contributing to OSSM!

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- [Volta](https://volta.sh/) for Node.js tooling (optional, for C-Next transpiler)
- Git

### Setup

```bash
# Clone the repository
git clone https://github.com/jlaustill/ossm.git
cd ossm

# Build the project
pio run

# Upload to Teensy 4.0
pio run -t upload

# Monitor serial output
pio device monitor
```

---

## Project Structure

```
ossm/
├── src/
│   ├── main.cnx                 # Entry point
│   ├── AppConfig.cnx            # Configuration structure
│   ├── Data/                    # Hardware abstraction layer
│   │   ├── ADS1115Manager.cnx   # ADC management
│   │   ├── MAX31856Manager.cnx  # Thermocouple
│   │   ├── BME280Manager.cnx    # Ambient sensor
│   │   └── ConfigStorage.cnx    # EEPROM
│   ├── Domain/                  # Business logic
│   │   ├── ossm.cnx             # Main orchestration
│   │   ├── SensorProcessor.cnx  # Raw → engineering units
│   │   ├── CommandHandler.cnx   # Config commands
│   │   └── SerialCommandHandler.cnx
│   └── Display/                 # Output layer
│       ├── J1939Bus.cnx         # CAN transmission
│       ├── J1939Encode.cnx      # Message encoding
│       └── SpnInfo.cnx          # SPN metadata
├── include/                     # Generated headers
├── lib/                         # Local libraries
├── hardware/                    # Schematics and PCB files
├── docs/                        # Documentation
└── platformio.ini               # Build configuration
```

---

## C-Next

OSSM is written in C-Next, a safer C dialect. Key differences from C:

| C | C-Next | Notes |
|---|--------|-------|
| `x = 5` | `x <- 5` | Assignment |
| `x == 5` | `x = 5` | Equality |
| `int` | `i32` | Fixed-width types |
| `struct` | `struct` | Same syntax |
| class methods | `scope Name { }` | Generates C functions |

### Transpiling

C-Next files (`.cnx`) transpile to C++ (`.cpp`):

```bash
# Install C-Next (via Volta)
volta install cnext

# Transpile all files
cnext src/

# Clean generated files
cnext --clean src/
```

The build system (PlatformIO) handles transpilation automatically via `cnext_build.py`.

---

## Development Workflow

### Making Changes

1. Create a feature branch: `git checkout -b feature/my-feature`
2. Make your changes
3. Test on hardware if possible
4. Run static analysis: `pio check`
5. Commit with descriptive message
6. Open a pull request

### Code Style

- Use C-Next syntax (not raw C/C++)
- Follow existing naming conventions:
  - `camelCase` for variables and functions
  - `PascalCase` for types and scopes
  - `UPPER_CASE` for constants
- Keep functions small and focused
- Add comments for non-obvious logic

### Commit Messages

Use conventional commit format:

```
feat: add support for new sensor type
fix: correct pressure scaling for SPN 100
docs: update serial command reference
refactor: extract CRC calculation to separate module
```

---

## Testing

### Hardware Testing

Most testing requires actual hardware. See `docs/QA-CHECKLIST.md` for the full test procedure.

### Static Analysis

```bash
# Run cppcheck
pio check

# Check specific environment
pio check -e teensy40
```

---

## Architecture Guidelines

### Adding a New Sensor Type

1. Create manager in `src/Data/` following existing patterns
2. Add configuration fields to `AppConfig.cnx`
3. Add processing logic to `SensorProcessor.cnx`
4. Add J1939 encoding to `Display/` layer
5. Update serial commands if needed
6. Document new SPNs in `docs/SPN-REFERENCE.md`

### Adding a New SPN

1. Add SPN metadata to `SpnInfo.cnx`
2. Add validation to `SpnCheck.cnx`
3. Add encoding to appropriate `J1939Encode` function
4. Update PGN transmission in `ossm.cnx`
5. Document in `docs/SPN-REFERENCE.md`

### Non-Blocking Requirement

All sensor reads must be non-blocking. Use state machines that advance on each `update()` call. Never use `delay()` or blocking I2C/SPI reads.

---

## Hardware Changes

Hardware files are in `hardware/V0.0.2/`:

- `Schematic_OSSM_*.pdf` - Circuit schematic
- `PCB_PCB_OSSM_*.pdf` - PCB layout

For hardware modifications, please open an issue first to discuss.

---

## Questions?

- Open an issue for bugs or feature requests
- Check existing issues before creating new ones
- Include hardware version and firmware version in bug reports

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
