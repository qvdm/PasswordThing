import sys

buf = bytearray('The quick brown fox jumps over the lazy dog'.encode('utf-8'))


def calc_crc() :
  crc_table =  [0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
                0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
                0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c ]


  crc = ~0;
  
  for index in range(len(buf)) :
    eebyte = buf[index]
     
    crc = crc_table[(crc ^ eebyte) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (eebyte >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;

  return crc;


gcrc = calc_crc()
print(hex(gcrc))









  

