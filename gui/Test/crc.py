import sys
import binascii
import zlib

buf = bytearray('The quick brown fox jumps over the lazy dog'.encode('utf-8'))

print(hex(zlib.crc32(buf)))
print(' ')
print(hex(binascii.crc32(buf)))




  

