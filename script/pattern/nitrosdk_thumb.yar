rule FS_InitFile_SDK_2_0_THUMB {
    meta:
        name = "FS_InitFile"
        sdk_version = "2.0"
    strings:
        $code = {
            00 22 
            02 60 
            01 68 
            41 60 
            02 83 
            82 60 
            0E 21 
            01 61 
        }
    condition:
        $code
}

rule FS_InitFile_SDK_3_0_THUMB {
    meta:
        name = "FS_InitFile"
        sdk_version = "3.0"
    strings:
        $code = {
            00 22 
            02 60 
            01 68 
            41 60 
            C2 61 
            C1 69 
            81 61 
            82 60 
            0E 21 
        }
    condition:
        $code
}

rule FS_InitFile_SDK_4_0_THUMB {
    meta:
        name = "FS_InitFile"
        sdk_version = "4.0"
    strings:
        $code = {
            00 22 
            02 60 
            42 60 
            C2 61 
            82 61 
            82 60 
            0E 21 
            01 61 
        }
    condition:
        $code
}

rule FS_InitFile_SDK_5_0_THUMB {
    meta:
        name = "FS_InitFile"
        sdk_version = "5.0"
    strings:
        $code = {
            23 21 
            00 22 
            09 02 
            11 43 
            82 60 
            42 60 
            02 60 
            C2 61 
        }
    condition:
        $code
}

rule FS_OpenFile_SDK_2_0_THUMB {
    meta:
        name = "FS_OpenFile"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? B5 
            82 B0 
            04 1C 
            00 A8 
            ?? F? ?? F?
            00 28 
            0? D0 
            [0-2] 
            00 AB 
            06 CB
            [0-2] 
            ?? F? ?? F?
        }
    condition:
        $code
}

rule FS_OpenFile_SDK_5_0_THUMB {
    meta:
        name = "FS_OpenFile"
        sdk_version = "5.0"
    strings:
        $code = {
            01 4B 
            01 22 
            18 47 
            C0 46
            ?? ?? ?? 02 
        }
    condition:
        $code
}

rule FS_OpenFileEx_SDK_2_0_THUMB {
    meta:
        name = "FS_OpenFileEx"
        sdk_version = "2.0"
    condition:
        false
}

rule FS_OpenFileEx_SDK_5_0_THUMB {
    meta:
        name = "FS_OpenFileEx"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? B5 
            CA B0 
            05 1C 
            16 1C 
            00 24 
            08 1C 
            00 A9 
            04 AA 
            00 94 
            ?? F? ?? F? 
            07 1C 
            15 D0 
            28 1C 
            ?? F? ?? F? 
        }
    condition:
        $code
}

rule FS_ReadFile_SDK_2_0_THUMB {
    meta:
        name = "FS_ReadFile"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? B5 
            [0-2]
            00 23 
            ?? F? ?? F?
            [0-2]
            ?? B?
        }
    condition:
        $code
}

rule FS_ReadFile_SDK_5_0_THUMB {
    meta:
        name = "FS_ReadFile"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? B5 
            8? B0 
            0? 1C 
            00 AB 
            2? 61 
            00 91 
            01 92 
            00 21 
            01 22 
            [0-2]
            ?? F? ?? F? 
        }
    condition:
        $code
}

rule FSi_ReadFileCore_SDK_2_0_THUMB {
    meta:
        name = "FSi_ReadFileCore"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? B5 
            [0-2]
            05 1C 
            14 1C 
            1E 1C 
            (AF | EF) 6A 
            (68 | A8) 6A 
            C0 1B 
            84 42 
            00 DD 
        }
    condition:
        $code
}

rule FSi_ReadFileCore_SDK_4_0_THUMB {
    meta:
        name = "FSi_ReadFileCore"
        sdk_version = "4.0"
    strings:
        $code = {
            ?? B5 
            [0-2]
            05 1C 
            EF 6A 
            A8 6A 
            14 1C 
            C0 1B 
            1E 1C 
            84 42 
            00 DD 
        }
    condition:
        $code
}

