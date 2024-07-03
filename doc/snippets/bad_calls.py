

#### This script checks if Ghidra misgenerated near call references ####

def ubytes(bs):
  return map(lambda b: b & 0xff, bs)

def check_near_calls():
  instructions = currentProgram.getListing().getInstructions(True)
  while instructions.hasNext():
      instruction = instructions.next()
      if instruction.getMnemonicString() == "CALL" and instruction.getDefaultOperandRepresentation(0).startswith("0x"):
        ibs = ubytes(instruction.getBytes())
        if ibs[0] == 0xE8:
          call_address = instruction.getAddress()
          cseg = call_address.getSegment()
          cofs = call_address.getSegmentOffset()
          disp = ibs[2]*0x100 + ibs[1]
          proper_ofs = (cofs+3 + disp)&0xFFFF
          refs = getReferencesFrom(call_address)
          for ref in refs:
            if ref.getReferenceType().toString() == "UNCONDITIONAL_CALL":
              adr = ref.getToAddress()
              seg = adr.getSegment()
              ofs = adr.getSegmentOffset()
              fadr = ref.getFromAddress()
              fseg = fadr.getSegment()
              fofs = fadr.getSegmentOffset()
              disp = ibs[2]*0x100 + ibs[1]
              proper_ofs = (fofs+3 + disp)&0xFFFF
              if ofs != proper_ofs:
                print("Call at {:04X}:{:04X}".format(fseg,fofs))
                print("  target is {:04X}:{:04X} but should be {:04X}:{:04X}"
                  .format(seg,ofs,cseg,proper_ofs))

check_near_calls()





#### This script fixes misgenerated near call references ####
from ghidra.program.model.symbol import RefType, SourceType

# Required to add references
reference_manager = currentProgram.getReferenceManager()

def ubytes(bs):
  return map(lambda b: b & 0xff, bs)

def fix_near_calls():
  instructions = currentProgram.getListing().getInstructions(True)
  while instructions.hasNext():
      instruction = instructions.next()
      if instruction.getMnemonicString() == "CALL" and instruction.getDefaultOperandRepresentation(0).startswith("0x"):
        ibs = ubytes(instruction.getBytes())
        if ibs[0] == 0xE8:
          call_address = instruction.getAddress()
          cseg = call_address.getSegment()
          cofs = call_address.getSegmentOffset()
          disp = ibs[2]*0x100 + ibs[1]
          proper_ofs = (cofs+3 + disp)&0xFFFF
          refs = getReferencesFrom(call_address)
          needs_fix = 0
          for ref in refs:
            if ref.getReferenceType().toString() == "UNCONDITIONAL_CALL":
              adr = ref.getToAddress()
              seg = adr.getSegment()
              ofs = adr.getSegmentOffset()
              fadr = ref.getFromAddress()
              fseg = fadr.getSegment()
              fofs = fadr.getSegmentOffset()
              if seg != cseg or ofs != proper_ofs: needs_fix = 1
          if needs_fix:
            # Likely all references are invalid
            for ref in refs: removeReference(ref)
            # Create the correct reference
            proper_adr = toAddr((cseg << 4) + proper_ofs)
            reference_manager.addMemoryReference(call_address, proper_adr, RefType.UNCONDITIONAL_CALL, SourceType.USER_DEFINED, 0)

fix_near_calls()
