[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('open','pick')][string]$Step,[ValidateSet('head','body','arms','legs')][string]$Slot='head',
    [ValidateRange(0,40)][int]$Row=0,[ValidateRange(0,4)][int]$Column=0,[string]$Capture='')
# I-0066 (1124): equipment list piloting of an announced Reference run (1280x720 window at the screen origin).
# open: Esc, Equipment, the slot, scroll so that inventory row $Row is the last visible row (or the top when it is
# visible already), hover the item and optionally capture the panel; pick: click the hovered position, then Esc.
# Grid geometry measured in 1118b: slots at y=368 (x 117/183/249/315), item cells x=128+66*column, y=215+74*row.
$ErrorActionPreference='Stop'
$pilot=Join-Path $PSScriptRoot 'Pilot-Game.ps1'
function P([string[]]$a){ pwsh -NoProfile -File $pilot @a | Out-Null }
$visible=[Math]::Min($Row,5); $scroll=$Row-$visible
$x=128+66*$Column; $y=215+74*$visible; if($visible -eq 5){ $y=585 }
if($Step -eq 'open'){
    P '-Action','key','-Vk','27'; Start-Sleep -Milliseconds 700
    P '-Action','click','-X','263','-Y','390'; Start-Sleep -Milliseconds 900
    $sx=@{head=117;body=183;arms=249;legs=315}[$Slot]; P '-Action','click','-X',"$sx",'-Y','368'; Start-Sleep -Milliseconds 900
    P '-Action','wheel','-X','260','-Y','400','-Notches','20'; Start-Sleep -Milliseconds 400
    if($scroll){ P '-Action','wheel','-X','260','-Y','400','-Notches',"-$scroll"; Start-Sleep -Milliseconds 400 }
    P '-Action','wheel','-X',"$x",'-Y',"$y",'-Notches','0'; Start-Sleep -Milliseconds 500
    if($Capture){ pwsh -NoProfile -File $pilot -Action shot -Path "$env:TEMP\equip-full.jpg" | Out-Null
        Add-Type -AssemblyName System.Drawing; $b=[Drawing.Bitmap]::FromFile("$env:TEMP\equip-full.jpg")
        $c=$b.Clone((New-Object Drawing.Rectangle 60,125,840,60),$b.PixelFormat); $c.Save($Capture,[Drawing.Imaging.ImageFormat]::Jpeg); $b.Dispose(); $c.Dispose() }
    "hovered $Slot row $Row column $Column at $x,$y"
}else{
    P '-Action','click','-X',"$x",'-Y',"$y"; Start-Sleep -Milliseconds 800
    P '-Action','key','-Vk','27'; Start-Sleep -Milliseconds 600
    "picked $Slot row $Row column $Column"
}
