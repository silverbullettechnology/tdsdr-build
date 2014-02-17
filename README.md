Silver Bullet Technology SDRDC Build Scripts
==============

Introduction
--------------
These Build Scripts automate the process of retrieving, configuring, and building the
Linux software supplied by Silver Bullet Technology (SBT) for use with its Software-
Defined Radio DaughterCard (SDRDC).


Prerequisites
--------------
To build the Linux software components (kernel, root filesystem, and devicetree) and use
the precompiled bootloader and FPGA bitstream (boot.bin):
- A modern Linux system is required; these scripts were developed under Xubuntu 13.10.
  64-bit is supported as long as 32-bit compatibility libraries installed.  
- A copy of PetaLogix PetaLinux SDK, v2013.4, available from Xilinx.com.
- A valid Xilinx license for PetaLinux; the free evaluation license is adequate

To build the bootloader and FPGA bitstream (boot.bin):
- A copy of Xilinx ISE Design Suite 14.7, available from Xilinx.com.
- A valid Xilinx license for ISE Design Suite; the free evaluation license is adequate


Setup Steps
--------------


Building Images
--------------


