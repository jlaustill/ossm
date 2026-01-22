import subprocess
import sys
from pathlib import Path

def transpile_cnext(source=None, target=None, env=None):
    """Transpile all .cnx files before build"""
    src_dir = Path("src")
    if not src_dir.exists():
        print("Error: src directory not found")
        if env:
            env.Exit(1)
        return 1

    cnx_files = list(src_dir.rglob("*.cnx"))
    if not cnx_files:
        print("No .cnx files found")
        return 0

    print(f"Transpiling {len(cnx_files)} C-Next files...")

    try:
        result = subprocess.run(
            ["cnext", "src", "-I", "include", "-o", "src", "--header-out", "include"],
            check=True,
            capture_output=True,
            text=True
        )
        if result.stdout:
            print(result.stdout)
        print("C-Next transpilation complete")
    except subprocess.CalledProcessError as e:
        print("C-Next transpilation failed:")
        print(e.stderr)
        if env:
            env.Exit(1)
        return 1
    except FileNotFoundError:
        print("Error: cnext command not found. Install C-Next transpiler.")
        if env:
            env.Exit(1)
        return 1

    return 0

# PlatformIO integration
try:
    Import("env")
    env.AddPreAction("buildprog", transpile_cnext)
except NameError:
    # Not running under PlatformIO
    pass

if __name__ == "__main__":
    sys.exit(transpile_cnext())
