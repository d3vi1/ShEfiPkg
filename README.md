# ShEfiPkg
Legacy code to boot Windows in EFI mode on Macs. Why would you do that? Because a lot of older Macs are able to UEFI boot Windows. That would give you native SSD TRIM, GPT Partition Support, possibly Optimus support, or at least it's a prerequisite to choosing which VGA card to use. Furthermore it should be a lot faster to boot and have better power management.
99% of the code was written in 2010-2011 and was my attempt at learning C and UEFI.

# EFIUpdateReleaseDXE
This DXE sets the UEFI release to 2.10 and adds an implementation of the RuntimeServices->QueryVariableInfo function that is needed by Windows UEFI to boot. It assumes that you have a standard UEFI NVRam Store.

# GOPSetRes
It is a simple application that I wrote as a helloworld to learn C and EFI.

# EFISetVGARegsDxe
The brains of the operation. Fundamentally implemented as a driver and it does this:
* Scans all the PCI devices to find VGA cards.
* Finds out which one is active at the time.
* Scans the firmware volumes for all vga option roms.
* Turns off (disconnects) the UEFI driver for the VGA card
* Loads the VGA option ROM in x86emu and attempts to POST it.
* It then unlocks the reserved 1st MB of physical memory using EfiLegacyRegionDxe (on most platforms that involves using MTRR), copies the IVT and VGA Option ROM Shadow from the x86emu buffer
* Reloads the UEFI driver back. Windows will know how to thunk to bios-mode for int10h when needed.

It uses the Scitech developed x86emu since I couldn't figure out how to use EfiLegacyBiosDxe to POST and then return to UEFI.

# EFISetVGARegsDXE
A simple replacement boot application:
* first loads the EFIUpdateReleaseDXE
* loads EFISetVGARegsDXE
* runs bootmgfw.efi from the standard path on the same partition.

What's missing is a replacement EDID override DXE in order to get 1024x768 from the onboard video card. For some reason, the Apple provided UEFI drivers don't provide anything except the native display panel resolution in GOP. A skeleton one is implemented in EFIInstallEdidOverride
