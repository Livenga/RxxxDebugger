#ifndef _SEEKFD_DUMP_H
#define _SEEKFD_DUMP_H

/**
 */
extern size_t seekfd_write_peekdata(
    int               fd,
    pid_t             pid,
    unsigned long int addr,
    unsigned long int size);

/**
 */
extern size_t seekfd_dump_readv(
    int               fd,
    pid_t             pid,
    unsigned long int iov_addr,
    unsigned long int iovcnt);

/**
 */
extern size_t seekfd_dump_writev(
    int               fd,
    pid_t             pid,
    unsigned long int iov_addr,
    unsigned long int iovcnt);

#endif
