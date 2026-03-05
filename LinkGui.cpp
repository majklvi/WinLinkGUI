// LinkGui.cpp - čisté WinAPI, Win10+ backend bez mklink.exe, "classic" UI.
// Query otevírá nové okno se strukturovaným GUI (checkboxy pro bool/flagy).
//
// Build (VS2022):
//   cl /EHsc /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS LinkGui.cpp user32.lib gdi32.lib comctl32.lib

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <cstdarg>

#pragma comment(lib, "comctl32.lib")

// ---- Compatibility defines ----
#ifndef FILE_FLAG_OPEN_REPARSE_POINT
#define FILE_FLAG_OPEN_REPARSE_POINT 0x00200000
#endif
#ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
#define SYMBOLIC_LINK_FLAG_DIRECTORY 0x1
#endif
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif
#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT 0xA0000003L
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK 0xA000000CL
#endif
#ifndef IO_REPARSE_TAG_APPEXECLINK
#define IO_REPARSE_TAG_APPEXECLINK 0x8000001BL
#endif
#ifndef FSCTL_GET_REPARSE_POINT
#define FSCTL_GET_REPARSE_POINT 0x000900A8
#endif
#ifndef FSCTL_SET_REPARSE_POINT
#define FSCTL_SET_REPARSE_POINT 0x000900A4
#endif
#ifndef FSCTL_DELETE_REPARSE_POINT
#define FSCTL_DELETE_REPARSE_POINT 0x000900AC
#endif
#ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE (16 * 1024)
#endif
#ifndef REPARSE_DATA_BUFFER_HEADER_SIZE
#define REPARSE_DATA_BUFFER_HEADER_SIZE 8
#endif
#ifndef SYMLINK_FLAG_RELATIVE
#define SYMLINK_FLAG_RELATIVE 0x00000001
#endif
#ifndef FILE_SUPPORTS_REPARSE_POINTS
#define FILE_SUPPORTS_REPARSE_POINTS 0x00000080
#endif
#ifndef FILE_SUPPORTS_HARD_LINKS
#define FILE_SUPPORTS_HARD_LINKS 0x00400000
#endif

