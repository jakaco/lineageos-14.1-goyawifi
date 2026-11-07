# SPDX-License-Identifier: Apache-2.0
"""Assemble the full source-built tablet system with the verified SM-T110 hardware payload."""
from pathlib import Path
import gzip,hashlib,json,os,re,shutil,struct,subprocess
NL=chr(10)
R=Path(os.environ.get('ANDROID7_BUILD_ROOT','/mnt/android7-jbuild'));W=Path(os.environ.get('GOYAWIFI_WORK_DIR','/path/to/local-work-overlays'));
assert W.is_dir(), 'Set GOYAWIFI_WORK_DIR to the locally prepared hardware overlays'
P=R/'src/out/target/product/goyawifi';A=P/'goya-full-assembly';S=A/'system';O=W/'full-lineage';O.mkdir(exist_ok=True)
assert (P/'system.img').is_file(),'Full systemimage has not finished'
assert not S.exists(),'Use a fresh assembly directory; preserve the existing candidate'
A.mkdir(exist_ok=True)
shutil.copytree(P/'system',S,symlinks=True)
# framework-res: config_animateScreenLights=true (r5, plain fade at screen off) and the
# SM-T110 power profile (r6, Settings > Battery usage).
assert hashlib.sha256((S/'framework/framework-res.apk').read_bytes()).hexdigest().startswith('d10bcaacbcb2c237')
# This is a Wi-Fi-only tablet. Keep Android's shared telephony framework
# libraries, but omit the phone UI/service packages that surface SIM alerts.
for app in ('Phone','TeleService'):
 for parent in ('app','priv-app'):
  shutil.rmtree(S/parent/app,ignore_errors=True)
# r4: no update server exists for this port, and Email/Exchange only log errors on this tablet.
for rel in ('priv-app/Updater','app/Email','app/Exchange2'):
 assert (S/rel).is_dir(),rel;shutil.rmtree(S/rel)
F=W/'framework-runtime/system';overlay={}
def place(src,rel):
 dst=S/rel;dst.parent.mkdir(parents=True,exist_ok=True)
 if dst.is_symlink():dst.unlink()
 shutil.copy2(src,dst);os.chmod(dst,0o755 if rel.startswith(('bin/','xbin/')) else 0o644)
 overlay[rel]=hashlib.sha256(dst.read_bytes()).hexdigest()
for x in json.loads((W/'graphics-vendor-runtime-manifest.json').read_text()):
 rel=x['path'].lstrip('/');place(F/rel,rel)
for rel in ('lib/libshim_goyawifi_binder.so','usr/keylayout/gpio-keys.kl','usr/idc/sec_touchscreen.idc'):
 if (F/rel).is_file():place(F/rel,rel)
place(W/'framework-runtime/tmp/galcore.ko','lib/modules/galcore.ko')
place(W/'framework-runtime/sbin/healthd','bin/healthd')
# r6: core feature list (tablet_core_hardware without compass/managed users). Without it
# there is no backup service and the Play Store hides apps that need a touchscreen.
place(Path(__file__).resolve().parents[1]/'device/samsung/goyawifi/configs/permissions/goyawifi-core-hardware.xml','etc/permissions/goyawifi-core-hardware.xml')
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
required={'mediaprovider.apk','downloadprovider.apk','documentsui.apk','externalstorageprovider.apk','gallery2.apk','jelly.apk','webview.apk','latinime.apk','exactcalculator.apk','deskclock.apk','trebuchet.apk','settings.apk','systemui.apk','goyacamera.apk','lineagesetupwizard.apk','eleven.apk','recorder.apk','audiofx.apk'}
assert not (required-apps),sorted(required-apps)
# SystemServer only starts UsbService and AppWidgetService when these features are declared.
for n in ('android.hardware.usb.accessory.xml','android.software.app_widgets.xml'):place(R/'src/frameworks/native/data/etc'/n,'etc/permissions/'+n)
# r4: replace the 2017-era root store with a current Mozilla one.
shutil.rmtree(S/'etc/security/cacerts')
for c in sorted((W/'cacerts-r4').iterdir()):place(c,'etc/security/cacerts/'+c.name)
# r4: Bromite 81 SystemWebView replaces the 2016 Chromium 52 build. It keeps the
# com.android.webview package name, and system apps need their native libraries unpacked.
import zipfile
wv=W/'webview-bromite-81/arm_SystemWebView.apk';assert hashlib.sha256(wv.read_bytes()).hexdigest()=='306e7c95ed348e13deb3e247eca4fc9595a29b77452ee54996ab7d7dda25805f'
place(wv,'app/webview/webview.apk')
with zipfile.ZipFile(wv) as z:
 for i in z.infolist():
  if i.filename.startswith('lib/armeabi-v7a/') and i.filename.endswith('.so'):
   rel='app/webview/lib/arm/'+i.filename.rsplit('/',1)[1];(S/rel).parent.mkdir(parents=True,exist_ok=True);(S/rel).write_bytes(z.read(i));os.chmod(S/rel,0o644);overlay[rel]=hashlib.sha256((S/rel).read_bytes()).hexdigest()
