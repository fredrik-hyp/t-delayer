#!/bin/bash
OPENSSL_INSTALL_DIR="../../../openssl"
LIBFLAGS=" $OPENSSL_INSTALL_DIR/lib/libssl.a $OPENSSL_INSTALL_DIR/lib/libcrypto.a"
gcc -std=c99  -lssl -lcrypto -I ../../ -I ../../../pov-common -I ../../../pov-database/src/c $LIBFLAGS  test_checklicense.c ../../../pov-common/common-license.c t_checklicense.c -o test_checklicense