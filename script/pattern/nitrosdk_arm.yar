rule FS_InitFile_SDK_2_0_ARM {
    meta:
        name = "FS_InitFile"
        sdk_version = "2.0"
    strings:
        $code = {
            00 30 A0 E3
            00 30 80 E5
            00 20 90 E5
            (0C | 0E) 10 A0 E3
            04 20 80 E5
            B8 31 C0 E1
            08 30 80 E5
            10 10 80 E5
        }
    condition:
        $code
}
        
rule FS_InitFile_SDK_3_0_ARM {
    meta:
        name = "FS_InitFile"
        sdk_version = "3.0"
    strings:
        $code = {
            00 30 A0 E3 
            00 30 80 E5
            00 20 90 E5 
            0E 10 A0 E3
            04 20 80 E5
            1C 30 80 E5
            1C 20 90 E5
            18 20 80 E5
        }

        $code2 = {
            00 20 A0 E3 
            00 20 80 E5 
            04 20 80 E5 
            1C 20 80 E5 
            18 20 80 E5 
            08 20 80 E5 
            0E 10 A0 E3 
            10 10 80 E5 
        }
    condition:
        $code or $code2
}
        
rule FS_InitFile_SDK_4_0_ARM {
    meta:
        name = "FS_InitFile"
        sdk_version = "4.0"
    strings:
        $code = {
            00 20 A0 E3 
            00 20 80 E5 
            04 20 80 E5 
            1C 20 80 E5 
            18 20 80 E5 
            08 20 80 E5
            0E 10 A0 E3
            10 10 80 E5 
        }

        $code2 = {
            00 20 A0 E3 
            0E 10 A0 E3 
            00 20 80 E5 
            04 20 80 E5 
            1C 20 80 E5 
            18 20 80 E5 
            08 20 80 E5 
            10 10 80 E5 
        }

    condition:
        $code or $code2
}

rule FS_InitFile_SDK_5_0_ARM {
    meta:
        name = "FS_InitFile"
        sdk_version = "5.0"
    strings:
        $code = {
            00 20 A0 E3 
            23 1C 82 E3 
            08 20 80 E5 
            04 20 80 E5 
            00 20 80 E5 
            1C 20 80 E5 
            18 20 80 E5 
            0C 10 80 E5
        }

        $code2 = {
            00 20 A0 E3  
            08 20 80 E5 
            04 20 80 E5 
            00 20 80 E5 
            1C 20 80 E5 
            18 20 80 E5 
            23 1C 82 E3
            0C 10 80 E5
        }
    condition:
        $code or $code2
}

rule FS_OpenFile_SDK_2_0_ARM {
    meta:
        name = "FS_OpenFile"
        sdk_version = "2.0"
    strings:
        $code = {
            10 40 2D E9 
            08 D0 4D E2 
            00 40 A0 E1 
            00 00 8D E2 
            ?? ?? ?? EB 
            00 00 50 E3 
            (08 | 07) 00 00 0A 
            00 10 8D E2 
            04 00 A0 E1 
            06 00 91 E8 
            ?? ?? ?? EB 
        }

        $code2 = {
            38 40 2D E9 
            08 D0 4D E2 
            00 40 8D E2 
            00 50 A0 E1 
            04 00 A0 E1 
            ?? ?? ?? EB 
            00 00 50 E3 
            06 00 00 0A 
            05 00 A0 E1 
            06 00 94 E8 
            ?? ?? ?? EB 
        }
    condition:
        $code or $code2
}

rule FS_OpenFile_SDK_5_0_ARM {
    meta:
        name = "FS_OpenFile"
        sdk_version = "5.0"
    strings:
        $code = {
            04 C0 9F E5 
            01 20 A0 E3 
            1C FF 2F E1 
            ?? ?? ?? 02
        }
    condition:
        $code
}

rule FS_OpenFileEx_SDK_2_0_ARM {
    meta:
        name = "FS_OpenFileEx"
        sdk_version = "2.0"
    condition:
        false
}