// Minimal REPARSE_DATA_BUFFER (mount point + symlink)
typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;

        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;

        struct {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

// ---- IDs (main window) ----
enum {
    IDC_TYPE   = 1001,
    IDC_LINK   = 1002,
    IDC_TARGET = 1003,
    IDC_CREATE = 1004,
    IDC_DELETE = 1005,
    IDC_QUERY  = 1006,
    IDC_UPDATE = 1007,
    IDC_MAINLOG = 1008,

    IDC_LBL_TYPE = 1101,
    IDC_LBL_LINK = 1102,
    IDC_LBL_TGT  = 1103
};

// ---- IDs (query window) ----
enum {
    QIDC_G_LINK   = 2001,
    QIDC_G_REP    = 2002,
    QIDC_G_TGT    = 2003,
    QIDC_G_EXTRA  = 2004,

    // link labels/edits
    QIDC_S_PATH   = 2100,
    QIDC_E_PATH   = 2101,
    QIDC_S_FID    = 2102,
    QIDC_E_FID    = 2103,
    QIDC_S_VSER   = 2104,
    QIDC_E_VSER   = 2105,
    QIDC_S_SIZE   = 2106,
    QIDC_E_SIZE   = 2107,
    QIDC_S_CTIME  = 2108,
    QIDC_E_CTIME  = 2109,
    QIDC_S_MTIME  = 2110,
    QIDC_E_MTIME  = 2111,
    QIDC_S_ATIME  = 2112,
    QIDC_E_ATIME  = 2113,

    QIDC_CB_ISDIR     = 2120,
    QIDC_CB_ISREPARSE = 2121,

    QIDC_CB_LV_REPARSE = 2130,
    QIDC_CB_LV_HARDLINK = 2131,

    // attribute flags checkboxes
    QIDC_CB_A_READONLY = 2160,
    QIDC_CB_A_HIDDEN   = 2161,
    QIDC_CB_A_SYSTEM   = 2162,
    QIDC_CB_A_ARCHIVE  = 2163,
    QIDC_CB_A_TEMP     = 2164,
    QIDC_CB_A_SPARSE   = 2165,
    QIDC_CB_A_COMP     = 2166,
    QIDC_CB_A_ENCRYPT  = 2167,
    QIDC_CB_A_OFFLINE  = 2168,
    QIDC_CB_A_REPARSE  = 2169,

    // reparse labels/edits
    QIDC_S_RTAG  = 2200,
    QIDC_E_RTAG  = 2201,
    QIDC_CB_R_SYMLINK  = 2202,
    QIDC_CB_R_JUNCTION = 2203,
    QIDC_CB_R_OTHER    = 2204,
    QIDC_CB_R_RELATIVE = 2205,

    QIDC_S_SUB   = 2210,
    QIDC_E_SUB   = 2211,
    QIDC_S_PRN   = 2212,
    QIDC_E_PRN   = 2213,
    QIDC_S_IMM   = 2214,
    QIDC_E_IMM   = 2215,
    QIDC_CB_IMM_EXISTS = 2216,

    // target labels/edits
    QIDC_CB_FOLLOW_OK = 2250,
    QIDC_S_FINAL  = 2251,
    QIDC_E_FINAL  = 2252,
    QIDC_S_TFID   = 2253,
    QIDC_E_TFID   = 2254,
    QIDC_S_TVSER  = 2255,
    QIDC_E_TVSER  = 2256,
    QIDC_S_TSIZE  = 2257,
    QIDC_E_TSIZE  = 2258,
    QIDC_S_TCTIME = 2259,
    QIDC_E_TCTIME = 2260,
    QIDC_S_TMTIME = 2261,
    QIDC_E_TMTIME = 2262,
    QIDC_S_TATIME = 2263,
    QIDC_E_TATIME = 2264,

    QIDC_CB_TV_REPARSE = 2270,
    QIDC_CB_TV_HARDLINK = 2271,

    // extra
    QIDC_E_EXTRA = 2401
};

static HINSTANCE g_hInst = NULL;
static HFONT g_hFont = NULL;

// ---- Helpers ----
static void SetCtlFont(HWND h) { if (g_hFont) SendMessageW(h, WM_SETFONT, (WPARAM)g_hFont, TRUE); }

static std::wstring GetTextW(HWND h)
{
    int len = GetWindowTextLengthW(h);
    std::wstring s;
    s.resize(len);
    if (len > 0) GetWindowTextW(h, &s[0], len + 1);
    return s;
}
static void SetTextW(HWND h, const std::wstring& s) { SetWindowTextW(h, s.c_str()); }

static void AppendTextW(HWND h, const std::wstring& s)
{
    int len = GetWindowTextLengthW(h);
    SendMessageW(h, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(h, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
}

static std::wstring WinErrW(DWORD err)
{
    WCHAR buf[512] = {0};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, err, 0, buf, (DWORD)(sizeof(buf)/sizeof(buf[0]) - 1), NULL);
    if (buf[0] == 0) {
        WCHAR tmp[64];
        swprintf_s(tmp, L"Error %lu", err);
        return tmp;
    }
    return buf;
}

static std::wstring Fmt(const wchar_t* fmt, ...)
{
    wchar_t buf[2048];
    buf[0] = 0;
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    return std::wstring(buf);
}

static std::wstring GetFullPathW(const std::wstring& p)
{
    DWORD need = GetFullPathNameW(p.c_str(), 0, NULL, NULL);
    if (!need) return p;
    std::wstring out;
    out.resize(need);
    DWORD got = GetFullPathNameW(p.c_str(), need, &out[0], NULL);
    if (!got) return p;
    while (!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

static std::wstring DirOfPathW(const std::wstring& p)
{
    size_t pos = p.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L"";
    return p.substr(0, pos);
}

static std::wstring JoinPathW(const std::wstring& a, const std::wstring& b)
{
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == L'\\' || a.back() == L'/') return a + b;
    return a + L"\\" + b;
}

static std::wstring StripNtPrefixW(const std::wstring& p)
{
    if (p.rfind(L"\\??\\", 0) == 0) return p.substr(4);
    if (p.rfind(L"\\\\?\\", 0) == 0) return p.substr(4);
    return p;
}

static std::wstring FileTimeToTextLocalW(const FILETIME& ft)
{
    FILETIME lft;
    SYSTEMTIME st;
    if (!FileTimeToLocalFileTime(&ft, &lft)) return L"(n/a)";
    if (!FileTimeToSystemTime(&lft, &st)) return L"(n/a)";
    return Fmt(L"%04u-%02u-%02u %02u:%02u:%02u",
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

static void SetCheck(HWND h, bool on)
{
    SendMessageW(h, BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0);
}

static const wchar_t* ReparseTagNameW(ULONG tag)
{
    switch (tag) {
        case IO_REPARSE_TAG_SYMLINK:     return L"IO_REPARSE_TAG_SYMLINK";
        case IO_REPARSE_TAG_MOUNT_POINT: return L"IO_REPARSE_TAG_MOUNT_POINT";
        case IO_REPARSE_TAG_APPEXECLINK: return L"IO_REPARSE_TAG_APPEXECLINK";
        default: return L"(unknown)";
    }
}

// ---- Data structures for Query window ----
struct VolumeInfo {
    bool ok = false;
    std::wstring root;
    std::wstring volName;
    std::wstring fsName;
    DWORD serial = 0;
    DWORD maxComp = 0;
    DWORD fsFlags = 0;
};

struct ReparseInfo {
    bool has = false;
    ULONG tag = 0;
    bool isSymlink = false;
    bool isMountPoint = false;
    ULONG symlinkFlags = 0;
    std::wstring substituteName;
    std::wstring printName;
    std::wstring err;
};

struct HandleInfo {
    bool ok = false;
    BY_HANDLE_FILE_INFORMATION fi{};
    std::wstring finalPath; // optional
    std::wstring err;
};

struct QueryData {
    std::wstring inputPath;
    std::wstring fullPath;
    DWORD attr = 0;
    std::wstring attrErr;

    VolumeInfo linkVol;
    VolumeInfo targetVol;

    HandleInfo linkHandle;   // OPEN_REPARSE_POINT (metadata link object)
    ReparseInfo reparse;     // FSCTL_GET_REPARSE_POINT parsed
    std::wstring immediateTarget;
    bool immediateExists = false;

    HandleInfo followHandle; // follow (resolved)
    std::vector<std::wstring> hardlinkNames;
    std::wstring hardlinkErr;

    std::wstring extraText;  // text pro položky nejdou přenést do GUI
};

static bool FillVolumeInfoFromPathW(const std::wstring& anyPath, VolumeInfo& out, std::wstring& err)
{
    out = VolumeInfo();
    err.clear();

    WCHAR root[MAX_PATH] = {0};
    if (!GetVolumePathNameW(anyPath.c_str(), root, MAX_PATH)) {
        err = L"GetVolumePathName failed: " + WinErrW(GetLastError());
        return false;
    }

    WCHAR volName[MAX_PATH] = {0};
    WCHAR fsName[MAX_PATH]  = {0};
    DWORD serial = 0, maxComp = 0, fsFlags = 0;

    if (!GetVolumeInformationW(root, volName, MAX_PATH, &serial, &maxComp, &fsFlags, fsName, MAX_PATH)) {
        err = L"GetVolumeInformation failed: " + WinErrW(GetLastError());
        return false;
    }

    out.ok = true;
    out.root = root;
    out.volName = volName;
    out.fsName = fsName;
    out.serial = serial;
    out.maxComp = maxComp;
    out.fsFlags = fsFlags;
    return true;
}

static bool FillHandleInfoW(const std::wstring& path, DWORD createFlags, HandleInfo& out)
{
    out = HandleInfo();

    HANDLE h = CreateFileW(path.c_str(),
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           createFlags,
                           NULL);
    if (h == INVALID_HANDLE_VALUE) {
        out.err = WinErrW(GetLastError());
        return false;
    }

    if (!GetFileInformationByHandle(h, &out.fi)) {
        out.err = WinErrW(GetLastError());
        CloseHandle(h);
        return false;
    }

    DWORD need = GetFinalPathNameByHandleW(h, NULL, 0, 0);
    if (need) {
        std::wstring buf;
        buf.resize(need + 2);
        DWORD got = GetFinalPathNameByHandleW(h, &buf[0], (DWORD)buf.size(), 0);
        if (got && got < buf.size()) {
            buf.resize(got);
            out.finalPath = buf;
        }
    }

    out.ok = true;
    CloseHandle(h);
    return true;
}

static std::wstring ReadNameFromPathBuffer(const BYTE* base, USHORT offBytes, USHORT lenBytes)
{
    if (!base || lenBytes == 0) return L"";
    const WCHAR* w = (const WCHAR*)(base + offBytes);
    int chars = (int)(lenBytes / sizeof(WCHAR));
    return std::wstring(w, w + chars);
}

static bool FillReparseInfoW(const std::wstring& path, ReparseInfo& out)
{
    out = ReparseInfo();

    HANDLE h = CreateFileW(path.c_str(),
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);
    if (h == INVALID_HANDLE_VALUE) {
        out.err = L"Open failed: " + WinErrW(GetLastError());
        return false;
    }

    BYTE buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    DWORD bytes = 0;
    BOOL ok = DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, buf, (DWORD)sizeof(buf), &bytes, NULL);
    CloseHandle(h);

    if (!ok) {
        out.err = L"FSCTL_GET_REPARSE_POINT failed: " + WinErrW(GetLastError());
        return false;
    }

    PREPARSE_DATA_BUFFER rdb = (PREPARSE_DATA_BUFFER)buf;
    out.has = true;
    out.tag = rdb->ReparseTag;

    if (out.tag == IO_REPARSE_TAG_MOUNT_POINT) {
        out.isMountPoint = true;
        const auto& mp = rdb->MountPointReparseBuffer;
        const BYTE* base = (const BYTE*)mp.PathBuffer;
        out.substituteName = ReadNameFromPathBuffer(base, mp.SubstituteNameOffset, mp.SubstituteNameLength);
        out.printName      = ReadNameFromPathBuffer(base, mp.PrintNameOffset,      mp.PrintNameLength);
        return true;
    }

    if (out.tag == IO_REPARSE_TAG_SYMLINK) {
        out.isSymlink = true;
        const auto& sl = rdb->SymbolicLinkReparseBuffer;
        const BYTE* base = (const BYTE*)sl.PathBuffer;
        out.symlinkFlags = sl.Flags;
        out.substituteName = ReadNameFromPathBuffer(base, sl.SubstituteNameOffset, sl.SubstituteNameLength);
        out.printName      = ReadNameFromPathBuffer(base, sl.PrintNameOffset,      sl.PrintNameLength);
        return true;
    }

    return true;
}

static std::wstring ResolveImmediateTargetHeuristicW(const std::wstring& linkPath, const ReparseInfo& ri)
{
    std::wstring cand = !ri.printName.empty() ? ri.printName : ri.substituteName;
    if (cand.empty()) return L"";
    std::wstring stripped = StripNtPrefixW(cand);

    if (stripped.rfind(L"\\\\", 0) == 0) return stripped;
    if (stripped.size() >= 3 && stripped[1] == L':' && (stripped[2] == L'\\' || stripped[2] == L'/')) return stripped;

    if (ri.isSymlink && (ri.symlinkFlags & SYMLINK_FLAG_RELATIVE)) {
        std::wstring baseDir = DirOfPathW(GetFullPathW(linkPath));
        if (!baseDir.empty()) return JoinPathW(baseDir, stripped);
    }
    return stripped;
}

static void EnumHardlinksW(const std::wstring& filePath, std::vector<std::wstring>& out, std::wstring& errOut)
{
    out.clear();
    errOut.clear();

    DWORD attr = GetFileAttributesW(filePath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        errOut = L"GetFileAttributes: " + WinErrW(GetLastError());
        return;
    }
    if (attr & FILE_ATTRIBUTE_DIRECTORY) return;

    WCHAR volPath[MAX_PATH] = {0};
    if (!GetVolumePathNameW(filePath.c_str(), volPath, MAX_PATH)) {
        errOut = L"GetVolumePathName: " + WinErrW(GetLastError());
        return;
    }

    DWORD len = 1024;
    std::vector<WCHAR> buf(len);

    HANDLE hFind = FindFirstFileNameW(filePath.c_str(), 0, &len, &buf[0]);
    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e == ERROR_MORE_DATA) {
            buf.resize(len + 2);
            DWORD len2 = len;
            hFind = FindFirstFileNameW(filePath.c_str(), 0, &len2, &buf[0]);
            if (hFind == INVALID_HANDLE_VALUE) {
                errOut = L"FindFirstFileName: " + WinErrW(GetLastError());
                return;
            }
            len = len2;
        } else {
            errOut = L"FindFirstFileName: " + WinErrW(e);
            return;
        }
    }

    auto addOne = [&](const WCHAR* nameFromRoot) {
        std::wstring full = volPath;
        if (nameFromRoot && nameFromRoot[0] == L'\\') full += (nameFromRoot + 1);
        else if (nameFromRoot) full += nameFromRoot;
        out.push_back(full);
    };

    addOne(&buf[0]);

    for (;;) {
        DWORD l = (DWORD)buf.size();
        BOOL ok = FindNextFileNameW(hFind, &l, &buf[0]);
        if (!ok) {
            DWORD e = GetLastError();
            if (e == ERROR_MORE_DATA) {
                buf.resize(l + 2);
                DWORD l2 = l;
                ok = FindNextFileNameW(hFind, &l2, &buf[0]);
                if (!ok) break;
                addOne(&buf[0]);
                continue;
            }
            if (e == ERROR_HANDLE_EOF) break;
            break;
        }
        addOne(&buf[0]);
    }

    FindClose(hFind);
}

static std::wstring FileIdTextW(const BY_HANDLE_FILE_INFORMATION& fi)
{
    return Fmt(L"0x%08lX%08lX", fi.nFileIndexHigh, fi.nFileIndexLow);
}
static std::wstring VolumeSerialTextW(const BY_HANDLE_FILE_INFORMATION& fi)
{
    return Fmt(L"0x%08lX", fi.dwVolumeSerialNumber);
}
static std::wstring SizeTextW(const BY_HANDLE_FILE_INFORMATION& fi)
{
    ULONGLONG size = ((ULONGLONG)fi.nFileSizeHigh << 32) | (ULONGLONG)fi.nFileSizeLow;
    return Fmt(L"%llu", (unsigned long long)size);
}

static void CollectQueryData(const std::wstring& inputPath, QueryData& q)
{
    q = QueryData();
    q.inputPath = inputPath;
    q.fullPath = GetFullPathW(inputPath);

    DWORD attr = GetFileAttributesW(q.fullPath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        q.attrErr = WinErrW(GetLastError());
        return;
    }
    q.attr = attr;

    // volumes
    {
        std::wstring e;
        FillVolumeInfoFromPathW(q.fullPath, q.linkVol, e);
        if (!e.empty()) q.extraText += L"Link volume: " + e + L"\r\n";
    }

    // link handle info (no-follow, OPEN_REPARSE_POINT)
    {
        DWORD flags = FILE_FLAG_OPEN_REPARSE_POINT;
        if (attr & FILE_ATTRIBUTE_DIRECTORY) flags |= FILE_FLAG_BACKUP_SEMANTICS;
        if (!FillHandleInfoW(q.fullPath, flags, q.linkHandle)) {
            q.extraText += L"Link handle open failed: " + q.linkHandle.err + L"\r\n";
        }
    }

    // reparse info
    if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
        if (!FillReparseInfoW(q.fullPath, q.reparse)) {
            q.extraText += L"Reparse: " + q.reparse.err + L"\r\n";
        } else {
            if (q.reparse.isSymlink || q.reparse.isMountPoint) {
                q.immediateTarget = ResolveImmediateTargetHeuristicW(q.fullPath, q.reparse);
                if (!q.immediateTarget.empty()) {
                    DWORD a2 = GetFileAttributesW(q.immediateTarget.c_str());
                    q.immediateExists = (a2 != INVALID_FILE_ATTRIBUTES);
                }
            }
        }
    }

    // follow handle info (resolved)
    {
        DWORD flags = 0;
        if (attr & FILE_ATTRIBUTE_DIRECTORY) flags |= FILE_FLAG_BACKUP_SEMANTICS;
        if (!FillHandleInfoW(q.fullPath, flags, q.followHandle)) {
            q.extraText += L"Follow open failed: " + q.followHandle.err + L"\r\n";
        } else {
            std::wstring final = StripNtPrefixW(q.followHandle.finalPath);
            if (!final.empty()) {
                std::wstring e;
                FillVolumeInfoFromPathW(final, q.targetVol, e);
                if (!e.empty()) q.extraText += L"Target volume: " + e + L"\r\n";
            }
        }
    }

    // hardlinks list for files
    if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        EnumHardlinksW(q.fullPath, q.hardlinkNames, q.hardlinkErr);
        if (!q.hardlinkErr.empty()) q.extraText += L"Hardlink enum: " + q.hardlinkErr + L"\r\n";
    }

    // extra dumps
    if (!q.hardlinkNames.empty()) {
        q.extraText += L"\r\nHardlink names (same file ID):\r\n";
        for (const auto& s : q.hardlinkNames) q.extraText += L"  " + s + L"\r\n";
    }
    if (q.reparse.has) {
        if (!q.reparse.substituteName.empty() || !q.reparse.printName.empty()) {
            q.extraText += L"\r\nRaw reparse strings:\r\n";
            if (!q.reparse.substituteName.empty()) q.extraText += L"  SubstituteName: " + q.reparse.substituteName + L"\r\n";
            if (!q.reparse.printName.empty())      q.extraText += L"  PrintName:      " + q.reparse.printName + L"\r\n";
        }
    }
}