rule FSi_ReadFileCore_SDK_5_0_THUMB {
    meta:
        name = "FSi_ReadFileCore"
        sdk_version = "5.0"
    condition:
        false
}

rule FS_SeekFile_SDK_2_0_THUMB {
    meta:
        name = "FS_SeekFile"
        sdk_version = "2.0"
    strings:
        $code = {
            00 2A 
            04 D0 
            01 2A 
            05 D0 
            02 2A 
            06 D0 
            08 E0 
            (42 | 02) 6A 
        }
    condition:
        $code
}

rule FS_SeekFile_SDK_5_0_THUMB {
    meta:
        name = "FS_SeekFile"
        sdk_version = "5.0"
    strings:
        $code = {
            08 B5 
            82 B0 
            00 AB 
            03 61 
            00 91 
            01 92 
            0E 21 
            01 22 
            ?? F? ?? F? 
        }

        $code2 = {
            70 B5 
            82 B0 
            05 1C 
            0C 1C 
            16 1C 
            ?? F? ?? F? 
            00 28 
            08 D1 
            00 A8 
            28 61 
            28 1C 
            0E 21 
        }
    condition:
        $code or $code2
}

rule FS_CloseFile_SDK_2_0_THUMB {
    meta:
        name = "FS_CloseFile"
        sdk_version = "2.0"
    strings:
        $code = {
            10 B5 
            (04 1C 
            08 21 
            |
            08 21 
            04 1C 
            )
            ?? F? ?? F? 
            00 28 
            0? D1 
            00 20 
        }
    condition:
        $code
}

rule FS_GetLength_SDK_2_0_THUMB {
    meta:
        name = "FS_GetLength"
        sdk_version = "2.0"
    condition:
        false
}

rule FS_GetLength_SDK_5_0_THUMB {
    meta:
        name = "FS_GetLength"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? B5 
            8? B0 
            00 24 
            01 A9 
            05 1C 
            01 94 
            ?? F? ?? F? 
            00 28 
            0? D1 
            00 A8 
            28 61 
            28 1C 
            0F 21 
            01 22 
            00 94 
            ?? F? ?? F? 
        }

        $code2 = {
            ?? B5 
            8? B0 
            00 21 
            01 91 
            01 A9 
            04 1C 
            ?? F? ?? F? 
            00 28 
            0? D1 
            00 A8 
            20 61 
            00 20 
            00 90 
            20 1C 
            0F 21 
            01 22 
            ?? F? ?? F? 
        }
    condition:
        $code or $code2
}

rule FS_CloseFile_SDK_5_0_THUMB {
    meta:
        name = "FS_CloseFile"
        sdk_version = "5.0"
    strings:
        $code = {
            01 4B 
            08 21 
            01 22 
            18 47 
            ?? ?? ?? 02 
        }
    condition:
        $code
}

rule FSi_SendCommand_SDK_2_0_THUMB {
    meta:
        name = "FSi_SendCommand"
        sdk_version = "2.0"
    condition:
        false
}

rule FSi_SendCommand_SDK_5_0_THUMB {
    meta:
        name = "FSi_SendCommand"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? B5 
            8? B0 
            05 1C 
            E8 68 
            00 91 
        } 
    condition:
        $code
}

rule FS_LoadOverlay_SDK_2_0_THUMB {
    meta:
        name = "FS_LoadOverlay"
        sdk_version = "2.0"
    strings:
        $code = {
            00 B5 
            8B B0 
            03 1C 
            0A 1C 
            00 A8 
            19 1C 
            ?? F? ?? F?
            00 28 
            04 D0 
            00 A8 
            ?? F? ?? F? 
            00 28 
            0? D1 
            [6-8]
            00 A8 
            ?? F? ?? F?
        }
    condition:
        $code
}