rule FS_OpenFileEx_SDK_5_0_ARM {
    meta:
        name = "FS_OpenFileEx"
        sdk_version = "5.0"
    strings:
        $code = {
            F8 40 2D E9 
            4A DF 4D E2 
            00 40 A0 E1 
            02 70 A0 E1 
            01 00 A0 E1 
            00 60 A0 E3 
            00 10 8D E2 
            10 20 8D E2 
            00 60 8D E5 
            ?? ?? ?? EB 
        }

        $code2 = {
            F8 43 2D E9 
            4A DF 4D E2 
            10 40 8D E2 
            00 70 A0 E1 
            01 00 A0 E1 
            02 60 A0 E1 
            00 50 A0 E3 
            00 10 8D E2 
            04 20 A0 E1 
            00 50 8D E5 
            ?? ?? ?? EB 
        }
    condition:
        $code or $code2
}

rule FS_ReadFile_SDK_2_0_ARM {
    meta:
        name = "FS_ReadFile"
        sdk_version = "2.0"
    strings:
        $code = {
            04 C0 9F E5
            00 30 A0 E3
            1C FF 2F E1
            ?? ?? ?? 02
        }
    condition:
        $code
}

rule FS_ReadFile_SDK_5_0_ARM {
    meta:
        name = "FS_ReadFile"
        sdk_version = "5.0"
    strings:
        $code = {
            38 40 2D E9 
            08 D0 4D E2 
            00 30 8D E2 
            00 50 A0 E1 
            10 30 85 E5 
            00 10 8D E5 
            01 40 A0 E3 
            04 20 8D E5 
            04 20 A0 E1 
            00 10 A0 E3 
            ?? ?? ?? EB 
        }

        $code2 = {
            10 40 2D E9 
            08 D0 4D E2 
            00 30 8D E2 
            00 40 A0 E1 
            10 30 84 E5 
            00 10 8D E5 
            04 20 8D E5 
            00 10 A0 E3 
            01 20 A0 E3 
            ?? ?? ?? EB 
        }
    condition:
        $code or $code2
}

rule FSi_ReadFileCore_SDK_2_0_ARM {
    meta:
        name = "FSi_ReadFileCore"
        sdk_version = "2.0"
    strings:
        $code = {
            (F0 | F8) (40 | 41) 2D E9 
            [0-4]
            00 70 A0 E1 
            (28 | 2C) 40 97 E5 
            (24 | 28) 00 97 E5 
            02 60 A0 E1 
        }
    condition:
        $code
}

rule FSi_ReadFileCore_SDK_5_0_ARM {
    meta:
        name = "FSi_ReadFileCore"
        sdk_version = "5.0"
    condition:
        false
}

rule FS_SeekFile_SDK_2_0_ARM {
    meta:
        name = "FS_SeekFile"
        sdk_version = "2.0"
    strings:
        $code = {
            00 00 52 E3 
            04 00 00 0A 
            01 00 52 E3 
            05 00 00 0A 
            02 00 52 E3 
            0? 00 00 0A 
            0? 00 00 EA 
            (24 | 20) 20 90 E5 
        }
    condition:
        $code
}

rule FS_SeekFile_SDK_5_0_ARM {
    meta:
        name = "FS_SeekFile"
        sdk_version = "5.0"
    strings:
        $code = {
            08 40 2D E9 
            08 D0 4D E2 
            00 30 8D E2 
            10 30 80 E5 
            00 10 8D E5 
            04 20 8D E5 
            0E 10 A0 E3 
            01 20 A0 E3 
            ?? ?? ?? EB 
        }

        $code2 = {
            70 40 2D E9 
            08 D0 4D E2 
            00 60 A0 E1 
            01 50 A0 E1 
            02 40 A0 E1 
            ?? ?? ?? EB 
            00 00 50 E3 
            08 D0 8D 12 
            70 80 BD 18 
            00 10 8D E2 
            10 10 86 E5 
            06 00 A0 E1 
            0E 10 A0 E3 
            01 20 A0 E3 
        }
    condition:
        $code or $code2
}

rule FS_GetLength_SDK_2_0_ARM {
    meta:
        name = "FS_GetLength"
        sdk_version = "2.0"
    condition:
        false
}

