# I-0138 tranches 5 and 5b: the HDR output's rows (on the Graphics tab since tranche 5b) and the live switch between the SDR and
# the HDR10 chains. One process, Eta's m40 zone save (-Continue at Firelink) through Run-AlphaCast's -OptionsTour and -OptionsSteps (offline,
# restored by hash).
#   <prefix>-cycle: hdr-cycle.flag (the request alternating every 600 engine frames between off and a forced HDR10 chain) through
#     the options' tour (Esc, System, the eight tabs), then the Graphics tab with the cursor moved down and up four times (the same two pictures, a still each,
#     in both states of the chain: the menu's dimming of the scene compared between them), world-pictures.flag writing the presented
#     picture and what the output pass read.
#   <prefix>-copy: a forced HDR10 chain with no-yebis-tonemap.flag (no tone map of ours, so every world frame's 8-bit picture is
#     copied into the float picture), in the game (I-0138 tranche 8: post-tone-old.flag, its first form, is gone).
#   <prefix>-resize: a forced HDR10 chain; Esc, System, the Graphics tab, Down to Resolution, E (its list), Down (another size), E,
#     Left then E (the game's « Keep current settings? »: kept), Esc three times: the chain resized to another size and the float
#     picture with it. (The first form, Right on the row, changed nothing: a value row opens a list.)
# On a screen with Windows' HDR on (the OLED: -Screen 2560x1440@280 and -GraphicsLevels « ScreenMode=WINDOW;Resolution-
# WindowScreenWidth=2560;Resolution-WindowScreenHeight=1440 », WORKFLOW « Live game rule »), tranche 5's last step:
#   <prefix>-auto: hdr-output.flag (HDR10 where Windows' HDR is on), in the game, world-pictures.flag's pictures;
#   <prefix>-autoresize: the same with -resize's steps;
#   <prefix>-dlaa, <prefix>-dlss: hdr-output.flag with dlss-dlaa.flag or dlss-quality.flag (DLSS on an HDR10 chain);
#   <prefix>-fg: hdr-output.flag with dlss-dlaa.flag and fg-on.flag (frame generation's chain and its HUD-less colour in HDR10);
#   <prefix>-fullscreen: hdr-output.flag in exclusive full screen (-GraphicsLevels' ScreenMode replaced by FULLSCREEN at the
#     screen's size for this run).
# I-0138 tranche 6 (the candidate looks on a real HDR10 chain, a module that has them):
#   <prefix>-lookcycle: hdr-output.flag with tone-look-cycle.flag (every look's HDR entry in turn, world-pictures.flag's pictures);
#   <prefix>-lookkeys: hdr-output.flag with tone-look-keys.flag: F2 six times (every look, back to the original), then F3 four
#     times (the four knees), a shot after each (key-step-<i>.png: the label; a GDI shot of the OLED can be stale, LIVE-RUN-NOTES).
# I-0138 tranche 5b (the HDR rows on the Graphics tab; tranche 5's <prefix>-menu run, made for its rows of the Advanced page, went
# with them):
#   <prefix>-tab: Esc, System, the eight tabs, then the Graphics tab; Down four times (the HDR output's row), Right (on: on a screen
#     with Windows' HDR the chain becomes HDR10; its two slider rows appear under it), Down, Right three times and Left (the peak:
#     400, 450, 500, 450 nits), Down, Right twice and Left three times (the paper white: 210, 220, 210, 200, 190), the Brightness
#     tab (its row locked while the chain is HDR10), the Graphics tab again (its rows built from the settings), Down five times,
#     Left twice (the peak back to automatic), Down, Right (the paper white back to 200), Up twice, Left (off: the chain SDR again,
#     the two rows gone since tranche 5d, greyed before it), Down twice and Up twice (tranche 5d: the cursor on Advanced Settings
#     in the sixth slot, then on Defaults, with the output off), the Brightness tab (its slider back), Esc three times. The
#     settings end as they began. Since tranche 5d the tab opens with seven rows (the output off) and has nine with it on.
#   <prefix>-tabon (tranche 5d, the merge's final review, M1): the tab first built with the HDR output already on, and the Advanced
#     page from it. Esc, System, the Graphics tab, Down four times, Right (on), Esc three times (one Esc from a page closes the
#     menus whole, the next opens the game's menu, the third closes it); then Esc, System, the Graphics tab again (opened with the
#     output on: nine rows on a new page), Down seven times (the cursor on Advanced Settings), E (its page), Down, Esc. As run
#     on 2026-11-15 (i138-t5d3-tabon) that Esc closed the menus whole, so the steps after it (Up three times, Left: meant to
#     switch the output off again) met no menu: the run ends with the output on in its own settings, which Run-StartupTrace
#     sets aside and restores (the player's file is never the run's). The Defaults press is not in it (it also resets the
#     screen mode).
#   <prefix>-advanced (I-0138 tranche 7): the Advanced page's rows, its new ones at its end (the colorimetry, its aberration sub-row
#     while Rekindled is chosen, the bloom (a slider), the light shafts, the film grain and the field of view (sliders), then the vignette). Esc, System, the
#     Graphics tab, Down five times (the cursor on Advanced Settings, the HDR output off), E (its page), Down 30 times (the page
#     scrolls to its last row, a shot each), Up five times (from the vignette, if the list does not wrap: the cursor on the colorimetry), Right (Rekindled: the aberration's row
#     appears under it), Left (the original again: it goes), Esc three times. The settings end as they began.
#   <prefix>-wheel (I-0141 tranche 11, Thomas 2026-11-17: the wheel did not scroll the Advanced page): Esc, System, the Graphics tab,
#     Down five times, E (the Advanced page), then the wheel over the list: 1 notch down, 3 down, 20 down (to the end), 2 up, 20 up
#     (to the top), a still after each; Esc three times. Rows native_rekindled_graphics "wheel" and "scroll_bar" in the journal.
#   <prefix>-hdrshot (Thomas, 2026-11-16 05:37: a 2560x1440 picture of the Graphics tab with the HDR output on): Esc, System, the
#     Graphics tab, Down four times (the HDR output), Right (on: its two sub-rows appear), a still, Esc three times; world-pictures.flag
#     writes the presented pictures too (a GDI shot of the OLED can be stale, LIVE-RUN-NOTES). The HDR output stays on in the run's
#     settings file, which Run-StartupTrace puts back at the end. Give it -Screen and -GraphicsLevels for 2560x1440 on the OLED.
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Prefix,[Parameter(Mandatory)][string]$BuildFolder,[Parameter(Mandatory)][string]$Log,
    [string]$Screen='1920x1080@60',[string]$NotBefore='',[string[]]$Runs=@('tab'),[string]$GraphicsLevels='',
    [string]$TestLanguage='')   # the game's language for the runs (Run-StartupTrace's -TestLanguage: french, english, ...): the rows' words in it
