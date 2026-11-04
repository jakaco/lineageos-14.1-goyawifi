# SPDX-License-Identifier: Apache-2.0
"""Assemble the full source-built tablet system with the verified SM-T110 hardware payload."""
from pathlib import Path
import gzip,hashlib,json,os,re,shutil,struct,subprocess
R=Path(os.environ.get('ANDROID7_BUILD_ROOT','/mnt/android7-jbuild'));W=Path(os.environ.get('GOYAWIFI_WORK_DIR','/path/to/local-work-overlays'));
assert W.is_dir(), 'Set GOYAWIFI_WORK_DIR to the locally prepared hardware overlays'P=R/'src/out/target/product/goyawifi';A=P/'goya-full-assembly';S=A/'system';O=W/'full-lineage';O.mkdir(exist_ok=True)
assert (P/'system.img').is_file(),'Full systemimage has not finished'
assert not S.exists(),'Use a fresh assembly directory; preserve the existing candidate'
A.mkdir(exist_ok=True)
shutil.copytree(P/'system',S,symlinks=True)
F=W/'framework-runtime/system';overlay={}
def place(src,rel):
 dst=S/rel;dst.parent.mkdir(parents=True,exist_ok=True)
 if dst.is_symlink():dst.unlink()
 shutil.copy2(src,dst);os.chmod(dst,0o755 if rel.startswith(('bin/','xbin/')) else 0o644)
 overlay[rel]=hashlib.sha256(dst.read_bytes()).hexdigest()
for x in json.loads((W/'graphics-vendor-runtime-manifest.json').read_text()):
 rel=x['path'].lstrip('/');place(F/rel,rel)
for rel in ('lib/libshim_goyawifi_binder.so','bin/goyawifi_pid_probe','usr/keylayout/gpio-keys.kl','usr/idc/sec_touchscreen.idc'):
 if (F/rel).is_file():place(F/rel,rel)
place(W/'framework-runtime/tmp/galcore.ko','lib/modules/galcore.ko')
place(W/'framework-runtime/sbin/healthd','bin/healthd')
# Keep only hardware-specific changes to the full platform libraries and apps.
for kind in ('wifi','audio','bluetooth','camera','hw-video','brightness','gps','sensors'):
 srcroot=W/(kind+'-runtime')/'system'
 for src in sorted(srcroot.rglob('*')):
  if not src.is_file():continue
  rel=src.relative_to(srcroot).as_posix()
  if rel.startswith(('app/LatinIME/','etc/security/cacerts/')):continue
  if kind=='camera' and rel in ('lib/libcameraservice.so','bin/cameraserver','lib/hw/camera.mrvl.so','app/GoyaCamera/GoyaCamera.apk') and (S/rel).is_file():continue
  if kind=='bluetooth' and rel in ('lib/libblas.so','lib/libclcore_neon.bc','lib/libclcore.bc','lib/libevent.so','lib/libchrome.so','lib/libprotobuf-cpp-full.so','lib/libRSCpuRef.so','lib/libRSDriver.so','framework/javax.obex.jar') and (S/rel).is_file():continue
  place(src,rel)
# Use the AOSP software codec registry on the downstream board.
md=R/'src/frameworks/av/media/libstagefright/data'
for p in sorted(md.glob('media_codecs_google_*.xml')):place(p,'etc/'+p.name)
if not (S/'etc/media_codecs.xml').exists():
 inc=[p.name for p in md.glob('media_codecs_google_*.xml') if p.name in ('media_codecs_google_audio.xml','media_codecs_google_video.xml')]
 assert len(inc)==2
 (S/'etc/media_codecs.xml').write_text('<?xml version="1.0" encoding="utf-8"?>\n<MediaCodecs>\n'+''.join(f'    <Include href="{n}"/>\n' for n in sorted(inc))+'</MediaCodecs>\n')
