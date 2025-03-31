rule OS_InitThread_constant {
    meta:
        type = "constant"
    strings:
        $constant = {
            7D 59 DB FD
            5B DD F9 7B
            [0-4]
            A0 FF ?F 02
        }
    condition:
        $constant
}

rule OS_GetInitArenaLo_constant {
    meta:
        type = "constant"
    strings:
        $constant = {
            ?? ?? FF 01 
            ?? ?? (3C | 7C | 7E | FE) 02 
            00 F0 (7F | FF) 02 
        }
    condition:
        $constant
}

rule OS_SaveContext {
    meta:
        type = "arm"
    strings:
        $code = {
            01 40 2D E9 
            48 00 80 E2 
            38 10 9F E5 
            31 FF 2F E1 
            01 40 BD E8 
            00 10 80 E2 
            00 20 0F E1 
            04 20 81 E4 
        }
    condition:
        $code
}

rule OS_IrqHandler {
    meta:
        type = "arm"
    strings:
        $code = {
            00 40 2D E9 
            01 C3 A0 E3 
            21 CE 8C E2 
            08 10 1C E5 
        }
    condition:
        $code
}

rule InitExpHeap_constant {
    meta:
        type = "constant"
    strings:
        $constant = {
            48 50 58 45 [0-4] 52 46 00 00
        }
    condition:
        $constant
}

rule SVC_WaitVBlankIntr {
    meta:
        type = "thumb"
    strings:
        $code = {
            00 22 05 DF 70 47 
        }
    condition:
        $code
}

rule Syscalls {
    meta:
        type = "thumb"
    strings:
        $SVC_SoftReset = {
            00 DF 70 47
        }
        $SVC_WaitByLoop = {
            03 DF 70 47
        }
        $SVC_WaitIntr = {
            00 22 04 DF 70 47 
        }
        $SVC_WaitVBlankIntr = {
            00 22 05 DF 70 47 
        }
        $SVC_Halt = {
            06 DF 70 47 
        }
        $SVC_Div = {
            09 DF 70 47 
        }
        $SVC_DivRem = {
            09 DF 08 1C 70 47 
        }
        $SVC_CpuSet = {
            0B DF 70 47
        }
        $SVC_CpuSetFast = {
            0C DF 70 47
        }
        $SVC_Sqrt = {
            0D DF 70 47
        }
        $SVC_GetCRC16 = {
            0E DF 70 47
        }
        $IsMmemExpanded = {
            0F DF 70 47
        }
        $SVC_UnpackBits = {
            10 DF 70 47
        }
        $SVC_UncompressLZ8 = {
            11 DF 70 47
        }
        $SVC_UncompressLZ16FromDevice = {
            12 DF 70 47
        }
        $SVC_UncompressHuffmanFromDevice = {
            13 DF 70 47
        }
        $SVC_UncompressRL8 = {
            14 DF 70 47
        }
        $SVC_UncompressRL16FromDevice = {
            15 DF 70 47
        }
    condition:
        any of them
}