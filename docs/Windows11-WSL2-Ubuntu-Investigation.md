# Investigating Ubuntu under WSL 2 for CardioPack

**Status: preliminary.** These steps are a starting point, not yet a tested or certified CardioPack installation.

Windows Subsystem for Linux (WSL 2) runs a Linux environment on Windows. CardioPack can use Linux paths such as `/home/yourname/CardioPack` and the Linux ESP-IDF toolchain. VirtualBox and an Ubuntu ISO are not required.

WSL is not quite the same as a full Ubuntu desktop: normally, an Ubuntu terminal is used alongside Windows applications, with VS Code connecting into Ubuntu. WSL 2 also uses a lightweight virtual machine behind the scenes, so hardware virtualization must be enabled.

## Getting started

1. On Windows, open **PowerShell as Administrator** and run:

   ```powershell
   wsl --install
   ```

   Restart Windows if prompted. This normally installs Ubuntu by default. If WSL is already installed, see [Microsoft's installation instructions](https://learn.microsoft.com/en-us/windows/wsl/install) for selecting an Ubuntu distribution.

2. Open **Ubuntu** from the Windows Start menu. On first launch, create a Linux username and password. Then run:

   ```bash
   sudo apt update
   sudo apt upgrade
   ```

3. In Windows PowerShell, check that Ubuntu is using **version 2**:

   ```powershell
   wsl --list --verbose
   ```

4. Install **VS Code on Windows** and its **WSL extension**. From the Ubuntu terminal, clone CardioPack into the Linux home directory:

   ```bash
   sudo apt install git
   cd ~
   git clone https://github.com/georgewicks/CardioPack.git
   cd CardioPack
   code CardioPack.code-workspace
   ```

   VS Code should open a WSL-connected window. Its interface runs on Windows, but the project tools and terminal run inside Ubuntu. Keeping the repository under `~/CardioPack`, rather than `/mnt/c/...`, is preferable for Linux development.

5. In that **WSL-connected** VS Code window, install the Espressif ESP-IDF extension in WSL and use its installation manager to install **ESP-IDF v6.0.2** for **ESP32-S3**. Then verify that the project builds before moving on to hardware.

## Two things to watch

* **Linux paths work, but the username may differ.** The workspace currently contains `/home/gwicks/...` and `/dev/ttyUSB0`. If the WSL username or installation path differs, those settings must be adjusted locally; the serial port also needs to be identified rather than assumed.
* **USB needs an extra step.** WSL does not automatically receive the ESP32 USB connection. Microsoft documents using [usbipd-win](https://learn.microsoft.com/en-us/windows/wsl/connect-usb) to attach it. First test the build without the board, then address USB passthrough and flashing separately.

WSL 2 looks like a promising way to retain the Linux conventions used by CardioPack. Test through the **build step first**; once that works, the USB steps can be documented and verified for the actual board.
