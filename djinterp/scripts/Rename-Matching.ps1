<#
.SYNOPSIS
    Bulk-rename files and/or folders by replacing a substring in their names.

.DESCRIPTION
    Searches a directory for items whose names contain a target string,
    then renames them by replacing that string with a replacement string.

.PARAMETER Path
    The root directory to search in.

.PARAMETER Find
    The substring to search for in item names.

.PARAMETER Replace
    The string to substitute in place of -Find. Defaults to empty string (removal).

.PARAMETER Extensions
    One or more extensions to include (e.g. ".txt", ".log"). Default: any extension.

.PARAMETER NoExtension
    Only include files that have no extension.

.PARAMETER ItemType
    What to rename: "Files", "Directories", or "Both". Default: "Files".

.PARAMETER Recurse
    Search subdirectories. Default: $true.

.PARAMETER IncludeHidden
    Include hidden items. Default: $false.

.PARAMETER WhatIf
    Preview changes without actually renaming anything.

.EXAMPLE
    .\Rename-Items.ps1 -Path "C:\Projects" -Find "old" -Replace "new"

.EXAMPLE
    .\Rename-Items.ps1 -Path . -Find "backup" -Replace "" -Extensions ".txt",".log" -Recurse:$false

.EXAMPLE
    .\Rename-Items.ps1 -Path . -Find "temp" -ItemType Both -IncludeHidden -WhatIf
#>

[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)]
    [ValidateScript({ Test-Path $_ -PathType Container })]
    [string]$Path,

    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string]$Find,

    [string]$Replace = "",

    [string[]]$Extensions,

    [switch]$NoExtension,

    [ValidateSet("Files", "Directories", "Both")]
    [string]$ItemType = "Files",

    [bool]$Recurse = $true,

    [switch]$IncludeHidden
)

# ── Resolve to absolute path ──────────────────────────────────────────
$Path = (Resolve-Path $Path).Path

# ── Build Get-ChildItem parameters ────────────────────────────────────
$gciParams = @{ Path = $Path }

if ($Recurse)       { $gciParams["Recurse"] = $true }
if ($IncludeHidden) { $gciParams["Force"]   = $true }

# ── Collect candidate items ───────────────────────────────────────────
$items = switch ($ItemType) {
    "Files"       { Get-ChildItem @gciParams -File }
    "Directories" { Get-ChildItem @gciParams -Directory }
    "Both"        { Get-ChildItem @gciParams }
}

# ── Filter: name must contain the search string ───────────────────────
$items = $items | Where-Object { $_.Name -like "*$Find*" }

# ── Filter: extension constraints ─────────────────────────────────────
if ($NoExtension) {
    $items = $items | Where-Object { $_.Extension -eq "" }
}
elseif ($Extensions -and $Extensions.Count -gt 0) {
    # Normalise: ensure each extension starts with a dot
    $normExt = $Extensions | ForEach-Object {
        if ($_ -notlike ".*") { ".$_" } else { $_ }
    }
    $items = $items | Where-Object { $normExt -contains $_.Extension }
}

# ── Sort deepest-path-first so child renames happen before parents ────
$items = $items | Sort-Object { $_.FullName.Length } -Descending

# ── Rename ────────────────────────────────────────────────────────────
$count = 0
foreach ($item in $items) {
    $newName = $item.Name -replace [regex]::Escape($Find), $Replace

    if ($newName -eq $item.Name) { continue }

    # Guard against name collisions
    $newPath = Join-Path $item.Directory.FullName $newName
    if ($item.PSIsContainer) {
        $newPath = Join-Path (Split-Path $item.FullName -Parent) $newName
    }

    if ((Test-Path $newPath) -and ($newPath -ne $item.FullName)) {
        Write-Warning "SKIP (target exists): $($item.FullName) -> $newName"
        continue
    }

    if ($PSCmdlet.ShouldProcess($item.FullName, "Rename to '$newName'")) {
        Rename-Item -LiteralPath $item.FullName -NewName $newName
        Write-Host "Renamed: $($item.FullName) -> $newName" -ForegroundColor Green
        $count++
    }
}

Write-Host "`nDone. $count item(s) renamed." -ForegroundColor Cyan