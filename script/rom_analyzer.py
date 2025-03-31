import yara
import ndspy.rom
import struct
import os
import sys
import re
from typing import List, Dict, Any
from ndspy_hotfix import NdspyHotfix

def parse_version(version_str: str) -> tuple:
    return tuple(map(int, version_str.split('.')))

def load_and_parse_rules(file_path: str) -> List[Dict]:
    with open(file_path, 'r') as f:
        content = f.read()
    
    rules = []
    rule_blocks = re.split(r'(?=\nrule\s+)', content.strip())
    
    for block in rule_blocks:
        meta = {}
        name_match = re.search(r'rule\s+(\w+)', block)
        if not name_match:
            continue
        
        meta_matches = re.finditer(r'(\w+)\s*=\s*"([^"]+)"', block)
        for m in meta_matches:
            meta[m.group(1)] = m.group(2)
        
        rules.append({
            'name': name_match.group(1),
            'meta': meta,
            'source': block.strip()
        })
    return rules

def find_closest_rules(rules: List[Dict], target_version: tuple, filtered_func_names : List[str] = None) -> str:
    rule_groups = {}
    
    for rule in rules:
        # 获取显示名称
        name = rule['meta'].get('name', rule['name'])
        
        if filtered_func_names and name not in filtered_func_names:
            continue
        
        if name not in rule_groups:
            rule_groups[name] = {
                'versioned': [],
                'non_versioned': []
            }
        
        if 'sdk_version' not in rule['meta']:
            rule_groups[name]['non_versioned'].append(rule['source'])
            continue
        
        try:
            rule_version = parse_version(rule['meta']['sdk_version'])
        except:
            continue
        
        rule_groups[name]['versioned'].append( (rule_version, rule['source']) )

    selected_rules = []
    
    for name, group in rule_groups.items():
        versioned = group['versioned']
        non_versioned = group['non_versioned']
        
        selected_rules.extend(non_versioned)
        
        if not versioned:
            continue
        
        le_versions = [v for v in versioned if v[0] <= target_version]
        gt_versions = [v for v in versioned if v[0] > target_version]
        
        if le_versions:
            max_version = max(v[0] for v in le_versions)
            selected_rules.extend([s for v, s in le_versions if v == max_version])
        elif gt_versions:
            min_version = min(v[0] for v in gt_versions)
            selected_rules.extend([s for v, s in gt_versions if v == min_version])
    
    rule_sources = ''
    
    for rule in selected_rules:
        rule_sources += rule + '\n'
    
    return rule_sources

def scan_sections(rules: yara.Rules, sections, default_symbol_type) -> List[Dict]:
    results = []
    for section in sections:
        matches = rules.match(data=section.data)
        for match in matches:
            symbol_name = match.meta.get('name', match.rule)
            symbol_type = match.meta.get('type', default_symbol_type)
            
            # 收集所有实例的地址
            addresses = set()
            for string in match.strings:
                for instance in string.instances:
                    # 核心修正点：使用 instance.offset 获取真实偏移
                    addresses.add(instance.offset + section.ramAddress)
            
            # 去重并排序地址
            sorted_addresses = sorted(addresses)
            
            # 构建结果
            symbol_info = {
                'name': symbol_name,
                'type': symbol_type,
                'addresses': sorted_addresses
            }
            
            # 合并同名同类型条目
            existing = next(
                (r for r in results 
                 if r['name'] == symbol_name and r['type'] == symbol_type),
                None
            )
            if existing:
                existing['addresses'].extend(sorted_addresses)
                existing['addresses'] = sorted(list(set(existing['addresses'])))
            else:
                results.append(symbol_info)
    
    return results

def detect_sdk_arch(codefile, arm_rules: List[Dict], thumb_rules: List[Dict], target_version: tuple):
    filtered_arm_rules = find_closest_rules(arm_rules, target_version, ['FS_InitFile'])
    filtered_thumb_rules = find_closest_rules(thumb_rules, target_version, ['FS_InitFile'])
    scan_results_arm = scan_sections(yara.compile(source=filtered_arm_rules), codefile.sections, 'arm')
    scan_results_thumb = scan_sections(yara.compile(source=filtered_thumb_rules), codefile.sections, 'thumb')
    if len(scan_results_arm) > 0:
        return 'arm'
    elif len(scan_results_thumb) > 0:
        return 'thumb'
    else:
        return None
        
