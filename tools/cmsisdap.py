# -*- coding: utf-8 -*-
"""Minimal CMSIS-DAP (HID) client - connect to SWD and read the DP IDCODE."""
import ctypes
import ctypes.wintypes as wt
import struct

hid = ctypes.WinDLL("hid.dll")
setupapi = ctypes.WinDLL("setupapi.dll")
kernel32 = ctypes.WinDLL("kernel32.dll")
ole32 = ctypes.WinDLL("ole32.dll")

HANDLE = ctypes.c_void_p

setupapi.SetupDiGetClassDevsW.restype = HANDLE
setupapi.SetupDiGetClassDevsW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p,
                                          ctypes.c_void_p, wt.DWORD]
setupapi.SetupDiEnumDeviceInterfaces.restype = wt.BOOL
setupapi.SetupDiEnumDeviceInterfaces.argtypes = [HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                                 wt.DWORD, ctypes.c_void_p]
setupapi.SetupDiGetDeviceInterfaceDetailW.restype = wt.BOOL
setupapi.SetupDiGetDeviceInterfaceDetailW.argtypes = [HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                                      wt.DWORD, ctypes.POINTER(wt.DWORD),
                                                      ctypes.c_void_p]
setupapi.SetupDiDestroyDeviceInfoList.argtypes = [HANDLE]

kernel32.CreateFileW.restype = HANDLE
kernel32.CreateFileW.argtypes = [ctypes.c_wchar_p, wt.DWORD, wt.DWORD, ctypes.c_void_p,
                                 wt.DWORD, wt.DWORD, ctypes.c_void_p]
kernel32.WriteFile.argtypes = [HANDLE, ctypes.c_char_p, wt.DWORD,
                               ctypes.POINTER(wt.DWORD), ctypes.c_void_p]
kernel32.ReadFile.argtypes = [HANDLE, ctypes.c_char_p, wt.DWORD,
                              ctypes.POINTER(wt.DWORD), ctypes.c_void_p]
kernel32.CloseHandle.argtypes = [HANDLE]

hid.HidD_GetAttributes.argtypes = [HANDLE, ctypes.c_void_p]
hid.HidD_GetPreparsedData.argtypes = [HANDLE, ctypes.POINTER(ctypes.c_void_p)]
hid.HidD_FreePreparsedData.argtypes = [ctypes.c_void_p]
hid.HidP_GetCaps.argtypes = [ctypes.c_void_p, ctypes.c_void_p]


class GUID(ctypes.Structure):
    _fields_ = [("Data1", wt.DWORD), ("Data2", wt.WORD), ("Data3", wt.WORD),
                ("Data4", ctypes.c_ubyte * 8)]


class HIDD_ATTRIBUTES(ctypes.Structure):
    _fields_ = [("Size", wt.DWORD), ("VendorID", wt.WORD), ("ProductID", wt.WORD),
                ("VersionNumber", wt.WORD)]


class HIDP_CAPS(ctypes.Structure):
    _fields_ = [("Usage", wt.USHORT), ("UsagePage", wt.USHORT),
                ("InputReportByteLength", wt.USHORT), ("OutputReportByteLength", wt.USHORT),
                ("FeatureReportByteLength", wt.USHORT), ("Reserved", wt.USHORT * 17),
                ("NumberLinkCollectionNodes", wt.USHORT), ("NumberInputButtonCaps", wt.USHORT),
                ("NumberInputValueCaps", wt.USHORT), ("NumberInputDataIndices", wt.USHORT),
                ("NumberOutputButtonCaps", wt.USHORT), ("NumberOutputValueCaps", wt.USHORT),
                ("NumberOutputDataIndices", wt.USHORT), ("NumberFeatureButtonCaps", wt.USHORT),
                ("NumberFeatureValueCaps", wt.USHORT), ("NumberFeatureDataIndices", wt.USHORT)]


def hid_guid():
    g = GUID()
    hid.HidD_GetHidGuid(ctypes.byref(g))
    return g


