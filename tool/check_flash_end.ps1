$b = [System.IO.File]::ReadAllBytes('C:\Users\rinry\m487_flash.bin')
$last48 = $b[524240..524287]
$sum = ($last48 | Measure-Object -Sum).Sum
Write-Host "Last 48 bytes sum: $sum"
if ($sum -eq 48 * 0xFF) {
    Write-Host "End of flash is blank (all 0xFF)"
} else {
    Write-Host "Last 48 bytes hex:"
    for($i=0; $i -lt 48; $i++) {
        Write-Host ("{0:X2} " -f $last48[$i]) -NoNewline
        if (($i+1) % 16 -eq 0) { Write-Host "" }
    }
}
