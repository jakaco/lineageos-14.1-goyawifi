#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Replace init.framework.rc in a matching Android boot image and update its boot header hash."""
import argparse, gzip, hashlib, json, struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--boot',type=Path,required=True);p.add_argument('--init-rc',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
b=a.boot.read_bytes();assert b[:8]==b'ANDROID!'
k,ka,r,ra,ss,sa,tags,page=struct.unpack_from('<8I',b,8);off=page+((k+page-1)//page)*page
kernel=b[page:page+k];arc=gzip.decompress(b[off:off+r]);pos=0;records=[];found=False
while True:
 h=arc[pos:pos+110];assert h[:6]==b'070701'
 f=[int(h[6+i*8:14+i*8],16) for i in range(13)];pos+=110
 name=arc[pos:pos+f[11]];pos=(pos+f[11]+3)&~3
 data=arc[pos:pos+f[6]];pos=(pos+f[6]+3)&~3
 if name==b'init.framework.rc\0':data=a.init_rc.read_bytes();found=True
 f[6]=len(data);rec=b'070701'+b''.join(f'{v:08x}'.encode() for v in f)+name
 records.append(rec+bytes(-len(rec)%4)+data+bytes(-len(data)%4))
 if name==b'TRAILER!!!\0':break
assert found
ram=gzip.compress(b''.join(records),compresslevel=9,mtime=0);assert len(ram)<=r
ram=ram.ljust(r,b'\0');header=bytearray(b[:page]);header[576:608]=hashlib.sha1(kernel+struct.pack('<I',k)+ram+struct.pack('<II',r,0)).digest().ljust(32,b'\0')
out=bytes(header)+b[page:off]+ram+bytes(-len(ram)%page);assert len(out)==len(b)<16777216
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(out)
print(json.dumps({'bytes':len(out),'sha256':hashlib.sha256(out).hexdigest(),'kernel_sha256':hashlib.sha256(kernel).hexdigest(),'ramdisk_span':r},indent=2))