def device_paths():
    g = hid_guid()
    hdev = setupapi.SetupDiGetClassDevsW(ctypes.byref(g), None, None, 0x12)
    if not hdev or hdev == HANDLE(-1).value:
        return []
    out = []
    i = 0
    while True:
        did = ctypes.create_string_buffer(32)
        ctypes.memmove(did, struct.pack("<I", 32), 4)
        if not setupapi.SetupDiEnumDeviceInterfaces(hdev, None, ctypes.byref(g), i,
                                                    ctypes.byref(did)):
            break
        i += 1
        need = wt.DWORD(0)
        setupapi.SetupDiGetDeviceInterfaceDetailW(hdev, ctypes.byref(did), None, 0,
                                                  ctypes.byref(need), None)
        buf = ctypes.create_string_buffer(need.value)
        # cbSize: 8 on 64-bit
        ctypes.memmove(buf, struct.pack("<I", 8), 4)
        if setupapi.SetupDiGetDeviceInterfaceDetailW(hdev, ctypes.byref(did), buf, need.value,
                                                     ctypes.byref(need), None):
            out.append(ctypes.wstring_at(ctypes.addressof(buf) + 4))
    setupapi.SetupDiDestroyDeviceInfoList(hdev)
    return out


def open_hid(path):
    h = kernel32.CreateFileW(ctypes.c_wchar_p(path), 0xC0000000, 3, None, 3, 0, None)
    if not h or h == HANDLE(-1).value:
        return None
    return h


class DAP:
    def __init__(self, handle, out_len, in_len):
        self.h = handle
        self.out_len = out_len
        self.in_len = in_len

    def xfer(self, payload):
        data = bytes([0]) + bytes(payload)
        data += bytes(max(0, self.out_len - len(data)))
        buf = ctypes.create_string_buffer(data, len(data))
        written = wt.DWORD(0)
        if not kernel32.WriteFile(self.h, buf, self.out_len, ctypes.byref(written), None):
            raise OSError("WriteFile failed")
        rbuf = ctypes.create_string_buffer(self.in_len)
        read = wt.DWORD(0)
        if not kernel32.ReadFile(self.h, rbuf, self.in_len, ctypes.byref(read), None):
            raise OSError("ReadFile failed")
        return bytearray(rbuf.raw[:read.value])

    def connect(self):
        r = self.xfer([0x02, 0x01])
        return r[1] if len(r) > 1 else 0

    def info(self, item):
        r = self.xfer([0x00, item])
        return bytes(r[2:]).split(b"\x00")[0].decode("ascii", "ignore")

    def configure(self):
        self.xfer([0x04, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00])

    def transfer(self, reqs):
        payload = [0x05, 0x00, len(reqs) & 0xFF, (len(reqs) >> 8) & 0xFF]
        for is_read, addr, data in reqs:
            payload.append((0x02 if is_read else 0x00) | (addr & 0x0C))
            if not is_read:
                payload.extend(struct.pack("<I", data or 0))
        r = self.xfer(payload)
        count = r[1] if len(r) > 1 else 0
        vals = []
        idx = 3
        for _ in range(count):
            if idx + 4 <= len(r):
                vals.append(struct.unpack("<I", bytes(r[idx:idx + 4]))[0])
            idx += 4
        return count, vals

    def rd(self, addr):
        _, v = self.transfer([(True, addr, None)])
        return v[0] if v else None

    def wr(self, addr, value):
        self.transfer([(False, addr, value)])


def main():
    paths = device_paths()
    print("HID devices:", len(paths))
    for p in paths:
        h = open_hid(p)
        if not h:
            continue
        a = HIDD_ATTRIBUTES()
        a.Size = ctypes.sizeof(a)
        hid.HidD_GetAttributes(h, ctypes.byref(a))
        pp = ctypes.c_void_p()
        caps = HIDP_CAPS()
        if hid.HidD_GetPreparsedData(h, ctypes.byref(pp)):
            hid.HidP_GetCaps(pp, ctypes.byref(caps))
            hid.HidD_FreePreparsedData(pp)
        tag = "%04X:%04X" % (a.VendorID, a.ProductID)
        print("  %s out=%d in=%d page=%04X" % (tag, caps.OutputReportByteLength,
                                               caps.InputReportByteLength, caps.UsagePage))
        if a.VendorID == 0x0D28 and a.ProductID == 0x0204 and caps.OutputReportByteLength:
            dap = DAP(h, caps.OutputReportByteLength, caps.InputReportByteLength)
            print("  firmware :", dap.info(0x04))
            print("  product  :", dap.info(0x02))
            port = dap.connect()
            print("  connect  :", port)
            if port:
                dap.configure()
                print("  IDCODE   : 0x%08X" % (dap.rd(0x00) or 0))
                print("  CTRL/STAT: 0x%08X" % (dap.rd(0x04) or 0))
        kernel32.CloseHandle(h)


if __name__ == "__main__":
    main()
