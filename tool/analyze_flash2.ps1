$b = [System.IO.File]::ReadAllBytes('C:\Users\rinry\m487_flash.bin')
$lastNonFF = -1
for($i=524287; $i -ge 0; $i--) {
    if ($b[$i] -ne 0xFF) { 
        $lastNonFF = $i
        break
    }
}
$nonFFCount = 0
for($i=0; $i -lt 524288; $i++) {
    if ($b[$i] -ne 0xFF) { $nonFFCount++ }
}
Write-Host "Total non-0xFF bytes: $nonFFCount"
Write-Host "Last non-0xFF address: 0x$($lastNonFF.ToString('X6')) ($lastNonFF)"
if ($lastNonFF -gt 0) {
    $start = [Math]::Max(0, $lastNonFF - 16)
    Write-Host "`nBytes around last non-FF:"
    for($i=$start; $i -le $lastNonFF+8 -and $i -lt 524288; $i++) {
        Write-Host ("{0:X6}: {1:X2}" -f $i, $b[$i]) -NoNewline
        if (($i - $start + 1) % 16 -eq 0) { Write-Host "" }
    }
}
