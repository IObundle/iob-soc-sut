#!/usr/bin/env python3
# Minicom python script to control serial communication

import sys
import os

####################################################################
# Configuration

def initial_config():
    global STRING_BUFFER_SIZE, INITIAL_EXPECT_DICT
    sys.stderr.write("[minicom] Started minicom script.\n")

    # Size of buffer that stores lasted received chars
    STRING_BUFFER_SIZE=30

    # Initial dictionary of expected strings and the function to call when it's received
    INITIAL_EXPECT_DICT = {
        "B000000000000":receive_zmodem,
        "B0100000023be":send_zmodem,
        "Welcome to Buildroot":finish
    }


####################################################################
# Functions to run on expected strings. Add more as needed.
#
# They receive the latest buffer as a parameter, and should return a new expected dictionary

def receive_zmodem(buffer):
    sys.stderr.write("[minicom] ZMODEM download requested\n")
    os.system("rz")
    return INITIAL_EXPECT_DICT

def send_zmodem(buffer):
    # Get filename from last line before 'rz' special string
    filename = buffer.split('\n')[-2] # FIXME: This selection may not be correct
    sys.stderr.write(f"[minicom] ZMODEM upload requested for file {filename}\n")
    os.system("sz "+filename)

    return INITIAL_EXPECT_DICT

def finish(buffer):
    sys.stderr.write("\n[minicom] Script finished.\n")
    exit(0)



####################################################################
# Main function

def main():
    initial_config()
    string_buffer = " " * STRING_BUFFER_SIZE
    expect_dict = INITIAL_EXPECT_DICT

    while True:
        try:
            char = sys.stdin.read(1)
        except UnicodeDecodeError:
            char = "."

        # Break the loop if no more input
        if not char:
            sys.stderr.write("[minicom] No more input.\n")
            break

        # Relay input to stderr (will be printed to minicom's stdout)
        sys.stderr.write(char)
        sys.stderr.flush()

        # Update buffer
        string_buffer = string_buffer[1:] + char

        # Check each of the expected strings, and launch the functions as needed
        for k, v in expect_dict.items():
            if string_buffer.endswith(k):
                expect_dict = v(string_buffer)


if __name__ == "__main__":
    main()
