#!/usr/bin/env python3
"""
#!/usr/bin/micropython
"""


def flash_execute_command(cmd_type, addr, cmd):
    print(
        f"flash_execute_command: cmd_type: {hex(cmd_type)} addr: {hex(addr)} cmd: {hex(cmd)}"
    )
    return 0x00


def flash_read_status_reg():
    return 0x02


def erase_flash_sector(flash_addr):
    print(f"flash_addr: {hex(flash_addr)}")

    cmd = 0x00
    cmd_type = 0x00
    addr = 0x00

    # Write Enable

    # Read Status Register until not busy
    while True:
        status = flash_read_status_reg()
        if status & 0x02:
            print("\t\tStatus: ready")
            break

    # Execute ERASE
    _ = flash_execute_command(cmd_type, addr, cmd)

    # wait for write in progress ready: 0: ready; 1: busy
    while True:
        status = flash_read_status_reg()
        if status & 0x01 == 0:
            print("\t\tWIP: ready")
            break
        break


def flash_memProgram(data, addr):
    print(f"flash_memProgram: {data.hex()} at addr: {hex(addr)}")


def flash_readmem(addr):
    print(f"flash_readmem at addr: {hex(addr)}")
    return bytes([0x00, 0x00, 0x00, 0x00])


def main():
    print("MicroPython SPI test")

    flash_addr = 0x01FFF000
    test_size = 64

    test_failed = False

    # Note: assume test_size fits in a single sector
    erase_flash_sector(flash_addr)

    # write data to flash
    for i in range(0, test_size, 4):
        word32bit = bytes([i + 3, i + 2, i + 1, i])
        flash_memProgram(word32bit, flash_addr + i)

    # read data back
    for i in range(0, test_size, 4):
        expected_data = bytes([i + 3, i + 2, i + 1, i])
        read_data = flash_readmem(flash_addr + i)
        print(f"expected_data: {expected_data.hex()} read_data: {read_data.hex()}")
        if expected_data != read_data:
            print("\t\tError: data mismatch")
            test_failed = True

    if test_failed:
        print("FAILURE: SPI Test failed")
    else:
        print("SUCCESS: SPI Test passed")


if __name__ == "__main__":
    main()
