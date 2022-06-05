//Example configuration file for REGFILEIF
//This example has 2 write registers a 2 read registers
`IOB_SWREG_W(REGFILEIF_REG1, 8, 0) // Write register: 8 bit
`IOB_SWREG_W(REGFILEIF_REG2, 16, 0) // Write register: 16 bit
`IOB_SWREG_R(REGFILEIF_REG3, 8, 0) // Read register: 8 bit
`IOB_SWREG_R(REGFILEIF_REG4, 16, 0) // Read register 16 bit
`IOB_SWREG_R(REGFILEIF_REG5, 32, 0) // Read register 32 bit. In this example, we use this to pass the sutMemoryMessage address.