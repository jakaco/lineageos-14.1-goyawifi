# Licensing and provenance

This repository combines device files and build scripts with patch files derived from upstream Android/LineageOS and the Linux kernel. Original copyright and license headers in each patch must be retained; the Android source projects use their own Apache/GPL and other component licenses, and the downstream kernel is GPL-2.0. This repository does not replace or relicense upstream work.

The Git repository does not contain Samsung proprietary blobs. The binary TWRP release image contains device-specific components derived from SM-T110 stock firmware and source-built components; those binary components are not covered by this repository's source patch licenses. Their owner and firmware license terms continue to apply. No permission to redistribute proprietary components is granted by this repository.

Apache-2.0 applies only to the original device configuration and build/packaging files listed in LICENSES/README.md. Patch files retain their upstream license notices. No license from this repository covers proprietary release binaries. Check each upstream project and component before reuse or redistribution.

## WebView

Release images from r4 on ship the unmodified Bromite SystemWebView 81.0.4044.76 for 32-bit ARM (`arm_SystemWebView.apk`, SHA-256 `306e7c95ed348e13deb3e247eca4fc9595a29b77452ee54996ab7d7dda25805f`), in place of the stock Chromium 52 WebView. Bromite is a Chromium fork licensed under GPL-3.0; Chromium components carry their own BSD-style and third-party licenses. Source and release: https://github.com/bromite/bromite/releases/tag/81.0.4044.76. The APK is not committed to this repository.
