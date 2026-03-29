$f = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\KEIL\HSUSBD_Mass_Storage_ShortPacket.uvprojx"
$lib = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\Library"
$content = Get-Content $f -Raw
# Replace forward slash version
$test = '..\..\..\..\Library'
$simpleReplace = $content.Replace($test, $lib)
Write-Host "Diff (forward slash): $($content.Length - $simpleReplace.Length)"
if ($content.Length -ne $simpleReplace.Length) {
    Set-Content -Path $f -Value $simpleReplace -NoNewline -Encoding UTF8
    Write-Host "Saved (forward slash)"
}
