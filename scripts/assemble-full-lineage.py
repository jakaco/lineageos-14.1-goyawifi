# SPDX-License-Identifier: Apache-2.0
"""Assemble the full source-built tablet system with the verified SM-T110 hardware payload."""
from pathlib import Path
import gzip,hashlib,json,os,re,shutil,struct,subprocess
R=Path(os.environ.get('ANDROID7_BUILD_ROOT','/mnt/android7-jbuild'));W=Path(os.environ.get('GOYAWIFI_WORK_DIR','/path/to/local-work-overlays'));
assert W.is_dir(), 'Set GOYAWIFI_WORK_DIR to the locally prepared hardware overlays'
P=R/'src/out/target/product/goyawifi';A=P/'goya-full-assembly';S=A/'system';O=W/'full-lineage';O.mkdir(exist_ok=True)
assert (P/'system.img').is_file(),'Full systemimage has not finished'
assert not S.exists(),'Use a fresh assembly directory; preserve the existing candidate'
A.mkdir(exist_ok=True)
shutil.copytree(P/'system',S,symlinks=True)
# This is a Wi-Fi-only tablet. Keep Android's shared telephony framework
# libraries, but omit the phone UI/service packages that surface SIM alerts.
for app in ('Phone','TeleService'):
 for parent in ('app','priv-app'):
  shutil.rmtree(S/parent/app,ignore_errors=True)
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
# SystemServer only starts UsbService and AppWidgetService when these features are declared.
for n in ('android.hardware.usb.accessory.xml','android.software.app_widgets.xml'):place(R/'src/frameworks/native/data/etc'/n,'etc/permissions/'+n)
# Pack the known-good boot template with the full platform properties and services.
b=(W/'full-lineage-rollback/boot-internal.img').read_bytes();assert b[:8]==b'ANDROID!'
k,ka,r,ra,ss,sa,tags,page=struct.unpack_from('<8I',b,8);off=page+((k+page-1)//page)*page
assert hashlib.sha256(b[page:page+k]).hexdigest()=='233e4f8e224cfa776d38c72fc82d1538353f14319c2acc5dfa04b9638576f502'
# r3 kernel: CONFIG_USB_ANDROID_SAMSUNG_MTP off, because Nougat's MtpServer needs the AOSP f_mtp ioctls and /dev/mtp_usb.
z=(R/'kernel-out/arch/arm/boot/zImage').read_bytes();assert hashlib.sha256(z).hexdigest()=='315313f4a0656aa5413638f5052c96ffe998d4634193590cc9f9416d31ef2f05'
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
old.update(new);old.update({'ro.product.model':'SM-T110','ro.product.device':'goyawifi','ro.cm.device':'goyawifi','ro.adb.secure':'1','ro.secure':'1','ro.debuggable':'0','ro.build.selinux':'0','ro.goya.full_lineage':'1','ro.goya.internal_install':'1','ro.goya.legacy_egl_queue':'true','ro.goya.no_gpu_color':'true','debug.goya.fb_retire':'true','debug.goya.hwc_full':'false','ro.bt.bdaddr_path':'/data/misc/bluedroid/factory-address','config.disable_bluetooth':'false','persist.sys.usb.config':'mtp,adb'})
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
U='/sys/class/android_usb/android0/'
usb_rc='''on property:goyawifi.framework.ready=1
    setprop sys.usb.config ${persist.sys.usb.config}

on property:sys.usb.config=none
    stop adbd
    write '''+U+'''enable 0
    setprop sys.usb.state ${sys.usb.config}

'''
# Stock T110 IDs: 6860 for MTP and 6866 for PTP. ADB-only keeps 6860, which this tablet already enumerates with.
for cfg,pid in (('adb','6860'),('mtp','6860'),('mtp,adb','6860'),('ptp','6866'),('ptp,adb','6866')):
 usb_rc+=''.join(l+'\n' for l in ('on property:sys.usb.config='+cfg,'    write '+U+'enable 0','    write '+U+'idVendor 04e8','    write '+U+'idProduct '+pid,'    write '+U+'functions ${sys.usb.config}','    write '+U+'enable 1')+(('    start adbd',) if 'adb' in cfg else ())+('    setprop sys.usb.state ${sys.usb.config}','')) 
assert 'sys.usb.config=' not in rc
rc=sensor_nodes+usb_rc+rc
ue=get('ueventd.rc');assert 'mtp_usb' not in ue
settext('ueventd.rc',ue.replace('/dev/android_adb 0660','/dev/mtp_usb 0660 root mtp\n/dev/usb_accessory 0660 root usb\n/dev/android_adb 0660',1))
ir=get('init.rc');assert 'iProduct goyawifi-Nougat-init' in ir
settext('init.rc',ir.replace('iProduct goyawifi-Nougat-init','iProduct SM-T110',1))
rc=rc.replace('service media /system/bin/mediaserver\n', 'service media /system/bin/mediaserver\n    setenv LD_PRELOAD /system/lib/libshim_goyawifi_binder.so\n', 1)
for name,user,groups in [('mediaextractor','mediaex','drmrpc mediadrm'),('mediacodec','mediacodec','camera drmrpc mediadrm'),('drmserver','drm','drmrpc mediadrm')]:
 assert (S/'bin'/name).is_file(),name
 assert not re.search(r'^service '+name+r'\b',rc,re.M)
 rc=rc.replace('    start media\n',f'    start {name}\n    start media\n',1)
 rc+=f'\nservice {name} /system/bin/{name}\n    class main\n    user {user}\n    group {groups}\n    disabled\n'
# These paths are created by stock post-fs-data on a normal LineageOS boot.
setup=get('sbin/internal-setup.sh')
# init applies persist.sys.usb.config once persistent properties are loaded.
assert '$T setprop sys.usb.config adb\n' in setup
setup=setup.replace('$T setprop sys.usb.config adb\n','',1)
pre='''
# libsuspend opens this as system; without it Android can never suspend.
$T chown 1000:1000 /sys/power/wakeup_count
$T chmod 660 /sys/power/wakeup_count
$T mkdir -p /data/misc/adb
$T chown 1000:2000 /data/misc/adb
$T chmod 2750 /data/misc/adb
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
header=bytearray(b[:page]);struct.pack_into('<I',header,8,len(z));header[576:608]=hashlib.sha1(z+struct.pack('<I',len(z))+rd+struct.pack('<I',r)+struct.pack('<I',0)).digest().ljust(32,b'\0')
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
