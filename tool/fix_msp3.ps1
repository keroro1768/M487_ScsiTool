$f = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\KEIL\HSUSBD_Mass_Storage_ShortPacket.uvprojx"
$lib = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\Library"
$content = Get-Content $f -Raw
$lines = $content -split "`n"
foreach ($line in $lines) {
    if ($line -match 'Library') {
        Write-Host "Library line: $($line.Substring(0, [Math]::Min(100, $line.Length)))"
        break
    }
}
# Try simple string replace
$test = '..\..\..\..\Library'
$simpleReplace = $content.Replace($test, $lib)
Write-Host "Simple replace diff: $(($content.Length - $simpleReplace.Length))"
if ($content.Length -ne $simpleReplace.Length) {
    Set-Content -Path $f -Value $simpleReplace -NoNewline -Encoding UTF8
    Write-Host "Saved with simple replace"
}
