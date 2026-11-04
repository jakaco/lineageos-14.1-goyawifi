# Build notes

1. Create a LineageOS 14.1 source tree and sync the exact revisions in `sources/lineage-14.1-resolved-manifest.xml`.
2. Fetch the downstream `T110-pmOS/linux_kernel_pxa988` revision in `sources/kernel-source.json`; use the ARM toolchain specified by your build host.
3. Apply the relevant patches from `patches/android/` and `patches/kernel/` to their matching pinned projects. Keep the source patch base and revision aligned.
4. Place the device files from `device/samsung/goyawifi/` at `device/samsung/goyawifi/` in the Android tree. The tested runtime overlays were generated from source-built components and the exact SM-T110 stock firmware; those vendor files are intentionally absent here.
5. Follow the staged build scripts in `scripts/`. They document the native kernel, Android core, graphics, services, ART, camera, and system assembly steps. The original scripts expect the isolated Ubuntu/J: build environment used for this release and may need host-path edits.
6. Build the TWRP ZIP with `scripts/build-twrp-zip.py --system-gz PATH --kernel PATH --output PATH`. The script refuses payloads whose SHA-256 does not match this release metadata.

The source sync and proprietary runtime extraction are separate: building from source alone does not recreate Samsung's closed-source hardware HALs.