rule FS_LoadOverlay_SDK_5_0_THUMB {
    meta:
        name = "FS_LoadOverlay"
        sdk_version = "5.0"
    strings:
        $code = {
            30 B5 
            8B B0 
            03 1C 
            00 AC 
            0A 1C 
            20 1C 
            19 1C 
            00 25 
            ?? F? ?? F?
            00 28 
            08 D0 
            20 1C 
            ?? F? ?? F? 
            00 28 
            03 D0 
            20 1C 
            ?? F? ?? F? 
        }
    condition:
        $code
}

rule FS_LoadOverlayInfo_SDK_2_0_THUMB {
    meta:
        name = "FS_LoadOverlayInfo"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? B5 
            9? B0 
            05 1C 
            0C 1C 
            [0-2]
            0? D1 
            2? 48 
            00 E0 
            2? 48 
        }
    condition:
        $code
}

rule FS_LoadOverlayInfo_SDK_5_0_THUMB {
    meta:
        name = "FS_LoadOverlayInfo"
        sdk_version = "5.0"
    strings:
        $code = {
            F0 B5 
            A9 B0 
            04 1C 
            00 20 
            0F 1C 
            56 01 
            00 90 
        }
    condition:
        $code
}

rule FS_LoadOverlayImage_SDK_2_0_THUMB {
    meta:
        name = "FS_LoadOverlayImage"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? B5 
            9? B0 
            04 1C 
            02 A8 
            ?? F? ?? F? 
            00 A8 
            21 1C 
            ?? F? ?? F? 
            (
            00 AB 
            06 CB 
            02 A8 
            |
            02 A8 
            00 AB 
            06 CB 
            )
            ?? F? ?? F? 
        }
    condition:
        $code
}

rule FS_LoadOverlayImage_SDK_5_0_THUMB {
    meta:
        name = "FS_LoadOverlayImage"
        sdk_version = "5.0"
    strings:
        $code = {
            ?? B5 
            9? B0 
            02 AF 
            04 1C 
            38 1C 
            00 26 
            ?? F? ?? F? 
            00 AD 
            28 1C 
            21 1C 
            ?? F? ?? F? 
        }

        $code2 = {
            ?? B5 
            9? B0 
            05 1C 
            02 A8 
            00 24 
            ?? F? ?? F? 
            00 A8 
            29 1C 
            ?? F? 39 F? 
        }
    condition:
        $code or $code2
}

rule FS_StartOverlay_SDK_2_0_THUMB {
    meta:
        name = "FS_StartOverlay"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? B5 
            [0-2]
            0? 1C 
            ?? F? ?? F? 
            0? 1C 
            ?? 48 
            00 88 
            02 28 
            ?? D1 
        }

        $code2 = {
            ?? B5 
            [0-2]
            05 1C 
            E8 7F 
            01 21 
            02 1C 
            0A 40 
            00 2A 
            0? D0 
            E9 69 
        }
    condition:
        $code or $code2
}

rule FS_StartOverlay_SDK_5_0_THUMB {
    meta:
        name = "FS_StartOverlay"
        sdk_version = "5.0"
    strings:
        $code = {
            F8 B5 
            05 1C 
            ?? F? ?? F? 
            0? 1C 
            ?? F? ?? (E? | F?) 
            01 28 
            2? D0 
            E8 69 
        }

        $code2 = {
            F8 B5 
            05 1C 
            ?? F? ?? F? 
            0? 1C 
            69 68 
            2? 48 
            81 42 
            0C D3 
            2? 48 
        }
    condition:
        $code or $code2
}


rule OS_WakeupThreadDirect {
    strings:
        $code = {
            ?? B5 
            [0-2]
            05 1C 
            ?? F? ?? E? 
            04 1C 
            01 20 
            68 66 
            ?? F? ?? F? 
            20 1C 
            ?? F? ?? E? 
        }
    condition:
        $code
}

