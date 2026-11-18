# CardioPack development setup: Windows 11 and Ubuntu in VirtualBox

**Status: on hold; not recommended or validated.** An attempted VirtualBox installation on the intended Windows 11 machine failed. The cause and scope of the failure are not yet established. Retain this draft for troubleshooting or a future retest; do not treat it as a verified installation procedure. VMware download problems were also reported on that machine. Native Windows ESP-IDF and Ubuntu under WSL 2 are alternatives to investigate, but neither has been validated for this workspace.

This guide installs Ubuntu Desktop in a VirtualBox virtual machine (VM), then installs the tools needed to build the CardioPack ESP32-ECGMonitor firmware. Windows remains the host operating system; Ubuntu runs inside a virtual computer.

## What this guide reproduces

* Reference machine observed: Ubuntu **26.04.1 LTS**, ESP-IDF **v6.0.2**, ESP32-S3 target, VS Code with the Espressif ESP-IDF extension.
* Repository: [georgewicks/CardioPack](https://github.com/georgewicks/CardioPack). The monitor project is `ESP32-ECGMonitor`; `ESP32-ECGSimulator` is a separate project.
* The monitor's tracked `dependencies.lock` records ESP-IDF 6.0.2, target `esp32s3`, and `espressif/esp-dsp` 1.8.2. The component manager needs Internet access on the first build.
* **Important:** the working `sdkconfig` is ignored by Git. A new clone uses the tracked `sdkconfig.defaults` and generated defaults, not necessarily every setting on the reference machine. The reference `sdkconfig` currently selects **2 MB flash** and disables PSRAM, even though the intended N16R8 board has 16 MB flash and 8 MB PSRAM. Hardware configuration is being reviewed separately; do not assume the reference settings are final.
* The tracked workspace file contains a reference-machine ESP-IDF path and serial port. These need to be changed for the VM, as described below. An exact VS Code extension version was not captured; record the version installed during testing.

Building firmware does not require the board. Flashing, serial monitoring, peripheral behavior, and acquisition do require appropriate hardware. An ESP32 emulator is not required for this guide.

## 1. Prepare the Windows host

1. Use a Windows 11 PC with an **Intel or AMD 64-bit processor** and hardware virtualization support. This procedure does not cover Windows on Arm. Check Task Manager > **Performance > CPU** for **Virtualization: Enabled**. If disabled, consult the PC manufacturer's instructions for enabling Intel VT-x or AMD-V in firmware setup.
2. Allow time for several large downloads and installations. A host with **16 GB RAM or more** is recommended; the VM can begin with **8 GB RAM, 4 virtual CPUs, and an 80 GB dynamically allocated virtual disk**. These are starting recommendations, not measured minimums. Keep enough RAM, CPU capacity, and storage free for Windows itself. A dynamic disk grows on the Windows drive as it is used.
3. Use a Windows account that can install software. Connect to the Internet. Back up important files before installing virtualization software.
4. To flash the device later, have the ESP32-S3 N16R8 board and a known **data-capable** USB cable. A charge-only cable will not work.

## 2. Download VirtualBox and Ubuntu

1. On Windows, visit [VirtualBox downloads](https://www.virtualbox.org/wiki/Downloads) and download the **Windows hosts** installer. Record the version.
2. Download the **Ubuntu Desktop 26.04 LTS AMD64/Intel-AMD ISO** from [Ubuntu's downloads](https://ubuntu.com/download/desktop) or its [alternative downloads](https://ubuntu.com/download/alternative-downloads). Select the 26.04 LTS release rather than a short-support interim release. The ISO is the installer image, not the VM application.
3. For download integrity, compare the ISO's SHA256 checksum against the value published by Ubuntu for the exact downloaded filename. In Windows PowerShell, replacing the path with the actual ISO path:

   ```powershell
   Get-FileHash "C:\Users\YOURNAME\Downloads\ubuntu-26.04-desktop-amd64.iso" -Algorithm SHA256
   ```

   Do not use the ISO if the hash differs. See [Ubuntu's checksum instructions](https://ubuntu.com/tutorials/how-to-verify-ubuntu).
4. Run the VirtualBox installer and follow its prompts. Approve its drivers and network components when Windows asks. Network connectivity may briefly be interrupted. Reboot Windows if requested.

**USB note:** An Extension Pack has separate [licensing terms](https://www.virtualbox.org/wiki/Licensing_FAQ); it is not the same thing as Guest Additions. I previously suggested it might be needed for USB passthrough. Do not assume that is true for the installed VirtualBox version: first try the built-in USB controller in section 7 and consult that version's documentation. If an Extension Pack is actually required for a feature, use the matching VirtualBox version and review its license before installing it.

## 3. Create and install the Ubuntu VM

1. Open VirtualBox Manager on Windows and choose **New**. Name the VM, for example `CardioPack-Ubuntu`. Select the downloaded ISO, type **Linux**, and the appropriate **Ubuntu (64-bit)** version.
2. If offered, choose **Skip Unattended Installation** to follow Ubuntu's installer interactively. Assign the recommended starting RAM and CPU values from section 1.
3. Create a new **virtual** hard disk, starting at 80 GB, dynamically allocated. Ensure the Windows drive containing the VM has adequate free space.
4. Start the VM. Follow the Ubuntu installer: choose language, keyboard, normal or interactive installation, and create a local Ubuntu username and password. Record the username; it will be used in the home directory path.
5. If the Ubuntu installer offers **Erase disk and install Ubuntu**, select it **only after verifying that the installer is running inside the VM and the selected disk is the newly created virtual disk**. Do not select or attach a physical Windows disk. If uncertain, stop rather than proceed.
6. Finish installation, restart the VM when prompted, and remove/eject the ISO from the VM's virtual optical drive if asked. Sign in to Ubuntu.
7. In Ubuntu, open **Terminal** with `Ctrl+Alt+T`, then update:

   ```bash
   sudo apt update
   sudo apt upgrade
   sudo reboot
   ```

   Enter the Ubuntu password when prompted; the terminal does not display password characters.

Optional: in the running VM, VirtualBox's **Devices > Insert Guest Additions CD Image** can install guest display/integration tools. This is separate from the Extension Pack. If display resizing works well enough, it can wait; follow the [VirtualBox manual](https://www.virtualbox.org/manual/) for the installed version.

## 4. Install basic development tools and VS Code

Run the following **inside Ubuntu**, not in Windows PowerShell:

```bash
sudo apt update
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv \
    cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
git --version
python3 --version
```

In the Ubuntu browser, download the **64-bit .deb** package from [Visual Studio Code](https://code.visualstudio.com/). Install it by opening the downloaded file with Ubuntu's software installer, or follow Microsoft's [Ubuntu installation instructions](https://code.visualstudio.com/docs/setup/linux). Start VS Code **inside Ubuntu**. GitHub Copilot is optional and is not required to build the project.

## 5. Install ESP-IDF v6.0.2

1. In Ubuntu's VS Code, open **Extensions** (`Ctrl+Shift+X`), search for **ESP-IDF**, and install the extension published by **Espressif Systems**.
2. Open the Command Palette (`Ctrl+Shift+P`) and run **ESP-IDF: Open ESP-IDF Installation Manager**. If unavailable, install the manager using [Espressif's installation instructions](https://docs.espressif.com/projects/idf-im-ui/en/latest/).
3. In the installation manager, select **ESP-IDF v6.0.2** and the **ESP32-S3** target and install the framework and its tools. If it asks for an installation location, use a path inside the Ubuntu home directory without spaces; record the actual ESP-IDF directory. Allow the downloads and any prerequisite prompts to complete.
4. In VS Code, run **ESP-IDF: Select Current ESP-IDF Version** and select the installed v6.0.2 setup. Then run **ESP-IDF: Doctor Command** and review the results. Command labels may differ slightly by extension version; see [Espressif's extension instructions](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html).
5. For terminal commands, open the Installation Manager's **Manage Installations > Open Dashboard**, then select v6.0.2 and **Open IDF Terminal**. This terminal has the environment activated. In it, check:

   ```bash
   idf.py --version
   ```

   The result should report ESP-IDF v6.0.2. Open a new activated terminal whenever needed; a regular terminal does not necessarily inherit the ESP-IDF environment. Espressif also documents an activation script in its [command-line setup](https://docs.espressif.com/projects/esp-idf/en/v6.0.2/esp32s3/get-started/linux-macos-start-project.html).

## 6. Clone, configure, and build

1. In the activated Ubuntu terminal, clone the repository into the Ubuntu filesystem:

   ```bash
   cd ~
   git clone https://github.com/georgewicks/CardioPack.git
   cd ~/CardioPack
   ```

   If the directory already exists, inspect it instead of overwriting it. A clone contains committed files only—not uncommitted changes or ignored local configuration from the reference machine.
2. In Ubuntu's VS Code, choose **File > Open Workspace from File**, and open `~/CardioPack/CardioPack.code-workspace`. It exposes both project folders and the repository root.
3. The workspace file currently sets `idf.currentSetup` to `/home/gwicks/.espressif/v6.0.2/esp-idf` and `idf.port` to `/dev/ttyUSB0`. These are **examples from the reference machine**, not guaranteed VM values. Use **ESP-IDF: Select Current ESP-IDF Version** for this workspace and select the actual installation. Set the serial port later after identifying it. If necessary, inspect the workspace's JSON via **File > Open Workspace from File** and edit its settings to match the VM. Changes to the tracked workspace file will appear in Git; do not commit personal paths or port settings accidentally.
4. For the first build in a **fresh clone**, use the activated terminal:

   ```bash
   cd ~/CardioPack/ESP32-ECGMonitor
   idf.py set-target esp32s3
   idf.py build
   ```

   `set-target` resets the project configuration and build state. Use it here only because this is a fresh clone; do not repeat it on an existing configured project without preserving any settings you need. The first build downloads managed components and may take several minutes.
5. A successful build ends with **Project build complete** and reports the application/partition size. Record the output, particularly the configured flash size and available app space. A build proves compilation, not correct hardware behavior.
6. In VS Code, use the ESP-IDF build command with **ESP32-ECGMonitor** selected as the project folder. Confirm it uses v6.0.2 and builds successfully too. For a build failure, save the first actual error and the full relevant log, not just the final failure line.

### About the N16R8 configuration

Do not assume the generated configuration enables the board's full 16 MB flash and 8 MB PSRAM. `sdkconfig.defaults` does not specify those settings. Review and test the board's flash size, PSRAM mode, partition layout, and any project-specific pins against the forthcoming hardware requirements before changing them. `idf.py menuconfig` or the ESP-IDF menuconfig command can edit a local configuration; changing flash and partitions can affect stored data and boot behavior. Record the final choices and validate them on hardware. Until then, a successful default build is a **build test**, not confirmation that the N16R8 is fully configured.

## 7. Connect and identify the ESP32-S3

> **Safety:** This is a development setup, not a validated medical device. Do not connect a person to the experimental ECG hardware while setting up, flashing, or testing it. Use a suitable isolated test/simulator setup and follow the project's hardware safety guidance.

1. Shut down the VM. In VirtualBox Manager, select it and open **Settings > USB**. Enable the USB controller and choose USB 3.0 (xHCI) if available. Save settings and start Ubuntu.
2. Connect the ESP32-S3 to the Windows host with a data cable. In the running VM window, choose **Devices > USB** and select the ESP32/USB serial device. Once captured, Windows normally cannot use that USB device until it is released back to the host. If multiple similar devices are listed, disconnect and reconnect the board to identify the new entry. Board reset can cause USB re-enumeration; recapture it if needed.
3. In an Ubuntu terminal, inspect the devices:

   ```bash
   lsusb
   ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
   ```

   Compare before and after connecting if needed. The serial port might be `/dev/ttyUSB0` **or** `/dev/ttyACM0`, among other possibilities. If nothing appears, check the USB capture, cable, board connector, and host USB drivers. Do not guess the port.
4. Give the Ubuntu user serial-port access:

   ```bash
   sudo usermod -aG dialout "$USER"
   ```

   Log out of Ubuntu and back in, then run `groups` and confirm `dialout` appears. Do not use `sudo` to run VS Code or the build as a workaround for serial permissions.
5. In VS Code, use the ESP-IDF port-selection command and choose the detected port for **ESP32-ECGMonitor**. Confirm that no other serial monitor is holding it.

## 8. Flash and monitor — only when ready

Flashing overwrites the corresponding firmware regions on the connected board. Do not perform this step if the current firmware must be preserved or the hardware configuration is not understood. A firmware upload is not a full-flash erase, but partition changes can still affect existing data.

In an activated ESP-IDF terminal, from `~/CardioPack/ESP32-ECGMonitor`, replace the example port with the port actually detected in section 7:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Confirm the build and flash complete and inspect the startup log. Exit the ESP-IDF monitor with `Ctrl+]`. If the boot log reports a flash-size mismatch or PSRAM error, stop and capture the message; do not infer physical flash capacity solely from the configured size shown in the firmware log.

The monitor application also depends on external boards and wiring. Peripheral errors, failed SD initialization, or prompts may be expected if those devices are not connected; they are not proof that the VM or ESP-IDF install failed. Only call the **hardware integration test** successful after checking the relevant ADC, RTC, SD, and motion-sensor behavior with the intended hardware.

## 9. Troubleshooting

* **VM won't start or is very slow:** verify virtualization is enabled, host resources are adequate, and consult the VirtualBox documentation for Windows Hyper-V/VBS conflicts. Avoid disabling Windows security features without understanding the implications.
* **Ubuntu can't access the Internet:** check the VM's network adapter (NAT is a reasonable starting point), then check Windows connectivity.
* **ESP-IDF version missing in VS Code:** confirm EIM completed the v6.0.2 installation; use the version-selection command and Doctor. A stale `idf.currentSetup` in workspace settings can override a correct user-level setting.
* **`idf.py` not found:** use EIM's activated IDF terminal; do not assume a regular terminal is activated.
* **Component download fails:** check Internet access to GitHub and the Espressif Component Registry, then retry. Preserve the error details; don't silently change dependency versions.
* **USB device not listed:** verify a data cable, Windows detection, VirtualBox USB controller and device capture. Try reconnecting; some boards have separate USB connectors. Check the installed VirtualBox version's USB documentation.
* **Permission denied or port busy:** confirm `dialout` membership after logging in again, correct port, and no competing monitor. If the device disappears after reset, recapture it in VirtualBox.
* **Flash/build configuration differs from the reference:** compare the VM's generated `sdkconfig` to the intended hardware requirements. The reference machine's ignored `sdkconfig` is not part of a clone. Do not copy unknown credentials or machine-specific paths into the repository.

## 10. Verification record

Complete this during the Windows test; leave unsuccessful or untested items unchecked.

* [ ] Windows 11 version/build and host CPU/RAM: ____________________
* [ ] VirtualBox version and Extension Pack version, if used: ____________________
* [ ] Ubuntu ISO version and SHA256 verified; installed Ubuntu version: ____________________
* [ ] VM RAM, CPUs and virtual disk size: ____________________
* [ ] VS Code and ESP-IDF extension versions: ____________________
* [ ] `idf.py --version` reports v6.0.2; target is ESP32-S3.
* [ ] Repository commit tested: `git rev-parse HEAD`: ____________________
* [ ] Clean clone builds successfully; app/partition sizes recorded: ____________________
* [ ] VS Code ESP-IDF build succeeds with the intended project selected.
* [ ] USB serial device appears in Ubuntu; port and board connector: ____________________
* [ ] Flash and monitor tested on the intended board, with permission: ____________________
* [ ] Actual flash/PSRAM detection and final partition configuration validated: ____________________
* [ ] External peripherals and acquisition tested with safe test equipment: ____________________
* [ ] Problems, exact error messages, and deviations recorded: ____________________

## References

* [VirtualBox user manual](https://www.virtualbox.org/manual/) and [downloads](https://www.virtualbox.org/wiki/Downloads)
* [Ubuntu Desktop installation tutorial](https://ubuntu.com/tutorials/install-ubuntu-desktop)
* [VS Code on Linux](https://code.visualstudio.com/docs/setup/linux)
* [ESP-IDF v6.0.2 ESP32-S3 getting started](https://docs.espressif.com/projects/esp-idf/en/v6.0.2/esp32s3/get-started/index.html)
* [ESP-IDF extension installation](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html)