// ---- Backend create/delete/update ----
static void MainLog(HWND hwnd, const std::wstring& s, bool clear)
{
    HWND h = GetDlgItem(hwnd, IDC_MAINLOG);
    if (clear) SetTextW(h, L"");
    AppendTextW(h, s);
}

static bool SetJunctionW(const std::wstring& linkDir, const std::wstring& targetDir, std::wstring& errOut)
{
    errOut.clear();

    DWORD attr = GetFileAttributesW(linkDir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryW(linkDir.c_str(), NULL)) {
            errOut = L"CreateDirectory failed: " + WinErrW(GetLastError()) + L"\r\n";
            return false;
        }
    } else if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        errOut = L"Link path exists but is not a directory.\r\n";
        return false;
    }

    std::wstring fullTarget = GetFullPathW(targetDir);

    DWORD tAttr = GetFileAttributesW(fullTarget.c_str());
    if (tAttr == INVALID_FILE_ATTRIBUTES || !(tAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        errOut = L"Target for junction should be an existing directory.\r\n";
        return false;
    }

    std::wstring subst = L"\\??\\";
    subst += fullTarget;
    std::wstring print = fullTarget;

    const USHORT substBytes = (USHORT)(subst.size() * sizeof(WCHAR));
    const USHORT printBytes = (USHORT)(print.size() * sizeof(WCHAR));
    const USHORT pathBytes  = (USHORT)(substBytes + sizeof(WCHAR) + printBytes + sizeof(WCHAR));

    const DWORD totalSize =
        (DWORD)FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) + pathBytes;

    std::string storage;
    storage.resize(totalSize);
    PREPARSE_DATA_BUFFER rdb = (PREPARSE_DATA_BUFFER)&storage[0];

    ZeroMemory(rdb, totalSize);
    rdb->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    rdb->ReparseDataLength = (USHORT)(8 + pathBytes);
    rdb->Reserved = 0;

    rdb->MountPointReparseBuffer.SubstituteNameOffset = 0;
    rdb->MountPointReparseBuffer.SubstituteNameLength = substBytes;
    rdb->MountPointReparseBuffer.PrintNameOffset      = substBytes + sizeof(WCHAR);
    rdb->MountPointReparseBuffer.PrintNameLength      = printBytes;

    BYTE* pb = (BYTE*)rdb->MountPointReparseBuffer.PathBuffer;
    memcpy(pb, subst.c_str(), substBytes);
    *(WCHAR*)(pb + substBytes) = L'\0';
    memcpy(pb + substBytes + sizeof(WCHAR), print.c_str(), printBytes);
    *(WCHAR*)(pb + substBytes + sizeof(WCHAR) + printBytes) = L'\0';

    HANDLE h = CreateFileW(linkDir.c_str(),
                           GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);
    if (h == INVALID_HANDLE_VALUE) {
        errOut = L"Open link dir failed: " + WinErrW(GetLastError()) + L"\r\n";
        return false;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        h,
        FSCTL_SET_REPARSE_POINT,
        rdb,
        (DWORD)(rdb->ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE),
        NULL,
        0,
        &bytesReturned,
        NULL
    );
    CloseHandle(h);

    if (!ok) {
        errOut = L"FSCTL_SET_REPARSE_POINT failed: " + WinErrW(GetLastError()) + L"\r\n";
        return false;
    }

    return true;
}

