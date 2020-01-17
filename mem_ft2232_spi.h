#ifndef MEM_FTDI2232_SPI_H
#define MEM_FTDI2232_SPI_H

#include "mem.h"

#include <stdint.h>
// using mpsse 2232 library for SPI access
extern "C" { 
#include "mpsse.h"
};

class Ftdi2232IF : public MemIF {
  public:
    Ftdi2232IF();
    ~Ftdi2232IF();

    bool access(bool write, unsigned int addr, int size, char* buffer);

  private:
    bool mem_write(uint32_t addr, uint8_t be, uint32_t wdata);
    bool mem_read(uint32_t addr, uint32_t *rdata);

    struct mpsse_context *spi = NULL;
};

#endif
