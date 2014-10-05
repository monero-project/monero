; for Unbound
; severity default Success Informational Warning Error

; .bin file created with:
; "/c/Program Files/Microsoft SDKs/Windows/v6.1/Bin/mc" -c gen_msg.mc
; mv MSG00001.bin gen_msg.bin
; rm gen_msg.h
; and pasted contents of gen_msg.rc into rsrc_unbound.rc

FacilityNames=(Server=0x1)
MessageIdTypeDef=DWORD

MessageID=0x1
Severity=Success
Facility=Server
SymbolicName=MSG_GENERIC_SUCCESS
Language=English
%1
.

MessageID=0x2
Severity=Informational
Facility=Server
SymbolicName=MSG_GENERIC_INFO
Language=English
%1
.

MessageID=0x3
Severity=Warning
Facility=Server
SymbolicName=MSG_GENERIC_WARN
Language=English
%1
.

MessageID=0x4
Severity=Error
Facility=Server
SymbolicName=MSG_GENERIC_ERR
Language=English
%1
.