static bool DeleteReparsePointIfAnyW(const std::wstring& path, std::wstring& errOut)
{
    errOut.clear();

    HANDLE h = CreateFileW(path.c_str(),
                           GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);
    if (h == INVALID_HANDLE_VALUE) {
        errOut = L"Open failed: " + WinErrW(GetLastError()) + L"\r\n";
        return false;
    }

    BYTE buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    DWORD bytes = 0;

    if (!DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, buf, (DWORD)sizeof(buf), &bytes, NULL)) {
        CloseHandle(h);
        return true;
    }

    PREPARSE_DATA_BUFFER rdb = (PREPARSE_DATA_BUFFER)buf;
    struct {
        ULONG ReparseTag;
        USHORT ReparseDataLength;
        USHORT Reserved;
    } del = { rdb->ReparseTag, 0, 0 };

    DWORD br = 0;
    BOOL ok = DeviceIoControl(h, FSCTL_DELETE_REPARSE_POINT, &del, sizeof(del), NULL, 0, &br, NULL);
    CloseHandle(h);

    if (!ok) {
        errOut = L"FSCTL_DELETE_REPARSE_POINT failed: " + WinErrW(GetLastError()) + L"\r\n";
        return false;
    }
    return true;
}

static bool CreateSymlinkW(const std::wstring& link, const std::wstring& target, bool isDir, std::wstring& errOut)
{
    errOut.clear();

    DWORD flags = isDir ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
    DWORD flagsTry = flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;

    if (CreateSymbolicLinkW(link.c_str(), target.c_str(), flagsTry)) return true;

    DWORD e = GetLastError();
    if (e == ERROR_INVALID_PARAMETER) {
        if (CreateSymbolicLinkW(link.c_str(), target.c_str(), flags)) return true;
        e = GetLastError();
    }

    errOut = L"CreateSymbolicLink failed: " + WinErrW(e) + L"\r\n";
    errOut += L"Note: symlinks často vyžadují Admin práva nebo Developer Mode.\r\n";
    return false;
}

// ---- Query result window ----
static const wchar_t* kQueryWndClass = L"LinkGui_QueryResult";

struct QueryWndState {
    QueryData* q = nullptr;

    // groups
    HWND gLink=nullptr, gRep=nullptr, gTgt=nullptr, gExtra=nullptr;

    // link labels/edits
    HWND sPath=nullptr, ePath=nullptr;
    HWND sFid=nullptr, eFid=nullptr;
    HWND sVser=nullptr, eVser=nullptr;
    HWND sSize=nullptr, eSize=nullptr;
    HWND sCT=nullptr, eCT=nullptr;
    HWND sMT=nullptr, eMT=nullptr;
    HWND sAT=nullptr, eAT=nullptr;

    HWND cbIsDir=nullptr, cbIsReparse=nullptr;
    HWND cbLvRep=nullptr, cbLvHl=nullptr;

    // attributes (checkboxes)
    HWND cbARo=nullptr, cbAHid=nullptr, cbASys=nullptr, cbAArc=nullptr, cbATmp=nullptr, cbASparse=nullptr, cbAComp=nullptr, cbAEnc=nullptr, cbAOff=nullptr, cbARep=nullptr;

    // reparse labels/edits + checkboxes
    HWND sRTag=nullptr, eRTag=nullptr;
    HWND cbRSym=nullptr, cbRJunc=nullptr, cbROther=nullptr, cbRRel=nullptr;
    HWND sSub=nullptr, eSub=nullptr;
    HWND sPrn=nullptr, ePrn=nullptr;
    HWND sImm=nullptr, eImm=nullptr;
    HWND cbImmExists=nullptr;

    // target labels/edits + checkboxes
    HWND cbFollowOk=nullptr;
    HWND sFinal=nullptr, eFinal=nullptr;
    HWND sTFid=nullptr, eTFid=nullptr;
    HWND sTVser=nullptr, eTVser=nullptr;
    HWND sTSize=nullptr, eTSize=nullptr;
    HWND sTCT=nullptr, eTCT=nullptr;
    HWND sTMT=nullptr, eTMT=nullptr;
    HWND sTAT=nullptr, eTAT=nullptr;

    HWND cbTvRep=nullptr, cbTvHl=nullptr;

    // extra
    HWND eExtra=nullptr;
};

static HWND MakeStatic(HWND parent, const wchar_t* text, int id)
{
    HWND h = CreateWindowExW(0, L"STATIC", text, WS_CHILD|WS_VISIBLE, 0,0,0,0, parent, (HMENU)(INT_PTR)id, g_hInst, NULL);
    SetCtlFont(h);
    return h;
}
static HWND MakeGroup(HWND parent, const wchar_t* text, int id)
{
    HWND h = CreateWindowExW(0, L"BUTTON", text, WS_CHILD|WS_VISIBLE|BS_GROUPBOX, 0,0,0,0, parent, (HMENU)(INT_PTR)id, g_hInst, NULL);
    SetCtlFont(h);
    return h;
}
static HWND MakeEditRO(HWND parent, int id, bool multiLine)
{
    DWORD style = WS_CHILD|WS_VISIBLE|ES_READONLY|WS_TABSTOP;
    if (multiLine) style |= ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL;
    else style |= ES_AUTOHSCROLL;
    HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", style, 0,0,0,0, parent, (HMENU)(INT_PTR)id, g_hInst, NULL);
    SetCtlFont(h);
    return h;
}
static HWND MakeCheck(HWND parent, const wchar_t* text, int id)
{
    HWND h = CreateWindowExW(0, L"BUTTON", text, WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 0,0,0,0, parent, (HMENU)(INT_PTR)id, g_hInst, NULL);
    SetCtlFont(h);
    return h;
}

static void DisableOutputCheckbox(HWND h)
{
    if (h) EnableWindow(h, FALSE);
}

