#!/usr/bin/env python3
import sys, re
import binascii
import os.path
import shutil
import struct

fname, = sys.argv[1:]
orig_fname = fname + ".orig"
print("Patching CSDK LOD file at %s" % fname)
if not os.path.isfile(orig_fname): print("\tOriginal SDK file %s not found" % orig_fname); sys.exit(1)
shutil.copyfile(orig_fname, fname)

# search ChangeWDPinStateInBoot address in map file
map_file = "hex/firmware_debug_final.map"
proc_addr = ""
if not os.path.isfile(map_file): print("\tMAP file %s not found"); sys.exit(1)
with open(map_file, 'r') as mf:
    for line in mf:
        if "ChangeWDPinStateInBoot" in line: proc_addr = line.strip().split()[0].replace("0xffffffff","")
if proc_addr == "": print("\tChangeWDPinStateInBoot address %s not found"); sys.exit(1)

print ("\tChangeWDPinStateInBoot address = %s" % proc_addr)
if len(proc_addr) != 8: print("\tChangeWDPinStateInBoot BAD address"); sys.exit(1)
rev_proc_addr = proc_addr[6:8] + proc_addr[4:6] + proc_addr[2:4] + proc_addr[0:2]
# print ("\tReversed ChangeWDPinStateInBoot address = %s" % rev_proc_addr)