$ErrorActionPreference='Stop'
$Runs=@($Runs | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cast="$repo/investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1"
$eta='C:\Users\User\.ds3-i0039-backups\shebang-zones-eta-m40-20261014-095203\DS30000.sl2';$etaHash='1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4'
$tabSteps='c:683,192;k:40;k:40;k:40;k:40;k:39;k:40;k:39;k:39;k:39;k:37;k:40;k:39;k:39;k:37;k:37;k:37;c:503,192;c:683,192;k:40;k:40;k:40;k:40;k:40;k:37;k:37;k:40;k:39;k:38;k:38;k:37;k:40;k:40;k:38;k:38;c:503,192;k:27;k:27;k:27'
$say={param($t) "$t" | Add-Content -LiteralPath $Log}
try{
    $device=(& "$repo/scripts/Get-Screens.ps1" -Key $Screen).device
    if(-not $device){throw "no $Screen screen"}
    $env:DS3_PILOT_SCREEN=$device;& $say "$(Get-Date -Format HH:mm:ss) screen $device"
    if($NotBefore){$at=[datetime]::ParseExact($NotBefore,'HH:mm',$null);while((Get-Date) -lt $at){Start-Sleep 5}}
    foreach($run in $Runs){
        if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){& $say "$run skipped: DarkSoulsIII already runs";continue}
        & $say "$(Get-Date -Format HH:mm:ss) $Prefix-$run"
        $castArgs=@{RunName="$Prefix-$run";BootstrapMode='Native';SaveCopy=$eta;SaveCopySha256=$etaHash;Continue=$true;Casts=0;BuildFolder=$BuildFolder;OptionsTour=$true}
        if($GraphicsLevels){$castArgs.GraphicsLevels=$GraphicsLevels}
        if($TestLanguage){$castArgs.TestLanguage=$TestLanguage}
        switch($run){
            'tab'{$castArgs.EffectFlags='test-no-ai';$castArgs.OptionsSteps=$tabSteps;$castArgs.Seconds=420}
            'advanced'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai';$castArgs.Seconds=420
                $castArgs.OptionsSteps='k:27;c:518,390;c:683,192;k:40;k:40;k:40;k:40;k:40;k:69;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:38;k:38;k:38;k:38;k:38;k:39;k:37;k:27;k:27;k:27'}
            'wheel'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai';$castArgs.Seconds=300
                $castArgs.OptionsSteps='k:27;c:518,390;c:683,192;k:40;k:40;k:40;k:40;k:40;k:69;w:700,420,-1;w:700,420,-3;w:700,420,-20;w:700,420,2;w:700,420,20;k:27;k:27;k:27'}
            'hdrshot'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures';$castArgs.Seconds=240
                $castArgs.OptionsSteps='k:27;c:518,390;c:683,192;k:40;k:40;k:40;k:40;k:39;k:40;k:38;k:27;k:27;k:27'}
            'tabon'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai';$castArgs.Seconds=420
                $castArgs.OptionsSteps='k:27;c:518,390;c:683,192;k:40;k:40;k:40;k:40;k:39;k:27;k:27;k:27;k:27;c:518,390;c:683,192;k:40;k:40;k:40;k:40;k:40;k:40;k:40;k:69;k:40;k:27;k:38;k:38;k:38;k:37;k:27;k:27;k:27'}
            'cycle'{$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-cycle';$castArgs.OptionsSteps='c:683,192;k:40;k:38;k:40;k:38;k:40;k:38;k:40;k:38;k:27;k:27;k:27';$castArgs.Seconds=300}
            'copy'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output-force,no-yebis-tonemap';$castArgs.Seconds=120}
            'resize'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output-force';$castArgs.OptionsSteps='k:27;c:518,390;c:683,192;k:40;k:69;k:40;k:69;k:37;k:69;k:27;k:27;k:27';$castArgs.Seconds=180}
            'auto'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output';$castArgs.Seconds=120}
            'autoresize'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output';$castArgs.OptionsSteps='k:27;c:518,390;c:683,192;k:40;k:69;k:40;k:69;k:37;k:69;k:27;k:27;k:27';$castArgs.Seconds=180}
            'dlaa'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output,dlss-dlaa';$castArgs.Seconds=120}
            'dlss'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output,dlss-quality';$castArgs.Seconds=120}
            'fg'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output,dlss-dlaa,fg-on';$castArgs.Seconds=120}
            'fullscreen'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output';$castArgs.Seconds=120
                $size=($Screen -split '@')[0] -split 'x';$castArgs.GraphicsLevels="ScreenMode=FULLSCREEN;Resolution-FullScreenWidth=$($size[0]);Resolution-FullScreenHeight=$($size[1])"}
            'lookcycle'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output,tone-look-cycle';$castArgs.Seconds=150}
            'lookkeys'{$castArgs.Remove('OptionsTour');$castArgs.EffectFlags='test-no-ai,world-pictures,hdr-output,tone-look-keys';$castArgs.Seconds=150
                $castArgs.KeySteps='113,0;w2;113,0;w2;113,0;w2;113,0;w2;113,0;w2;113,0;w2;114,0;w2;114,0;w2;114,0;w2;114,0;w2'}
            default{throw "unknown run $run"} }
        & $cast @castArgs *>&1 | ForEach-Object { & $say "$_" } }
}catch{& $say "ERROR $_"}
finally{& $say "SERIES-END $(Get-Date -Format HH:mm:ss)"}