static void PopulateQueryGui(QueryWndState* st)
{
    const QueryData& q = *st->q;

    SetTextW(st->ePath, q.fullPath);

    bool isDir = (q.attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    bool isRep = (q.attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    SetCheck(st->cbIsDir, isDir);
    SetCheck(st->cbIsReparse, isRep);

    // attribute subset
    SetCheck(st->cbARo,    (q.attr & FILE_ATTRIBUTE_READONLY) != 0);
    SetCheck(st->cbAHid,   (q.attr & FILE_ATTRIBUTE_HIDDEN) != 0);
    SetCheck(st->cbASys,   (q.attr & FILE_ATTRIBUTE_SYSTEM) != 0);
    SetCheck(st->cbAArc,   (q.attr & FILE_ATTRIBUTE_ARCHIVE) != 0);
    SetCheck(st->cbATmp,   (q.attr & FILE_ATTRIBUTE_TEMPORARY) != 0);
    SetCheck(st->cbASparse,(q.attr & FILE_ATTRIBUTE_SPARSE_FILE) != 0);
    SetCheck(st->cbAComp,  (q.attr & FILE_ATTRIBUTE_COMPRESSED) != 0);
    SetCheck(st->cbAEnc,   (q.attr & FILE_ATTRIBUTE_ENCRYPTED) != 0);
    SetCheck(st->cbAOff,   (q.attr & FILE_ATTRIBUTE_OFFLINE) != 0);
    SetCheck(st->cbARep,   (q.attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0);

    // link handle
    if (q.linkHandle.ok) {
        const auto& fi = q.linkHandle.fi;
        SetTextW(st->eFid,  FileIdTextW(fi));
        SetTextW(st->eVser, VolumeSerialTextW(fi));
        SetTextW(st->eSize, SizeTextW(fi));
        SetTextW(st->eCT, FileTimeToTextLocalW(fi.ftCreationTime));
        SetTextW(st->eMT, FileTimeToTextLocalW(fi.ftLastWriteTime));
        SetTextW(st->eAT, FileTimeToTextLocalW(fi.ftLastAccessTime));
    } else {
        SetTextW(st->eFid,  L"(n/a)");
        SetTextW(st->eVser, L"(n/a)");
        SetTextW(st->eSize, L"(n/a)");
        SetTextW(st->eCT, L"(n/a)");
        SetTextW(st->eMT, L"(n/a)");
        SetTextW(st->eAT, L"(n/a)");
    }

    // volume feature checkboxes
    if (q.linkVol.ok) {
        SetCheck(st->cbLvRep, (q.linkVol.fsFlags & FILE_SUPPORTS_REPARSE_POINTS) != 0);
        SetCheck(st->cbLvHl,  (q.linkVol.fsFlags & FILE_SUPPORTS_HARD_LINKS) != 0);
    } else {
        SetCheck(st->cbLvRep, false);
        SetCheck(st->cbLvHl, false);
    }

    // reparse
    if (q.reparse.has) {
        SetTextW(st->eRTag, Fmt(L"0x%08lX (%s)", q.reparse.tag, ReparseTagNameW(q.reparse.tag)));
        SetCheck(st->cbRSym, q.reparse.isSymlink);
        SetCheck(st->cbRJunc, q.reparse.isMountPoint);
        SetCheck(st->cbROther, (!q.reparse.isSymlink && !q.reparse.isMountPoint));
        SetCheck(st->cbRRel, (q.reparse.isSymlink && ((q.reparse.symlinkFlags & SYMLINK_FLAG_RELATIVE) != 0)));

        SetTextW(st->eSub, StripNtPrefixW(q.reparse.substituteName));
        SetTextW(st->ePrn, StripNtPrefixW(q.reparse.printName));
        SetTextW(st->eImm, q.immediateTarget);
        SetCheck(st->cbImmExists, q.immediateExists);
    } else {
        SetTextW(st->eRTag, L"(none)");
        SetCheck(st->cbRSym, false);
        SetCheck(st->cbRJunc, false);
        SetCheck(st->cbROther, false);
        SetCheck(st->cbRRel, false);
        SetTextW(st->eSub, L"");
        SetTextW(st->ePrn, L"");
        SetTextW(st->eImm, L"");
        SetCheck(st->cbImmExists, false);
    }

    // target/follow
    SetCheck(st->cbFollowOk, q.followHandle.ok);
    if (q.followHandle.ok) {
        const auto& fi = q.followHandle.fi;
        SetTextW(st->eFinal, StripNtPrefixW(q.followHandle.finalPath));
        SetTextW(st->eTFid,  FileIdTextW(fi));
        SetTextW(st->eTVser, VolumeSerialTextW(fi));
        SetTextW(st->eTSize, SizeTextW(fi));
        SetTextW(st->eTCT, FileTimeToTextLocalW(fi.ftCreationTime));
        SetTextW(st->eTMT, FileTimeToTextLocalW(fi.ftLastWriteTime));
        SetTextW(st->eTAT, FileTimeToTextLocalW(fi.ftLastAccessTime));
    } else {
        SetTextW(st->eFinal, L"(open failed)");
        SetTextW(st->eTFid,  L"(n/a)");
        SetTextW(st->eTVser, L"(n/a)");
        SetTextW(st->eTSize, L"(n/a)");
        SetTextW(st->eTCT, L"(n/a)");
        SetTextW(st->eTMT, L"(n/a)");
        SetTextW(st->eTAT, L"(n/a)");
    }

    if (q.targetVol.ok) {
        SetCheck(st->cbTvRep, (q.targetVol.fsFlags & FILE_SUPPORTS_REPARSE_POINTS) != 0);
        SetCheck(st->cbTvHl,  (q.targetVol.fsFlags & FILE_SUPPORTS_HARD_LINKS) != 0);
    } else {
        SetCheck(st->cbTvRep, false);
        SetCheck(st->cbTvHl, false);
    }

    // extra
    std::wstring extra = q.extraText;
    if (extra.empty()) extra = L"(no extra text)";
    SetTextW(st->eExtra, extra);

    // output checkboxes should not be clickable
    HWND cbs[] = {
        st->cbIsDir, st->cbIsReparse,
        st->cbLvRep, st->cbLvHl,
        st->cbARo, st->cbAHid, st->cbASys, st->cbAArc, st->cbATmp, st->cbASparse, st->cbAComp, st->cbAEnc, st->cbAOff, st->cbARep,
        st->cbRSym, st->cbRJunc, st->cbROther, st->cbRRel, st->cbImmExists,
        st->cbFollowOk,
        st->cbTvRep, st->cbTvHl
    };
    for (HWND h : cbs) DisableOutputCheckbox(h);
}

static void LayoutQueryWindow(HWND hwnd, QueryWndState* st)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    const int m = 10;
    const int g = 8;
    const int labelW = 130;
    const int rowH = 20;
    const int rowGap = 4;
    const int cbH = 18;

    int W = rc.right - rc.left;
    int H = rc.bottom - rc.top;

    int x = m, y = m;
    int w = W - 2*m;

    int hLink = 170;
    int hRep  = 170;
    int hTgt  = 180;
    int hExtra = H - (y + hLink + g + hRep + g + hTgt + g + m);
    if (hExtra < 100) hExtra = 100;

    MoveWindow(st->gLink,  x, y, w, hLink, TRUE); y += hLink + g;
    MoveWindow(st->gRep,   x, y, w, hRep,  TRUE); y += hRep  + g;
    MoveWindow(st->gTgt,   x, y, w, hTgt,  TRUE); y += hTgt  + g;
    MoveWindow(st->gExtra, x, y, w, hExtra,TRUE);

    auto GroupOrigin = [&](HWND hGroup, int& gx, int& gy) {
        RECT r{}; GetWindowRect(hGroup, &r);
        POINT p{r.left, r.top}; ScreenToClient(hwnd, &p);
        gx = p.x + 10;
        gy = p.y + 22;
    };

    auto Row = [&](HWND hS, HWND hE, int gx, int& gy, int editW) {
        MoveWindow(hS, gx, gy + 3, labelW, rowH, TRUE);
        MoveWindow(hE, gx + labelW, gy, editW, rowH, TRUE);
        gy += rowH + rowGap;
    };

    // Link group: left column rows + right column attributes
    int lgx, lgy;
    GroupOrigin(st->gLink, lgx, lgy);

    int rightColW = (w - 20) / 3;
    int leftColW  = (w - 20) - rightColW - 10;

    int editW = leftColW - labelW;
    if (editW < 120) editW = 120;

    int curY = lgy;
    Row(st->sPath, st->ePath, lgx, curY, editW);
    Row(st->sFid,  st->eFid,  lgx, curY, editW);
    Row(st->sVser, st->eVser, lgx, curY, editW);
    Row(st->sSize, st->eSize, lgx, curY, editW);
    Row(st->sCT,   st->eCT,   lgx, curY, editW);
    Row(st->sMT,   st->eMT,   lgx, curY, editW);
    Row(st->sAT,   st->eAT,   lgx, curY, editW);

    int boolY = lgy + hLink - 22 - cbH*2 - 6;
    MoveWindow(st->cbIsDir,     lgx,           boolY, 130, cbH, TRUE);
    MoveWindow(st->cbIsReparse, lgx + 140,     boolY, 150, cbH, TRUE);
    MoveWindow(st->cbLvRep,     lgx,           boolY + cbH + 2, 220, cbH, TRUE);
    MoveWindow(st->cbLvHl,      lgx + 230,     boolY + cbH + 2, 200, cbH, TRUE);

    // attributes column
    int ax = lgx + leftColW + 10;
    int ay = lgy;
    int half = rightColW / 2;
    int cbW = half - 6;
    auto Place2 = [&](HWND a, HWND b) {
        MoveWindow(a, ax,        ay, cbW, cbH, TRUE);
        MoveWindow(b, ax+half,   ay, cbW, cbH, TRUE);
        ay += cbH + 2;
    };
    Place2(st->cbARo,  st->cbAHid);
    Place2(st->cbASys, st->cbAArc);
    Place2(st->cbATmp, st->cbASparse);
    Place2(st->cbAComp,st->cbAEnc);
    Place2(st->cbAOff, st->cbARep);

    // Reparse group
    int rgx, rgy;
    GroupOrigin(st->gRep, rgx, rgy);

    int repRightW = rightColW;
    int repLeftW  = (w - 20) - repRightW - 10;
    int repEditW  = repLeftW - labelW;
    if (repEditW < 120) repEditW = 120;

    int ry = rgy;
    Row(st->sRTag, st->eRTag, rgx, ry, repEditW);
    Row(st->sSub,  st->eSub,  rgx, ry, repEditW);
    Row(st->sPrn,  st->ePrn,  rgx, ry, repEditW);
    Row(st->sImm,  st->eImm,  rgx, ry, repEditW);
    MoveWindow(st->cbImmExists, rgx, ry + 2, 220, cbH, TRUE);

    int rtx = rgx + repLeftW + 10;
    int rty = rgy;
    MoveWindow(st->cbRSym,   rtx, rty, repRightW, cbH, TRUE); rty += cbH + 2;
    MoveWindow(st->cbRJunc,  rtx, rty, repRightW, cbH, TRUE); rty += cbH + 2;
    MoveWindow(st->cbROther, rtx, rty, repRightW, cbH, TRUE); rty += cbH + 2;
    MoveWindow(st->cbRRel,   rtx, rty, repRightW, cbH, TRUE);

    // Target group
    int tgx, tgy;
    GroupOrigin(st->gTgt, tgx, tgy);

    MoveWindow(st->cbFollowOk, tgx, tgy, 200, cbH, TRUE);
    int ty = tgy + cbH + 4;

    int tgtEditW = (w - 20) - labelW;
    if (tgtEditW < 120) tgtEditW = 120;

    Row(st->sFinal, st->eFinal, tgx, ty, tgtEditW);
    Row(st->sTFid,  st->eTFid,  tgx, ty, tgtEditW);
    Row(st->sTVser, st->eTVser, tgx, ty, tgtEditW);
    Row(st->sTSize, st->eTSize, tgx, ty, tgtEditW);
    Row(st->sTCT,   st->eTCT,   tgx, ty, tgtEditW);
    Row(st->sTMT,   st->eTMT,   tgx, ty, tgtEditW);
    Row(st->sTAT,   st->eTAT,   tgx, ty, tgtEditW);

    MoveWindow(st->cbTvRep, tgx, tgy + hTgt - 22 - cbH, 250, cbH, TRUE);
    MoveWindow(st->cbTvHl,  tgx + 260, tgy + hTgt - 22 - cbH, 220, cbH, TRUE);

    // Extra group
    int egx, egy;
    GroupOrigin(st->gExtra, egx, egy);
    MoveWindow(st->eExtra, egx, egy, w - 20, hExtra - 32, TRUE);
}

static LRESULT CALLBACK QueryWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    QueryWndState* st = (QueryWndState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CREATE: {
            auto* cs = (CREATESTRUCTW*)lParam;
            QueryData* q = (QueryData*)cs->lpCreateParams;

            st = new QueryWndState();
            st->q = q;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);

            st->gLink  = MakeGroup(hwnd, L"Link object (no-follow)", QIDC_G_LINK);
            st->gRep   = MakeGroup(hwnd, L"Reparse & immediate target", QIDC_G_REP);
            st->gTgt   = MakeGroup(hwnd, L"Resolved target (followed)", QIDC_G_TGT);
            st->gExtra = MakeGroup(hwnd, L"Extra / raw text", QIDC_G_EXTRA);

            // link labels/edits
            st->sPath = MakeStatic(hwnd, L"Path:", QIDC_S_PATH);
            st->ePath = MakeEditRO(hwnd, QIDC_E_PATH, false);

            st->sFid  = MakeStatic(hwnd, L"File ID:", QIDC_S_FID);
            st->eFid  = MakeEditRO(hwnd, QIDC_E_FID, false);

            st->sVser = MakeStatic(hwnd, L"Volume serial:", QIDC_S_VSER);
            st->eVser = MakeEditRO(hwnd, QIDC_E_VSER, false);

            st->sSize = MakeStatic(hwnd, L"Size (bytes):", QIDC_S_SIZE);
            st->eSize = MakeEditRO(hwnd, QIDC_E_SIZE, false);

            st->sCT = MakeStatic(hwnd, L"Created:", QIDC_S_CTIME);
            st->eCT = MakeEditRO(hwnd, QIDC_E_CTIME, false);

            st->sMT = MakeStatic(hwnd, L"Modified:", QIDC_S_MTIME);
            st->eMT = MakeEditRO(hwnd, QIDC_E_MTIME, false);

            st->sAT = MakeStatic(hwnd, L"Accessed:", QIDC_S_ATIME);
            st->eAT = MakeEditRO(hwnd, QIDC_E_ATIME, false);

            st->cbIsDir = MakeCheck(hwnd, L"IsDirectory", QIDC_CB_ISDIR);
            st->cbIsReparse = MakeCheck(hwnd, L"IsReparsePoint", QIDC_CB_ISREPARSE);

            st->cbLvRep = MakeCheck(hwnd, L"LinkVol: supports reparse points", QIDC_CB_LV_REPARSE);
            st->cbLvHl  = MakeCheck(hwnd, L"LinkVol: supports hard links", QIDC_CB_LV_HARDLINK);

            // attr checkboxes
            st->cbARo    = MakeCheck(hwnd, L"READONLY", QIDC_CB_A_READONLY);
            st->cbAHid   = MakeCheck(hwnd, L"HIDDEN",   QIDC_CB_A_HIDDEN);
            st->cbASys   = MakeCheck(hwnd, L"SYSTEM",   QIDC_CB_A_SYSTEM);
            st->cbAArc   = MakeCheck(hwnd, L"ARCHIVE",  QIDC_CB_A_ARCHIVE);
            st->cbATmp   = MakeCheck(hwnd, L"TEMPORARY",QIDC_CB_A_TEMP);
            st->cbASparse= MakeCheck(hwnd, L"SPARSE",   QIDC_CB_A_SPARSE);
            st->cbAComp  = MakeCheck(hwnd, L"COMPRESSED",QIDC_CB_A_COMP);
            st->cbAEnc   = MakeCheck(hwnd, L"ENCRYPTED",QIDC_CB_A_ENCRYPT);
            st->cbAOff   = MakeCheck(hwnd, L"OFFLINE",  QIDC_CB_A_OFFLINE);
            st->cbARep   = MakeCheck(hwnd, L"REPARSE_POINT", QIDC_CB_A_REPARSE);

            // reparse labels/edits
            st->sRTag = MakeStatic(hwnd, L"Reparse tag:", QIDC_S_RTAG);
            st->eRTag = MakeEditRO(hwnd, QIDC_E_RTAG, false);

            st->cbRSym  = MakeCheck(hwnd, L"Type: Symbolic link", QIDC_CB_R_SYMLINK);
            st->cbRJunc = MakeCheck(hwnd, L"Type: Junction / Mount point", QIDC_CB_R_JUNCTION);
            st->cbROther= MakeCheck(hwnd, L"Type: Other reparse", QIDC_CB_R_OTHER);
            st->cbRRel  = MakeCheck(hwnd, L"Symlink flag: RELATIVE", QIDC_CB_R_RELATIVE);

            st->sSub = MakeStatic(hwnd, L"Substitute:", QIDC_S_SUB);
            st->eSub = MakeEditRO(hwnd, QIDC_E_SUB, false);

            st->sPrn = MakeStatic(hwnd, L"Print:", QIDC_S_PRN);
            st->ePrn = MakeEditRO(hwnd, QIDC_E_PRN, false);

            st->sImm = MakeStatic(hwnd, L"Immediate target:", QIDC_S_IMM);
            st->eImm = MakeEditRO(hwnd, QIDC_E_IMM, false);
            st->cbImmExists = MakeCheck(hwnd, L"Immediate target exists", QIDC_CB_IMM_EXISTS);

            // target
            st->cbFollowOk = MakeCheck(hwnd, L"Open follow succeeded", QIDC_CB_FOLLOW_OK);

            st->sFinal = MakeStatic(hwnd, L"Final path:", QIDC_S_FINAL);
            st->eFinal = MakeEditRO(hwnd, QIDC_E_FINAL, false);

            st->sTFid = MakeStatic(hwnd, L"Target file ID:", QIDC_S_TFID);
            st->eTFid = MakeEditRO(hwnd, QIDC_E_TFID, false);

            st->sTVser = MakeStatic(hwnd, L"Target vol serial:", QIDC_S_TVSER);
            st->eTVser = MakeEditRO(hwnd, QIDC_E_TVSER, false);

            st->sTSize = MakeStatic(hwnd, L"Target size (bytes):", QIDC_S_TSIZE);
            st->eTSize = MakeEditRO(hwnd, QIDC_E_TSIZE, false);

            st->sTCT = MakeStatic(hwnd, L"Target created:", QIDC_S_TCTIME);
            st->eTCT = MakeEditRO(hwnd, QIDC_E_TCTIME, false);

            st->sTMT = MakeStatic(hwnd, L"Target modified:", QIDC_S_TMTIME);
            st->eTMT = MakeEditRO(hwnd, QIDC_E_TMTIME, false);

            st->sTAT = MakeStatic(hwnd, L"Target accessed:", QIDC_S_TATIME);
            st->eTAT = MakeEditRO(hwnd, QIDC_E_TATIME, false);

            st->cbTvRep = MakeCheck(hwnd, L"TargetVol: supports reparse points", QIDC_CB_TV_REPARSE);
            st->cbTvHl  = MakeCheck(hwnd, L"TargetVol: supports hard links", QIDC_CB_TV_HARDLINK);

            // extra
            st->eExtra = MakeEditRO(hwnd, QIDC_E_EXTRA, true);

            PopulateQueryGui(st);
            LayoutQueryWindow(hwnd, st);
            return 0;
        }

        case WM_SIZE:
            if (st) LayoutQueryWindow(hwnd, st);
            return 0;

        case WM_DESTROY:
            if (st) {
                if (st->q) delete st->q;
                delete st;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void OpenQueryWindow(HWND hwndOwner, const std::wstring& path)
{
    QueryData* q = new QueryData();
    CollectQueryData(path, *q);

    std::wstring title = L"Query - " + q->fullPath;
    HWND w = CreateWindowExW(
        0,
        kQueryWndClass,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        860, 720,
        hwndOwner, NULL, g_hInst,
        q
    );
    if (w) {
        ShowWindow(w, SW_SHOW);
        UpdateWindow(w);
    } else {
        delete q;
    }
}

// ---- Main window UI ----
static void CreateClassicFont(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    int logPix = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(hwnd, hdc);

    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(8, logPix, 72);
    lf.lfWeight = FW_NORMAL;
    lstrcpyW(lf.lfFaceName, L"MS Sans Serif");
    g_hFont = CreateFontIndirectW(&lf);
}

static void CreateMainUi(HWND hwnd)
{
    CreateClassicFont(hwnd);

    const int m = 10;
    const int labelW = 70;
    const int rowH = 22;
    const int comboDropH = 220;

    int x = m, y = m;

    HWND sType = CreateWindowExW(0, L"STATIC", L"Type:", WS_CHILD|WS_VISIBLE, x, y+4, labelW, rowH, hwnd, (HMENU)(INT_PTR)IDC_LBL_TYPE, g_hInst, NULL);
    SetCtlFont(sType);

    HWND cbType = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                                  x+labelW, y, 320, comboDropH, hwnd, (HMENU)IDC_TYPE, g_hInst, NULL);
    SetCtlFont(cbType);
    SendMessageW(cbType, CB_ADDSTRING, 0, (LPARAM)L"Symbolic link (file)");
    SendMessageW(cbType, CB_ADDSTRING, 0, (LPARAM)L"Symbolic link (dir)");
    SendMessageW(cbType, CB_ADDSTRING, 0, (LPARAM)L"Hard link (file)");
    SendMessageW(cbType, CB_ADDSTRING, 0, (LPARAM)L"Junction (dir)");
    SendMessageW(cbType, CB_SETCURSEL, 0, 0);

    y += rowH + 8;

    HWND sLink = CreateWindowExW(0, L"STATIC", L"Link:", WS_CHILD|WS_VISIBLE, x, y+4, labelW, rowH, hwnd, (HMENU)(INT_PTR)IDC_LBL_LINK, g_hInst, NULL);
    SetCtlFont(sLink);

    HWND eLink = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                                 x+labelW, y, 520, rowH, hwnd, (HMENU)IDC_LINK, g_hInst, NULL);
    SetCtlFont(eLink);

    y += rowH + 8;

    HWND sTgt = CreateWindowExW(0, L"STATIC", L"Target:", WS_CHILD|WS_VISIBLE, x, y+4, labelW, rowH, hwnd, (HMENU)(INT_PTR)IDC_LBL_TGT, g_hInst, NULL);
    SetCtlFont(sTgt);

    HWND eTgt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                                x+labelW, y, 520, rowH, hwnd, (HMENU)IDC_TARGET, g_hInst, NULL);
    SetCtlFont(eTgt);

    y += rowH + 10;

    HWND bCreate = CreateWindowExW(0, L"BUTTON", L"Create", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                                   x+labelW, y, 90, 26, hwnd, (HMENU)IDC_CREATE, g_hInst, NULL);
    SetCtlFont(bCreate);

    HWND bDelete = CreateWindowExW(0, L"BUTTON", L"Delete", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                                   x+labelW + 96, y, 90, 26, hwnd, (HMENU)IDC_DELETE, g_hInst, NULL);
    SetCtlFont(bDelete);

    HWND bQuery = CreateWindowExW(0, L"BUTTON", L"Query...", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                                  x+labelW + 192, y, 90, 26, hwnd, (HMENU)IDC_QUERY, g_hInst, NULL);
    SetCtlFont(bQuery);

    HWND bUpdate = CreateWindowExW(0, L"BUTTON", L"Update", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                                   x+labelW + 288, y, 90, 26, hwnd, (HMENU)IDC_UPDATE, g_hInst, NULL);
    SetCtlFont(bUpdate);

    y += 26 + 10;

    HWND eLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Ready.\r\n",
                                WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY|WS_VSCROLL,
                                x, y, 700, 260, hwnd, (HMENU)IDC_MAINLOG, g_hInst, NULL);
    SetCtlFont(eLog);
}