def get_require_symbols(sdk_version, is_twl):
    symbols = ['PtrToArenaLo',
               'FS_InitFile',
               'FS_OpenFile',
               'FS_ReadFile',
               'FS_SeekFile',
               'FS_CloseFile',
               'OS_CreateThread',
               'OS_SleepThread',
               'OS_WakeupThreadDirect',
               'OS_SaveContext',
               'OSi_IrqThreadQueue',
               'TP_GetCalibratedPoint',
               'LanucherThreadContext',
               'HW_BUTTON_XY_BUF',
               'HW_TOUCHPANEL_BUF',
               'FS_LoadOverlay']
    if sdk_version >= (5, 0):
        symbols.append('FS_GetLength')
        if is_twl:
            symbols.append('PtrToArenaLo_twl')
    
    return symbols
    
def get_optional_symbols():
    symbols = ['FS_LoadOverlayInfo',
               'FS_LoadOverlayImage',
               'FS_StartOverlay',
               'OS_AllocFromHeap',
               'OS_FreeToHeap',
               'FndAllocFromExpHeapEx',
               'FndFreeToExpHeap',
               'SVC_WaitVBlankIntr']
    return symbols
    
def find_os_thread_addrs(sections, scan_results: List[Dict]):
    result = next((item for item in scan_results if item['name'] == 'OS_InitThread_constant'), None)
    if not result:
        print("OS_InitThread_constant not found")
        return
    
    addr = result['addresses'][0]
        
    for section in sections:
        if addr >= section.ramAddress and addr < section.ramAddress + len(section.data):
            func_offset = addr - section.ramAddress
        
            mov_rd = None
            mov_offset = None
            is_thumb = False

            # 先尝试扫描ARM模式 (4字节对齐)
            for i in range(func_offset - 0x140, min(len(section.data), func_offset + 0x140), 4):
                if i + 4 > len(section.data):
                    continue
                ins = struct.unpack("<I", section.data[i:i+4])[0]
                # 匹配 MOV Rd, #0x10 的ARM指令: E3A0d010
                if (ins & 0xFFFF0FFF) == 0xE3A00010:
                    mov_rd = (ins >> 12) & 0xF
                    mov_offset = i
                    break

            # 如果没找到再尝试Thumb模式 (2字节对齐)
            if mov_rd is None:
                for i in range(func_offset - 0x100, min(len(section.data), func_offset + 0x100), 2):
                    if i + 2 > len(section.data):
                        continue
                    ins = struct.unpack("<H", section.data[i:i+2])[0]
                    # 匹配 MOV Rd, #0x10 的Thumb指令: 20xx（xx=0x10）
                    if (ins & 0xF800) == 0x2000 and (ins & 0x00FF) == 0x10:
                        mov_rd = (ins >> 8) & 0x7
                        mov_offset = i
                        is_thumb = True
                        break

            if mov_rd is None:
                break

            # 扫描str指令（根据模式不同）
            str_rm = None
            str_imm = None

            # ARM模式str扫描
            if not is_thumb:
                for i in range(mov_offset, min(len(section.data), mov_offset + 0x100), 4):
                    if i + 4 > len(section.data):
                        break
                    ins = struct.unpack("<I", section.data[i:i+4])[0]
                    # 匹配 STR Rd, [Rm, #imm]
                    if (ins & 0xFFF00000) == 0xE5800000:
                        current_rd = (ins >> 12) & 0xF
                        if current_rd == mov_rd:
                            str_rm = (ins >> 16) & 0xF
                            str_imm = ins & 0xFFF
                            break
            else:  # Thumb模式str扫描
                for i in range(mov_offset, min(len(section.data), mov_offset + 0x100), 2):
                    if i + 2 > len(section.data):
                        break
                    ins = struct.unpack("<H", section.data[i:i+2])[0]
                    # 匹配 STR Rd, [Rm, #imm]（Thumb）
                    if (ins & 0xF800) == 0x6000:
                        current_rd = ins & 0x7
                        if current_rd == mov_rd:
                            str_rm = (ins >> 3) & 0x7
                            imm5 = (ins >> 6) & 0x1F
                            str_imm = imm5 * 4
                            break

            if str_rm is None or str_imm is None:
                break

            # 扫描ldr指令（根据模式不同）
            ldr_val = None

            # ARM模式ldr扫描
            if not is_thumb:
                for i in range(mov_offset, max(0, mov_offset - 0x100), -4):
                    if i + 4 > len(section.data):
                        continue
                    ins = struct.unpack("<I", section.data[i:i+4])[0]
                    # 匹配 LDR Rm, [PC, #offset]
                    if (ins & 0xFFFF0000) == 0xE59F0000 and ((ins >> 12) & 0xF) == str_rm:
                        imm = ins & 0xFFF
                        pc_addr = i + 8  # ARM模式PC偏移
                        target_addr = pc_addr + imm + section.ramAddress
                        # 转换到section内偏移
                        if section.ramAddress <= target_addr < section.ramAddress + len(section.data):
                            ldr_val = struct.unpack("<I", section.data[target_addr - section.ramAddress:][:4])[0]
                            break
            else:  # Thumb模式ldr扫描
                for i in range(mov_offset, max(0, mov_offset - 0x100), -2):
                    if i + 2 > len(section.data):
                        continue
                    ins = struct.unpack("<H", section.data[i:i+2])[0]
                    # 匹配 LDR Rm, [PC, #imm]
                    if (ins & 0xF800) == 0x4800 and ((ins >> 8) & 0x7) == str_rm:
                        imm8 = ins & 0xFF
                        pc_addr = (i + 4) & 0xFFFFFFFC  # Thumb模式PC对齐
                        target_addr = pc_addr + imm8 * 4 + section.ramAddress
                        # 转换到section内偏移
                        if section.ramAddress <= target_addr < section.ramAddress + len(section.data):
                            ldr_val = struct.unpack("<I", section.data[target_addr - section.ramAddress:][:4])[0]
                            break

            # 新增BL指令扫描逻辑
            bl_target = None
            search_range = 0x200  # 向前搜索范围
            func_type = None
            if is_thumb:
                # Thumb模式BL指令扫描
                base = func_offset & ~1  # 确保对齐
                for i in range(base, max(0, base - search_range), -2):
                    if i >= 2 and (i + 2) <= len(section.data):
                        # 检查32位BL指令
                        op1, op2 = struct.unpack("<HH", section.data[i-2:i+2])
                        if (op1 & 0xF800) == 0xF000 and (op2 & 0xE800) == 0xE800:
                            # 计算偏移量
                            s = ((op1 & 0x7FF) << 11) | (op2 & 0x7FF)
                            s = s << 10
                            s = int.from_bytes(s.to_bytes(4, byteorder='little', signed=False), 
                                                    byteorder='little', 
                                                    signed=True) # 符号扩展
                            s = s >> 9
                            # 计算目标地址（Thumb模式PC+4 + offset）
                            pc = (i - 2) + 4
                            bl_target = section.ramAddress + pc + s
                            if op2 & 0x1000:
                                func_type = 'thumb'
                            else:
                                func_type = 'arm'
                                bl_target = bl_target & ~3
                            break
            else:
                # ARM模式BL指令扫描
                base = func_offset & ~3  # 确保4字节对齐
                for i in range(base, max(0, base - search_range), -4):
                    if i + 4 <= len(section.data):
                        op = struct.unpack("<I", section.data[i:i+4])[0]
                        # 匹配BL/BLX指令
                        if (op & 0xFF000000) in (0xEB000000, 0xFA000000):
                            # 计算偏移量
                            offset = (op & 0x00FFFFFF) << 8
                            offset = int.from_bytes(offset.to_bytes(4, byteorder='little', signed=False), 
                                                    byteorder='little', 
                                                    signed=True) # 符号扩展
                            offset = offset >> 6
                            # 计算目标地址（ARM模式PC+8 + offset）
                            pc = i + 8
                            bl_target = section.ramAddress + pc + offset
                            if op & 0x01000000:
                                func_type = 'arm'
                            else:
                                func_type = 'thumb'
                            break
        
            if bl_target is not None:
                scan_results.append({
                    'name': 'OS_CreateThread',
                    'type': func_type,
                    'addresses': [bl_target]
                })

            if ldr_val is not None:
                thread_context = ldr_val + (str_imm - 0x70)
                scan_results.append({
                    'name': 'LanucherThreadContext',
                    'type': 'constant',
                    'addresses': [thread_context]})
                break
        
