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

adr = getState().getCurrentAddress()

# For each of the 544 seg:ofs far pointers at the current address,
# print associated 0-ended C-string:
for i in range(544):
  sadr = toAddr((getShort(adr.add(i*4 + 2)) << 4) + getShort(adr.add(i*4)))
  print(getCstr(sadr))


