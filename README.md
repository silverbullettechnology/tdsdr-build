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


Ubuntu-specific Setup
--------------
At this writing Ubuntu 13.10 is the supported build environment.  In general it must be
set up to support development under PetaLinux, which has some specific requirements as
detailed below.  The flavor of GUI (Ubuntu, Kubuntu, or Xubuntu) should not make a
significant difference, but primary development is done under Xubuntu, which is also
recommended for use in Virtual Machines due to its relatively lightweight XFCE GUI.

PetaLinux and the SDRDC scripts require typical development software with a few specific
packages.  Install these with *apt-get*:
```
sudo apt-get install build-essential git gawk dos2unix realpath zip
```

64-bit installations of Ubuntu will require the 32-bit compatiblity versions of certain
libraries installed.  The easiest way to accomplish this is with the *apt-get* tool:
```
sudo apt-get install lib32bz2-1.0 lib32ncurses5 lib32z1 libselinux1:i386
sudo apt-get install libstdc++6:i386 zlib1g-dev
```

Ubuntu systems use the Debian Almquist shell (dash) by default, which does not support
some syntax used in certain PetaLinux scripts.  You must switch to the GNU Bourne-again
shell (bash) to use PetaLinux, both system-wide and for your user account.  To set the
shell system-wide, run:
```
    sudo dpkg-reconfigure dash
```

Select No with the left/right arrow keys and press Enter to switch the system-wide default
to bash.  For your user account, run:
```
    chsh
```

Enter your login password if prompted, set your Login Shell to "/bin/bash" and press
Enter.  You will have to log out and back in for the change to take effect.  These setup
steps should be required only once per installation of Ubuntu, and the *chsh* step
required once per user account.


Setup Steps
--------------
1. If you don't have a copy already, download the _PetaLinux 13.04 Installation archive
   for Zynq and MicroBlaze_ from
   [here](http://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/petalinux/2013-04.html). 
   You will need a valid Xilinx account to download the SDK installation archive.  Save it
   somewhere convenient like your home directory.  It is strongly recommended you also
   download the _PetaLinux 13.04 Board Support Package for Xilinx ZC702 evaluation kit_
   from the same location to test your PetaLinux installation.

2. You will need a valid Xilinx license installed in your Linux machine to use PetaLinux.
   An evaluation license is adequate and can be created [here](http://www.xilinx.com/getlicense). 
   These are node-locked licenses for which you'll need your Linux machine's ethernet MAC
   address as a Host ID.  You can get this by running at the command prompt:
   ```
   ifconfig
   ```
   
   Once created, the license file should be emailed to you, and should be downloadable
   from the license management tool on the Xilinx website.  The resulting *Xilinx.lic*
   file should be copied to the *.Xilinx* directory under your home directory:
   ```
   mkdir -p $HOME/.Xilinx
   cp -i /path/to/Xilinx.lic $HOME/.Xilinx
   ```

3. It is strongly recommended you unpack the PetaLinux tarball to a temporary directory,
   install the ZC702 BSP downloaded in step 1, and test your Xilinx license and software
   dependencies before continuing with the SDRDC build.  A Xilinx guide to the process is 
   [UG976](http://www.xilinx.com/support/documentation/sw_manuals/petalinux2013_04/ug976-petalinux-installation.pdf).

4. Clone a copy of the SDRDC build scripts from the SBT github repository and cd into the
   resulting directory.  Edit the file *Config* and set the variable *SBT_PETALINUX_SDK*
   to point to the PetaLinux SDK tarball downloaded in step 1
   ```
   git clone -b sdrdc-stable https://github.com/silverbullettechnology/sdrdc-build.git
   cd sdrdc-build
   nano Config
   ```

5. Run the SDRDC setup scripts:
   ```
   make setup
   ```

   This will clone a copy of the Linux kernel tree from github into your sdrdc-build
   working directory.  As this can take some time, you may want instead to set up a local
   clone of this repository as a cache, which will make setting up subsequent working
   directories much faster.  Instructions for this optional step will be added soon.

   The setup scripts will unpack, install, and patch the PetaLinux SDK.  You must agree to
   the PetaLinux license terms to use the PetaLinux SDK.

   PetaLinux will also try to test whether your user account is authorized to use *sudo*
   to run commands as root.  This is not necessary for building the SDRDC firmware, and
   you can press Enter at the password prompt to skip this test.  The warning message this
   produces can be ignored.


Building Images
--------------
Once the setup steps are complete, you can build with the command:
```
make
```

This will fully clean and rebuild the Linux kernel and source software components, and
produce a timestamped directory under *out*.  The build files are also archived into a
.zip file with the same timestamp.

Copy the four files (*boot.bin*, *kernel.img*, *devtree.img*, and *ramdisk.img*) to the
micro-SD card, insert into the SDRDC, and apply 5V power

You can also cd into the *petalinux* directory and use the normal PetaLinux tools after
sourcing the *settings.sh* file as described in UG976:
```
cd petalinux
. settings.sh
petalinux-config-kernel
petalinux-config-apps
make -C software/petalinux all
```

This approach is faster, but not recommended pending some issues with building the
readline library.  If the PetaLinux build fails the recommended action is to change back
to the sdrdc-build working directory and re-run:
```
make
```





