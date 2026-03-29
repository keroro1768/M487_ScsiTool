$b = [System.IO.File]::ReadAllBytes('C:\Users\rinry\m487_flash.bin')
$nonFF = 0
$lastAddr = 0
for($i=0; $i -lt 524288; $i++) {
    if ($b[$i] -ne 0xFF) { 
        $nonFF++
        if ($i -gt 524000) { $lastAddr = $i }
    }
}
Write-Host "Total non-0xFF bytes: $nonFF"
Write-Host "Last non-0xFF address: $lastAddr (0x$($lastAddr.ToString('X6')))"
# Show a few bytes around the last non-FF address
if ($lastAddr -gt 0) {
    $start = [Math]::Max(0, $lastAddr - 10)
    Write-Host "Bytes around last non-FF:"
    for($i=$start; $i -le $lastAddr+5 -and $i -lt 524288; $i++) {
        Write-Host ("{0:X6}: {1:X2}" -f $i, $b[$i]) -NoNewline
        if (($i - $start + 1) % 8 -eq 0) { Write-Host "" }
    }
}
