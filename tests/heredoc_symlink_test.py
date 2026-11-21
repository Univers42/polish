#!/usr/bin/env python3
"""Regression test: heredoc backing file uses mkstemp in $TMPDIR and rejects symlink clobber.

THE BUG:
Heredocs over 4096 bytes used an unlinked temporary file with a predictable
path `/tmp/heredoc_<pid>_<idx>` opened with O_RDWR | O_CREAT | O_TRUNC without
O_EXCL. If an unprivileged local attacker or pre-existing symlink pointed to
a target file, writing the heredoc clobbered the target.

THE FIX:
`hdoc_file_backing` uses `mkstemp()` in `$TMPDIR` (falling back to `/tmp`) with
an anonymous unlinked descriptor. `mkstemp()` uses `O_CREAT | O_EXCL`, so it
will never follow an existing symlink or overwrite existing files.

Acceptance criteria:
- a test with a planted symlink is not followed
- large heredocs (>4096 bytes) round-trip with byte-for-byte fidelity

Usage: python3 heredoc_symlink_test.py [/path/to/polish]
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "polish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def main():
    tmpdir = tempfile.mkdtemp(prefix="hdoc_symlink_test_")
    try:
        # Create a canary target file
        canary_path = os.path.join(tmpdir, "canary.txt")
        with open(canary_path, "w") as f:
            f.write("CANARY_UNTOUCHED\n")

        # Plant symlinks that might match predictable names
        # e.g., predictable /tmp/heredoc_* and any polish_hdoc_*
        for i in range(10):
            sym1 = os.path.join(tmpdir, f"heredoc_{os.getpid()}_{i}")
            sym2 = os.path.join(tmpdir, f"heredoc_{i}")
            try:
                os.symlink(canary_path, sym1)
                os.symlink(canary_path, sym2)
            except OSError:
                pass

        # Generate heredoc body > 4096 bytes to force file backing
        chunk = "0123456789abcdef" * 32 + "\n"  # 513 bytes
        large_body = chunk * 10  # 5130 bytes (> 4096 HDOC_PIPE_MAX)

        script = f"cat << 'EOF'\n{large_body}EOF\n"

        env = os.environ.copy()
        env["TMPDIR"] = tmpdir
        env["HELLISH_NO_BANNER"] = "1"
        env["HELLISH_NO_ANIM"] = "1"
        env["HELLISH_NO_UPDATE_CHECK"] = "1"

        res = subprocess.run(
            [SHELL, "-c", script],
            env=env,
            capture_output=True,
            text=True
        )

        check("large heredoc executes successfully", res.returncode == 0, f"exit code {res.returncode}")
        check("large heredoc output matches input", res.stdout == large_body, f"len got {len(res.stdout)} vs expected {len(large_body)}")

        # Verify canary was NOT clobbered
        with open(canary_path, "r") as f:
            canary_content = f.read()
        check("canary file not clobbered by heredoc", canary_content == "CANARY_UNTOUCHED\n", f"got {repr(canary_content)}")

    finally:
        import shutil
        shutil.rmtree(tmpdir, ignore_errors=True)

    if FAILS:
        print(f"\nFAILED: {len(FAILS)} checks")
        sys.exit(1)
    else:
        print("\nAll heredoc symlink tests passed.")
        sys.exit(0)


if __name__ == "__main__":
    main()
