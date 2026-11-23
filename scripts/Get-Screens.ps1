param([string]$Key='')
# I-0098 (Thomas, 2026-10-24: one gate base per screen): the screens attached now, each with its device name, its place on the
# desktop, its refresh rate (the desktop mode's, a whole number: 280 for 279.96 Hz) and its base key « <width>x<height>@<hz> ».
# With -Key, the one screen of that key (or of that device name, \\.\DISPLAY1), or an error when none or several match.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
if(-not ('I98Screens' -as [type])){ Add-Type @'
using System;using System.Runtime.InteropServices;
public static class I98Screens{
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)]public struct DEVMODE{[MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)]public string name;
  public short specVersion,driverVersion,size,driverExtra;public int fields,x,y,orientation,fixedOutput;public short color,duplex,yResolution,ttOption,collate;
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)]public string form;public short logPixels;public int bitsPerPel,width,height,flags,frequency,icm,intent,media,dither,r1,r2,panningWidth,panningHeight;}
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern bool EnumDisplaySettingsW(string device,int mode,ref DEVMODE dm);
 public static int[] Mode(string device){DEVMODE m=new DEVMODE();m.size=(short)Marshal.SizeOf(typeof(DEVMODE));
  if(!EnumDisplaySettingsW(device,-1,ref m))return new int[]{0,0,0};return new int[]{m.width,m.height,m.frequency};}
}
'@ }
$screens=@([Windows.Forms.Screen]::AllScreens | ForEach-Object { $m=[I98Screens]::Mode($_.DeviceName)
    [pscustomobject][ordered]@{key="$($m[0])x$($m[1])@$($m[2])";device=$_.DeviceName;primary=$_.Primary;x=$_.Bounds.X;y=$_.Bounds.Y
        width=$_.Bounds.Width;height=$_.Bounds.Height;hz=$m[2]} })
if(-not $Key){ return $screens }
$found=@($screens | Where-Object { $_.key -eq $Key -or $_.device -eq $Key })
if($found.Count -ne 1){ throw "screen '$Key': $($found.Count) match among $(@($screens | ForEach-Object { "$($_.key) ($($_.device))" }) -join ', ')" }
$found[0]