rule FS_GetLength_SDK_5_0_ARM {
    meta:
        name = "FS_GetLength"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? 40 2D E9 
            08 D0 4D E2 
            00 ?0 A0 E3 
            04 10 8D E2 
            00 ?0 A0 E1 
            04 ?0 8D E5 
            ?? ?? ?? EB 
            00 00 50 E3 
            0? 00 00 1A 
            00 10 8D E2 
            10 10 8? E5 
            [0-4]
            0? 00 A0 E1 
            0F 10 A0 E3 
            01 20 A0 E3 
            00 ?0 8D E5 
            ?? ?? ?? EB 
        }
    condition:
        $code
}

rule FS_CloseFile_SDK_2_0_ARM {
    meta:
        name = "FS_CloseFile"
        sdk_version = "2.0"
    strings:
        $code = {
            10 40 2D E9 
            08 10 A0 E3 
            00 40 A0 E1 
            ?? ?? ?? EB 
            00 00 50 E3 
            00 00 A0 ?3 
            10 ?0 BD 08 
        }
    condition:
        $code
}

rule FS_CloseFile_SDK_5_0_ARM {
    meta:
        name = "FS_CloseFile"
        sdk_version = "5.0"
    strings:
        $code = {
            08 C0 9F E5 
            08 10 A0 E3 
            01 20 A0 E3 
            1C FF 2F E1 
            ?? ?? ?? 02
        }
    condition:
        $code
}

rule FSi_SendCommand_SDK_2_0_ARM {
    meta:
        name = "FSi_SendCommand"
        sdk_version = "2.0"
    condition:
        false
}

rule FSi_SendCommand_SDK_5_0_ARM {
    meta:
        name = "FSi_SendCommand"
        sdk_version = "5.0"
    strings:
        $code = {
            F8 43 2D E9 
            00 90 A0 E1 
            0C 00 99 E5 
            00 ?0 A0 E3 
            01 00 10 E3 
        } 
    condition:
        $code
}

rule FS_LoadOverlay_SDK_2_0_ARM {
    meta:
        name = "FS_LoadOverlay"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? 40 2D E9 
            2? D0 4D E2 
            00 30 A0 E1 
            01 20 A0 E1 
            00 00 8D E2 
            03 10 A0 E1 
            ?? ?? ?? EB 
            00 00 50 E3 
            0? 00 00 0A 
            00 00 8D E2 
            ?? ?? ?? EB 
            00 00 50 E3
            0? 00 00 1A
            2? D0 8D E2 
            00 00 A0 E3 
            (
            00 40 BD E8 
            1E FF 2F E1 
            | 
            00 80 BD E8
            )
            00 00 8D E2 
            ?? ?? ?? EB 
        }
    condition:
        $code
}

rule FS_LoadOverlay_SDK_5_0_ARM {
    meta:
        name = "FS_LoadOverlay"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? 40 2D E9 
            2C D0 4D E2 
            [0-4]
            00 30 A0 E1 
            01 20 A0 E1 
            ?? ?? ?? ??
            03 10 A0 E1 
            00 ?0 A0 E3 
            ?? ?? ?? EB 
            00 00 50 E3 
            06 00 00 0A 
            ?? ?? ?? ?? 
            ?? ?? ?? EB 
            00 00 50 E3 
            02 00 00 0A 
            ?? ?? ?? ??
            ?? ?? ?? EB 
        }
    condition:
        $code
}

rule FS_LoadOverlayInfo_SDK_2_0_ARM {
    meta:
        name = "FS_LoadOverlayInfo"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? 40 2D E9 
            ?? D0 4D E2 
            01 40 B0 E1 
            00 50 A0 E1 
            ?? 0? 9F 05 
            ?? 0? 9F 15 
            00 ?0 90 E5 
            00 00 5? E3 
            ?? 00 00 0A 
        }
    condition:
        $code
}

