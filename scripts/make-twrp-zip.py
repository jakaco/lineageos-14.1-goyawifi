#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Build a checksum-pinned TWRP ZIP from a system ext4 gzip and kernel partition image."""
import argparse, hashlib, json, stat, zipfile
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
meta = json.loads((repo / "packaging/twrp/release-metadata.json").read_text())
parser = argparse.ArgumentParser()
parser.add_argument("--system-gz", type=Path, required=True)
parser.add_argument("--kernel", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
a = parser.parse_args()
def sha(p):
    h = hashlib.sha256()
    with p.open("rb") as f:
        for b in iter(lambda:f.read(1024*1024), b""): h.update(b)
    return h.hexdigest()
assert sha(a.system_gz) == meta["system_ext4_gzip_sha256"]
assert sha(a.kernel) == meta["kernel_partition_sha256"]
entries = [
    ("system.ext4.gz", a.system_gz, zipfile.ZIP_STORED, 0o100644),
    ("kernel-partition.img", a.kernel, zipfile.ZIP_DEFLATED, 0o100644),
    ("META-INF/com/google/android/update-binary", repo/"packaging/twrp/META-INF/com/google/android/update-binary", zipfile.ZIP_DEFLATED, 0o100755),
    ("META-INF/com/google/android/updater-script", repo/"packaging/twrp/META-INF/com/google/android/updater-script", zipfile.ZIP_DEFLATED, 0o100644),
    ("README.md", repo/"packaging/twrp/README.md", zipfile.ZIP_DEFLATED, 0o100644),
    ("release-metadata.json", repo/"packaging/twrp/release-metadata.json", zipfile.ZIP_DEFLATED, 0o100644),
    ("SHA256SUMS", repo/"packaging/twrp/SHA256SUMS", zipfile.ZIP_DEFLATED, 0o100644),
]
a.output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(a.output, "w", allowZip64=True) as z:
    for name, path, method, mode in entries:
        info=zipfile.ZipInfo(name, (2026,11,4,0,0,0)); info.compress_type=method
        info.external_attr=(stat.S_IFREG|mode)<<16
        z.writestr(info,path.read_bytes())
with zipfile.ZipFile(a.output) as z:
    assert z.testzip() is None
    assert z.read("META-INF/com/google/android/update-binary").startswith(b"#!/sbin/sh")
    assert z.getinfo("META-INF/com/google/android/update-binary").external_attr >> 16 & 0o111
print(json.dumps({"zip":str(a.output),"bytes":a.output.stat().st_size,"sha256":sha(a.output)},indent=2))
