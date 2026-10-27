// Bayonetta Origins: Cereza and the Lost Demon (title 0100CF5010FEC000, build a1ce2940...) - always full-detail models.
// Every model renders its LOD0 mesh set at any distance (event_000 holds no models, so cutscenes draw these same
// pl / em / st models). Shadow-only meshes are left alone. Every patched model is logged as an [ORLOD] line in the emulator log.
//
// Models are WMB3 files (NieR:Automata layout, version 0x20210402) inside the .dat archives. Header +0x40 / +0x44:
// batch table (28-byte records, index count at +0x14); +0x48 / +0x4C: LOD table, 20-byte records
//   { u32 nameOffset ("LOD0" / "LOD1" / "LOD2" / "SHADOW"), s32 lodLevel (0, 1, 2, -1 = shadow),
//     u32 batchStart, u32 batchInfoOffset, u32 batchCount }.
// Cereza (pl0000) is 39k / 19k / 7k triangles at LOD0 / 1 / 2, Cheshire (pl2000) 22k / 13k / 9k; stages, enemies,
// items and weapons have the same three levels.
//
// The game finds a model in its loaded .dat with FindFileByExt(dat, "wmb", n) @0x153DBD4 (it walks the extension
// table and returns datBase + fileOffset, so the WMB is used in place) or FindFileByName(dat, name) @0x153DE20.
// Hooking both and pointing every LOD1 / LOD2 record at the LOD0 batches before the caller builds the model makes
// whichever level the renderer picks draw LOD0, without having to find the distance check itself. The patch is
// idempotent, so the many repeat lookups of one buffer cost one compare loop each.
#include "lib.hpp"
#include <cstring>

// LOD every other level is redirected to. 0 = full detail (release). 2 = test build: everything lowest detail, which
// proves on screen that the redirect reaches the renderer (Cereza turns visibly low-poly up close).
#ifndef LOD_TARGET
#define LOD_TARGET 0
#endif

// 1 = also hide the white lenses of Cereza's glasses on every skin. The lens is one batch per LOD with a 64x64 white
// albedo: 0x1F753DC3 in the shared skin texture pack pl00f0 (plf000, pl0001-pl0008), 0x4E23DC29 in pl0000 / pl0020.
// Keyed on that texture, not on the Glass00 shader, because pl0001 / pl0002 use the glass shader for costume parts too.
#ifndef LOD_NO_LENS
#define LOD_NO_LENS 0
#endif

