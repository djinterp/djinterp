<# NewModuleLayout.ps1
Creates a module/project layout in one or many target folders under -Root.

Examples:
  # Single folder (old behavior):
  .\NewModuleLayout.ps1 -Root "D:\blah\foo"

  # Many folders under a parent root:
  .\NewModuleLayout.ps1 -Root "D:\blah" -Targets "foo","bar","derp"

  # With file copying:
  .\NewModuleLayout.ps1 -Root "D:\blah" -Targets foo,bar `
    -LicensePath "D:\templates\LICENSE_MIT.txt" `
    -ReadmePath  "D:\templates\README.md" `
    -CopyFile    "D:\templates\.gitignore" `
    -CopyFile    "D:\templates\clang-format:.clang-format"
#>

[CmdletBinding(SupportsShouldProcess=$true)]
param(
  [Parameter(Mandatory=$true)]
  [ValidateNotNullOrEmpty()]
  [string] $Root,

  # Targets are subfolders under -Root. If omitted/empty, -Root itself is used.
  [string[]] $Targets = @(),

  [string] $LicensePath,
  [string] $ReadmePath,

  # "src" or "src:relative\dest.ext" (dest is relative to EACH target root)
  [string[]] $CopyFile = @(),

  [switch] $Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-FullPath([string]$p) {
  if ([string]::IsNullOrWhiteSpace($p)) { return $null }
  return (Resolve-Path -LiteralPath $p).Path
}

function Ensure-Dir([string]$path) {
  if ($PSCmdlet.ShouldProcess($path, "Create directory")) {
    New-Item -ItemType Directory -Path $path -Force | Out-Null
  }
}

function Parse-CopySpec([string]$spec) {
  # Accept "src" or "src:destRel" using LAST ':' so "C:\..." is safe
  $idx = $spec.LastIndexOf(':')
  if ($idx -le 1) {
    return [pscustomobject]@{ Src = $spec; DestRel = (Split-Path -Leaf $spec) }
  }
  $src  = $spec.Substring(0, $idx)
  $dest = $spec.Substring($idx + 1)
  if ([string]::IsNullOrWhiteSpace($dest)) { $dest = Split-Path -Leaf $src }
  return [pscustomobject]@{ Src = $src; DestRel = $dest }
}

function Copy-FileRelative([string]$DestRoot, [string]$src, [string]$destRelative) {
  $srcFull = Resolve-FullPath $src
  if (-not (Test-Path -LiteralPath $srcFull)) { throw "Source file not found: $src" }

  $destFull = Join-Path $DestRoot $destRelative
  $destDir  = Split-Path -Parent $destFull
  if ($destDir -and -not (Test-Path -LiteralPath $destDir)) { Ensure-Dir $destDir }

  $params = @{ LiteralPath = $srcFull; Destination = $destFull }
  if ($Force) { $params.Force = $true }

  if ($PSCmdlet.ShouldProcess("$srcFull -> $destFull", "Copy file")) {
    Copy-Item @params
  }
}

function Copy-License([string]$DestRoot, [string]$licenseSrc) {
  $srcFull = Resolve-FullPath $licenseSrc
  $ext = [IO.Path]::GetExtension($srcFull)
  $destName = if ([string]::IsNullOrEmpty($ext)) { "LICENSE" } else { "LICENSE$ext" }
  Copy-FileRelative $DestRoot $licenseSrc $destName
}

function Copy-Readme([string]$DestRoot, [string]$readmeSrc) {
  $srcFull = Resolve-FullPath $readmeSrc
  $ext = [IO.Path]::GetExtension($srcFull)
  $destName = if ([string]::IsNullOrEmpty($ext)) { "README" } else { "README$ext" }
  Copy-FileRelative $DestRoot $readmeSrc $destName
}

# ---- layout template (or load from file if you prefer)
$Template = @"
.confg^
----.msvs^
--------modules
--------testing
bin
build
----cmake
----make
----msvs
docs
inc
lib
res
scripts
src
tests
"@

$StructureScript = Join-Path $PSScriptRoot "DirectoryLayoutFromTemplate.ps1"
if (-not (Test-Path -LiteralPath $StructureScript)) {
  throw "Missing helper script: $StructureScript"
}

$RootFull = [IO.Path]::GetFullPath($Root)
Ensure-Dir $RootFull

# If no targets given, apply layout to Root itself
if (-not $Targets -or $Targets.Count -eq 0) {
  $Targets = @("")  # sentinel = RootFull
}

foreach ($t in $Targets) {
  $t2 = if ($null -eq $t) { "" } else { $t.Trim() }
  $DestRoot = if ([string]::IsNullOrWhiteSpace($t2)) { $RootFull } else { Join-Path $RootFull $t2 }

  Ensure-Dir $DestRoot

  # Create structure
  & $StructureScript `
    -Root $DestRoot `
    -Template $Template `
    -ChildToken "----" `
    -HiddenToken "^" `
    -HiddenTokenPosition "Suffix" `
    -DefaultItemType "Directory" `
    -Force:$Force `
    -WhatIf:$WhatIfPreference

  # Copy optional files into EACH target root
  if ($LicensePath) { Copy-License $DestRoot $LicensePath }
  if ($ReadmePath)  { Copy-Readme  $DestRoot $ReadmePath  }

  foreach ($spec in $CopyFile) {
    $p = Parse-CopySpec $spec
    Copy-FileRelative $DestRoot $p.Src $p.DestRel
  }

  Write-Host "Done: $DestRoot"
}