# r4: Bluetooth setup that retries a bad saved address and falls back to a stable generated one.
place(Path(__file__).resolve().parents[1]/'device/samsung/goyawifi/configs/goya-bluetooth-setup.sh','etc/goya-bluetooth-setup.sh')
# Pack the known-good boot template with the full platform properties and services.
b=(W/'full-lineage-rollback/boot-internal.img').read_bytes();assert b[:8]==b'ANDROID!'
k,ka,r,ra,ss,sa,tags,page=struct.unpack_from('<8I',b,8);off=page+((k+page-1)//page)*page
assert hashlib.sha256(b[page:page+k]).hexdigest()=='233e4f8e224cfa776d38c72fc82d1538353f14319c2acc5dfa04b9638576f502'
# r5 kernel: Linux 3.4.113 tree from kernel/source.json (r3 MTP config kept), built into kernel-out-r5.
KO=R/'kernel-out-r5';z=(KO/'arch/arm/boot/zImage').read_bytes();assert hashlib.sha256(z).hexdigest()=='b46ee8ee9dc6e752917c47f2c75b902d810e67cb0cfff3490247ba1c8e834cba'
# Modules must match the r5 kernel's vermagic.
for m in ('drivers/marvell/graphics/galcore_src/galcore.ko','drivers/marvell/sd8787/wlan_src/mlan.ko','drivers/marvell/sd8787/wlan_src/sd8xxx.ko','drivers/marvell/sd8787/mbtc_src/mbt8xxx.ko','net/netfilter/xt_mark.ko','net/netfilter/xt_connmark.ko','net/netfilter/xt_IDLETIMER.ko','net/netfilter/xt_conntrack.ko'):
 assert (S/'lib/modules'/Path(m).name).is_file(),m;place(KO/m,'lib/modules/'+Path(m).name)
arc=gzip.decompress(b[off:off+r]);q=0;entries={}
while True:
 assert arc[q:q+6]==b'070701'
 f=[int(arc[q+6+i*8:q+14+i*8],16) for i in range(13)];q+=110;n=arc[q:q+f[11]-1].decode();q=(q+f[11]+3)&~3
 d=arc[q:q+f[6]];q=(q+f[6]+3)&~3
 if n=='TRAILER!!!':break
 entries[n]=(f[1],f[2],f[3],d)
# init: same source and product as the template's, plus shell access to sys.powerctl so adb reboot works on user builds
# (r3), and services with an SELinux label start with SELinux disabled (r6, needed for Magisk).
ini=(R/'src/out/target/product/goyawifi-init/root/init').read_bytes();assert hashlib.sha256(ini).hexdigest()=='015ad0db7c48e1583cc03ebe35ad7c70d3ec178436a237526f9d41d2092b670d'
assert hashlib.sha256(entries['init'][3]).hexdigest()=='ad0633c9e77e23483ce6db96b4b28eb132dff5d9cd82d17cba1ea20d8887248d'
entries['init']=entries['init'][:3]+(ini,)
def get(n):return entries[n][3].decode()
def settext(n,s):m,u,g,_=entries[n];entries[n]=(m,u,g,s.encode())
def props(s):return dict(l.split('=',1) for l in s.splitlines() if '=' in l and not l.lstrip().startswith('#'))
old=props(get('default.prop'));new=props((S/'build.prop').read_text());assert new.get('ro.cm.build.version')=='14.1',new.get('ro.cm.version')
old.update(new);old.update({'ro.product.model':'SM-T110','ro.product.device':'goyawifi','ro.cm.device':'goyawifi','ro.adb.secure':'1','ro.secure':'1','ro.debuggable':'1','ro.build.selinux':'0','ro.goya.full_lineage':'1','ro.goya.internal_install':'1','ro.goya.legacy_egl_queue':'true','ro.goya.no_gpu_color':'true','debug.goya.fb_retire':'true','debug.goya.hwc_full':'false','ro.bt.bdaddr_path':'/data/misc/bluedroid/factory-address','config.disable_bluetooth':'false','persist.sys.usb.config':'mtp,adb','pm.dexopt.install':'speed','pm.dexopt.bg-dexopt':'speed','ro.radio.noril':'true'})
assert all(len(v.encode())<=91 for v in old.values()),[(n,len(v)) for n,v in old.items() if len(v.encode())>91]
settext('default.prop',''.join(f'{k}={v}\n' for k,v in old.items()))
rc=get('init.framework.rc')
sensor_nodes='''on boot
    # AOSP init.rc sets this; without it fork fails for any process with a large private mapping.
    write /proc/sys/vm/overcommit_memory 1
    # The kernel default pins the CPU at its top frequency.
    write /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor interactive
    # Other AOSP init.rc values.
    write /proc/sys/kernel/randomize_va_space 2
    write /proc/sys/vm/mmap_min_addr 32768
    write /proc/sys/net/unix/max_dgram_qlen 600
    write /proc/sys/kernel/sched_tunable_scaling 0
    write /proc/sys/kernel/sched_latency_ns 10000000
    write /proc/sys/kernel/hung_task_timeout_secs 0
    write /proc/sys/vm/min_free_order_shift 4
    write /proc/sys/vm/dirty_expire_centisecs 200
    write /proc/sys/vm/dirty_background_ratio 5
    write /proc/sys/kernel/sysrq 0
    write /proc/sys/kernel/kptr_restrict 2
    write /proc/sys/net/ipv4/conf/all/accept_redirects 0
    write /proc/sys/net/ipv6/conf/all/accept_redirects 0
    write /proc/sys/net/ipv4/ping_group_range "0 2147483647"
    chown system system /sys/class/input/input4/enable
    chmod 0660 /sys/class/input/input4/enable
    chown system system /sys/class/input/input4/poll_delay
    chmod 0660 /sys/class/input/input4/poll_delay
    write /sys/class/input/input4/poll_delay 66

'''
assert 'sys/class/input/input4/enable' not in rc
U='/sys/class/android_usb/android0/'
usb_rc='''on property:goyawifi.framework.ready=1
    # The 156 MHz floor drops the GPU mid-animation and caused most launcher jank.
    write /sys/devices/platform/galcore/gpu/gpu0/gpufreq/scaling_min_freq 312000
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
# r4: mount microSD through vold. The encryptable flag marks it adoptable, which makes it visible to apps.
sd_entry='/devices/platform/sdhci-pxav3.0/mmc_host* auto auto defaults voldmanaged=sdcard1:auto,encryptable=userdata\n'
for n in ('fstab.pxa988','fstab.unknown'):
 f=get(n);assert 'voldmanaged' not in f;settext(n,f.rstrip()+'\n'+sd_entry)
# r6: /dev/fuse 0666 as in AOSP ueventd.rc; MTP's document provider mounts appfuse through it.
settext('ueventd.rc',ue.replace('/dev/android_adb 0660','/dev/mtp_usb 0660 root mtp\n/dev/usb_accessory 0660 root usb\n/dev/fuse 0666 root root\n/dev/android_adb 0660',1))
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
# r5: RLIMIT_NICE is set by init itself (setrlimit below), as in AOSP init.rc.
assert setup.count('/system/bin/goyawifi_pid_probe 1\n')==1
setup=setup.replace('/system/bin/goyawifi_pid_probe 1\n','',1)
# r4: run healthd's charger mode when the bootloader reports a plug-in while powered off.
T_LINE='T=/system/bin/toybox\n';assert setup.count(T_LINE)==1
setup=setup.replace(T_LINE,T_LINE+'if /sbin/busybox grep -q lpcharge=1 /proc/cmdline; then\n    echo OFF_MODE_CHARGING\n    exec /system/bin/healthd -c\nfi\n',1)
# r6: 256 MB of compressed swap in RAM (the kernel is built without Samsung's hooks that kept
# reclaim from ever swapping), and the deadline I/O scheduler so UI reads don't queue behind
# background apps. Without swap, memory pressure at boot evicted app code over and over.
# r6: MTP's document provider mounts appfuse here; without it, opening the microSD card in
# Settings > Storage crashed. Created after the /mnt tmpfs is mounted, as AOSP init.rc does.
MNT_LINE='$T mount -t tmpfs -o mode=0755,uid=0,gid=1000 tmpfs /storage\n';assert setup.count(MNT_LINE)==1
setup=setup.replace(MNT_LINE,MNT_LINE+'$T mkdir -p /mnt/appfuse\n$T chmod 0711 /mnt/appfuse\n',1)
WIFI_LINE='if /system/bin/sh /system/etc/goya-wifi-setup.sh; then\n';assert setup.count(WIFI_LINE)==1
setup=setup.replace(WIFI_LINE,'''echo deadline > /sys/block/mmcblk0/queue/scheduler
if [ -b /dev/block/zram0 ]; then
    echo 268435456 > /sys/block/zram0/disksize
    /system/bin/mkswap /dev/block/zram0 >/dev/null && /system/bin/swapon /dev/block/zram0
    echo 100 > /proc/sys/vm/swappiness
fi
'''+WIFI_LINE,1)
for d in ('res','res/images','res/images/charger'):entries[d]=(0o40755,0,0,b'')
for n in ('battery_scale.png','battery_fail.png'):entries['res/images/charger/'+n]=(0o100644,0,0,(W/'charger-res'/n).read_bytes())
# init applies persist.sys.usb.config once persistent properties are loaded.
assert '$T setprop sys.usb.config adb\n' in setup
setup=setup.replace('$T setprop sys.usb.config adb\n','',1)
pre='''
# libsuspend opens this as system; without it Android can never suspend.
$T chown 1000:1000 /sys/power/wakeup_count
$T chmod 660 /sys/power/wakeup_count
# sdcard runs as media_rw and must be able to reach its vfat mount.
$T mkdir -p /mnt/media_rw
$T chown 0:1023 /mnt/media_rw
$T chmod 0750 /mnt/media_rw
# WebView shares its native library's RELRO section from here; without it every app maps a private copy.
$T mkdir -p /data/misc/shared_relro
$T chown 1037:1037 /data/misc/shared_relro
$T chmod 771 /data/misc/shared_relro
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
old_hdr=("# RAM-only framework bring-up. Services stay disabled until files, permissions"+NL
         +"# and volatile data directories have been staged and checked through root ADB."+NL+"on init"+NL)
assert rc.count(old_hdr)==1
rc=rc.replace(old_hdr,"# Core services start once internal-setup.sh has prepared storage and devices."+NL+"on init"+NL
    +"    # RLIMIT_NICE 40, as in AOSP init.rc, so apps can raise thread priority."+NL+"    setrlimit 13 40 40"+NL
    # r6: the Marvell gralloc HAL needs the binder shim. Services get it through LD_PRELOAD;
    # this makes the linker add it for any process that loads gralloc (adb shell screencap crashed).
    +"    export LD_SHIM_LIBS /system/lib/hw/gralloc.mrvl.so|libshim_goyawifi_binder.so"+NL
    # r6: debugfs, as vendor init scripts mount it, and the AOSP /d link. Battery stats read
    # kernel wakelocks from /d/wakeup_sources, and tracing writes to its trace_marker.
    +"    mount debugfs debugfs /sys/kernel/debug"+NL
    +"    chmod 0755 /sys/kernel/debug"+NL
    +"    symlink /sys/kernel/debug /d"+NL
    # r6: per-app process groups, as in AOSP init.rc, so Android can kill an app together
    # with every process it started. The kernel has cgroups for cpuacct only: Android 7.1
    # no longer uses the cpu cgroup, and cpusets lose CPUs on hotplug in this kernel.
    +"    mkdir /acct"+NL
    +"    mount cgroup none /acct cpuacct"+NL
    +"    mkdir /acct/uid"+NL,1)
# LineageOS root (addonsu): this init does not read /system/etc/init, so the su
# daemon from superuser.rc lives here. It runs only when /system/xbin/su exists and
# Root access is set to Apps or Apps and ADB in Developer options.
su_rc=NL.join(['','service su_daemon /system/xbin/su --daemon','    user root','    group root','    disabled','',
    'on property:persist.sys.root_access=0','    stop su_daemon','',
    'on property:persist.sys.root_access=2','    stop su_daemon','',
    'on property:persist.sys.root_access=1','    start su_daemon','',
    'on property:persist.sys.root_access=3','    start su_daemon',''])
assert 'su_daemon' not in rc
rc=rc+su_rc
# r6: fire the standard boot triggers at the matching points of this boot sequence, so
# rc hooks that rely on them (Magisk's post-fs-data and late-start steps) run.
a=NL.join(['on property:goyawifi.runtime.ready=1','    load_persist_props',''])
assert rc.count(a)==1;rc=rc.replace(a,NL.join(['on property:goyawifi.runtime.ready=1','    trigger post-fs-data','    load_persist_props','']),1)
a=NL.join(['on property:goyawifi.framework.ready=1','    start bootanim',''])
assert rc.count(a)==1;rc=rc.replace(a,NL.join(['on property:goyawifi.framework.ready=1','    trigger nonencrypted','    start bootanim','']),1)
settext('sbin/internal-setup.sh',setup);settext('init.framework.rc',rc)
# r5: the bring-up first-boot script marked setup complete, set a 10-minute screen timeout and
# forced the launcher, which skipped or broke the setup wizard on a new install. Keep only the
# one-time precompile of the UI apps.
fb=get('sbin/internal-firstboot.sh')
for line in ('/system/bin/settings put global device_provisioned 1','/system/bin/settings put secure user_setup_complete 1',
             '/system/bin/settings put system screen_off_timeout 600000','/system/bin/wm dismiss-keyguard',
             '/system/bin/am start -a android.intent.action.MAIN -c android.intent.category.HOME'):
 assert fb.count(line+NL)==1,line;fb=fb.replace(line+NL,'',1)
assert 'settings put' not in fb and 'cmd package compile -m speed' in fb
# r6: disable Settings screens that need a modem; they crash on this Wi-Fi-only tablet.
extra=NL.join([
 'for c in RadioInfo BandMode CarrierSelection DateTimeSettingsSetupWizard Settings\\$ApnSettingsActivity \\',
 '         Settings\\$ApnEditorActivity Settings\\$IccLockSettingsActivity Settings\\$ImeiInformationActivity sim.SimDialogActivity; do',
 '    /system/bin/pm disable "com.android.settings/.$c" >/dev/null 2>&1',
 'done',''])
mark='/system/bin/toybox touch /data/system/goya-internal-provisioned'
assert fb.count(mark)==1
fb=fb.replace(mark,extra+mark,1)
settext('sbin/internal-firstboot.sh',fb)
for cp in ('BOOTCLASSPATH','SYSTEMSERVERCLASSPATH'):
 for j in re.search(r'^    export '+cp+r' (.*)$',rc,re.M)[1].split(':'):assert (S/j.removeprefix('/system/')).is_file(),j
for p in re.findall(r'^service \S+ (/system/\S+)',rc,re.M):assert p=='/system/xbin/su' or (S/p.removeprefix('/system/')).is_file(),p
# r6: ship the built SELinux policy as /sepolicy. SELinux stays disabled (init never loads
# it), but Magisk's magiskinit patches /sepolicy at boot and crashes if it is missing.
sp=(P/'root/sepolicy').read_bytes();assert hashlib.sha256(sp).hexdigest()=='efaa3d76dbb2835a6195eac2ddca15ad82100e7ce355ee55ca440ce1c9efe152'
assert 'sepolicy' not in entries
entries['sepolicy']=(0o100644,0,0,sp)
cpio=bytearray()
for i,(n,(m,u,g,d)) in enumerate(list(entries.items())+[('TRAILER!!!',(0,0,0,b''))],1):
 name=n.encode()+b'\0';f=[i,m,u,g,2 if m&0o170000==0o040000 else 1,0,len(d),0,0,0,0,len(name),0]
 cpio+=b'070701'+''.join(f'{v:08x}' for v in f).encode()+name;cpio+=bytes(-len(cpio)%4);cpio+=d;cpio+=bytes(-len(cpio)%4)
# r6 kernel boots any ramdisk size, so the header carries the real size (standard boot
# image; Magisk and other tools can unpack it). r5 and older padded it to a fixed span.
cpio+=bytes(-len(cpio)%512);rd=gzip.compress(cpio,compresslevel=9,mtime=0);r=len(rd)
header=bytearray(b[:page]);struct.pack_into('<I',header,8,len(z));struct.pack_into('<I',header,16,r);header[48:64]=b'goyawifi'.ljust(16,bytes(1));header[576:608]=hashlib.sha1(z+struct.pack('<I',len(z))+rd+struct.pack('<I',r)+struct.pack('<I',0)).digest().ljust(32,b'\0')
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
