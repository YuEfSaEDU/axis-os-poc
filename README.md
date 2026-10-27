# axis-os-poc

Executed-proof harness for AXIS OS bug-bounty research (Bugcrowd `axis-os-public`).

**What it does:** downloads an official Axis firmware image from `ftp.axis.com`
(SHA256-verified against the published checksum), extracts the squashfs rootfs, and
executes a chosen aarch64 binary from that rootfs under `qemu-aarch64-static` in a
chroot — producing a verbatim execution log as the PoC artifact.

**Why:** the program provides no device access; running the actual shipped binaries
under emulation is the executed-proof substrate for static findings until hardware or
the Axis virtual loan system is available.

**ROE:** official firmware sources only; benign inputs; no attacks on Axis
infrastructure; no third-party/proprietary code committed to this repo (firmware is
downloaded at run time and never pushed).

## Usage

Run the `firmware-exec-poc` workflow with:
- `firmware_url`: e.g. `http://ftp.axis.com/pub/axis/software/PACS/C1310-E_Mk_II/12_11_118/C1310-E_Mk_II_12_11_118.bin`
- `firmware_sha256`: published integrity checksum
- `run_cmd`: e.g. `/bin/busybox echo smoke-test` or a specific CGI handler invocation
