/* This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define FBIODISPON 0x4689
#define FBIO_REFRESH_ALWAYS     0x468d
#define FBIO_REFRESH_EVENTS     0x468e
#define FBIO_GET_PHYS 0x4691

struct fb_phys {
  unsigned int fb_phys;
  unsigned int fb_len;
  unsigned int lcdd_phys;
};

volatile unsigned int *lcd;
unsigned int *lcdd;
unsigned int *cpm;
unsigned int *gpio;
unsigned int *io;
int fbd;
struct fb_phys fb_info;

void i2c(int addr, int val)
{
  char cmd[80];
  sprintf(cmd, "i2cset -y i2c-gpio0 0x76 0x%x 0x%x", addr, val);
  system(cmd);
}

void ctel_on(int pal)
{
  i2c(4, 0xc1); /* disable DAC, power down */

  if (pal)
    i2c(0xa, 0x13);
  else
    i2c(0xa, 0x10);

  i2c(0xb, 3);
  i2c(0xd, 3);
  i2c(0xe, 0);

  if (pal) {
    i2c(0x11, 0x19);
    i2c(0x13, 0x6c);
  }
  else {
    i2c(0x11, 0x11);
    i2c(0x13, 0xe0);
  }

  i2c(0x17, 4);

  if (pal)
    i2c(0x19, 0x12);
  else
    i2c(0x19, 0x10);

  i2c(0x1f, 0x41);

  if (pal) {
    i2c(0x23, 0x7a);
    i2c(0x28, 0x1d);
    i2c(0x29, 0x7c);
    i2c(0x2a, 0);
    i2c(0x2b, 0x20);
    i2c(0x2c, 0xf5);
    i2c(0x2d, 0x80);
  }
  else {
    i2c(0x23, 0x7f);
    i2c(0x28, 0x18);
    i2c(0x29, 0x60);
    i2c(0x2a, 0x40);
    i2c(0x2b, 0x1b);
    i2c(0x2c, 0x7e);
    i2c(0x2d, 0x48);
  }
  i2c(0x2e, 0x38);
  i2c(0x30, 0x12);
  i2c(0x31, 0x13);
  i2c(0x33, 0);
  i2c(0x39, 0x12);
  i2c(0x63, 0xc2);
  i2c(4, 0);	/* enable DAC, power up */
}

void ctel_off(void)
{
  i2c(4, 0xc1);
}

void map_io(void)
{
  fbd = open("/dev/fb0", O_RDWR);
  ioctl(fbd, FBIO_GET_PHYS, &fb_info);
  fprintf(stderr,"FB at 0x%x, len %d, LCDDA at 0x%x\n", fb_info.fb_phys, fb_info.fb_len, fb_info.lcdd_phys);
  
  int mem = open("/dev/mem", O_RDWR);
  if (!mem) {
    perror("/dev/mem");
    exit(1);
  }
  lcd = mmap(0, 0x60, PROT_READ|PROT_WRITE, MAP_SHARED, mem, 0x13050000);
  if (!lcd) {
    perror("lcd");
    exit(2);
  }
  lcdd = mmap(0, 0x10, PROT_READ|PROT_WRITE, MAP_SHARED, mem, fb_info.lcdd_phys);
  if (!lcdd) {
    perror("lcdd");
    exit(3);
  }
  cpm = mmap(0, 0x78, PROT_READ|PROT_WRITE, MAP_SHARED, mem, 0x10000000);
  if (!cpm) {
    perror("cpm");
    exit(4);
  }
  gpio = mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, mem, 0x10010000);
  if (!gpio) {
    perror("gpio");
    exit(5);
  }
  io = mmap(0, (64 << 20), PROT_READ|PROT_WRITE, MAP_SHARED, mem, 0x10000000);
  if (!io) {
    perror("io");
    exit(6);
  }
}

void slcd_off(void)
{
    ioctl(fbd, FBIO_REFRESH_EVENTS);
}