namespace {

template <typename T>
T& Abs(ptrdiff_t off) { return *reinterpret_cast<T*>(exl::util::modules::GetTargetStart() + off); }

constexpr ptrdiff_t kFindFileByExt  = 0x153DBD4;  // void*(void* datPair, const char* ext, u32 nth)
constexpr ptrdiff_t kFindFileByName = 0x153DE20;  // void*(void* datPair, const char* name)

constexpr u32 kWmb3Magic = 0x33424D57;  // "WMB3"

struct WmbLod {
    u32 name;
    s32 level;
    u32 batchStart;
    u32 batchInfo;
    u32 batchCount;
};
static_assert(sizeof(WmbLod) == 20);

u32 g_models = 0;   // models patched
u32 g_seen = 0;     // distinct WMB3 lookups that needed no change (already patched, or nothing to redirect)

template <typename T>
T At(const u8* base, u32 off) {
    T v;
    std::memcpy(&v, base + off, sizeof(T));   // WMB tables are not always 4-byte aligned (pl0000: LOD1 batch info at 0x4D3401)
    return v;
}

u32 Tris(const u8* w, const WmbLod& l) {
    u32 batches = At<u32>(w, 0x40), count = At<u32>(w, 0x44), idx = 0;
    for (u32 k = 0; k < l.batchCount && l.batchStart + k < count; k++)
        idx += At<u32>(w, batches + 28 * (l.batchStart + k) + 0x14);
    return idx / 3;
}

#if LOD_NO_LENS
bool IsLensMaterial(const u8* w, u32 mat) {
    u32 mats = At<u32>(w, 0x68), count = At<u32>(w, 0x6C);   // 0x30-byte records; +0x18 / +0x1C textures {name, hash}
    if (mat >= count) return false;
    u32 tex = At<u32>(w, mats + 0x30 * mat + 0x18), n = At<u32>(w, mats + 0x30 * mat + 0x1C);
    for (u32 k = 0; k < n && k < 32; k++) {
        u32 h = At<u32>(w, tex + 8 * k + 4);
        if (h == 0x1F753DC3 || h == 0x4E23DC29) return true;
    }
    return false;
}

// Drops every lens batch from each LOD record by rotating the record's batches [k, end) left by one, in both the
// 28-byte batch table and the 24-byte batch info array (they are parallel), then shrinking the count. Rotating keeps
// the draw order of the other batches (alpha-sorted hair). The lens ends up just past the record's end, where nothing
// draws it, so a repeat lookup finds nothing left to remove.
void RemoveLens(void* p, u8* w, WmbLod* lods, u32 cnt) {
    u32 batches = At<u32>(w, 0x40);
    for (u32 i = 0; i < cnt; i++) {
        WmbLod& l = lods[i];
        for (u32 k = 0; k < l.batchCount;) {
            if (!IsLensMaterial(w, At<u32>(w, l.batchInfo + 24 * k + 8))) { k++; continue; }
            u8* b = w + batches + 28 * (l.batchStart + k);
            u8* info = w + l.batchInfo + 24 * k;
            u32 rest = l.batchCount - k - 1;
            u8 tb[28], ti[24];
            Logging.Log("[ORLOD] wmb %p: LOD%d lens batch %u (%u tris) hidden", p, l.level, l.batchStart + k,
                        At<u32>(b, 0x14) / 3);
            std::memcpy(tb, b, 28);
            std::memmove(b, b + 28, 28 * rest);
            std::memcpy(b + 28 * rest, tb, 28);
            std::memcpy(ti, info, 24);
            std::memmove(info, info + 24, 24 * rest);
            std::memcpy(info + 24 * rest, ti, 24);
            l.batchCount--;
        }
    }
}
#endif

void PatchWmb(void* p) {
    if (!p) return;
    u8* w = static_cast<u8*>(p);
    if (At<u32>(w, 0) != kWmb3Magic) return;
    u32 off = At<u32>(w, 0x48), cnt = At<u32>(w, 0x4C);
    if (cnt < 1 || cnt > 16 || off < 0x80 || off > 0x10000000) return;

    WmbLod lods[16];
    std::memcpy(lods, w + off, cnt * sizeof(WmbLod));
#if LOD_NO_LENS
    // Before the redirect, so LOD1 / LOD2 inherit LOD0's shrunk range.
    RemoveLens(p, w, lods, cnt);
    std::memcpy(w + off, lods, cnt * sizeof(WmbLod));
#endif
    if (cnt < 2) return;
    int src = -1;
    for (u32 i = 0; i < cnt; i++)
        if (lods[i].level == LOD_TARGET) { src = static_cast<int>(i); break; }
    if (src < 0) return;   // no such level (single-LOD and LOD0+SHADOW models stop here at LOD_TARGET 2)

    const WmbLod& s = lods[src];
    int changed = 0;
    for (u32 i = 0; i < cnt; i++) {
        WmbLod& l = lods[i];
        if (l.level < 0 || l.level == LOD_TARGET) continue;   // shadow / outline proxies keep their own meshes
        if (l.batchStart == s.batchStart && l.batchInfo == s.batchInfo && l.batchCount == s.batchCount) continue;
        if (changed == 0)
            Logging.Log("[ORLOD] wmb %p: %u lods, LOD%d %u tris", p, cnt, LOD_TARGET, Tris(w, s));
        Logging.Log("[ORLOD]   LOD%d %u batches / %u tris -> LOD%d", l.level, l.batchCount, Tris(w, l), LOD_TARGET);
        l.batchStart = s.batchStart;
        l.batchInfo = s.batchInfo;
        l.batchCount = s.batchCount;
        changed++;
    }
    if (changed) {
        std::memcpy(w + off, lods, cnt * sizeof(WmbLod));
        g_models++;
        if (g_models % 25 == 0) Logging.Log("[ORLOD] %u models patched (%u repeat lookups)", g_models, g_seen);
    } else {
        g_seen++;
    }
}

HOOK_DEFINE_TRAMPOLINE(FindByExtHook) {
    static void* Callback(void* dat, const char* ext, u32 nth) {
        void* r = Orig(dat, ext, nth);
        if (r && ext && ext[0] == 'w' && ext[1] == 'm' && ext[2] == 'b' && ext[3] == 0) PatchWmb(r);
        return r;
    }
};

HOOK_DEFINE_TRAMPOLINE(FindByNameHook) {
    static void* Callback(void* dat, const char* name) {
        void* r = Orig(dat, name);
        if (r && name) {
            size_t n = std::strlen(name);
            if (n > 4 && std::strcmp(name + n - 4, ".wmb") == 0) PatchWmb(r);
        }
        return r;
    }
};

bool Check(ptrdiff_t off, u32 expected) {
    u32 got = Abs<u32>(off);
    if (got != expected) Logging.Log("[ORLOD] %lx MISMATCH got=%08x want=%08x", off, got, expected);
    return got == expected;
}

}  // namespace

extern "C" void exl_main(void* x0, void* x1) {
    exl::hook::Initialize();
    Logging.Log("[ORLOD] v0.2.0 target=LOD%d nolens=%d", LOD_TARGET, LOD_NO_LENS);

    // Wrong game build: install nothing. Prologue + the extension-table / name-table loads of each lookup.
    if (!Check(kFindFileByExt, 0xA9BB7BFD) || !Check(kFindFileByExt + 0x20, 0xB9400D09) ||
        !Check(kFindFileByName, 0xA9BC7BFD) || !Check(kFindFileByName + 0x2C, 0xB9401008))
        return;

    FindByExtHook::InstallAtOffset(kFindFileByExt);
    FindByNameHook::InstallAtOffset(kFindFileByName);
    Logging.Log("[ORLOD] hooks installed");
}

extern "C" NORETURN void exl_exception_entry() {
    EXL_ABORT("Default exception handler called!");
}
