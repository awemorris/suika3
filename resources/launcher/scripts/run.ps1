Add-Type -AssemblyName System.Windows.Forms

$dialog = New-Object System.Windows.Forms.OpenFileDialog
$dialog.Title = "Open Ray Script or NovelML Scenario"
$dialog.Filter = "NovelML Scenario Files (*.novel)|*.novel|Ray Script Files (*.ray)|*.ray|All Files (*.*)|*.*"
$dialog.InitialDirectory = [Environment]::GetFolderPath("MyDocuments")
$dialog.Multiselect = $false

if ($dialog.ShowDialog() -eq "OK") {
    $file = $dialog.FileName
    $dir = Split-Path $file -Parent

    $binDir = (Get-Location).Path
    $binDir = Split-Path $binDir -Parent
    $command = Join-Path $binDir "\\bin\\suika3.exe"

    Set-Location $dir
    Start-Process $command
}