rule FS_LoadOverlayInfo_SDK_5_0_ARM {
    meta:
        name = "FS_LoadOverlayInfo"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? 4? 2D E9 
            A0 D0 4D E2 
            01 ?0 B0 E1 
            ?? ?1 9F 05 
            00 ?0 A0 E1 
            ?? ?1 9F 15 
            82 ?2 A0 E1 
            00 00 9? E5 
            00 ?0 A0 E3 
            00 00 50 E3 
            1? 00 00 0A 
        }

        $code2 = {
            F8 43 2D E9 
            A0 D0 4D E2 
            01 40 B0 E1 
            00 50 A0 E1 
            ?? 9? 9F 05 
            ?? 0? 9F E5 
            ?? 9? 9F 15 
            82 82 A0 E1 
            00 70 A0 E3 
            00 00 52 E1 
            02 00 00 3A 
            ?? ?? ?? EB 
        }
    condition:
        $code or $code2
}

rule FS_LoadOverlayImage_SDK_2_0_ARM {
    meta:
        name = "FS_LoadOverlayImage"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? 40 2D E9 
            ?? D0 4D E2 
            00 ?0 A0 E1 
            08 00 8D E2 
            ?? ?? ?? EB 
            00 00 8D E2 
            0? 10 A0 E1 
            ?? ?? ?? EB 
            00 10 8D E2 
            08 00 8D E2 
            06 00 91 E8 
            ?? ?? ?? EB 
        }
    condition:
        $code
}

rule FS_LoadOverlayImage_SDK_5_0_ARM {
    meta:
        name = "FS_LoadOverlayImage"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? 4? 2D E9 
            ?? D0 4D E2 
            [8-12]
            00 ?0 A0 E3 
            ?? ?? ?? EB 
            [0-4]
            0? 10 A0 E1 
            [0-4]
            ?? ?? ?? EB 
            [4-8]
            06 00 9? E8 
            ?? ?? ?? EB 
        }
    condition:
        $code
}

rule FS_StartOverlay_SDK_2_0_ARM {
    meta:
        name = "FS_StartOverlay"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? 40 2D E9 
            00 50 A0 E1 
            ?? ?? ?? EB 
            ?? 1? 9F E5 
            00 40 A0 E1 
            B0 00 D1 E1 
            02 00 50 E3 
            1? 00 00 1A 
            1? 10 ?5 E5 
        }
    condition:
        $code
}

rule FS_StartOverlay_SDK_5_0_ARM {
    meta:
        name = "FS_StartOverlay"
        sdk_version = "5.0"
    strings:
        $code = {
            38 40 2D E9 
            00 50 A0 E1 
            ?? ?? ?? EB 
            00 40 A0 E1 
            ?? ?? ?? EB 
            01 00 50 E3 
            1C 00 00 0A 
            1C 00 95 E5 
            00 30 A0 E3 
        }

        $code2 = {
            38 40 2D E9 
            00 50 A0 E1 
            ?? ?? ?? EB 
            04 20 95 E5 
            ?? 1? 9F E5 
            00 40 A0 E1 
            01 00 52 E1 
            0A 00 00 3A 
            ?? 0? 9F E5 
            00 00 52 E1 
            ?? 00 00 2A 
        }
    condition:
        $code or $code2
}


rule OS_WakeupThreadDirect {
    strings:
        $code = {
            3? 40 2D E9 
            [0-4]
            00 50 A0 E1 
            ?? ?? ?? EB 
            01 10 A0 E3 
            00 40 A0 E1 
            64 10 85 E5 
            ?? ?? ?? EB 
            04 00 A0 E1 
            ?? ?? ?? EB 
            [0-4]
            3? ?0 BD E8 
        }
    condition:
        $code
}

rule OS_SleepThread_SDK_2_0_ARM {
    meta:
        name = "OS_SleepThread"
        sdk_version = "2.0"
    strings:
        $code = {
            30 40 2D E9 
            04 D0 4D E2 
            00 50 A0 E1 
            ?? ?? ?? EB 
            ?? 1? 9F E5 
            00 40 A0 E1 
            00 00 91 E5 
            00 00 55 E3 
            00 20 90 E5 
            06 00 00 0A 
            6C 00 92 E5 
        }
    condition:
        $code
}

