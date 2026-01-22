import subprocess
import sys
from pathlib import Path

def transpile_cnext():
    """Transpile all .cnx files before build"""
    src_dir = Path("src")
    if not src_dir.exists():
        print("Error: src directory not found")
        return 1

    cnx_files = list(src_dir.rglob("*.cnx"))
    if not cnx_files:
        print("No .cnx files found")
        return 0

    print(f"C-Next: Transpiling {len(cnx_files)} files...")

    try:
        result = subprocess.run(
            ["cnext", "src", "-I", "include", "-o", "src", "--header-out", "include"],
            check=True,
            capture_output=True,
            text=True
        )
        if result.stdout:
            # Print just the summary, not all file names
            lines = result.stdout.strip().split('\n')
            for line in lines:
                if line.startswith(('Compiled', 'Collected', 'Generated')):
                    print(f"C-Next: {line}")
        print("C-Next: Transpilation complete")
        return 0
    except subprocess.CalledProcessError as e:
        print("C-Next: Transpilation FAILED")
        print(e.stderr)
        return 1
    except FileNotFoundError:
        print("C-Next: ERROR - cnext command not found")
        print("Install C-Next transpiler: npm install -g c-next")
        return 1

# PlatformIO integration - run immediately during pre: script phase
try:
    Import("env")
    # Run transpilation now, before build starts
    result = transpile_cnext()
    if result != 0:
        env.Exit(1)
except NameError:
    # Not running under PlatformIO, allow standalone execution
    pass

if __name__ == "__main__":
    sys.exit(transpile_cnext())
