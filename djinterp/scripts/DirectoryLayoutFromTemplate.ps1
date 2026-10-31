<#  Copy-StructureFromTemplate.ps1

Creates directories/files from a text template.

Template rules:
- Each non-empty, non-comment line describes ONE item.
- Depth is indicated by repeating -ChildToken at the start of the line.
  Example (ChildToken "----"):
    root
    ----child
    --------grandchild

- Hidden is indicated by -HiddenToken (prefix or suffix).
- Directory/File can be indicated by DirToken/FileToken (prefix or suffix).
- If no type token is present, -DefaultItemType applies.

Examples (defaults: ChildToken="----", HiddenToken="^", TypeTokenPosition="Suffix"):

  .confg^
  ----.msvs^
  --------modules
  --------testing

If you also want file support using a file marker:
  README.md*          # if -FileToken="*"
  docs/               # if -DirToken="/"

Notes:
- Hidden is implemented via Windows file attributes (not dot-prefix).
- For files, it creates empty files unless they already exist (use -Force to overwrite).
#>

[CmdletBinding(SupportsShouldProcess = $true)]
param(
  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string] $Root,

  [Parameter(ParameterSetName="FromString", Mandatory = $true)]
  [string] $Template,

  [Parameter(ParameterSetName="FromFile", Mandatory = $true)]
  [string] $TemplatePath,

  # How depth/child-of is represented (repeated at the start of a line)
  [string] $ChildToken = "----",

  # Hidden marker
  [string] $HiddenToken = "^",
  [ValidateSet("Prefix","Suffix")]
  [string] $HiddenTokenPosition = "Suffix",

  # Optional type markers (directory/file)
  [string] $DirToken = "",
  [string] $FileToken = "",
  [ValidateSet("Prefix","Suffix")]
  [string] $TypeTokenPosition = "Suffix",

  # If no DirToken/FileToken is present
  [ValidateSet("Directory","File")]
  [string] $DefaultItemType = "Directory",

  # Overwrite existing files (and create dirs even if present)
  [switch] $Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Ensure-Dir([string] $path) {
  if ($PSCmdlet.ShouldProcess($path, "Create directory")) {
    New-Item -ItemType Directory -Path $path -Force | Out-Null
  }
}

function Ensure-File([string] $path) {
  $dir = Split-Path -Parent $path
  if ($dir -and -not (Test-Path -LiteralPath $dir)) {
    Ensure-Dir $dir
  }

  if (Test-Path -LiteralPath $path) {
    if (-not $Force) { return }
    if ($PSCmdlet.ShouldProcess($path, "Overwrite file")) {
      Set-Content -LiteralPath $path -Value "" -Encoding UTF8
    }
    return
  }

  if ($PSCmdlet.ShouldProcess($path, "Create file")) {
    New-Item -ItemType File -Path $path -Force | Out-Null
  }
}

function Set-HiddenAttr([string] $path) {
  if (-not (Test-Path -LiteralPath $path)) { return }
  if ($PSCmdlet.ShouldProcess($path, "Set Hidden attribute")) {
    $item = Get-Item -LiteralPath $path
    if (($item.Attributes -band [IO.FileAttributes]::Hidden) -eq 0) {
      $item.Attributes = $item.Attributes -bor [IO.FileAttributes]::Hidden
    }
  }
}

function Strip-Token([string] $s, [string] $token, [string] $pos) {
  if ([string]::IsNullOrEmpty($token)) { return ,@($s, $false) }

  if ($pos -eq "Suffix") {
    if ($s.EndsWith($token)) {
      return ,@($s.Substring(0, $s.Length - $token.Length), $true)
    }
  } else {
    if ($s.StartsWith($token)) {
      return ,@($s.Substring($token.Length), $true)
    }
  }

  return ,@($s, $false)
}

