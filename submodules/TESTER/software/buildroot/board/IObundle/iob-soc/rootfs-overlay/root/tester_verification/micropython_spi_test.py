#!/usr/bin/micropython

SUBSECTOR_SIZE = 4096

# Command types
COMM = 0
COMMANS = 1
COMMADDR_ANS = 2
COMM_DTIN = 3
COMMADDR_DTIN = 4
COMMADDR = 5

# Commands
PAGE_PROGRAM = 0x02
READ = 0x03
READ_STATUSREG = 0x05
WRITE_ENABLE = 0x06
SUB_ERASE = 0x20

# Command field offsets
CMD_COMMAND = 0
CMD_NDATA_BITS = 8
CMD_DUMMY_CYCLES = 16
CMD_FRAME_STRUCT = 20

SPI_SYSFS = "/sys/devices/virtual/iob_spi_master/iob_spi_master0"

SPI_SYSFILE_FL_RESET = SPI_SYSFS + "/fl_reset"
SPI_SYSFILE_FL_DATAIN = SPI_SYSFS + "/fl_datain"
SPI_SYSFILE_FL_ADDRESS = SPI_SYSFS + "/fl_address"
SPI_SYSFILE_FL_COMMAND = SPI_SYSFS + "/fl_command"
SPI_SYSFILE_FL_COMMANDTP = SPI_SYSFS + "/fl_commandtp"
SPI_SYSFILE_FL_VALIDFLG = SPI_SYSFS + "/fl_validflg"
SPI_SYSFILE_FL_READY = SPI_SYSFS + "/fl_ready"
SPI_SYSFILE_FL_DATAOUT = SPI_SYSFS + "/fl_dataout"
SPI_SYSFILE_VERSION = SPI_SYSFS + "/version"


def iob_sysfs_read_file(fname):
    # Open file for read
    with open(fname, "r") as file:
        # Read uint32_t value from file in ASCII
        read_value = int(file.readline().strip())
        return read_value


def iob_sysfs_write_file(fname, value):
    with open(fname, "w") as f:
        f.write(str(value))
    return


def spi_set_command(cmd):
    iob_sysfs_write_file(SPI_SYSFILE_FL_COMMAND, cmd)


def spi_set_command_type(cmd_type):
    iob_sysfs_write_file(SPI_SYSFILE_FL_COMMANDTP, cmd_type)


def spi_set_valid(value):
    iob_sysfs_write_file(SPI_SYSFILE_FL_VALIDFLG, value)


def spi_set_address(addr):
    iob_sysfs_write_file(SPI_SYSFILE_FL_ADDRESS, addr)


def spi_set_datain(data_in):
    iob_sysfs_write_file(SPI_SYSFILE_FL_DATAIN, data_in)


def spi_get_ready():
    return iob_sysfs_read_file(SPI_SYSFILE_FL_READY)


def spi_get_data_out():
    return iob_sysfs_read_file(SPI_SYSFILE_FL_DATAOUT)


def spi_wait_for_ready():
    while True:
        if spi_get_ready():
            break


def flash_execute_command(cmd_type, data_in, addr, cmd):
    data_out = 0x00
    print("flash_execute_command:")
    print(f"\tcmd_type: {hex(cmd_type)}")
    print(f"\tdata_in: {hex(data_in)}")
    print(f"\taddr: {hex(addr)}")
    print(f"\tcmd: {hex(cmd)}")

    spi_set_command(cmd)
    spi_set_command_type(cmd_type)

    spi_wait_for_ready()

    if cmd_type == COMM:
        print("\t\tCommand type: COMM")
        spi_set_valid(1)
        spi_set_valid(0)
    elif cmd_type == COMMANS:
        print("\t\tCommand type: COMMANS")
        spi_set_valid(1)
        spi_set_valid(0)
        spi_wait_for_ready()
        data_out = spi_get_data_out()
    elif cmd_type == COMMADDR_ANS:
        print("\t\tCommand type: COMMADDR_ANS")
        spi_set_address(addr)
        spi_set_valid(1)
        spi_set_valid(0)
        spi_wait_for_ready()
        data_out = spi_get_data_out()
    elif cmd_type == COMM_DTIN:
        print("\t\tCommand type: COMM_DTIN")
        spi_set_datain(data_in)
        spi_set_valid(1)
        spi_set_valid(0)
    elif cmd_type == COMMADDR_DTIN:
        print("\t\tCommand type: COMMADDR_DTIN")
        spi_set_address(addr)
        spi_set_datain(data_in)
        spi_set_valid(1)
        spi_set_valid(0)
    elif cmd_type == COMMADDR:
        print("\t\tCommand type: COMMADDR")
        spi_set_address(addr)
        spi_set_valid(1)
        spi_set_valid(0)
    else:
        print("\t\tCommand type: UNKNOWN")
        spi_set_valid(1)
        spi_set_valid(0)
    return data_out


