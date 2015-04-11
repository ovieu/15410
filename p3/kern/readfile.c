/** @file readfile.c
 *  @brief This file implements the readfile system call.
 *
 *  @author Patrick Koenig (phkoenig)
 *  @author Jack Sorrell (jsorrell)
 *  @bug No known bugs.
 */

#include <syscall.h>
#include <stdlib.h>
#include <loader.h>
#include <vm.h>
#include <simics.h>

int readfile(char *filename, char *buf, int count, int offset)
{
    if (str_check(filename, USER_FLAGS) < 0) {
        return -1;
    }

    return getbytes(filename, offset, count, buf);
}