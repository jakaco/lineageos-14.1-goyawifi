#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Write release name and image hashes into the TWRP installer files.

Usage: set-release.py RELEASE DATE SYSTEM_EXT4_GZ KERNEL_PARTITION_IMG
Example: set-release.py r5 2026-11-06 system.ext4.gz kernel-partition.img
"""
import gzip, hashlib, json, re, struct, sys
from pathlib import Path

release, date, system_gz, kernel = sys.argv[1], sys.argv[2], Path(sys.argv[3]), Path(sys.argv[4])
twrp = Path(__file__).resolve().parents[1] / 'packaging/twrp'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def sha_stream(f):
    h = hashlib.sha256()
    while chunk := f.read(1 << 20):
        h.update(chunk)
    return h.hexdigest()


with system_gz.open('rb') as f:
    gz_sha = sha_stream(f)
with gzip.open(system_gz, 'rb') as f:
    raw_sha = sha_stream(f)
k = kernel.read_bytes()
assert len(k) == 16777216 and k[:8] == b'ANDROID!', 'not a kernel partition image'
zsize, page = struct.unpack_from('<I', k, 8)[0], struct.unpack_from('<I', k, 36)[0]

meta = {
    'name': 'LineageOS 14.1 for Galaxy Tab 3 Lite 7.0 (SM-T110)',
    'release': release,
    'date': date,
    'device': 'goyawifi',
    'android': '7.1.2',
    'kernel': '3.4.113',
    'webview': 'Bromite SystemWebView 81.0.4044.76',
    'system_ext4_sha256': raw_sha,
    'system_ext4_gzip_sha256': gz_sha,
    'kernel_partition_sha256': sha(k),
    'zimage_sha256': sha(k[page:page + zsize]),
    'partitions': {'system': 'mmcblk0p15', 'kernel': 'mmcblk0p10'},
}
(twrp / 'release-metadata.json').write_text(json.dumps(meta, indent=2) + '\n', newline='\n')

ub = twrp / 'META-INF/com/google/android/update-binary'
s = ub.read_text()
for key, value in (('SYSTEM_GZ_SHA', gz_sha), ('SYSTEM_RAW_SHA', raw_sha), ('KERNEL_SHA', sha(k)), ('RELEASE', release)):
    s, n = re.subn(rf'^{key}=.*$', f'{key}={value}', s, flags=re.M)
    assert n == 1, key
ub.write_text(s, newline='\n')

(twrp / 'SHA256SUMS').write_text(
    f'# SPDX-License-Identifier: Apache-2.0\n{gz_sha}  system.ext4.gz\n{sha(k)}  kernel-partition.img\n', newline='\n')
print(json.dumps(meta, indent=2))
