/*
 * (C) Copyright 2007-2008 Michal Simek
 *
 * Michal SIMEK <monstr@monstr.eu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * CAUTION: This file is automatically generated by libgen.
 * Version: Xilinx EDK 14.5 EDK_P.58f
 * Generate by U-BOOT v4.00.c
 * Project description at http://www.monstr.eu/uboot/
 */

#define XILINX_BOARD_NAME	"TDSDR"

/* ARM is ps7_cortexa9_0 */
#define XPAR_CPU_CORTEXA9_CORE_CLOCK_FREQ_HZ	666666687

/* Interrupt controller is ps7_scugic_0 */
#define XILINX_PS7_INTC_BASEADDR		0xf8f00100

/* System Timer Clock Frequency */
#define XILINX_PS7_CLOCK_FREQ	333333343

/* Uart console is ps7_uart_1 */
#define XILINX_PS7_UART
#define XILINX_PS7_UART_BASEADDR	0xe0001000
#define XILINX_PS7_UART_CLOCK_HZ	50000000

/* IIC pheriphery is ps7_i2c_0 */
#define XILINX_PS7_IIC_BASEADDR		0xe0004000

/* GPIO is gpio_sw*/
#define XILINX_GPIO_BASEADDR	0x41220000

/* SDIO controller is ps7_sd_0 */
#define XILINX_PS7_SDIO_BASEADDR		0xe0100000

/* Main Memory is ps7_ddr_0 */
#define XILINX_RAM_START	0x00000000
#define XILINX_RAM_SIZE		0x40000000

/* Flash Memory is ps7_qspi_0 */
#define XILINX_PS7_QSPI_FLASH_BASEADDR	0xE000D000
#define XILINX_SPI_FLASH_MAX_FREQ	50000000
#define XILINX_SPI_FLASH_CS	0

/* Sysace doesn't exist */

/* Ethernet controller is ps7_ethernet_0 */
#define XILINX_PS7_GEM_BASEADDR			0xe000b000

/* Definitions for peripheral AXI_AD9361_0 */
#define XPAR_AXI_AD9361_0_BASEADDR 0x79000000
#define XPAR_AXI_AD9361_0_HIGHADDR 0x7900FFFF

/* Definitions for peripheral AXI_AD9361_1 */
#define XPAR_AXI_AD9361_1_BASEADDR 0x79020000
#define XPAR_AXI_AD9361_1_HIGHADDR 0x7902FFFF

/* Definitions for peripheral AXI_DMAC_0 */
#define XPAR_AXI_DMAC_0_BASEADDR 0x7C400000
#define XPAR_AXI_DMAC_0_HIGHADDR 0x7C40FFFF


/* Definitions for peripheral AXI_DMAC_1 */
#define XPAR_AXI_DMAC_1_BASEADDR 0x7C420000
#define XPAR_AXI_DMAC_1_HIGHADDR 0x7C42FFFF


/* Definitions for peripheral AXI_DMAC_2 */
#define XPAR_AXI_DMAC_2_BASEADDR 0x7C440000
#define XPAR_AXI_DMAC_2_HIGHADDR 0x7C44FFFF


/* Definitions for peripheral AXI_DMAC_3 */
#define XPAR_AXI_DMAC_3_BASEADDR 0x7C460000
#define XPAR_AXI_DMAC_3_HIGHADDR 0x7C46FFFF


/* Temporary definitions for SRIO */
#define XPAR_AXI_SRIO_INITIATOR_FIFO_BASEADDR              0x43C00000
#define XPAR_AXI_SRIO_INITIATOR_FIFO_AXI4_BASEADDR         0x43C10000
#define XPAR_FABRIC_AXI_SRIO_INITIATOR_FIFO_INTERRUPT_INTR 61
#define XPAR_AXI_SRIO_TARGET_FIFO_BASEADDR                 0x43C20000
#define XPAR_AXI_SRIO_TARGET_FIFO_AXI4_BASEADDR            0x43C30000
#define XPAR_FABRIC_AXI_SRIO_TARGET_FIFO_INTERRUPT_INTR    62
#define XPAR_SRIO_GEN2_0_BASEADDR                          0x50000000
#define XPAR_SRIO_SYS_REG_0_BASEADDR                       0x83C00000
