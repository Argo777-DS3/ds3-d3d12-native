[CmdletBinding()]
param([Parameter(Mandatory)][string]$Out,[string]$Trace='',[ValidateRange(1,10)][int]$Losses=3,[ValidateRange(30,600)][int]$StartAfterSeconds=150,
    [ValidateRange(2,60)][int]$AwaySeconds=6,[ValidateRange(5,120)][int]$BackSeconds=20,[switch]$Close)
# I-0101 (STUDY §3.6 test): focus losses of a running DarkSoulsIII in exclusive full screen, as a player's Alt-Tab: Alt+Tab
# sent as input (SendInput), the game away for a few seconds, then Alt+Tab again to come back. Each step records the game
# window's state (foreground, minimised, its rectangle) in $Out. With -Close, the game is then closed as by its close box
# (WM_SYSCOMMAND SC_CLOSE posted to its window). Never launches it.
$ErrorActionPreference='Stop'
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class I101Focus {
 [StructLayout(LayoutKind.Sequential)] public struct RECT{public int L,T,R,B;}
 [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT{public ushort wVk,wScan;public uint dwFlags,time;public IntPtr extra;}
 [StructLayout(LayoutKind.Explicit,Size=40)] public struct INPUT{[FieldOffset(0)]public uint type;[FieldOffset(8)]public KEYBDINPUT ki;}
 [DllImport("user32.dll")] static extern uint SendInput(uint n,INPUT[] inputs,int size);
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint msg,IntPtr w,IntPtr l);
 [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h,out RECT r);
 static INPUT Key(ushort vk,bool up){var i=new INPUT();i.type=1;i.ki.wVk=vk;i.ki.dwFlags=up?2u:0u;return i;}
 public static uint AltTab(){var k=new INPUT[]{Key(0x12,false),Key(0x09,false),Key(0x09,true),Key(0x12,true)};
  uint sent=0;foreach(var one in k){sent+=SendInput(1,new[]{one},Marshal.SizeOf(typeof(INPUT)));System.Threading.Thread.Sleep(60);}return sent;}
}
'@
$rows=[System.Collections.Generic.List[object]]::new()
function State([string]$step){$p=Get-Process DarkSoulsIII -ErrorAction SilentlyContinue | Select-Object -First 1
    $h=if($p){$p.MainWindowHandle}else{[IntPtr]::Zero};$r=New-Object I101Focus+RECT;$null=if($h -ne [IntPtr]::Zero){[I101Focus]::GetWindowRect($h,[ref]$r)}
    $rows.Add([ordered]@{step=$step;time=(Get-Date).ToString('HH:mm:ss.fff');alive=[bool]$p;foreground=($h -ne [IntPtr]::Zero -and [I101Focus]::GetForegroundWindow() -eq $h);
        iconic=($h -ne [IntPtr]::Zero -and [I101Focus]::IsIconic($h));rect=@($r.L,$r.T,$r.R,$r.B)})
    $rows | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $Out -Encoding utf8}
$deadline=(Get-Date).AddSeconds(240);while(-not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $deadline){Start-Sleep 2}
if(-not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){State 'no_game';return}
# With -Trace (the run's startup.jsonl): the wait starts at the first world frame (the pilot has passed the title by then).
if($Trace){$deadline=(Get-Date).AddSeconds(300);while((Get-Date) -lt $deadline -and -not ((Test-Path -LiteralPath $Trace) -and (Select-String -LiteralPath $Trace -Pattern 'native_model_draw_submitted' -SimpleMatch -Quiet))){Start-Sleep 2}}
Start-Sleep -Seconds $StartAfterSeconds;State 'before'
for($i=1;$i -le $Losses;$i++){
    if(-not (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){State "gone_$i";break}
    $null=[I101Focus]::AltTab();Start-Sleep -Milliseconds 800;State "away_$i";Start-Sleep -Seconds $AwaySeconds;State "still_away_$i"
    $null=[I101Focus]::AltTab();Start-Sleep -Seconds 3;State "back_$i";Start-Sleep -Seconds $BackSeconds;State "settled_$i"}
if($Close -and (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue)){
    $p=Get-Process DarkSoulsIII | Select-Object -First 1;State 'before_close'
    $null=[I101Focus]::PostMessage($p.MainWindowHandle,0x0112,[IntPtr]0xF060,[IntPtr]::Zero)
    $deadline=(Get-Date).AddSeconds(40);while((Get-Process -Id $p.Id -ErrorAction SilentlyContinue) -and (Get-Date) -lt $deadline){Start-Sleep -Milliseconds 250}
    State 'after_close'}
