#!/bin/sh
# Builds DigiThru's handlers for the host with the firmware stubbed, and runs the checks.
set -e
cd "$(dirname "$0")"
sed -e 's/^typedef int int32_t;//' -e 's/^typedef unsigned int uint32_t;//' -e 's/^typedef unsigned char uint8_t;//' \
    -e 's/((seq_send_t)SEQ_SEND_AT)/(SEQ_SEND_AT)/' -e 's/((mtrk_chan_t)MTRK_CHAN_AT)/(MTRK_CHAN_AT)/' \
    -e 's/(\*(volatile int32_t \*)\([A-Z_]*\))/(*(volatile int *)(\1))/' ../digithru.c > dt_host.c
cc -Wall -fsanitize=undefined,address -fno-sanitize-recover=all -o host_test host_test.c
./host_test
rm -f host_test dt_host.c
