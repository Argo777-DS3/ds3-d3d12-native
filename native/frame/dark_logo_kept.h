#pragma once
// D-0182 (I-0135; the coordinator's agreement of 2026-11-12 06:03, option a): the game's Bandai Namco logo bytes, kept once.
// The module frees every game texture's bytes once uploaded (I-0096), and the page's image is then a resident resource bound as
// is, so the logo made for black (dark_logo.h) has nothing to start from. The texture admission (14_scene_step_and_storage.inc)
// keeps a copy of ONE texture, chosen by its TPF entry name MENU_BNE_Logo, with the resource it was uploaded to; the UI session
// (game_ui_session.cpp, DarkLogoAdapt) makes the logo for black from it once, then frees the copy and recognises the image by
// that resource; once the page has gone, everything is released and nothing more kept. One instance for the module (an inline
// function's static is shared by every unit).
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <mutex>
#include <vector>

namespace ds3native::frame {
struct DarkLogoKeptSource {
    std::mutex lock;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;   // the logo's upload: how the page's image is recognised
    std::vector<uint8_t> bc1;                          // its first mip, freed once the logo for black is made
    uint32_t width{},height{};
    bool closed{};                                     // the page has gone: nothing more is kept (final review L2)
};
inline DarkLogoKeptSource& DarkLogoKept(){static DarkLogoKeptSource kept;return kept;}
}  // namespace ds3native::frame
