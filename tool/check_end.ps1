$b = [System.IO.File]::ReadAllBytes('C:\Users\rinry\m487_flash.bin')
$end = $b[524240..524287]
Format-Hex ([byte[]]$end)
