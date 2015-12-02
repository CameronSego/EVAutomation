
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

FILE * device_file = 0;

int sendack(const uint8_t * data, size_t data_size)
{
  printf("sending: { ");
  for(int i = 0 ; i < data_size ; i ++)
    printf("%02X ", data[i]);
  printf("}\n");

  fwrite(data, 1, data_size, device_file);

  uint8_t * ackdata = malloc(data_size);
  fread(ackdata, 1, data_size, device_file);

  if(ferror(device_file))
    printf("Error!\n");

  printf("received: { ");
  for(int i = 0 ; i < data_size ; i ++)
    printf("%02X ", ackdata[i]);
  printf("}\n");

  if(memcmp(data, ackdata, data_size) == 0)
  {
    free(ackdata);
    return 1;
  }
  else
  {
    free(ackdata);
    return 0;
  }
}

void listen(uint16_t arbid, uint16_t mask)
{
  printf("Listening for arbid %X with mask %X... ", arbid, mask);
  fflush(stdout);
  uint8_t startcode[] = {
    '%', 
    'l', 
    (arbid >> 8) & 0xFF,
    arbid & 0xFF,
    (mask >> 8) & 0xFF,
    mask & 0xFF
  };
  if(sendack(startcode, sizeof(startcode)/sizeof(*startcode)))
  {
    printf("Acknowledged!\n");
    fflush(stdout);
  }
  else
  {
    printf("Bad acknowledgement.\n");
    fflush(stdout);
    return;
  }

  while(1)
  {
    uint8_t logdata[4+8];
    fread(logdata, 1, sizeof(logdata)/sizeof(*logdata), device_file);

    uint32_t timestamp = 0;
    timestamp |= logdata[0] << 24;
    timestamp |= logdata[1] << 16;
    timestamp |= logdata[2] <<  8;
    timestamp |= logdata[3];

    printf("%u,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X\n", timestamp,
        logdata[4],logdata[5],logdata[6],logdata[7],
        logdata[8],logdata[9],logdata[10],logdata[11]);
  }
}
void transmit(uint16_t arbid, uint8_t * data)
{
  uint8_t startcode[] = {
    '%', 
    't', 
    (arbid >> 8) & 0xFF, 
    arbid & 0xFF, 
    data[0],
    data[1],
    data[2],
    data[3],
    data[4],
    data[5],
    data[6],
    data[7]
  };

  if(sendack(startcode, sizeof(startcode)/sizeof(*startcode)))
  {
    printf("Acknowledged!\n");
    fflush(stdout);
  }
  else
  {
    printf("Bad acknowledgement.\n");
    fflush(stdout);
  }
}

void printhelp()
{
  printf(
"Usage: CANSniffer <device/file> -t <arbid (hex)> B0.B1.B2.B3.B4.B5.B6.B7 (hex)\n"
"       CANSniffer <device/file> -l <arbid (hex)>\n"
"       CANSniffer <device/file> -l <arbid (hex)> <mask (hex)>\n"
  );
  exit(1);
}

int main(int argc, char ** argv)
{
  if(argc < 4)
  {
    printhelp();
    return 1;
  }
  else
  {
    device_file = fopen(argv[1], "a+b");

    if(!device_file)
    {
      printf("Can't open `%s` for r/w.\n", argv[1]);
      return 1;
    }

    if(strcmp(argv[2], "-t") == 0)
    {
      if(argc < 5) {
        printhelp();
        return 1;
      }

      uint16_t arbid;
      sscanf(argv[3], "%X", &arbid);
      printf("Transmitting arbid: %X\n", arbid);
      uint32_t data32[8];
      sscanf(argv[4], "%2X.%2X.%2X.%2X.%2X.%2X.%2X.%2X", data32+0,data32+1,data32+2,data32+3,data32+4,data32+5,data32+6,data32+7);

      uint8_t data[8];
      for(int i = 0 ; i < 8 ; i ++)
        data[i] = data32[i];
      printf("data: { %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X }\n", data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);

      transmit(arbid, data);
    }
    else if(strcmp(argv[2], "-l") == 0)
    {
      uint16_t arbid, mask;
      sscanf(argv[3], "%X", &arbid);
      mask = 0xffff;

      if(argc >= 5) {
        sscanf(argv[4], "%X", &mask);
      }

      listen(arbid, mask);
    }

    fclose(device_file);
    device_file = 0;
  }

  return 0;
}