static void LayoutMainWindow(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int W = rc.right - rc.left;
    int H = rc.bottom - rc.top;

    const int m = 10;
    const int labelW = 70;
    const int rowH = 22;

    int x = m, y = m;

    HWND cbType = GetDlgItem(hwnd, IDC_TYPE);
    HWND eLink  = GetDlgItem(hwnd, IDC_LINK);
    HWND eTgt   = GetDlgItem(hwnd, IDC_TARGET);
    HWND bCreate= GetDlgItem(hwnd, IDC_CREATE);
    HWND bDelete= GetDlgItem(hwnd, IDC_DELETE);
    HWND bQuery = GetDlgItem(hwnd, IDC_QUERY);
    HWND bUpdate= GetDlgItem(hwnd, IDC_UPDATE);
    HWND eLog   = GetDlgItem(hwnd, IDC_MAINLOG);

    int editW = (W - 2*m) - labelW;
    if (editW < 220) editW = 220;

    MoveWindow(cbType, x + labelW, y, editW, 220, TRUE);
    y += rowH + 8;

    MoveWindow(eLink, x + labelW, y, editW, rowH, TRUE);
    y += rowH + 8;

    MoveWindow(eTgt, x + labelW, y, editW, rowH, TRUE);
    y += rowH + 10;

    int btnW = 90, btnH = 26, gap = 6;
    int bx = x + labelW;
    MoveWindow(bCreate, bx, y, btnW, btnH, TRUE); bx += btnW + gap;
    MoveWindow(bDelete, bx, y, btnW, btnH, TRUE); bx += btnW + gap;
    MoveWindow(bQuery,  bx, y, btnW, btnH, TRUE); bx += btnW + gap;
    MoveWindow(bUpdate, bx, y, btnW, btnH, TRUE);

    y += btnH + 10;

    int logH = H - y - m;
    if (logH < 100) logH = 100;
    MoveWindow(eLog, x, y, W - 2*m, logH, TRUE);
}

