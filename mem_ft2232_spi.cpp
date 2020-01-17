#include "mem_ft2232_spi.h"

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define WREG1   "\x11\x1f"
#define RREG1   "\x07"

Ftdi2232IF::Ftdi2232IF() {
  // open spidev
  char *data = NULL;
  int reg1 = 0;
  if((spi = MPSSE(SPI0, FIVE_MHZ, MSB)) != NULL && spi->open)
    {
      printf("FTDI2232 device opened!\n");
      printf("Set dummy cycles to 0x1f\n");
      Start(spi);
      Write(spi, WREG1, sizeof(WREG1)-1);
      Write(spi, RREG1, sizeof(RREG1)-1);
      data = Read(spi,4);
      Stop(spi);
      for (int i = 0; i < 4; ++i)
	reg1 |= data[i] << (24 - i*8);
      if (reg1 != 0x1f){
	perror("Failed to set dummy cyles to 0x1f");
	delete data;
	exit(1);
      }
      delete data;
      printf("Setup spi device sucessfully!\n");
    }
  else
    {
      perror("Device not found\n");
      exit(1);
    }
}

Ftdi2232IF::~Ftdi2232IF() {
  Close(spi);
}

bool
Ftdi2232IF::mem_write(uint32_t addr, uint8_t be, uint32_t wdata) {
  char wr_buf[9];

  wr_buf[0] = 0x02; // write command
  // address
  wr_buf[1] = addr >> 24;
  wr_buf[2] = addr >> 16;
  wr_buf[3] = addr >>  8;
  wr_buf[4] = addr >>  0;
  wr_buf[5] = wdata >> 24;
  wr_buf[6] = wdata >> 16;
  wr_buf[7] = wdata >>  8;
  wr_buf[8] = wdata >>  0;

  // write to spidev
  Start(spi);
  if (Write(spi, wr_buf, 9) != MPSSE_OK) {
    perror("Write Error\n");

    return false;
  }
  Stop(spi);
  return true;
}

bool
Ftdi2232IF::mem_read(uint32_t addr, uint32_t *rdata) {

  char wr_buf[5];
  char* data = NULL;

  memset(wr_buf, 0, 5);

  wr_buf[0] = 0x0B; // read command
  // address
  wr_buf[1] = addr >> 24;
  wr_buf[2] = addr >> 16;
  wr_buf[3] = addr >> 8;
  wr_buf[4] = addr;
  Start(spi);
  if (Write(spi, wr_buf, 5) != MPSSE_OK)
    perror("Failed to write to SPI Slave\n");
  data = Read(spi, 8);
  Stop(spi);

  *rdata = 0;
  *rdata |= (data[4] & 0xff) << 24;
  *rdata |= (data[5] & 0xff) << 16;
  *rdata |= (data[6] & 0xff) << 8;
  *rdata |= (data[7] & 0xff);
  delete data;
  return true;
}

bool
Ftdi2232IF::access(bool write, unsigned int addr, int size, char* buffer) {

  bool retval = true;
  uint32_t rdata;
  uint8_t be;

  if (write) {
    // write
    // first align address, if needed

    // bytes
    if (addr & 0x1) {
      be = 1 << (addr & 0x3);
      retval = retval && this->mem_write(addr, be, *((uint32_t*)buffer));
      addr   += 1;
      size   -= 1;
      buffer += 1;
    }

    // half-words
    if (addr & 0x2) {
      be = 0x3 << (addr & 0x3);
      retval = retval && this->mem_write(addr, be, *((uint32_t*)buffer));
      addr   += 2;
      size   -= 2;
      buffer += 2;
    }

    while (size >= 4) {
      retval = retval && this->mem_write(addr, 0xF, *((uint32_t*)buffer));
      addr   += 4;
      size   -= 4;
      buffer += 4;
    }

    // half-words
    if (addr & 0x2) {
      be = 0x3 << (addr & 0x3);
      retval = retval && this->mem_write(addr, be, *((uint32_t*)buffer));
      addr   += 2;
      size   -= 2;
      buffer += 2;
    }

    // bytes
    if (addr & 0x1) {
      be = 1 << (addr & 0x3);
      retval = retval && this->mem_write(addr, be, *((uint32_t*)buffer));
      addr   += 1;
      size   -= 1;
      buffer += 1;
    }
  } else {
    // read

    // bytes
    if (addr & 0x1) {
      retval = retval && this->mem_read(addr, &rdata);
      buffer[0] = rdata;
      addr   += 1;
      size   -= 1;
      buffer += 1;
    }

    // half-words
    if (addr & 0x2) {
      retval = retval && this->mem_read(addr, &rdata);
      buffer[0] = rdata;
      buffer[1] = rdata >> 8;
      addr   += 2;
      size   -= 2;
      buffer += 2;
    }

    while (size >= 4) {
      retval = retval && this->mem_read(addr, &rdata);
      buffer[0] = rdata;
      buffer[1] = rdata >> 8;
      buffer[2] = rdata >> 16;
      buffer[3] = rdata >> 24;
      addr   += 4;
      size   -= 4;
      buffer += 4;
    }

    // half-words
    if (addr & 0x2) {
      retval = retval && this->mem_read(addr, &rdata);
      buffer[0] = rdata;
      buffer[1] = rdata >> 8;
      addr   += 2;
      size   -= 2;
      buffer += 2;
    }

    // bytes
    if (addr & 0x1) {
      retval = retval && this->mem_read(addr, &rdata);
      buffer[0] = rdata;
      addr   += 1;
      size   -= 1;
      buffer += 1;
    }
  }
    
  return retval;
}