rule OS_SleepThread_SDK_2_0_THUMB {
    meta:
        name = "OS_SleepThread"
        sdk_version = "2.0"
    strings:
        $code = {
            ?? B5 
            [0-2]
            05 1C 
            ?? F? ?? E? 
            04 1C 
            0? 48 
            00 68 
            01 68 
            00 2D 
            0? D0 
            28 88 
        }
    condition:
        $code
}

rule OS_SleepThread_SDK_3_0_THUMB {
    meta:
        name = "OS_SleepThread"
        sdk_version = "3.0"
    strings:
        $code = {
            ?? B5 
            [0-2]
            05 1C 
            ?? F? ?? E?
            06 1C 
            0? 48 
            (
            ?? 68 
            04 68 
            00 2D
            |
            00 2D
            ?? 68 
            04 68 
            )
            0? D0 
            (
            A5 67 
            28 1C 
            21 1C 
            |
            28 1C 
            21 1C 
            A5 67 
            )
            ?? F? ?? F?
        }
    condition:
        $code
}


rule TP_GetCalibratedPoint {
    strings:
        $code = {
            ?? B5 
            [0-2]
            05 1C 
            (
            ?? 48 
            0C 1C 
            |
            0C 1C
            ?? 48 
            )
            ?0 8E 
            00 28 
            0? D1 
            20 88 
            28 80 
            60 88 
            68 80 
        }
    condition:
        $code
}


rule OS_AllocFromHeap {
    strings:
        $code = {
            ?? B5 
            [0-2]
            06 1C 
            0C 1C 
            15 1C 
            ?? F? ?? E? 
            (07 1C | ?? 49) 
        }
    condition:
        $code
}

rule OS_FreeToHeap {
    strings:
        $code = {
            ?? B5 
            [0-2]
            06 1C 
            0D 1C 
            14 1C 
            ?? F? ?? E?
            07 1C 
            B1 00 
        }
    condition:
        $code
}

rule FndAllocFromExpHeapEx {
    strings:
        $code = {
            (
            ?? B5 
            ?? B0
            |
            ?? B5
            )
            00 29 
            00 D1 
            01 21 
            (
            C9 1C 
            03 23 
            |
            03 23 
            C9 1C 
            )
            99 43 
            00 2A 
            0? DB 
            ?? F? ?? F? 
        }
    condition:
        $code
}

rule FndFreeToExpHeap {
    strings:
        $code = {
            ?? B5 
            8? B0 
            [2-4]
            10 3? 
            [0-2]
            24 3?
            (00 A8 | ?? 1C) 
            2? 1C 
            ?? F? ?? F? 
            2? 1C 
            08 30 
            2? 1C 
            ?? F? ?? F? 
            2? 1C 
            (00 A9 | ?? 1C) 
            ?? F? ?? F? 
        }
        
        $code2 = {
            ?? B5 
            8? B0 
            [2-4]
            24 3? 
            [0-2]
            10 3? 
            (00 A8 | ?? 1C) 
            2? 1C 
            ?? F? ?? F? 
            2? 1C 
            08 30 
            2? 1C 
            ?? F? ?? F? 
            2? 1C 
            (00 A9 | ?? 1C) 
            ?? F? ?? F? 
        }
    condition:
        $code or $code2
}

rule OS_IsRunOnTwl_SDK_2_0_THUMB {
    meta:
        name = "OS_IsRunOnTwl"
        sdk_version = "2.0"
    condition:
        false
}

rule OS_IsRunOnTwl_SDK_5_0_THUMB {
    meta:
        name = "OS_IsRunOnTwl"
        sdk_version = "5.0"
    strings:
        $code = {
            0? 48 
            C0 69 
            00 28 
            0? D1 
            0? 48 
            01 78 
            03 20 
            08 40 
        }
    condition:
        $code
}