// ---- Main actions ----
static int GetTypeIndex(HWND hwnd) { return (int)SendMessageW(GetDlgItem(hwnd, IDC_TYPE), CB_GETCURSEL, 0, 0); }

static void DoCreate(HWND hwnd)
{
    std::wstring link = GetTextW(GetDlgItem(hwnd, IDC_LINK));
    std::wstring target = GetTextW(GetDlgItem(hwnd, IDC_TARGET));

    MainLog(hwnd, L"", true);

    if (link.empty() || target.empty()) {
        MainLog(hwnd, L"Missing Link or Target.\r\n", false);
        return;
    }

    int type = GetTypeIndex(hwnd);
    std::wstring err;
    bool ok = false;

    if (type == 0) ok = CreateSymlinkW(link, target, false, err);
    else if (type == 1) ok = CreateSymlinkW(link, target, true, err);
    else if (type == 2) {
        if (CreateHardLinkW(link.c_str(), target.c_str(), NULL)) ok = true;
        else err = L"CreateHardLink failed: " + WinErrW(GetLastError()) + L"\r\n";
    }
    else if (type == 3) ok = SetJunctionW(link, target, err);
    else err = L"Unknown type.\r\n";

    if (ok) MainLog(hwnd, L"OK.\r\n", false);
    else    MainLog(hwnd, err, false);
}

