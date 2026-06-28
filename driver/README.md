# WDM Sample Driver

A minimal Windows Driver Model (WDM) kernel-mode driver, plus a GitHub
Actions workflow that compiles it.

## Files

| File | Purpose |
| --- | --- |
| `wdmdriver.c` | Driver source: `DriverEntry`, `AddDevice`, PnP / pass-through dispatch and `Unload`. |
| `wdmdriver.inf` | Installation information file. |
| `wdmdriver.vcxproj` / `wdmdriver.sln` | Visual Studio / WDK build project (Universal, WDM driver type). |

The driver creates a functional device object, attaches it to the device
stack and forwards IRPs to the lower driver. It performs no real hardware
work — it is intended as a build-verification / learning starting point.

## Building locally

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the
   C++ workload.
2. Install the [Windows Driver Kit (WDK)](https://learn.microsoft.com/windows-hardware/drivers/download-the-wdk)
   and its Visual Studio extension.
3. Open `wdmdriver.sln` and build, or from a Developer prompt:

   ```
   msbuild driver\wdmdriver.sln /p:Configuration=Release /p:Platform=x64
   ```

The output `wdmdriver.sys` is **test-signed only**. To load it on a real
machine you must enable test signing (`bcdedit /set testsigning on`).

## Building in CI

`.github/workflows/build-driver.yml` runs on a Windows runner. It installs
the Windows SDK + WDK and the WDK Visual Studio extension, builds the Debug
and Release x64 configurations with MSBuild, and uploads the resulting
`.sys` / `.inf` as build artifacts.
