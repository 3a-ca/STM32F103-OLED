# -*- coding: utf-8 -*-
"""Try harder to attach: line reset + JTAG-to-SWD switch + connect under reset."""
import ctypes
import sys
import time

sys.path.insert(0, r"C:\Users\30477\Documents\deepseek-harness\default-workspace\stm32_oled\tools")
import cmsisdap as M


def find_dap():
    for p in M.device_paths():
        h = M.open_hid(p)
        if not h:
            continue
        a = M.HIDD_ATTRIBUTES()
        a.Size = ctypes.sizeof(a)
        M.hid.HidD_GetAttributes(h, ctypes.byref(a))
        if a.VendorID == 0x0D28 and a.ProductID == 0x0204:
            return h
        M.kernel32.CloseHandle(h)
    return None


def main():
    h = find_dap()
    if not h:
        print("no DAPLink")
        return
    d = M.DAP(h, 65, 65)
    d.connect()
    d.configure()

    def pins(mask, value, wait_us=0):
        r = d.xfer([0x10, mask, value, (wait_us & 0xFF), (wait_us >> 8) & 0xFF,
                    (wait_us >> 16) & 0xFF, (wait_us >> 24) & 0xFF])
        return list(r[1:4])

    def swj(bits, data):
        n = (bits + 7) // 8
        payload = [0x12, bits & 0xFF, (bits >> 8) & 0xFF]
        payload += [(data >> (8 * i)) & 0xFF for i in range(n)]
        d.xfer(payload)

    print("pins before:", pins(0x80, 0x80, 1000))

    # 1) 拉低复位
    print("nRESET low :", pins(0x80, 0x00, 1000))
    time.sleep(0.05)

    # 2) 线复位 + 切 SWD
    swj(64, 0xFFFFFFFFFFFFFFFF)
    swj(16, 0x9E)          # 0xE79E LSB first -> 0x9E, 0xE7
    swj(16, 0xE7)
    swj(64, 0xFFFFFFFFFFFFFFFF)

    # 3) 释放复位并立即读 IDCODE（多试几次）
    print("nRESET high:", pins(0x80, 0x80, 100))

    for i in range(5):
        r = d.xfer([0x05, 0x00, 0x01, 0x00, 0x02])   # read DP IDCODE
        body = list(r[1:12])
        print("try %d: %s" % (i, body))
        if len(body) >= 4 and body[2] == 0x01:
            val = body[3] | (body[4] << 8) | (body[5] << 16) | (body[6] << 24)
            print("IDCODE = 0x%08X" % val)
            break
        time.sleep(0.05)

    # 4) 读 DHCSR 看看内核是否活着
    r = d.xfer([0x05, 0x00, 0x01, 0x00, 0x02])
    M.kernel32.CloseHandle(h)


main()