static void DoDelete(HWND hwnd)
{
    std::wstring link = GetTextW(GetDlgItem(hwnd, IDC_LINK));
    MainLog(hwnd, L"", true);

    if (link.empty()) {
        MainLog(hwnd, L"Missing Link path.\r\n", false);
        return;
    }

    DWORD attr = GetFileAttributesW(link.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        MainLog(hwnd, L"GetFileAttributes failed: " + WinErrW(GetLastError()) + L"\r\n", false);
        return;
    }

    bool isDir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;

    if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
        std::wstring e;
        DeleteReparsePointIfAnyW(link, e);
    }

    BOOL ok = isDir ? RemoveDirectoryW(link.c_str()) : DeleteFileW(link.c_str());
    if (!ok) {
        MainLog(hwnd, L"Delete failed: " + WinErrW(GetLastError()) + L"\r\n", false);
        return;
    }

    MainLog(hwnd, L"Deleted.\r\n", false);
}

static void DoQuery(HWND hwnd)
{
    std::wstring path = GetTextW(GetDlgItem(hwnd, IDC_LINK));
    if (path.empty()) {
        MainLog(hwnd, L"", true);
        MainLog(hwnd, L"Missing path.\r\n", false);
        return;
    }
    OpenQueryWindow(hwnd, path);
}

static void DoUpdate(HWND hwnd)
{
    MainLog(hwnd, L"", true);
    MainLog(hwnd, L"Update = delete + create\r\n\r\n", false);

    std::wstring link = GetTextW(GetDlgItem(hwnd, IDC_LINK));
    if (link.empty()) {
        MainLog(hwnd, L"Missing Link path.\r\n", false);
        return;
    }

    DWORD attr = GetFileAttributesW(link.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) {
        bool isDir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
            std::wstring e;
            DeleteReparsePointIfAnyW(link, e);
        }

        BOOL ok = isDir ? RemoveDirectoryW(link.c_str()) : DeleteFileW(link.c_str());
        if (!ok) {
            MainLog(hwnd, L"Delete step failed: " + WinErrW(GetLastError()) + L"\r\n", false);
            return;
        }
        MainLog(hwnd, L"Old link removed.\r\n", false);
    } else {
        MainLog(hwnd, L"Old link not found; creating new.\r\n", false);
    }

    DoCreate(hwnd);
}

// ---- Main window proc ----
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            CreateMainUi(hwnd);
            LayoutMainWindow(hwnd);
            return 0;

        case WM_SIZE:
            LayoutMainWindow(hwnd);
            return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_CREATE: DoCreate(hwnd); return 0;
                    case IDC_DELETE: DoDelete(hwnd); return 0;
                    case IDC_QUERY:  DoQuery(hwnd);  return 0;
                    case IDC_UPDATE: DoUpdate(hwnd); return 0;
                }
            }
            return 0;

        case WM_DESTROY:
            if (g_hFont) { DeleteObject(g_hFont); g_hFont = NULL; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---- WinMain ----
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    g_hInst = hInst;
    InitCommonControls();

    // Best-effort vypnutí themingu – bez linkování uxtheme.lib
    HMODULE hUx = LoadLibraryW(L"uxtheme.dll");
    if (hUx) {
        typedef void (WINAPI *PFNSetThemeAppProperties)(DWORD);
        PFNSetThemeAppProperties p = (PFNSetThemeAppProperties)GetProcAddress(hUx, "SetThemeAppProperties");
        if (p) p(0);
        FreeLibrary(hUx);
    }

    // register main class
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"LinkGui_Main";
    if (!RegisterClassW(&wc)) return 0;

    // register query class
    WNDCLASSW qc = {};
    qc.lpfnWndProc   = QueryWndProc;
    qc.hInstance     = hInst;
    qc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    qc.hIcon         = LoadIcon(NULL, IDI_INFORMATION);
    qc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    qc.lpszClassName = kQueryWndClass;
    if (!RegisterClassW(&qc)) return 0;

    HWND hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Link GUI (WinAPI, native API backend)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        780, 540,
        NULL, NULL, hInst, NULL
    );
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}