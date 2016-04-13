# Top-level Makefile for SBT SDRDC build scripts
# Copyright (C) 2014 Silver Bullet Technology
 
export SBT_TOP := $(PWD)

include Config

.PHONY: all build setup clean reset

all: setup build

build:
	@$(SHELL) $(SBT_TOOLS)/petalinux-make.sh -C software/petalinux-dist all PV=$(PV)
	@mkdir -p $(SBT_TOP)/out/$(SBT_NOW)
	@cp -v $(SBT_PETALINUX)/software/petalinux-dist/images/kernel.img  out/$(SBT_NOW)/kernel.img
	@cp -v $(SBT_PETALINUX)/software/petalinux-dist/images/devtree.img out/$(SBT_NOW)/devtree.img
	@cp -v $(SBT_TOP)/sbt/prebuilt/empty-ramdisk.img                   out/$(SBT_NOW)/ramdisk.img
ifneq ($(SBT_BOOT_BIN),)
	@cp -v $(SBT_BOOT_BIN)                                             out/$(SBT_NOW)/boot.bin
endif
	@(cd $(SBT_TOP)/out/$(SBT_NOW) && zip -9 $(SBT_TOP)/out/$(SBT_BSP)-$(SBT_NOW).zip *)
	@rm -f $(SBT_TOP)/out/current
	@ln -sf $(SBT_NOW) $(SBT_TOP)/out/current

setup: 
	@$(SHELL) $(SBT_TOOLS)/setup-git.sh
	@$(SHELL) $(SBT_TOOLS)/setup-petalinux.sh
	@$(SHELL) $(SBT_TOOLS)/setup-vendors.sh
	@$(SHELL) $(SBT_TOOLS)/setup-links.sh
	@$(MAKE)  .reset

reset:
	@rm -rf  .reset
	@$(MAKE) .reset

.reset:
	@$(SHELL) $(SBT_TOOLS)/petalinux-make.sh -C software/petalinux-dist mrproper
	@$(SHELL) $(SBT_TOOLS)/petalinux-make.sh -C software/petalinux-dist SilverBulletTech/$(SBT_BSP)_defconfig
	@touch $@

clean:
	@$(SHELL) $(SBT_TOOLS)/petalinux-make.sh -C software/petalinux-dist clean
	@rm -rf out/

