; RULES.ASI provide macros for to easily specify symbols define in .c files
        INCLUDE RULES.ASI

Code_seg@

DOS_GetExtendedErrorInfo equ 59h
ERROR_SHARING_VIOLATION equ 20h
ERROR_DEV_NOT_EXIST equ 37h


ExtSym@         GSavedErrorHandler, DWORD, __CDECL__
ExtSym@         GIOErrorCode, WORD, __CDECL__
ExtSym@         GIOErrorRetryCount, BYTE, __CDECL__


public _ioErrorHndlr
_ioErrorHndlr proc
    PUSH      AX
    PUSH      BX
    PUSH      CX
    PUSH      DX
    PUSH      SI
    PUSH      DI
    PUSH      BP
    PUSH      DS
    PUSH      ES
    ;; DOS: GET EXTENDED ERROR INFORMATION 
    ;;  Return: AX = extended error code (see #01680)
    ;;   BH = error class (see #01682)
    ;;   BL = recommended action (see #01683)
    ;;   CH = error locus (see #01684)
    ;;   ES:DI may be pointer (see #01681, #01680)
    ;;   CL, DX, SI, BP, and DS destroyed
    MOV       AH,DOS_GetExtendedErrorInfo
    XOR       BX,BX
    INT       21h
    POP       ES
    POP       DS
    POP       BP
    POP       DI
    POP       SI
    POP       DX
    POP       CX
    POP       BX
    MOV       [_GIOErrorCode],AX
    CMP       AX,ERROR_SHARING_VIOLATION
    JZ        prompt_for_new_input
    CMP       [_GIOErrorRetryCount],0
    JZ        pass_to_parent_handler
    CMP       AX,ERROR_DEV_NOT_EXIST
    JNZ       pass_to_parent_handler
    DEC       [_GIOErrorRetryCount]
    POP       AX
    MOV       AL,1 ;Ask DOS to retry
    IRET
prompt_for_new_input label near
    POP       AX
    MOV       AL,3 ;prompt user to re-enter input 
    IRET
pass_to_parent_handler  label near
    POP       AX
    JMP       FAR [_GSavedErrorHandler]
_ioErrorHndlr endp


Code_EndS@

        end
        