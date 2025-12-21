#pragma once

struct GameDowngradeData {
    u64 title_id;
    const char* build_id;
    const u8* patch_data;
    u64 patch_size;
};


static const u8 smo_patch[] = {
    0x49, 0x50, 0x53, 0x33, 0x32, 0x00, 0x53, 0x70, 0x44, 0x00,
    0x04, 0x00, 0x60, 0xB8, 0x52, 0x00, 0x53, 0x70, 0x68, 0x00,
    0x04, 0x06, 0x00, 0x00, 0x14, 0x45, 0x45, 0x4F, 0x46
};

constexpr GameDowngradeData g_downgrade_data[] = {
    {   // Super Mario Odyssey
        0x0100000000010000, "3CA12DFAAF9C82DA064D1698DF79CDA1",
        smo_patch, sizeof(smo_patch)
    },
};

inline const GameDowngradeData* getDowngradeDataForTitle(u64 title_id) {
    size_t count = sizeof(g_downgrade_data) / sizeof(GameDowngradeData);
    for (size_t i = 0; i < count; i++) {
        if (g_downgrade_data[i].title_id == title_id) {
            return &g_downgrade_data[i];
        }
    }
    return nullptr;
}

inline u8 get_program_id_offset(u64 title_id)
{
    return 0;
}

inline std::string get_romfs_path(u64 title_id) {
    return fmt::format("sdmc:/atmosphere/contents/{:016X}/romfs.bin", title_id);
}

inline std::string get_exefs_path(u64 title_id) {
    return fmt::format("sdmc:/atmosphere/contents/{:016X}/exefs.nsp", title_id);
}

inline std::string get_patch_path(const GameDowngradeData* downgrade_data) {
    return fmt::format("sdmc:/atmosphere/exefs_patches/downgrade/{}.ips", downgrade_data->build_id);
}