def build_command(frame_struct, dummy_cycles, ndata_bits, command):
    cmd = 0x00
    cmd = cmd | (command << CMD_COMMAND)
    cmd = cmd | (ndata_bits << CMD_NDATA_BITS)
    cmd = cmd | (dummy_cycles << CMD_DUMMY_CYCLES)
    cmd = cmd | (frame_struct << CMD_FRAME_STRUCT)
    return cmd


def flash_read_status_reg():
    status_cmd = build_command(
        frame_struct=0x00,
        dummy_cycles=0x00,
        ndata_bits=1 * 8,
        command=READ_STATUSREG,
    )
    read_data = flash_execute_command(
        cmd_type=COMMANS,
        data_in=0,
        addr=0,
        cmd=status_cmd,
    )
    return read_data


def erase_flash_subsector(flash_addr):
    print(f"erase_flash_subsector: {hex(flash_addr)}")

    # Write Enable
    _ = flash_execute_command(cmd_type=COMM, data_in=0, addr=0, cmd=WRITE_ENABLE)

    # Wait for write enable latch to be set
    while True:
        if flash_read_status_reg() & 0x02:
            print("\t\tPost write enable: ready")
            break

    # Execute ERASE
    _ = flash_execute_command(
        cmd_type=COMMADDR,
        data_in=0,
        addr=flash_addr,
        cmd=SUB_ERASE,
    )

    # wait for write in progress ready: 0: ready; 1: busy
    while True:
        if (flash_read_status_reg() & 0x01) == 0:
            print("\t\tPost erase WIP: ready")
            break


# Write data up to 32bits
def flash_program_32bit(data, addr, nbytes=4):
    print(f"flash_program_32bit: {hex(data)} at addr: {hex(addr)}")

    # Write Enable
    _ = flash_execute_command(cmd_type=COMM, data_in=0, addr=0, cmd=WRITE_ENABLE)

    # Wait for write enable latch to be set
    while True:
        if flash_read_status_reg() & 0x02:
            print("\t\tPost write enable: ready")
            break

    program_cmd = build_command(
        frame_struct=0x00,
        dummy_cycles=0x00,
        ndata_bits=nbytes * 8,
        command=PAGE_PROGRAM,
    )

    _ = flash_execute_command(
        cmd_type=COMMADDR_DTIN,
        data_in=data,
        addr=addr,
        cmd=program_cmd,
    )

    # wait for write in progress ready: 0: ready; 1: busy
    while True:
        if (flash_read_status_reg() & 0x01) == 0:
            print("\t\tPost program WIP: ready")
            break


def flash_readmem(addr):
    print(f"flash_readmem at addr: {hex(addr)}")
    read_cmd = build_command(
        frame_struct=0x00,
        dummy_cycles=0x00,
        ndata_bits=4 * 8,
        command=READ,
    )
    return flash_execute_command(
        cmd_type=COMMADDR_ANS,
        data_in=0,
        addr=addr,
        cmd=read_cmd,
    )


def main():
    print("MicroPython SPI test")

    flash_addr = 0x01FFF000
    test_size = 16

    test_failed = False

    # assume test_size fits in a single subsector
    assert (
        test_size <= SUBSECTOR_SIZE
    ), f"Test size {test_size} exceeds subsector size {SUBSECTOR_SIZE}"
    erase_flash_subsector(flash_addr)

    # write data to flash
    for i in range(0, test_size, 4):
        word32bit = i
        word32bit = word32bit | ((i + 1) << (1 * 8))
        word32bit = word32bit | ((i + 2) << (2 * 8))
        word32bit = word32bit | ((i + 3) << (3 * 8))
        if test_size > (i + 4):
            nbytes = 4
        else:
            nbytes = test_size - i

        flash_program_32bit(data=word32bit, addr=flash_addr + i, nbytes=nbytes)

    # read data back
    for i in range(0, test_size, 4):
        expected_data = i
        expected_data = expected_data | ((i + 1) << (1 * 8))
        expected_data = expected_data | ((i + 2) << (2 * 8))
        expected_data = expected_data | ((i + 3) << (3 * 8))
        read_data = flash_readmem(flash_addr + i)
        print(f"expected_data: {hex(expected_data)} read_data: {hex(read_data)}")
        if expected_data != read_data:
            print("\t\tError: data mismatch")
            test_failed = True

    if test_failed:
        print("FAILURE: SPI Test failed")
    else:
        print("SUCCESS: SPI Test passed")


if __name__ == "__main__":
    main()