def find_osi_irq_thread_queue(sections, scan_results: List[Dict]):
    result = next((item for item in scan_results if item['name'] == 'OS_IrqHandler'), None)
    if not result:
        return
    
    addr = result['addresses'][0]
    
    ldr_val = None
    
    for section in sections:
        if addr >= section.ramAddress and addr < section.ramAddress + len(section.data):
            func_offset = addr - section.ramAddress
            
            # ARM模式ldr扫描
            for i in range(func_offset, min(func_offset + 0x80, len(section.data)), 4):
                if i + 4 > len(section.data):
                    continue
                ins = struct.unpack("<I", section.data[i:i+4])[0]
                # 匹配 LDR R12, [PC, #offset]
                if (ins & 0xFFFFF000) == 0xE59FC000:
                    imm = ins & 0xFFF
                    pc_addr = i + 8
                    target_addr = pc_addr + imm + section.ramAddress
                    # 转换到section内偏移
                    if section.ramAddress <= target_addr < section.ramAddress + len(section.data):
                        ldr_val = struct.unpack("<I", section.data[target_addr - section.ramAddress:][:4])[0]
                        break
                
    if ldr_val is not None:
        scan_results.append({
            'name': 'OSi_IrqThreadQueue',
            'type': 'constant',
            'addresses': [ldr_val]
        })
    
        
