[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Path,

    [switch]$IndentByDepth,

    [string]$IndentString = "`t",

    [int]$IncludeLineIndent = 1,

    [switch]$RelativePaths
)

function Get-NormalizedFullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$InputPath
    )

    return [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $InputPath).Path)
}

function Get-RelativePathSafe {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath,

        [Parameter(Mandatory = $true)]
        [string]$TargetPath
    )

    $baseUri   = [System.Uri]((Get-NormalizedFullPath $BasePath).TrimEnd('\') + '\')
    $targetUri = [System.Uri](Get-NormalizedFullPath $TargetPath)
    $relative  = $baseUri.MakeRelativeUri($targetUri).ToString()

    return [System.Uri]::UnescapeDataString($relative).Replace('/', '\')
}

function Get-DepthFromRelativePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    $parent = Split-Path -Path $RelativePath -Parent

    if ([string]::IsNullOrWhiteSpace($parent)) {
        return 0
    }

    return ($parent -split '[\\/]').Count
}

$rootPath = Get-NormalizedFullPath $Path

$files = Get-ChildItem -LiteralPath $rootPath -Recurse -File |
    Where-Object { $_.Extension -in '.h', '.hpp' } |
    Sort-Object FullName

foreach ($file in $files) {
    $includeLines = Select-String -LiteralPath $file.FullName -Pattern '^\s*#\s*include\b' |
        ForEach-Object { $_.Line.Trim() }

    if (-not $includeLines) {
        continue
    }

    $relativePath = Get-RelativePathSafe -BasePath $rootPath -TargetPath $file.FullName

    $depthIndentLevel = 0
    if ($IndentByDepth) {
        $depthIndentLevel = Get-DepthFromRelativePath -RelativePath $relativePath
    }

    $fileIndent    = $IndentString * $depthIndentLevel
    $includeIndent = $IndentString * ($depthIndentLevel + $IncludeLineIndent)

    $displayName = if ($RelativePaths) { $relativePath } else { $file.FullName }

    "$fileIndent$displayName"

    foreach ($line in $includeLines) {
        "$includeIndent$line"
    }

    ""
}