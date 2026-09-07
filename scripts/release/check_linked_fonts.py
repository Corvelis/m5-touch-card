"""Fail before publication if a GNU FreeFont bitmap is linked again."""
import argparse
from pathlib import Path
import re
import subprocess


def check_symbols(symbols, stackchan=False):
    if re.search(r"\b(?:FreeSans|FreeSerif|FreeMono)[A-Za-z0-9_]*", symbols):
        raise ValueError("GNU FreeFont-derived bitmap is linked; use Touch Digits instead")
    if stackchan:
        for name in ("tc::stackDigits", "tc::stackClockDigits", "fonts::efontJA_16"):
            if name not in symbols:
                raise ValueError(f"Expected font missing: {name}")


def check_elf(nm, elf, stackchan=False):
    symbols = subprocess.check_output([str(nm), "-C", "--defined-only", str(elf)], text=True)
    check_symbols(symbols, stackchan)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--nm", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--stackchan", action="store_true")
    args = parser.parse_args()
    check_elf(args.nm, args.elf, args.stackchan)
    print("Linked-font check passed")