def find_arena_lo(codefile, scan_results):
    # 获取目标扫描结果
    result = next((item for item in scan_results if item['name'] == 'OS_GetInitArenaLo_constant'), None)
    if not result or not result['addresses']:
        return None
    
    addr = result['addresses'][0]
    target_section = None
    offset = 0
    
    # 查找对应的section
    for section in codefile.sections:
        if addr >= section.ramAddress and addr < section.ramAddress + len(section.data):
            offset = addr - section.ramAddress
            target_section = section
            break
    
    if not target_section:
        return None
    
    section_data = target_section.data
    search_start = max(0, offset - 0x20)
    search_end = offset - 0x4
    imm_offset = None
    
    # 先尝试ARM模式搜索 (4字节对齐)
    for i in range(search_end, search_start, -4):
        if i + 4 > len(section_data):
            continue
        
        # 检查当前指令
        ins = struct.unpack("<I", section_data[i:i+4])[0]
        
        # 匹配 POP {..., pc} 格式
        if ((ins & 0xFFFF0000) == 0xE8BD0000 and (ins & 0x00008000)) or ins == 0xE12FFF1E:
            # 验证下一条指令位置
            imm_offset = i + 4
            break
    
    # 如果没找到，尝试Thumb模式 (2字节对齐)
    if not imm_offset:
        thumb_search_start = max(0, offset - 0x20)
        thumb_search_end = offset - 0x2
        
        # 搜索 MOVS R0, #0 (0x2000)
        for i in range(thumb_search_end, thumb_search_start, -2):
            if i + 2 > len(section_data):
                continue
            
            ins = struct.unpack("<H", section_data[i:i+2])[0]
            if ins == 0x2000:  # MOVS R0, #0
                # 向后查找最多3条指令
                for look_ahead in range(1, 4):
                    check_addr = i + (look_ahead * 2)
                    if check_addr + 2 > len(section_data):
                        break
                    
                    # 检查POP或BX指令
                    check_ins = struct.unpack("<H", section_data[check_addr:check_addr+2])[0]
                    
                    # 匹配 POP {..., pc} (格式 0xBC??)
                    if (check_ins & 0xFF00) == 0xBD00 or (check_ins & 0xFF00) == 0x4700:
                        immediate_addr = check_addr + 2
                        imm_offset = (immediate_addr + 3) & ~3
                        break
                
                if imm_offset:
                    break
    
    if imm_offset is None:
        return False
    
    if codefile.is_twl:
        ptr_to_arena_lo_twl = imm_offset + target_section.ramAddress
        arena_lo_twl = struct.unpack_from('<I', section_data, imm_offset)[0]
        ptr_to_arena_lo = ptr_to_arena_lo_twl + 4
        arena_lo = struct.unpack_from('<I', section_data, imm_offset + 4)[0]
        scan_results.append({
            'name': 'ArenaLo_twl',
            'type': 'constant',
            'addresses': [arena_lo_twl]
        })
        scan_results.append({
            'name': 'PtrToArenaLo_twl',
            'type': 'constant',
            'addresses': [ptr_to_arena_lo_twl]
        })
    else:
        arena_lo = struct.unpack_from('<I', section_data, imm_offset)[0]
        ptr_to_arena_lo = imm_offset + target_section.ramAddress
        
    scan_results.append({
        'name': 'ArenaLo',
        'type': 'constant',
        'addresses': [arena_lo]
    })
    scan_results.append({
        'name': 'PtrToArenaLo',
        'type': 'constant',
        'addresses': [ptr_to_arena_lo]
    })
    
    return True

