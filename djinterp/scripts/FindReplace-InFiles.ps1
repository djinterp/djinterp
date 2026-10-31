<#
.SYNOPSIS
    Find and replace text inside matching files.

.DESCRIPTION
    FindReplace-InFiles.ps1 searches files under a directory, finds text in file contents,
    and replaces it.

    Supports literal text or regex matching, optional recursion, extension filtering,
    hidden-file inclusion, and WhatIf/Confirm.

.PARAMETER Path
    Root directory to search.

.PARAMETER Find
    Text or regex pattern to find.

.PARAMETER Replace
    Replacement text. Defaults to empty string.

.PARAMETER Extensions
    One or more file extensions to include, e.g. ".hpp", ".cpp".
    If omitted, all files are considered.

.PARAMETER NoExtension
    Only include files that have no extension.

.PARAMETER Recurse
    Search subdirectories. Defaults to $true.

.PARAMETER IncludeHidden
    Include hidden files.

.PARAMETER Regex
    Treat -Find as a regular expression.

.PARAMETER CaseSensitive
    Perform a case-sensitive match.

.PARAMETER PassThru
    Output objects for files that had matches.

.EXAMPLE
    .\FindReplace-InFiles.ps1 -Path "D:\code" -Find "DLifetime" -Replace "iterator_type" -Extensions ".hpp"

.EXAMPLE
    .\FindReplace-InFiles.ps1 -Path . -Find "\bDLifetime\b" -Replace "iterator_type" -Regex -Extensions ".hpp",".cpp"

.EXAMPLE
    .\FindReplace-InFiles.ps1 -Path . -Find "foo" -Replace "bar" -Recurse:$false -WhatIf

.EXAMPLE
    .\FindReplace-InFiles.ps1 -Path . -Find "old_name" -Replace "new_name" -Extensions ".hpp",".cpp",".cxx" -PassThru
#>
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [Parameter(Mandatory)]
    [ValidateScript({ Test-Path $_ -PathType Container })]
    [string] $Path,

    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string] $Find,

    [string] $Replace = "",

    [string[]] $Extensions,

    [switch] $NoExtension,

    [bool] $Recurse = $true,

    [switch] $IncludeHidden,

    [switch] $Regex,

    [switch] $CaseSensitive,

    [switch] $PassThru
)

function Get-DetectedEncoding
{
    param(
        [Parameter(Mandatory)]
        [string] $FilePath
    )

    $fs = [System.IO.File]::OpenRead($FilePath)
    try
    {
        $bom = New-Object byte[] 4
        $read = $fs.Read($bom, 0, 4)

        if ($read -ge 3 -and $bom[0] -eq 0xEF -and $bom[1] -eq 0xBB -and $bom[2] -eq 0xBF)
        {
            return [System.Text.UTF8Encoding]::new($true)
        }
        elseif ($read -ge 2 -and $bom[0] -eq 0xFF -and $bom[1] -eq 0xFE)
        {
            return [System.Text.Encoding]::Unicode
        }
        elseif ($read -ge 2 -and $bom[0] -eq 0xFE -and $bom[1] -eq 0xFF)
        {
            return [System.Text.Encoding]::BigEndianUnicode
        }
        elseif ($read -ge 4 -and $bom[0] -eq 0xFF -and $bom[1] -eq 0xFE -and $bom[2] -eq 0x00 -and $bom[3] -eq 0x00)
        {
            return [System.Text.Encoding]::UTF32
        }
        else
        {
            return [System.Text.UTF8Encoding]::new($false)
        }
    }
    finally
    {
        $fs.Dispose()
    }
}

function Test-ExtensionMatch
{
    param(
        [Parameter(Mandatory)]
        [System.IO.FileInfo] $File,

        [string[]] $Extensions,

        [bool] $NoExtension
    )

    if ($NoExtension)
    {
        return [string]::IsNullOrEmpty($File.Extension)
    }

    if (-not $Extensions -or $Extensions.Count -eq 0)
    {
        return $true
    }

    foreach ($ext in $Extensions)
    {
        $normalized = if ($ext.StartsWith('.')) { $ext } else { ".$ext" }
        if ($File.Extension -ieq $normalized)
        {
            return $true
        }
    }

    return $false
}

$gciParams = @{
    LiteralPath = $Path
    File        = $true
    Force       = [bool]$IncludeHidden
}

if ($Recurse)
{
    $gciParams.Recurse = $true
}

$files = Get-ChildItem @gciParams | Where-Object {
    Test-ExtensionMatch -File $_ -Extensions $Extensions -NoExtension ([bool]$NoExtension)
}

Write-Verbose "Files scanned: $($files.Count)"

$totalMatchedFiles = 0
$totalReplacements = 0

foreach ($file in $files)
{
    $encoding = Get-DetectedEncoding -FilePath $file.FullName
    $oldText  = [System.IO.File]::ReadAllText($file.FullName, $encoding)

    $matchCount = 0
    $newText    = $oldText

    if ($Regex)
    {
        $options = [System.Text.RegularExpressions.RegexOptions]::Multiline
        if (-not $CaseSensitive)
        {
            $options = $options -bor [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
        }

        $matchCount = [System.Text.RegularExpressions.Regex]::Matches($oldText, $Find, $options).Count

        if ($matchCount -gt 0)
        {
            $newText = [System.Text.RegularExpressions.Regex]::Replace($oldText, $Find, $Replace, $options)
        }
    }
    else
    {
        if ($CaseSensitive)
        {
            $escapedFind = [regex]::Escape($Find)
            $matchCount  = [regex]::Matches($oldText, $escapedFind).Count

            if ($matchCount -gt 0)
            {
                $newText = $oldText.Replace($Find, $Replace)
            }
        }
        else
        {
            $escapedFind = [regex]::Escape($Find)
            $matchCount  = [regex]::Matches(
                $oldText,
                $escapedFind,
                [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
            ).Count

            if ($matchCount -gt 0)
            {
                $newText = [regex]::Replace(
                    $oldText,
                    $escapedFind,
                    [System.Text.RegularExpressions.MatchEvaluator]{
                        param($m)
                        $Replace
                    },
                    [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
                )
            }
        }
    }

    if ($matchCount -gt 0)
    {
        $totalMatchedFiles++
        $totalReplacements += $matchCount

        Write-Verbose "Matched $matchCount occurrence(s) in $($file.FullName)"

        if ($newText -ne $oldText)
        {
            if ($PSCmdlet.ShouldProcess($file.FullName, "Replace '$Find' with '$Replace'"))
            {
                [System.IO.File]::WriteAllText($file.FullName, $newText, $encoding)
            }
        }

        if ($PassThru)
        {
            [pscustomobject]@{
                Path         = $file.FullName
                Replacements = $matchCount
                Changed      = ($newText -ne $oldText)
            }
        }
    }
}

Write-Verbose "Matched files: $totalMatchedFiles"
Write-Verbose "Total replacements: $totalReplacements"