void slcd_on(void)
{
  ioctl(fbd, FBIODISPON);
  ioctl(fbd, FBIO_REFRESH_ALWAYS);
}

void lcdc_on(int pal)
{
  unsigned int old_cpccr = cpm[0];
  unsigned int old_cppcr = cpm[0x10/4];
  unsigned int old_lpcdr = cpm[0x64/4];
  fprintf(stderr,"cpccr 0x%x cppcr 0x%x lpcdr 0x%x\n", old_cpccr, old_cppcr, old_lpcdr);

    
  cpm[0] = 0x40432220; //|= 0x10000;
  cpm[0x10/4] = 0x1b000520;
  cpm[0x64/4] = 0xd;

  lcdd[0] = fb_info.lcdd_phys; // pointing to ourself is what the specs tell us to do for a single screen
  lcdd[1] = fb_info.fb_phys; // framebuffer address
  lcdd[2] = 0xdeadbeef; /* user data; we don't use it */
  lcdd[3] = 0xc0009600; /* interrupts on, 153600 bytes buffer length */
  
  lcd[0/4] = 0x900;
  lcd[4/4] = 0xa;

  if (pal) {
    lcd[8/4] = 0x7d;
    lcd[0xc/4] = 0x036c0112;
    lcd[0x10/4] = 0x02240364;
    lcd[0x14/4] = 0x1b010b;
  }
  else {
    lcd[8/4] = 0x3c;
    lcd[0xc/4] = 0x02e00110;
    lcd[0x10/4] = 0x019902d9;
    lcd[0x14/4] = 0x1d010d;
  }
    
  lcd[0x18/4] = 0;
  lcd[0x1c/4] = 0;
  lcd[0x20/4] = 0;
  lcd[0x24/4] = 0;
  lcd[0x34/4] = 0; // reset status register
  
  gpio[0x214/4] = 0x200000;
  gpio[0x228/4] = 0x020000;
  gpio[0x234/4] = 0x280000;
  gpio[0x244/4] = 0x080000;
  gpio[0x264/4] = 0x200000;
  
  lcd[0x40/4] = fb_info.lcdd_phys; // lcd descriptor address
  lcd[0x30/4] = 0x2000000c;// | 0x3f80;	// 16-word burst length, enabled, 15/16 bpp (and mask all interrupts)
}

void lcdc_off(void)
{
  lcd[0] = 0x80000000;
}

int main(int argc, char **argv)
{
  int tvon = 1;
  int pal = 0;
  if (argc > 1) {
    if (!strcmp(argv[1], "pal")) pal = 1;
    else if (!strcmp(argv[1], "ntsc")) pal = 0;
    else if (!strcmp(argv[1], "off")) tvon = 0;
  }
  
  map_io();

  ctel_off();
    
  if (tvon) {
    slcd_off();
    lcdc_on(pal);
    ctel_on(pal);
  }
  else {
    if (!(lcd[0] & 0x80000000)) {
      fprintf(stderr,"SHUTTING DOWN!\n");
      lcdc_off();
      slcd_on();
    }
  }
  
  int i;
#if 1
  for (i = 0; i < 0x60; i+=4) {
    fprintf(stderr, "0x%x: 0x%x\n", 0x13050000 + i, lcd[i/4]);
  }
#endif

  if (argc > 2 && !strcmp(argv[2], "debug")) {
    char addr[20], val[20], action[5];
    for(;;) {
      fscanf(stdin, "%s %s %s", action, addr, val);
      unsigned int naddr = strtoul(addr, 0, 0) / 4;
      unsigned int nval = strtoul(val, 0, 0);
      if (action[0] == 'w') {
        fprintf(stderr,"0x%08x = 0x%x\n", naddr*4 + 0x10000000, nval);
        io[naddr] = nval;
      }
      else {
        fprintf(stderr,"0x%08x: 0x%08x\n", naddr*4 + 0x10000000, io[naddr]);
      }
    }
  }

  close(fbd);
  return 0;
}