def find_injectable_areas(sections, arm9_ram_addr, scan_results):
    syscalls = next((item for item in scan_results if item['name'] == 'Syscalls'), None)
    if not syscalls:
        print("未找到Syscalls的地址")
        return 0
    
    syscall_addrs = sorted([addr for addr in syscalls['addresses'] if addr < arm9_ram_addr + 0x800])
    syscall_end_addrs = []
    
    if len(syscall_addrs) <= 1:
        print("Syscalls的地址不在ARM9的secure area内")
        return 0
    
    main_section = None
    
    for section in sections:
        if section.ramAddress == arm9_ram_addr:
            main_section = section
            break
    if not main_section:
        print("未找到ARM9的section")
        return 0    
        
    for i in range(len(syscall_addrs) - 1):
        offset = syscall_addrs[i] - main_section.ramAddress
        for j in range(2, 8, 2):
            op = struct.unpack("<H", main_section.data[offset + j:offset + j + 2])[0]
            if op == 0x4770:
                syscall_end_addrs.append(syscall_addrs[i] + j + 2)
                break
    
    injectable_areas = []
    for i in range(len(syscall_end_addrs)):
        start = (syscall_end_addrs[i] + 3) & ~3
        end = syscall_addrs[i+1] & ~3
        injectable_areas.append((start, end))
        
    injectable_areas = sorted(injectable_areas, key=lambda x: x[1] - x[0], reverse=True)
    
    print("以下是可注入overlay_ldr的区域：")
    print('--------------------------------')
    for i in range(min(10, len(injectable_areas))):
        print(f"{hex(injectable_areas[i][0])} - {hex(injectable_areas[i][1])}, 大小: {injectable_areas[i][1] - injectable_areas[i][0]} 字节")
    print('--------------------------------')
    
    return injectable_areas[0][0]

def calc_shared_variable_addr(sections, scan_results):
    result = next((item for item in scan_results if item['name'] == 'OS_GetInitArenaLo_constant'), None)
    if not result or not result['addresses']:
        return
    
    addr = result['addresses'][0]
    
    for section in sections:
        if addr >= section.ramAddress and addr < section.ramAddress + len(section.data):
            offset = addr - section.ramAddress
            shared_memory_addr = struct.unpack("<I", section.data[offset+8: offset+12])[0]
            scan_results.append({
                'name': 'HW_BUTTON_XY_BUF',
                'type': 'constant',
                'addresses': [shared_memory_addr + 0xFA8]
            })
            scan_results.append({
                'name': 'HW_TOUCHPANEL_BUF',
                'type': 'constant',
                'addresses': [shared_memory_addr + 0xFAA]
            })
            break
        
