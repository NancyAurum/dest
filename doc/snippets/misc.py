# Get the offset of the user cursor:
getState().getCurrentAddress().getOffset()

# Ghidra also defines currentAddress and currentProgram:
ghidra.program.model.data.DataUtilities.isUndefinedData(currentProgram, currentAddress)

# Get segmented address components:
adr = getState().getCurrentAddress()
"adr {:04X}:{:04X}".format(adr.getSegment(),adr.getSegmentOffset())

# Increment address:
adr.add(5)

# function to read unsigned short from an address
def getWord(adr):
  return getShort(adr)&0xFFFF

# Convert a far ptr to Ghidra's address
toAddr((getWord(adr.add(2)) << 4) + getWord(adr))

# Print far pointer at adr:
"adr {:04X}:{:04X}".format(getShort(adr),getShort(adr.add(2)))

# Read null-terminated C-string from memory:
def getCstr(addr):
  s = []
  while True:
    try:
      c = getByte(addr)
      if c == 0: break
      s.append(chr(c))
      addr = addr.add(1)
    except MemoryAccessException:
      break
  return ''.join(s)

# Convert signed Java bytes to unsigned:
def ubytes(bytes):
  return map(lambda b: b & 0xff, bytes)

# Print byte list as string:
def hd(bytes):
  return ' '.join('{:02x}'.format(byte&0xff) for byte in bytes)
