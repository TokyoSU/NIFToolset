#!/usr/bin/env python3
"""Static consistency checks for the native C ABI and managed ownership layer."""
from __future__ import annotations

import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
NATIVE = ROOT / "NIFToolset.Native"
MANAGED = ROOT / "NIFToolset.Managed"


def fail(message: str) -> None:
    print(f"ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def read_all(paths: list[Path]) -> str:
    return "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in paths)


headers = sorted(NATIVE.glob("*.h"))
sources = sorted(NATIVE.glob("*.cpp"))
header_text = read_all(headers)
source_text = read_all(sources)
managed_text = read_all(sorted(MANAGED.glob("*.cs")))

export_pattern = re.compile(
    r"NIFTOOLSET_NATIVE_ENTRY\s+.+?\s+(NIF_[A-Za-z0-9_]+)\s*\([^;]*?\)\s*;",
    re.DOTALL,
)
exports = set(export_pattern.findall(header_text))
if not exports:
    fail("No native exports were found")

missing_definitions = sorted(
    name for name in exports if not re.search(rf"\b{re.escape(name)}\s*\(", source_text)
)
if missing_definitions:
    fail("Exports without definitions: " + ", ".join(missing_definitions))

imports = set(re.findall(r"extern\s+[^;]*?\b(NIF_[A-Za-z0-9_]+)\s*\(", managed_text))
missing_imports = sorted(imports - exports)
if missing_imports:
    fail("Managed imports absent from native headers: " + ", ".join(missing_imports))

unbound_exports = sorted(exports - imports)
if unbound_exports:
    fail("Native exports missing from the managed project: " + ", ".join(unbound_exports))

if "public static unsafe partial class NativeMethods" not in managed_text:
    fail("The complete low-level NativeMethods API is not public")

public_imports = set(re.findall(r"public\s+static\s+extern\s+[^;]*?\b(NIF_[A-Za-z0-9_]+)\s*\(", managed_text))
expected_internal = set(re.findall(r"TryRelease\(NativeMethods\.(NIF_[A-Za-z0-9_]+)\)", managed_text))
non_public_exports = exports - public_imports
if non_public_exports != expected_internal:
    unexpected = sorted(non_public_exports - expected_internal)
    missing_internal = sorted(expected_internal - non_public_exports)
    details = []
    if unexpected:
        details.append("unexpected non-public exports: " + ", ".join(unexpected))
    if missing_internal:
        details.append("release methods unexpectedly public: " + ", ".join(missing_internal))
    fail("Managed visibility mismatch: " + "; ".join(details))

opaque_handles = set(
    re.findall(r"typedef\s+struct\s+NIF_[A-Za-z0-9_]+Handle_t\s*\*\s*(NIF_[A-Za-z0-9_]+Handle)\s*;", header_text)
)
if "typedef void* NIF_" in header_text:
    fail("A native handle is still declared as an untyped void pointer")

safe_handles = set(re.findall(r"public\s+sealed\s+class\s+(Safe[A-Za-z0-9_]+Handle)\s*:", managed_text))
if len(opaque_handles) != len(safe_handles):
    fail(f"Opaque handle/SafeHandle count differs: {len(opaque_handles)} native, {len(safe_handles)} managed")

release_methods = re.findall(r"TryRelease\(NativeMethods\.(NIF_[A-Za-z0-9_]+)\)", managed_text)
if len(release_methods) != len(safe_handles):
    fail(f"Expected one release method per SafeHandle, got {len(release_methods)} for {len(safe_handles)}")
missing_releases = sorted(set(release_methods) - exports)
if missing_releases:
    fail("SafeHandle release functions absent from native headers: " + ", ".join(missing_releases))

for path in [*headers, *sources, *sorted(MANAGED.glob("*.cs")), *sorted(ROOT.glob("*.md"))]:
    for line_number, raw_line in enumerate(path.read_bytes().splitlines(), start=1):
        line = raw_line[:-1] if raw_line.endswith(b"\r") else raw_line
        if line.endswith((b" ", b"\t")):
            fail(f"Trailing whitespace in {path.relative_to(ROOT)}:{line_number}")

# Public headers are intentionally valid C as well as C++.
for compiler, suffix, standard in (("cc", ".c", "c11"), ("c++", ".cpp", "c++20")):
    executable = shutil.which(compiler)
    if not executable:
        continue
    with tempfile.TemporaryDirectory() as temporary_directory:
        source = Path(temporary_directory) / f"headers{suffix}"
        source.write_text('#include "NIFToolset.Native.h"\nint main(void) { return 0; }\n', encoding="utf-8")
        result = subprocess.run(
            [executable, f"-std={standard}", "-Wall", "-Wextra", "-Werror", "-fsyntax-only", "-I", str(NATIVE), str(source)],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            fail(f"Public header compilation with {compiler} failed:\n{result.stderr}")

object_model_check = subprocess.run(
    [sys.executable, str(MANAGED / "verify_object_model.py")],
    capture_output=True,
    text=True,
)
if object_model_check.returncode != 0:
    fail("Managed object model verification failed:\n" + object_model_check.stdout + object_model_check.stderr)

print(f"OK: {len(exports)} native exports, {len(imports)} managed imports, {len(opaque_handles)} owned handle types")
print(object_model_check.stdout.strip())