def find_matched_func_addr(sections, scan_results, source, target, offset):
    source = next((item for item in scan_results if item['name'] == source), None)
    target = next((item for item in scan_results if item['name'] == target), None)
    if not source or not target:
        return
    addrs = source['addresses']
    target_addrs = target['addresses']
    if target['type'] == 'thumb':
        target_addrs = [a | 1 for a in target_addrs]
    for addr in addrs:
        for section in sections:
            if addr >= section.ramAddress and addr < section.ramAddress + len(section.data):
                addr = addr - section.ramAddress
                target_addr = struct.unpack("<I", section.data[addr + offset: addr + offset + 4])[0]
                try:
                    index = target_addrs.index(target_addr)
                    source['addresses'] = [addr + section.ramAddress]
                    target['addresses'] = [target['addresses'][index]]
                    return
                except ValueError:
                    continue
                
def find_matched_func_call(sections, scan_results, source, target, size):
    source = next((item for item in scan_results if item['name'] == source), None)
    target = next((item for item in scan_results if item['name'] == target), None)
    if not source or not target:
        return
    addrs = source['addresses']
    target_addrs = target['addresses']
    for addr in addrs:
        for section in sections:
            if addr >= section.ramAddress and addr < section.ramAddress + len(section.data):
                addr = addr - section.ramAddress
                target_addr = None
                if source['type'] == 'thumb': 
                    for i in range(addr, min(len(section.data), addr + size), 2):
                        if (i + 4) > len(section.data):
                            break
                        op1, op2 = struct.unpack("<HH", section.data[i:i+4])
                        if (op1 & 0xF800) == 0xF000 and (op2 & 0xE800) == 0xE800:
                            s = ((op1 & 0x7FF) << 11) | (op2 & 0x7FF)
                            s = s << 10
                            s = int.from_bytes(s.to_bytes(4, byteorder='little', signed=False), 
                                                byteorder='little', 
                                                signed=True)
                            s = s >> 9
                            pc = i + 4
                            target_addr = section.ramAddress + pc + s
                            if not op2 & 0x1000:
                                target_addr = target_addr & ~3
                            break
                        
                if target_addr == None:
                    continue
                try:
                    index = target_addrs.index(target_addr)
                    source['addresses'] = [addr + section.ramAddress]
                    target['addresses'] = [target['addresses'][index]]
                    return
                except ValueError:
                    continue

def verify_symbols(sections, sdk_version, scan_results):
    def detect_symbol(scan_results, name, arch):
        result = next((item for item in scan_results if item['name'] == name), None)
        if not result:
            return False
        if result['type'] != arch:
            return False
        return True
    
    match_table = [
        ('FS_ReadFile', 'FSi_ReadFileCore', 'arm', 0xC), 
        ('FS_ReadFile', 'FSi_ReadFileCore', 'thumb', -0x10)
    ] if sdk_version < (5, 0) else [
        ('FS_CloseFile', 'FSi_SendCommand', 'arm', 0x10), 
        ('FS_CloseFile', 'FSi_SendCommand', 'thumb', 0x8),
        ('FS_OpenFile', 'FS_OpenFileEx', 'arm', 0xC), 
        ('FS_OpenFile', 'FS_OpenFileEx', 'thumb', 0x8)
    ]
    
    for source_name, target_name, arch_type, offset in match_table:
        if detect_symbol(scan_results, source_name, arch_type):
            if offset >= 0:
                find_matched_func_addr(sections, scan_results, source_name, target_name, offset)
            else:
                find_matched_func_call(sections, scan_results, source_name, target_name, -offset)
                        