rule OS_SleepThread_SDK_3_0_ARM {
    meta:
        name = "OS_SleepThread"
        sdk_version = "3.0"
    strings:
        $code = {
            70 40 2D E9 
            00 60 A0 E1 
            ?? ?? ?? EB 
            ?? 1? 9F E5 
            00 ?0 A0 E1 
            0? 00 91 E5 
            00 00 56 E3 
            00 ?0 90 E5 
            03 00 00 0A 
            06 00 A0 E1 
            0? 10 A0 E1 
            78 60 8? E5 
            ?? ?? ?? EB 
        }
    condition:
        $code
}

rule TP_GetCalibratedPoint_SDK_2_0_ARM {
    meta:
        name = "TP_GetCalibratedPoint"
        sdk_version = "2.0"
    strings:
        $code = {
            70 40 2D E9 
            ?? 2? 9F E5 
            B? 23 D2 E1 
            00 00 52 E3 
            0? 00 00 1A 
            B0 ?0 D1 E1 
            B2 ?0 D1 E1 
            (
            B0 30 C0 E1 
            B2 20 C0 E1 
            |
            B4 ?0 D1 E1 
            B6 ?0 D1 E1 
            )
        }
    condition:
        $code
}

rule OS_AllocFromHeap {
    strings:
        $code = {
            ?? 40 2D E9 
            [0-4]
            00 ?0 A0 E1 
            01 50 A0 E1 
            02 70 A0 E1 
            ?? ?? ?? EB 
            ?? 1? 9F E5 
            00 ?0 A0 E1 
            0? 11 91 E7 
            00 00 51 E3 
            0? 00 00 1A 
            ?? ?? 00 EB 
        }
    condition:
        $code
}

rule OS_FreeToHeap {
    strings:
        $code = {
            ?? ?? 2D E9 
            [0-4]
            00 70 A0 E1 
            01 ?0 A0 E1 
            02 ?0 A0 E1 
            ?? ?? ?? EB 
            ?? 1? 9F E5 
            00 ?0 A0 E1 
            07 01 91 E7 
            00 00 5? E3 
            00 ?0 90 B5 
        }
    condition:
        $code
}

rule FndAllocFromExpHeapEx {
    strings:
        $code = {
            ?? 40 2D E9 
            [0-4]
            00 00 51 E3 
            01 10 A0 03 
            03 10 81 E2 
            00 00 52 E3 
            03 10 C1 E3 
            0? 00 00 BA 
            ?? ?? ?? EB 
            [0-4]
            (
            ?? ?? BD E8 
            1E FF 2F E1|
            ?? ?? BD E8
            )
            00 20 62 E2 
            ?? ?? ?? EB 
        }
    condition:
        $code
}

rule FndFreeToExpHeap {
    strings:
        $code = {
            ?? 40 2D E9 
            ?? D0 4D E2 
            10 ?0 41 E2 
            [0-4]
            00 ?0 A0 E1 
            ?? ?? ?? ??
            0? 10 A0 E1 
            ?? ?? ?? EB 
            0? 10 A0 E1 
            2C 00 8? E2 
            ?? ?? ?? EB 
            ?? ?? ?? ??
            24 00 8? E2 
            ?? ?? ?? EB 
            ?? D0 8D E2 
        }

        $code2 = {
            ?? 40 2D E9 
            ?? D0 4D E2 
            10 40 41 E2 
            24 50 80 E2 
            00 00 8D E2 
            04 10 A0 E1 
            ?? ?? ?? EB 
            04 10 A0 E1 
            08 00 85 E2 
            ?? ?? ?? EB 
            00 10 8D E2 
            05 00 A0 E1 
            ?? ?? ?? EB 
        }
    condition:
        $code or $code2
}

rule OS_IsRunOnTwl_SDK_2_0_ARM {
    meta:
        name = "OS_IsRunOnTwl"
        sdk_version = "2.0"
    condition:
        false
}

rule OS_IsRunOnTwl_SDK_5_0_ARM {
    meta:
        name = "OS_IsRunOnTwl"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? 0? 9F E5 
            1C 00 90 E5 
            00 00 50 E3 
            0? 00 00 1A 
            ?? 0? 9F E5 
            01 20 A0 E3 
            00 00 D0 E5 
            01 10 A0 E3 
        }
    condition:
        $code
}