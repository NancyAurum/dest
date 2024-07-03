#### This script checks if Ghidra misgenerated references to the 0000:XXXX ###

from ghidra.program.model.address import SegmentedAddressSpace

def ubytes(bs):
  return map(lambda b: b & 0xff, bs)

def check_bad_refs():
  instructions = currentProgram.getListing().getInstructions(True)
  while instructions.hasNext():
      instruction = instructions.next()
      ref_address = instruction.getAddress()
      cseg = ref_address.getSegment()
      cofs = ref_address.getSegmentOffset()
      refs = getReferencesFrom(ref_address)
      for ref in refs:
        adr = ref.getToAddress()
        adrspc = adr.getAddressSpace()
        if isinstance(adrspc, SegmentedAddressSpace):  # adrspc.isMemorySpace():
          seg = adr.getSegment()
          ofs = adr.getSegmentOffset()
          fadr = ref.getFromAddress()
          fseg = fadr.getSegment()
          fofs = fadr.getSegmentOffset()
          if seg == 0:
            print("Instruction at {:04X}:{:04X}".format(fseg,fofs))
            print("  target is {:04X}:{:04X}"
              .format(seg,ofs,cseg))

check_bad_refs()



#### This script clears the misgenerated references to the 0000:XXXX ###

from ghidra.program.model.address import SegmentedAddressSpace

def ubytes(bs):
  return map(lambda b: b & 0xff, bs)

def clear_bad_refs():
  instructions = currentProgram.getListing().getInstructions(True)
  while instructions.hasNext():
      instruction = instructions.next()
      ref_address = instruction.getAddress()
      cseg = ref_address.getSegment()
      cofs = ref_address.getSegmentOffset()
      refs = getReferencesFrom(ref_address)
      for ref in refs:
        adr = ref.getToAddress()
        adrspc = adr.getAddressSpace()
        if isinstance(adrspc, SegmentedAddressSpace):  # adrspc.isMemorySpace():
          seg = adr.getSegment()
          ofs = adr.getSegmentOffset()
          fadr = ref.getFromAddress()
          fseg = fadr.getSegment()
          fofs = fadr.getSegmentOffset()
          if seg == 0:
            removeReference(ref)

clear_bad_refs()