def check_required_symbols(scan_results, sdk_verison, is_twl):
    require_symbols = get_require_symbols(sdk_verison, is_twl)
    for symbol in require_symbols:
        result = next((item for item in scan_results if item['name'] == symbol), None)
        if not result:
            if symbol == 'FS_LoadOverlay':
                print("未找到FS_LoadOverlay的地址")
                print("这可能是因为：")
                print("1. 该游戏没有使用Overlay，遇到这种情况，请使用其他方式加载插件，比如作为arm9的模块通过autoload加载")
                print("2. 该游戏使用了自定义的Overlay加载函数，请进行逆向工程，找到自定义的加载函数的地址，然后修改overlay_ldr")
            else:
                print(f"未找到{symbol}的地址")
            return False
        
def check_symbols_type(scan_results):
    arm_symbols = []
    thumb_symbols = []
    arm_symbols = [item['name'] for item in scan_results if item['type'] == 'arm' and item['name'] != 'OS_SaveContext']
    thumb_symbols = [item['name'] for item in scan_results if item['type'] == 'thumb' and item['name'] != 'SVC_WaitVBlankIntr']
    
    if len(arm_symbols) == 0 and len(thumb_symbols) > 0:
        return 1
    
    if len(arm_symbols) > 0 and len(thumb_symbols) == 0:
        return 0
    
    if len(arm_symbols) > 0 and len(thumb_symbols) > 0:
        print("发现同时存在ARM和Thumb的函数")
        print("为了确保正确链接到函数，请打开common/lib/src/nitro/import.c，删除下列ARM函数的定义")
        for arm_symbol in arm_symbols:
            print(arm_symbol)
        return 1
    
    return 0
        
def analyze_overlay(rom: ndspy.rom.NintendoDSRom):
    overlay_table = rom.loadArm9Overlays()
    if not overlay_table or len(overlay_table) == 0:
        print("该游戏没有overlay")
        return 0
    
    print("以下是该游戏的Overlay信息：")
    print('--------------------------------')
        
    overlay_format = (
        "Overlay {id}: "
        "load_addr=0x{load_addr:08X} "
        "end_addr=0x{end_addr:08X} "
        "file_id={file_id} "
        "static_init_begin=0x{static_init_begin:08X} "
        "static_init_end=0x{static_init_end:08X}"
    )
    
    for id, overlay in overlay_table.items():
        info = {
            'id': id,
            'load_addr': overlay.ramAddress,
            'end_addr': overlay.ramAddress + overlay.ramSize + overlay.bssSize,
            'file_id': overlay.fileID,
            'static_init_begin': overlay.staticInitStart,
            'static_init_end': overlay.staticInitEnd
        }
        print(overlay_format.format(**info))
        
    print('--------------------------------')
    
    return len(overlay_table)

