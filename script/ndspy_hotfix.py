import struct
import ndspy.rom
import ndspy.codeCompression as codeCompression
from ndspy.code import MainCodeFile

class NdspyHotfix:
    original_code_init = MainCodeFile.__init__
    original_code_save = MainCodeFile.save
    original_section_init = MainCodeFile.Section.__init__
    
    @staticmethod
    def new_code_init(self, data: bytes, ramAddress: int, codeSettingsPointerAddress: int | None = None):
        self.sections = []
        self.ramAddress = ramAddress
        self.is_twl = False

        data = codeCompression.decompress(data)

        self.codeSettingsOffs = None
        if codeSettingsPointerAddress:
            # (codeSettingsPointerAddress might be None if it's not
            # available, or 0 if the ROM has it set to 0)
            try:
                codeSettingsAddr, = struct.unpack_from(
                    '<I', data, codeSettingsPointerAddress - ramAddress - 4)
                self.codeSettingsOffs = codeSettingsAddr - ramAddress
                assert 0 <= self.codeSettingsOffs < len(data) - 4

            except Exception:
                # Something was probably out of range. Fall back to the
                # manual search
                self.codeSettingsOffs = None

        if self.codeSettingsOffs is None:
            # Manual search algorithm used as a fallback
            self.codeSettingsOffs = self._searchForCodeSettingsOffs(data)
        
        if self.codeSettingsOffs != None:
            copyTableBegin, copyTableEnd, dataBegin = struct.unpack_from('<3I', data, self.codeSettingsOffs)
            sdk_ver_minor, sdk_ver_major = struct.unpack_from('2B', data, self.codeSettingsOffs + 0x1A)
            if sdk_ver_major >= 5:
                for i in range(0, 0x8000, 4):
                    if data[i:i+8] == b'\x63\x14\xC0\xDE\xDE\xC0\x14\x63':
                        self.is_twl = True
                        break
            copyTableBegin -= ramAddress
            copyTableEnd -= ramAddress
            dataBegin -= ramAddress
        else:
            copyTableBegin = copyTableEnd = 0
            dataBegin = len(data)
            
        def makeSection(
            ramAddr: int,
            ramLen: int,
            fileOffs: int,
            initFuncTable: int | None, 
            bssSize: int,
            implicit: bool = False,
        ) -> None:
            sdata = data[fileOffs : fileOffs + ramLen]
            self.sections.append(self.Section(sdata,
                                              ramAddr,
                                              bssSize,
                                              implicit=implicit, 
                                              initFuncTable=initFuncTable))
        
        makeSection(ramAddress, dataBegin, 0, 0, 0, implicit=True)
        
        copyTablePos  = copyTableBegin
        while copyTablePos < copyTableEnd:
            initFuncTable = None
            if self.is_twl:
                secRamAddr, secSize, initFuncTable, bssSize = \
                    struct.unpack_from('<4I', data, copyTablePos)
                copyTablePos += 16
            else:
                secRamAddr, secSize, bssSize = \
                    struct.unpack_from('<3I', data, copyTablePos)
                copyTablePos += 12

            makeSection(secRamAddr, secSize, dataBegin, initFuncTable, bssSize)

            dataBegin += secSize
    
    @staticmethod
    def new_code_save(self, *, compress: bool = False) -> bytes:
        """
        Generate a bytes object representing this code file.
        """
        data = bytearray()

        for s in self.sections:
            data.extend(s.data)

            # Align to 0x04
            while len(data) % 4:
                data.append(0)

        # These loops are NOT identical!
        # The first one only operates on sections with length != 0,
        # and the second operates on sections with length == 0!

        sectionTable = bytearray()

        for s in self.sections:
            if s.implicit: continue
            if len(s.data) == 0: continue
            if hasattr(self, 'is_twl') and self.is_twl:
                sectionTable.extend(
                    struct.pack('<4I', s.ramAddress, len(s.data), getattr(s, 'initFuncTable', 0), s.bssSize))
            else:
                sectionTable.extend(
                    struct.pack('<3I', s.ramAddress, len(s.data), s.bssSize))


        for s in self.sections:
            if s.implicit: continue
            if len(s.data) != 0: continue
            if hasattr(self, 'is_twl') and self.is_twl:
                sectionTable.extend(
                    struct.pack('<4I', s.ramAddress, len(s.data), getattr(s, 'initFuncTable', 0), s.bssSize))
            else:
                sectionTable.extend(
                    struct.pack('<3I', s.ramAddress, len(s.data), s.bssSize))


        sectionTableOffset = len(data)
        data.extend(sectionTable)

        def setInt(addr: int, val: int) -> None:
            struct.pack_into('<I', data, addr, val)

        sectionTableAddr = self.ramAddress + sectionTableOffset
        sectionTableEnd = sectionTableAddr + len(sectionTable)

        cso = self.codeSettingsOffs
        if cso is not None:
            setInt(cso + 0x00, sectionTableAddr)
            setInt(cso + 0x04, sectionTableEnd)
            setInt(cso + 0x08, self.ramAddress + len(self.sections[0].data))
        else:
            # Welp, hopefully we only have one section :P
            pass

        if compress:
            data = bytearray(codeCompression.compress(data, True))
            setInt(cso + 0x14, self.ramAddress + len(data))
        else:
            setInt(cso + 0x14, 0)

        return data

    @staticmethod
    def new_section_init(self, data: bytes, ramAddress: int, bssSize: int, implicit: bool = False, initFuncTable = None):
        self.initFuncTable = initFuncTable
        return NdspyHotfix.original_section_init(self, data, ramAddress, bssSize, implicit=implicit)
    
    @staticmethod
    def apply():
        MainCodeFile.__init__ = NdspyHotfix.new_code_init
        MainCodeFile.save = NdspyHotfix.new_code_save
        MainCodeFile.Section.__init__ = NdspyHotfix.new_section_init
        
    @staticmethod
    def revert():
        MainCodeFile.__init__ = NdspyHotfix.original_code_init
        MainCodeFile.save = NdspyHotfix.original_code_save
        MainCodeFile.Section.__init__ = NdspyHotfix.original_section_init
