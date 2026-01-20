# Static Analysis Details

This document describes the static analysis tools and checks used in the OSSM codebase.

## C-Next Built-in Checks (27 Error Codes)

C-Next runs 7 semantic analyzers during compilation, enforcing safety rules at transpile time:

| Category | Error Codes | Key Rules |
|----------|-------------|-----------|
| **Initialization** | E0381 | Use-before-init detection (Rust-style flow analysis) |
| **Control Flow** | E0701, E0702 | MISRA 14.4 (boolean conditions), MISRA 13.5 (no function calls in conditions) |
| **Arithmetic** | E0800-E0804 | Division/modulo by zero, float modulo forbidden |
| **NULL Safety** | E0901-E0908 | C library NULL returns must be checked, flow-sensitive tracking |
| **Critical Sections** | E0853-E0855 | No return/break/continue inside critical sections |
| **Preprocessing** | E0501-E0503 | No function-like macros, no value defines, no .c includes |
| **Sizeof** | E0601-E0602 | MISRA 13.6 (no side effects in sizeof) |
| **Naming** | E0227 | Parameter naming conflicts with scope variables |

### MISRA C:2012 Rules Enforced

- Rule 3.1 & 3.2: Comment validation (no nested `/* */`, no line-splice)
- Rule 13.5: No function calls in control flow conditions
- Rule 13.6: No side effects in sizeof operand
- Rule 14.4: Boolean-only conditions in do-while
- Rule 21.3: No dynamic memory allocation (by design)

## External Static Analysis Tools

### cppcheck

General-purpose C/C++ static analyzer.

**Command:**
```bash
cppcheck --enable=all --suppress=missingIncludeSystem -I src src/
```

**Warning categories:**

| Category | Description |
|----------|-------------|
| `unusedFunction` | Function defined but never called |
| `cstyleCast` | C-style cast used instead of C++ cast |
| `constParameterPointer` | Pointer parameter could be declared const |

### cppcheck MISRA Addon

MISRA C:2012 compliance checking via cppcheck addon.

**Command:**
```bash
cppcheck --addon=misra --enable=all --suppress=missingIncludeSystem -I src src/
```

**Known detections in handwritten code:**
- Rule 12.3: Comma operator (FlexCAN template syntax)
- Rule 17.2: Recursion (intentional in handleQuery fall-through)

### flawfinder

Security-focused scanner for C/C++ (CWE database).

**Command:**
```bash
flawfinder --minlevel=1 src/
```

**CWE categories detected:**

| Level | CWE | Pattern | Risk |
|-------|-----|---------|------|
| 2 | CWE-119/120 | Static `char[]` arrays | Buffer overflow potential |
| 1 | CWE-120 | `read` in loops | Boundary check needed |
| 1 | CWE-126 | `strlen` | Non-null-terminated string crash |
| 1 | CWE-120 | `strncpy` | May not null-terminate |

### clang-tidy

LLVM-based linter (requires platform headers).

**Note:** Not runnable on embedded targets without Arduino toolchain headers.

## Analysis Comparison

| What external tools catch | What C-Next prevents (compile-time) |
|---------------------------|-------------------------------------|
| cstyleCast (style) | Buffer overflows (bounds-checked slices) |
| constParameterPointer (style) | Uninitialized variables (E0381) |
| unusedFunction (style) | Function calls in conditions (E0702/MISRA 13.5) |
| CWE-119/120 buffer risks | Array bounds violations |
| MISRA 12.3 comma operator | |
| MISRA 17.2 recursion | |
| | Division by zero (E0800-E0803) |
| | NULL pointer dereference (E0901-E0908) |
| | Integer overflow (`+<-` wrapping operator) |

**Key insight:** External tools find style issues and potential risks after compilation. C-Next prevents safety issues at transpile time, before code generation.