function Parse-Line([string] $line) {
  $raw = $line.TrimEnd()
  if ([string]::IsNullOrWhiteSpace($raw)) { return $null }
  if ($raw.TrimStart().StartsWith("#")) { return $null }

  # depth by repeating ChildToken at start
  $depth = 0
  $s = $raw
  while (-not [string]::IsNullOrEmpty($ChildToken) -and $s.StartsWith($ChildToken)) {
    $depth++
    $s = $s.Substring($ChildToken.Length)
  }
  $s = $s.Trim()

  if ([string]::IsNullOrWhiteSpace($s)) {
    throw "Invalid line (no name after child tokens): '$line'"
  }

  # hidden?
  $hidden = $false
  $tmp = Strip-Token $s $HiddenToken $HiddenTokenPosition
  $s = $tmp[0]; if ($tmp[1]) { $hidden = $true }
  $s = $s.Trim()

  # type?
  $type = $null
  if (-not [string]::IsNullOrEmpty($DirToken)) {
    $tmp2 = Strip-Token $s $DirToken $TypeTokenPosition
    if ($tmp2[1]) { $type = "Directory"; $s = $tmp2[0].Trim() }
  }
  if (-not $type -and -not [string]::IsNullOrEmpty($FileToken)) {
    $tmp3 = Strip-Token $s $FileToken $TypeTokenPosition
    if ($tmp3[1]) { $type = "File"; $s = $tmp3[0].Trim() }
  }
  if (-not $type) { $type = $DefaultItemType }

  if ([string]::IsNullOrWhiteSpace($s)) {
    throw "Invalid line (name became empty after stripping tokens): '$line'"
  }

  # basic filename validation (avoid obvious invalid chars)
  $invalid = [IO.Path]::GetInvalidFileNameChars()
  foreach ($ch in $invalid) {
    if ($s.Contains([string]$ch)) {
      throw "Invalid name '$s' (contains invalid filename char '$ch') in line: '$line'"
    }
  }

  return [pscustomobject]@{
    Depth  = $depth
    Name   = $s
    Type   = $type
    Hidden = $hidden
  }
}

# ---- read template
if ($PSCmdlet.ParameterSetName -eq "FromFile") {
  $TemplatePath = (Resolve-Path -LiteralPath $TemplatePath).Path
  $Template = Get-Content -LiteralPath $TemplatePath -Raw
}

$Root = [IO.Path]::GetFullPath($Root)
Ensure-Dir $Root

$lines = $Template -split "`r?`n"

# stack of directory paths by depth (only directories go into the stack)
$dirStack = @()
$maxDepthSeen = 0

foreach ($line in $lines) 
{
  $node = Parse-Line $line
  if (-not $node)
  {
	  continue
  }

  if ($node.Depth -gt ($dirStack.Count)) 
  {
    throw "Template jumps too deep at line: '$line' (depth=$($node.Depth), current dir depth=$($dirStack.Count))."
  }

  $parentPath = if ($node.Depth -eq 0) { $Root } else { $dirStack[$node.Depth - 1] }

  $fullPath = Join-Path $parentPath $node.Name

  if ($node.Type -eq "Directory") {
    Ensure-Dir $fullPath
    if ($node.Hidden) { Set-HiddenAttr $fullPath }

    # update stack at this depth (truncate deeper)
    if ($dirStack.Count -gt $node.Depth) {
      $dirStack[$node.Depth] = $fullPath
      if ($dirStack.Count -gt ($node.Depth + 1)) {
        $dirStack = $dirStack[0..$node.Depth]
      }
    } else {
      $dirStack += $fullPath
    }
  }
  else {
    Ensure-File $fullPath
    if ($node.Hidden) { Set-HiddenAttr $fullPath }

    # files do NOT become parents; truncate any deeper stack beyond parent depth
    if ($dirStack.Count -gt $node.Depth) {
      $dirStack = $dirStack[0..($node.Depth - 1)]
    }
  }
}

Write-Host "Done. Created structure at: $Root"