nop = "0065"
patch_list = {
    # in lod file byte sequence reversed !!!

    0x08187034: ("Issue 28 -- user name (ApiPdpContextToCFWPdpContext)", bytearray.fromhex('c0c2c1c2c2c2c3c2'), bytearray.fromhex('c0c2' + (nop * 3))),
    0x08187066: ("Issue 28 -- password (ApiPdpContextToCFWPdpContext)", bytearray.fromhex('a0c2a1c2a2c2a3c2'), bytearray.fromhex('a0c2' + (nop * 3))),
    0x08187094: ("Issue 28 -- APN (ApiPdpContextToCFWPdpContext)", bytearray.fromhex('80c281c282c283c2'), bytearray.fromhex('80c2' + (nop * 3))),
    0x0817c05a: ("Issue 50 -- double socket.connect (tcp_connect)", bytearray.fromhex('7018ccec'), bytearray.fromhex(nop * 2)),
    0x080B466C: ("Issue Unicode character", bytearray.fromhex('0110026A'), bytearray.fromhex('0065026A')), 

    0x081853C4: ("PM_Restart on incoming call", bytearray.fromhex('B018743F'), bytearray.fromhex('D0183F18')), 

    # elf address 0x81C01C7E ... -> call ChangeWDPinStateInBoot (function hal_HstSendEvent() in hal_host.c)
    # see https://github.com/cherryding1/RDA8955_W17.44_IDH/blob/master/soft/platform/chip/hal/src/hal_host.c
    # if A9 hangs with Detected XCPU in the GDB loop (0x9db00000) - double check part4 in following statements (ChangeWDPinStateInBoot address from map file)
    0x0801473C: ("Replace hal_HstSendEvent with WD pin state change (part1)", bytearray.fromhex('00686267'), bytearray.fromhex('006803B2')), 
    0x08014740: ("Replace hal_HstSendEvent with WD pin state change (part2)", bytearray.fromhex('0810031A'), bytearray.fromhex('006540EA')), 
    0x08014744: ("Replace hal_HstSendEvent with WD pin state change (part3)", bytearray.fromhex('4F0704D3'), bytearray.fromhex('00651110')), 
    0x08014748: ("Replace hal_HstSendEvent with WD pin state change (part4)", bytearray.fromhex('049373E2'), bytearray.fromhex(rev_proc_addr)), # reversed ChangeWDPinStateInBoot address from map file

    # elf address 0x8817845a
    # NOT WORKED! err code: TaskName: mpy Task -- lwip_recv_tcp: p == NULL, error is "Operation would block."!
    #0x08178458: ("Replace in netconn_recv_data(): sys_arch_mbox_fetch() to sys_arch_mbox_tryfetch()", bytearray.fromhex('8ceb0d23'), bytearray.fromhex('8ceb0065')),

    # elf address 0x880fb346 "move v1, timeout" -> "li v1, 0x32"
    0x080FB344: ("Replace in  sys_arch_mbox_fetch(): timeout from 100 to 50", bytearray.fromhex('04676667'), bytearray.fromhex('0467326b')),

    # elf address 0x880fb346 "move v1, timeout" -> "li v1, 0xC8"
    #0x080FB344: ("Replace in  sys_arch_mbox_fetch(): timeout from 100 to 20", bytearray.fromhex('04676667'), bytearray.fromhex('0467146b')),

    # elf address 0x88175fb0 "bnez err, LAB_..." -> "bnez mem, LAB_..."
    #0x08175FB0: ("Replace in  netconn_drain()->lwip_netconn_is_err_msg(): check mem for NULL", bytearray.fromhex('042d0eb4'), bytearray.fromhex('042c0eb4')),
    0x08175FB0: ("Replace in  netconn_drain()->lwip_netconn_is_err_msg(): check mem for NULL (part1)", bytearray.fromhex('042d0eb4'), bytearray.fromhex('15241425')),
    0x08175FB4: ("Replace in  netconn_drain()->lwip_netconn_is_err_msg(): check mem for NULL (part2)", bytearray.fromhex('7018ccec'), bytearray.fromhex('00650065')),

    # elf 0x8200e6cc  sxs_IoInit() -> call ChangeWDPinStateInBoot
    # not used
    #0x08029414: ("Replace sxs_IoInit with WD pin state change (part1)", bytearray.fromhex('50185B12'), bytearray.fromhex('02B240EA')), 
    #0x08029418: ("Replace sxs_IoInit with WD pin state change (part2)", bytearray.fromhex('00655018'), bytearray.fromhex('00650210')), 
    #0x0802941C: ("Replace sxs_IoInit with WD pin state change (part3)", bytearray.fromhex('6D12016C'), bytearray.fromhex(rev_proc_addr)), # reversed ChangeWDPinStateInBoot address from map file

    # elf 0x8200e6cc  sxs_IoInit() -> call BootStartWDTimer
    # not used
    # ATTENTION: illegal BootStartWDTimer address
    #0x08029414: ("Replace sxs_IoInit with BootStartWDTimer (part1)", bytearray.fromhex('50185B12'), bytearray.fromhex('02B240EA')), 
    #0x08029418: ("Replace sxs_IoInit with BootStartWDTimer (part2)", bytearray.fromhex('00655018'), bytearray.fromhex('00650210')), 
    #0x0802941C: ("Replace sxs_IoInit with BootStartWDTimer (part3)", bytearray.fromhex('6D12016C'), bytearray.fromhex('!CD322688!')), # reversed BootStartWDTimer address from map file

    # change startup PIN initial state
    # see https://github.com/cherryding1/RDA8955_W17.44_IDH/blob/master/soft/target/8955_modem/include/tgt_board_cfg.h
    # not used

    # change LED0 startup level (connect LED to pin 7)
    #0x08032F14: ("Set WD pin power OFF (part1)", bytearray.fromhex('00000000'), bytearray.fromhex('04000000')), # PMD_LEVEL_TYPE_LDO
    #0x08032F18: ("Set WD pin power OFF (part2)", bytearray.fromhex('00000100'), bytearray.fromhex('07000200')), # -pin=7, type=2
    #0x08032F1C: ("Set WD pin power OFF (part3)", bytearray.fromhex('00000000'), bytearray.fromhex('00000000')), # powerOnState = OFF

    # change LED3 startup level (connect LED to pin 7)
    #0x08032F38: ("Set WD pin power ON (part1)", bytearray.fromhex('05000000'), bytearray.fromhex('04000000')), # PMD_LEVEL_TYPE_LDO
    #0x08032F3C: ("Set WD pin power ON (part2)", bytearray.fromhex('00000100'), bytearray.fromhex('07000200')), # -pin=7, type=2
    #0x08032F40: ("Set WD pin power ON (part3)", bytearray.fromhex('00000000'), bytearray.fromhex('01000000')), # powerOnState = ON

    # set param1 with data coding scheme in SDK's SMS_ListMessage_rsp to get DCS as param1 while processing API_EVENT_ID_SMS_LIST_MESSAGE (modcellular_notify_sms_list)
    0x08106E58: ("Return from SMS_ListMessage_rsp SMS DCS in param1 (skip Trace)", bytearray.fromhex('3D6740F0'), bytearray.fromhex('0E101197')),
    0x08106E5C: ("Return from SMS_ListMessage_rsp SMS DCS in param1 (set param1)", bytearray.fromhex('D0A1A0F4'), bytearray.fromhex('E09F20F0')),
    0x08106E60: ("Return from SMS_ListMessage_rsp SMS DCS in param1 (set param1)", bytearray.fromhex('1CB5B018'), bytearray.fromhex('F9A7E1DC')),
    0x08106E64: ("Return from SMS_ListMessage_rsp SMS DCS in param1 (set param1)", bytearray.fromhex('743F016C'), bytearray.fromhex('89110065')),
    0x08107174: ("Return from SMS_ListMessage_rsp SMS DCS in param1 (branch new)", bytearray.fromhex('2104006F'), bytearray.fromhex('21047116')),
    0x08107178: ("Return from SMS_ListMessage_rsp SMS DCS in param1 (set zeros )", bytearray.fromhex('E1DCE2DC'), bytearray.fromhex('006FE2DC')),

    # set correct SPI params doDelay and diDelay (see machine_hw_spi.c)
    # Attention: doing this patch leads to loosing SPI_DATA_BITS_16 config
    0x080234AC: ("Set correct SPI params doDelay = 0 for phase = 1 (part 1)", bytearray.fromhex('022307D2'), bytearray.fromhex('02230065')),
    #    if (in_config.cpha != 0x0) {  hal_config.doDelay = 1; hal_config.diDelay = 1; }
    #    replace with --------------->
    #    if (in_config.cpha != 0x0) {  hal_config.diDelay = 1; } // set doDelay = 0 for cpha=1
    #
    #    8200876c 02  23           beqz       v1,LAB_82008772
    #    8200876e 07  d2           sw         v0,hal_config.doDelay (sp)
    #    replace with --------------->
    #    8200876c 02  23           beqz       v1,LAB_82008772
    #    8200876e 00  65           nop

    0x08023514: ("Set correct SPI params diDelay = 2 for phase = 0 (part 1)", bytearray.fromhex('16921072'), bytearray.fromhex('19920072')),
    0x08023518: ("Set correct SPI params diDelay = 2 for phase = 0 (part 2)", bytearray.fromhex('0261106C'), bytearray.fromhex('0261026C')),
    0x0802351C: ("Set correct SPI params diDelay = 2 for phase = 0 (part 3)", bytearray.fromhex('0BD42837'), bytearray.fromhex('08D42837')),
    #if (config.dataBits == 16) hal_config.frameSize = 16;
    # replace with --------------->
    #if (in_config.cpha == 0x0) hal_config.diDelay = 2; // correct diDelay for cpha=0
    #
    #    820087d4 16  92           lw         dataBits ,hal_config +0x48 (sp)
    #    820087d6 10  72           cmpi       dataBits ,16
    #    820087d8 02  61           btnez      LAB_820087de
    #    820087da 10  6c           li         in_id ,16
    #    820087dc 0b  d4           sw         in_id ,hal_config.frameSize (sp)
    #    820087de 28  37           sll        in_config_rxMode ,spi_id ,0x2
    #    replace with --------------->
    #    820087d4 19  92           lw         dataBits ,0x64 (sp)
    #    820087d6 00  72           cmpi       dataBits ,0x0
    #    820087d8 02  61           btnez      LAB_820087de
    #    820087da 02  6c           li         in_id ,0x2
    #    820087dc 08  d4           sw         in_id ,0x20 (sp)
    #    820087de 28  37           sll        in_config_rxMode ,spi_id ,0x2

}
keys_l = sorted(patch_list.keys())

