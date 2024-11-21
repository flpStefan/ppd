$param1 = $args[0] # Nume fisier exe
$param2 = $args[1] # No of reader_threads
$param3 = $args[2] # No of worker_threads
$param4 = $args[3] # No of runs

# Executare exe in cmd mode

$suma = 0

for ($i = 0; $i -lt $param4; $i++){
    Write-Host "Rulare" ($i+1)
    $a = (cmd /c .\$param1 $param2 $param3 2`>`&1)

    $execTime = [double]::Parse($a.Trim(), [System.Globalization.CultureInfo]::InvariantCulture)

    Write-Host "Timp de executie: $execTime ms"
    $suma += $execTime
    Write-Host ""
}
$media = $suma / $param4
Write-Host "Timp de executie mediu:" $media

# Creare fisier .csv
if (!(Test-Path outC.csv)){
    New-Item outC.csv -ItemType File
    #Scrie date in csv
    Set-Content outC.csv 'Tip Matrice,Tip alocare,Nr threads,Timp executie'
}

# Append
Add-Content outC.csv ",,$($args[1]),$($media)"