def analyze_rom(nds_path: str):
    # 加载ROM文件
    rom = ndspy.rom.NintendoDSRom.fromFile(nds_path)
    
    codefile = rom.loadArm9()
    
    # 解析SDK版本
    if codefile.codeSettingsOffs is None:
        print("未找到SDK版本信息")
    
    sdk_ver_minor, sdk_ver_major = struct.unpack_from(
        '2B', codefile.sections[0].data, codefile.codeSettingsOffs + 0x1A
    )
    sdk_version = (sdk_ver_major, sdk_ver_minor)
    print(f"SDK版本: {'.'.join(map(str, sdk_version))}")
    script_dir = os.path.dirname(os.path.abspath(__file__))

    arm_rules = load_and_parse_rules(os.path.join(script_dir, 'pattern/nitrosdk_arm.yar'))
    thumb_rules = load_and_parse_rules(os.path.join(script_dir, 'pattern/nitrosdk_thumb.yar'))
    with open(os.path.join(script_dir, 'pattern/nitrosdk_common.yar'), 'r') as f:
        common_rules = f.read()
    
    arch = detect_sdk_arch(codefile, arm_rules, thumb_rules, sdk_version)
    
    if arch:
        if arch == 'arm':
            filtered_rules = find_closest_rules(arm_rules, sdk_version)
        else:
            filtered_rules = find_closest_rules(thumb_rules, sdk_version)
        filtered_rules = filtered_rules + '\n' + common_rules
        
        scan_results = scan_sections(yara.compile(source=filtered_rules), codefile.sections, arch)
        find_os_thread_addrs(codefile.sections, scan_results)
        find_osi_irq_thread_queue(codefile.sections, scan_results)
        
        calc_shared_variable_addr(codefile.sections, scan_results)
        find_arena_lo(codefile, scan_results)
        
        require_symbols = get_require_symbols(sdk_version, codefile.is_twl)
        optional_symbols = get_optional_symbols()
        missing_symbols = [s for s in require_symbols + optional_symbols if not any(r['name'] == s for r in scan_results)]
        if len(missing_symbols) > 0:
            if arch == 'arm':
                filtered_rules = find_closest_rules(thumb_rules, sdk_version, missing_symbols)
                scan_results.extend(scan_sections(yara.compile(source=filtered_rules), codefile.sections, 'thumb'))
            else:
                filtered_rules = find_closest_rules(arm_rules, sdk_version, missing_symbols)
                scan_results.extend(scan_sections(yara.compile(source=filtered_rules), codefile.sections, 'arm'))

        verify_symbols(codefile.sections, sdk_version, scan_results)
        
        num_of_overlay = analyze_overlay(rom)
        
        check_required_symbols(scan_results, sdk_version, codefile.is_twl)
        for item in scan_results:
            addrs = ', '.join(hex(a) for a in sorted(item['addresses']))
            if item['name'] in require_symbols + optional_symbols and len(item['addresses']) > 1:
                print(f"符号{item['name']}具有多个地址")
                print(f"{item['name']} ({item['type']}) @ {addrs}")
        
        print("以下是内存相关的信息：")
        print('--------------------------------')
        arena_lo = next((item for item in scan_results if item['name'] == 'ArenaLo'), None)
        if arena_lo:
            print(f"ArenaLo: {hex(arena_lo['addresses'][0])}")
        arena_lo_twl = next((item for item in scan_results if item['name'] == 'ArenaLo_twl'), None)
        if arena_lo_twl:
            print(f"ArenaLo_twl: {hex(arena_lo_twl['addresses'][0])}")
        print('--------------------------------')
            
        overlay_ldr_addr = find_injectable_areas(codefile.sections, codefile.ramAddress, scan_results)
        
        scan_results = [item for item in scan_results if item['name'] in require_symbols + optional_symbols]
        link_type = check_symbols_type(scan_results)
        save_symbols(scan_results, 'symbols.ld')
        print("符号已保存到symbols.ld")
        
        build_config = {
            'OVERLAY_ID': num_of_overlay,
            'OVERLAY_ADDR': f"0x{arena_lo_twl['addresses'][0] if arena_lo_twl else arena_lo['addresses'][0] if arena_lo else 0:08X}",
            'OVERLAY_NAME': f"overlay_{num_of_overlay:04}",
            'OVERLAY_LDR_ADDR': f"0x{overlay_ldr_addr:08X}",
            'INJECT_OVERLAY_ID': 0,
            'IS_NITROSDK_THUMB': link_type,
            'NITROSDK_VER': f"0x{sdk_version[0] << 8 | sdk_version[1]:04X}",
        }
        
        save_build_config(build_config, 'config.mk')
        print("构建配置已保存到config.mk")
            
def save_symbols(scan_results, path):
    with open(path, 'w') as f:
        for item in scan_results:
            addr = item['addresses'][0]
            if item['type'] == 'thumb':
                f.write(f"{item['name']} = 0x{addr:08X}|1;\n")
            else:
                f.write(f"{item['name']} = 0x{addr:08X};\n") 
    
def save_build_config(config, path):
    with open(path, 'w') as f:
        for key, value in config.items():
            f.write(f"{key} := {value}\n")

if __name__ == '__main__':
    NdspyHotfix.apply()
    
    if len(sys.argv) < 2:
        print("Usage: python rom_analyzer.py <nds_path>")
        sys.exit(1)
    
    analyze_rom(sys.argv[1])