address = None
patching_in_progress = None
patching_now = None
checksum = 0
data = []

with open(fname, 'r') as f:
    for n, line in enumerate(f):
        if line.startswith("#"):
            if line.startswith("#checksum"):
                data.append("#checksum={:08x}\n".format(checksum))
            else:
                data.append(line)  # Comment line
        elif line.startswith("@"):
            address = int(line[1:], 16)
            data.append(line)
        else:
            line_bytes = bytearray.fromhex(line[:-1])[::-1]
            address_to = address + len(line_bytes)
            for address_p, (name, old, new) in patch_list.items():
                address_p_to = address_p + len(old)
                address_o = max(address, address_p)
                address_o_to = min(address_to, address_p_to)
                if address_o < address_o_to:
                    if patching_in_progress is None:
                        patching_in_progress = address_p
                        print("\tPatching: %s" % name)
                    elif patching_in_progress != address_p:
                        raise RuntimeError("Failed to patch 0x{:08x}: overlapping addresses".format(patching_in_progress))
                    current_patch = line_bytes[address_o - address:address_o_to - address]
                    old_patch = old[address_o - address_p:address_o_to - address_p]
                    new_patch = new[address_o - address_p:address_o_to - address_p]
                    #print("current_patch = ", binascii.hexlify(current_patch))
                    #print("    old_patch = ", binascii.hexlify(old_patch))
                    #print("    new_patch = ", binascii.hexlify(new_patch))
                    if old_patch != new_patch:
                        if new_patch == current_patch:
                            if patching_now in (None, False):
                                patching_now = False
                            else:
                                raise RuntimeError("Failed to patch 0x{:08x}: data mismatch".format(patching_in_progress))
                        elif old_patch == current_patch:
                            if patching_now in (None, True):
                                patching_now = True
                                line_bytes = line_bytes[:address_o - address] + new_patch + line_bytes[address_o_to - address:]
                            else:
                                raise RuntimeError("Failed to patch 0x{:08x}: data mismatch".format(patching_in_progress))
                        else:
                            raise RuntimeError("Failed to patch 0x{:08x}: data mismatch".format(patching_in_progress))
                        #print(" {:d}:{:d}".format(address_o - address_p, address_o_to - address_p))
                    if address_o_to == address_p_to:
                        patching_in_progress = None
                        patching_now = None
            address = address_to
            #checksum += int.from_bytes(line_bytes, 'little')
            checksum += struct.unpack("<L", line_bytes)[0]
            checksum = checksum & 0xFFFFFFFF
            #data.append(line_bytes[::-1].hex()+"\n")
            data.append("".join("%02x" % b for b in reversed(line_bytes)) + "\n")
with open(fname, 'w') as f:
    f.write(''.join(data))
print("Done")