apps={p.name.lower() for p in S.rglob('*.apk')}
required={'mediaprovider.apk','downloadprovider.apk','documentsui.apk','externalstorageprovider.apk','gallery2.apk','jelly.apk','webview.apk','latinime.apk','exactcalculator.apk','deskclock.apk','trebuchet.apk','settings.apk','systemui.apk','goyacamera.apk','lineagesetupwizard.apk','updater.apk','eleven.apk','recorder.apk','audiofx.apk'}
assert not (required-apps),sorted(required-apps)
# Pack the known-good boot template with the full platform properties and services.
b=(W/'full-lineage-rollback/boot-internal.img').read_bytes();assert b[:8]==b'ANDROID!'
k,ka,r,ra,ss,sa,tags,page=struct.unpack_from('<8I',b,8);off=page+((k+page-1)//page)*page
z=b[page:page+k];assert hashlib.sha256(z).hexdigest()=='233e4f8e224cfa776d38c72fc82d1538353f14319c2acc5dfa04b9638576f502'
arc=gzip.decompress(b[off:off+r]);q=0;entries={}
while True:
 assert arc[q:q+6]==b'070701'
 f=[int(arc[q+6+i*8:q+14+i*8],16) for i in range(13)];q+=110;n=arc[q:q+f[11]-1].decode();q=(q+f[11]+3)&~3
 d=arc[q:q+f[6]];q=(q+f[6]+3)&~3
 if n=='TRAILER!!!':break
 entries[n]=(f[1],f[2],f[3],d)
def get(n):return entries[n][3].decode()
def settext(n,s):m,u,g,_=entries[n];entries[n]=(m,u,g,s.encode())
def props(s):return dict(l.split('=',1) for l in s.splitlines() if '=' in l and not l.lstrip().startswith('#'))
old=props(get('default.prop'));new=props((S/'build.prop').read_text());assert new.get('ro.cm.build.version')=='14.1',new.get('ro.cm.version')
old.update(new);old.update({'ro.product.model':'SM-T110','ro.product.device':'goyawifi','ro.cm.device':'goyawifi','ro.adb.secure':'0','ro.secure':'0','ro.debuggable':'1','ro.build.selinux':'0','ro.goya.full_lineage':'1','ro.goya.internal_install':'1','ro.goya.legacy_egl_queue':'true','ro.goya.no_gpu_color':'true','debug.goya.fb_retire':'true','debug.goya.hwc_full':'false','ro.bt.bdaddr_path':'/data/misc/bluedroid/factory-address','config.disable_bluetooth':'false'})
assert all(len(v.encode())<=91 for v in old.values()),[(n,len(v)) for n,v in old.items() if len(v.encode())>91]
settext('default.prop',''.join(f'{k}={v}\n' for k,v in old.items()))
rc=get('init.framework.rc')
sensor_nodes='''on boot
    chown system system /sys/class/input/input4/enable
    chmod 0660 /sys/class/input/input4/enable
    chown system system /sys/class/input/input4/poll_delay
    chmod 0660 /sys/class/input/input4/poll_delay
    write /sys/class/input/input4/poll_delay 66

'''
assert 'sys/class/input/input4/enable' not in rc
rc=sensor_nodes+rc
rc=rc.replace('service media /system/bin/mediaserver\n', 'service media /system/bin/mediaserver\n    setenv LD_PRELOAD /system/lib/libshim_goyawifi_binder.so\n', 1)
for name,user,groups in [('mediaextractor','mediaex','drmrpc mediadrm'),('mediacodec','mediacodec','camera drmrpc mediadrm'),('drmserver','drm','drmrpc mediadrm')]:
 assert (S/'bin'/name).is_file(),name
 assert not re.search(r'^service '+name+r'\b',rc,re.M)
 rc=rc.replace('    start media\n',f'    start {name}\n    start media\n',1)
 rc+=f'\nservice {name} /system/bin/{name}\n    class main\n    user {user}\n    group {groups}\n    disabled\n'
# These paths are created by stock post-fs-data on a normal LineageOS boot.
setup=get('sbin/internal-setup.sh');pre='''
for attempt in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
    [ ! -c /dev/ttyS2 ] || break
    /sbin/busybox sleep 1
done
if [ -c /dev/ttyS2 ] && [ -e /proc/driver/sirf ]; then
    if $T chown 1000:1000 /dev/ttyS2 /proc/driver/sirf &&
       $T chmod 0660 /dev/ttyS2 /proc/driver/sirf; then
        echo GOYA_GPS_READY
    else
        echo GOYA_GPS_PERMISSION_FAILED
    fi
else
    echo GOYA_GPS_NODE_MISSING
fi
$T mkdir -p /data/system/gps/mrvlgps
$T chown 1000:1000 /data/system/gps /data/system/gps/mrvlgps
$T chmod 0770 /data/system/gps /data/system/gps/mrvlgps
for attempt in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
    [ ! -e /sys/class/backlight/panel/brightness ] || break
    /sbin/busybox sleep 1
done
if [ -e /sys/class/backlight/panel/brightness ]; then
    $T chown 1000:1000 /sys/class/backlight/panel/brightness
    $T chmod 0660 /sys/class/backlight/panel/brightness
    echo GOYA_BACKLIGHT_READY
else
    echo GOYA_BACKLIGHT_MISSING
fi
for d in /data/drm /data/misc/media /data/misc/mediadrm; do
    $T mkdir -p "$d"
done
$T chown 1019:1019 /data/drm
$T chmod 770 /data/drm
$T chown 1013:1013 /data/misc/media
$T chmod 770 /data/misc/media
$T chown 1031:1031 /data/misc/mediadrm
$T chmod 770 /data/misc/mediadrm
'''
setup=setup.replace('echo INTERNAL_SETUP_READY',pre+'echo INTERNAL_SETUP_READY',1)
settext('sbin/internal-setup.sh',setup);settext('init.framework.rc',rc)
for cp in ('BOOTCLASSPATH','SYSTEMSERVERCLASSPATH'):
 for j in re.search(r'^    export '+cp+r' (.*)$',rc,re.M)[1].split(':'):assert (S/j.removeprefix('/system/')).is_file(),j
for p in re.findall(r'^service \S+ (/system/\S+)',rc,re.M):assert (S/p.removeprefix('/system/')).is_file(),p
cpio=bytearray()
for i,(n,(m,u,g,d)) in enumerate(list(entries.items())+[('TRAILER!!!',(0,0,0,b''))],1):
 name=n.encode()+b'\0';f=[i,m,u,g,2 if m&0o170000==0o040000 else 1,0,len(d),0,0,0,0,len(name),0]
 cpio+=b'070701'+''.join(f'{v:08x}' for v in f).encode()+name;cpio+=bytes(-len(cpio)%4);cpio+=d;cpio+=bytes(-len(cpio)%4)
cpio+=bytes(-len(cpio)%512);rd=gzip.compress(cpio,compresslevel=9,mtime=0);assert len(rd)<=r,(len(rd),r);rd=rd.ljust(r,b'\0')
header=bytearray(b[:page]);header[576:608]=hashlib.sha1(z+struct.pack('<I',k)+rd+struct.pack('<I',r)+struct.pack('<I',0)).digest().ljust(32,b'\0')
boot=bytes(header)+z+bytes(-len(z)%page)+rd+bytes(-len(rd)%page)
assert len(boot)<16777216
(O/'boot-lineage.img').write_bytes(boot);(O/'kernel-partition.img').write_bytes(boot.ljust(16777216,b'\0'));(O/'init.framework.rc').write_text(rc);(O/'internal-setup.sh').write_text(setup)
# Audit runtime ELF architecture and dependency names without mutating the source tree.
elfs=[];provided={p.name for p in S.rglob('*') if p.is_file()};missing=[]
for p in sorted(S.rglob('*')):
 if not p.is_file() or p.is_symlink():continue
 with p.open('rb')as f:h=f.read(20)
 if h[:4]!=b'\x7fELF':continue
 assert h[4]==1 and struct.unpack_from('<H',h,18)[0]==40,p
 needed=re.findall(r'\(NEEDED\).*?\[(.*?)\]',subprocess.check_output(['readelf','-dW',str(p)],text=True));elfs.append(p.relative_to(S).as_posix())
 missing.extend((elfs[-1],n) for n in needed if Path(n).name not in provided)
assert not missing,missing
report=dict(lineage_version=new['ro.cm.version'],kernel_sha256=hashlib.sha256(z).hexdigest(),boot_sha256=hashlib.sha256(boot).hexdigest(),kernel_partition_sha256=hashlib.sha256((O/'kernel-partition.img').read_bytes()).hexdigest(),ramdisk_span=r,apps=sorted(apps),arm_elfs=len(elfs),device_overlay=overlay,system_files=sum(p.is_file() for p in S.rglob('*')))
(O/'assembly.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ('apps','device_overlay')},indent=2))
