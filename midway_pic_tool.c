//license:BSD-3-Clause
//copyright-holders:Pat Daderko, Aaron Giles

//Tool to read and (optionally) modify serial number and/or date from PIC16F57 dump used in many Atari/Midway games
//by Pat Daderko (DogP) based on MAME midway_serial_pic_device::generate_serial_data from midwayic.cpp by Aaron Giles
//
//Usage: midway_pic_tool <filename> [new SN (0-999999999)] [new MM (1-12)] [new DD (0-30)] [new YYYY (1980-2155)]
//Enter '-' to leave parameter unchanged

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char *argv[])
{
  uint8_t modfile=0;
  int32_t temp, year, month, day;
  uint32_t serial_number;
  uint8_t pic_data[16], serial_digit[9];
  FILE *binfile;

  //print usage?
  if ((argc<2) || (argc>6))
  {
    printf("Usage: %s <filename> [new SN (0-999999999)] [new MM (1-12)] [new DD (0-30)] [new YYYY (1980-2155)]\nEnter '-' to leave parameter unchanged\nNOTE: Entering new values modifies original file\n", argv[0]);
    return 1;
  }
  
  //open dump file
  binfile = fopen(argv[1], "r+b");
  if (binfile==NULL)
  {
    printf("Error opening file\n");
    return 1;
  }
  
  //make sure file is at least as long as necessary (will read/modify bytes 0-30)
  fseek(binfile, 0, SEEK_END);
  if (ftell(binfile)<31)
  {
    printf("Error: File size too small\n");
    fclose(binfile);
    return 1;
  }
  fseek(binfile, 0, SEEK_SET);

  
  //PIC data is scrambled in flash dump (order looks to be identical for all dumps)
  //from disassembly of PIC code, order from flash dump to pic_data array (0-15) is: 6, 14, 3, 8, 0, 12, 7, 9, 11, 1, 15, 4, 2, 10, 5, 13
  //need to skip RETLW opcodes (0x08) in file
  fseek(binfile, 6*2, SEEK_SET);
  pic_data[0]=fgetc(binfile);
  fseek(binfile, 14*2, SEEK_SET);
  pic_data[1]=fgetc(binfile);
  fseek(binfile, 3*2, SEEK_SET);
  pic_data[2]=fgetc(binfile);
  fseek(binfile, 8*2, SEEK_SET);
  pic_data[3]=fgetc(binfile);
  fseek(binfile, 0*2, SEEK_SET);
  pic_data[4]=fgetc(binfile);
  fseek(binfile, 12*2, SEEK_SET);
  pic_data[5]=fgetc(binfile);
  fseek(binfile, 7*2, SEEK_SET);
  pic_data[6]=fgetc(binfile);
  fseek(binfile, 9*2, SEEK_SET);
  pic_data[7]=fgetc(binfile);
  fseek(binfile, 11*2, SEEK_SET);
  pic_data[8]=fgetc(binfile);
  fseek(binfile, 1*2, SEEK_SET);
  pic_data[9]=fgetc(binfile);
  fseek(binfile, 15*2, SEEK_SET);
  pic_data[10]=fgetc(binfile);
  fseek(binfile, 4*2, SEEK_SET);
  pic_data[11]=fgetc(binfile);
  fseek(binfile, 2*2, SEEK_SET);
  pic_data[12]=fgetc(binfile);
  fseek(binfile, 10*2, SEEK_SET);
  pic_data[13]=fgetc(binfile);
  fseek(binfile, 5*2, SEEK_SET);
  pic_data[14]=fgetc(binfile); //looks to be unused
  fseek(binfile, 13*2, SEEK_SET);
  pic_data[15]=fgetc(binfile); //looks to be unused


  //determine original serial number
  temp = ((((pic_data[9]<<16)|(pic_data[8]<<8)|pic_data[7])-0x1f3f0)/0x1bcd)-5*pic_data[13];
  serial_digit[1]=(temp/100)%10;
  serial_digit[7]=(temp/10)%10;
  serial_digit[4]=temp%10;
  
  temp = ((((pic_data[6]<<24)|(pic_data[5]<<16)|(pic_data[4]<<8)|pic_data[3])-0x71e259)/0x107f)-2*pic_data[13]-pic_data[12];
  serial_digit[2]=(temp/10000)%10;
  serial_digit[0]=(temp/100)%10;
  serial_digit[8]=(temp/10)%10;
  serial_digit[6]=temp%10;

  temp=((((pic_data[2]<<16)|(pic_data[1]<<8)|pic_data[0])-0x3d74)/0x245)-pic_data[12];
  serial_digit[3]=(temp/100)%10;
  serial_digit[5]=(temp/10)%10;
  
  serial_number=serial_digit[0]*100000000 + serial_digit[1]*10000000 + serial_digit[2]*1000000 + serial_digit[3]*100000 + serial_digit[4]*10000 + serial_digit[5]*1000 + serial_digit[6]*100 + serial_digit[7]*10 + serial_digit[8];

  printf("Original S/N: %09d\n",serial_number);

  
  //determine original date
  temp=((pic_data[10]<<8)|pic_data[11]);
  year=(temp/0x174)+1980;
  temp-=((year-1980)*0x174);
  month=(temp/0x1f)+1;
  day=temp-((month-1)*0x1f); //day appears to be 0-30, not 1-31 (as displayed by Rush games at least)
  
  printf("Original date: %02d/%02d/%04d\n",month,day,year);


  //change serial number (if desired)
  if ((argc>2) && strcmp(argv[2],"-"))
  {
    temp=atoi(argv[2]);
    if ((temp>=0) && (temp<=999999999))
    {
      serial_number=temp;
      modfile=1;
    }
    else
      printf("Warning: Invalid S/N entered\n");
  }


  //change date (if desired)
  if ((argc>3) && (strcmp(argv[3],"-")))
  {
    temp=atoi(argv[3]);
    if ((temp>=1) && (temp<=12))
    {
      month = atoi(argv[3]);
      modfile=1;
    }
    else
      printf("Warning: Invalid month entered\n");
  }
  if ((argc>4) && (strcmp(argv[4],"-")))
  {
    temp=atoi(argv[4]);
    if ((temp>=0) && (temp<=30))
    {
      day=temp;
      modfile=1;
    }
    else
      printf("Warning: Invalid day entered\n");
  }
  if ((argc>5) && (strcmp(argv[5],"-")))
  {
    temp=atoi(argv[5]);
    if ((temp>=1980) && (temp<=2155))
    {
      year=temp;
      modfile=1;
    }
    else
      printf("Warning: Invalid year entered\n");
  }


  if (modfile==1) //if any values changed, update and modify file
  {
    serial_digit[0] = (serial_number / 100000000) % 10;
    serial_digit[1] = (serial_number / 10000000) % 10;
    serial_digit[2] = (serial_number / 1000000) % 10;
    serial_digit[3] = (serial_number / 100000) % 10;
    serial_digit[4] = (serial_number / 10000) % 10;
    serial_digit[5] = (serial_number / 1000) % 10;
    serial_digit[6] = (serial_number / 100) % 10;
    serial_digit[7] = (serial_number / 10) % 10;
    serial_digit[8] = (serial_number / 1) % 10;
    
    temp = ((serial_digit[4] + serial_digit[7] * 10 + serial_digit[1] * 100) + 5 * pic_data[13]) * 0x1bcd + 0x1f3f0;
    pic_data[7] = temp & 0xff;
    pic_data[8] = (temp >> 8) & 0xff;
    pic_data[9] = (temp >> 16) & 0xff;
    
    temp = ((serial_digit[6] + serial_digit[8] * 10 + serial_digit[0] * 100 + serial_digit[2] * 10000) + 2 * pic_data[13] + pic_data[12]) * 0x107f + 0x71e259;
    pic_data[3] = temp & 0xff;
    pic_data[4] = (temp >> 8) & 0xff;
    pic_data[5] = (temp >> 16) & 0xff;
    pic_data[6] = (temp >> 24) & 0xff;
    
    temp = ((serial_digit[5] * 10 + serial_digit[3] * 100) + pic_data[12]) * 0x245 + 0x3d74;
    pic_data[0] = temp & 0xff;
    pic_data[1] = (temp >> 8) & 0xff;
    pic_data[2] = (temp >> 16) & 0xff;
    
    temp = 0x174 * (year - 1980) + 0x1f * (month - 1) + day;
    pic_data[10] = (temp >> 8) & 0xff;
    pic_data[11] = temp & 0xff;
    
    //put pic_data back into file
    fseek(binfile, 6*2, SEEK_SET);
    fputc(pic_data[0], binfile);
    fseek(binfile, 14*2, SEEK_SET);
    fputc(pic_data[1], binfile);
    fseek(binfile, 3*2, SEEK_SET);
    fputc(pic_data[2], binfile);
    fseek(binfile, 8*2, SEEK_SET);
    fputc(pic_data[3], binfile);
    fseek(binfile, 0*2, SEEK_SET);
    fputc(pic_data[4], binfile);
    fseek(binfile, 12*2, SEEK_SET);
    fputc(pic_data[5], binfile);
    fseek(binfile, 7*2, SEEK_SET);
    fputc(pic_data[6], binfile);
    fseek(binfile, 9*2, SEEK_SET);
    fputc(pic_data[7], binfile);
    fseek(binfile, 11*2, SEEK_SET);
    fputc(pic_data[8], binfile);
    fseek(binfile, 1*2, SEEK_SET);
    fputc(pic_data[9], binfile);
    fseek(binfile, 15*2, SEEK_SET);
    fputc(pic_data[10], binfile);
    fseek(binfile, 4*2, SEEK_SET);
    fputc(pic_data[11], binfile);
    fseek(binfile, 2*2, SEEK_SET);
    fputc(pic_data[12], binfile);
    fseek(binfile, 10*2, SEEK_SET);
    fputc(pic_data[13], binfile);
    fseek(binfile, 5*2, SEEK_SET);
    fputc(pic_data[14], binfile); //looks to be unused
    fseek(binfile, 13*2, SEEK_SET);
    fputc(pic_data[15], binfile); //looks to be unused
    
    printf("New S/N: %09d\n",serial_number);
    printf("New date: %02d/%02d/%04d\n",month,day,year);
  }
  
  fclose(binfile);
  
  return 0;
}
