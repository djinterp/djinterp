function Convert-ArrayToVector
{
    [CmdletBinding(SupportsShouldProcess = $true, DefaultParameterSetName = 'ByPath')]
    param
    (
        # --- ParamSet: files ---
        [Parameter(ParameterSetName='ByPath', Mandatory, ValueFromPipeline, ValueFromPipelineByPropertyName)]
        [Alias('FullName')]
        [string[]] $Path,

        # --- ParamSet: directory ---
        [Parameter(ParameterSetName='ByDirectory', Mandatory)]
        [string] $Directory,

        # Resolve relative -Path/-Directory against this base (optional).
        # If omitted, relative inputs are evaluated against the current location.
        [string] $Base,

        # Directory scan options (directory mode only)
        [Parameter(ParameterSetName='ByDirectory')]
        [switch] $Recurse,

        [Parameter(ParameterSetName='ByDirectory')]
        [string[]] $Include = @('*.c','*.h'),

        [Parameter(ParameterSetName='ByDirectory')]
        [string[]] $Exclude,

        # Create .bak files next to targets before modifying
        [switch] $Backup,

        # Show a quick scan of any leftover whole-word 'array'/'arrays' after changes
        [switch] $ScanLeftovers,

        # Output encoding
        [ValidateSet('utf8','utf8BOM','ascii','unicode','utf7','utf32','bigendianunicode','oem')]
        [string] $Encoding = 'utf8'
    )

    begin
    {
        # Replacement maps (specific → generic order matters)
        $idents = [ordered]@{
            '\bdest_array\b'      = 'dest_vector'
            '\bsource_array\b'    = 'source_vector'
            '\bempty_array\b'     = 'empty_vector'
            '\bboundary_array\b'  = 'boundary_vector'
            '\bsingle_array\b'    = 'single_vector'
            '\barray\b'           = 'vector'   # whole word, won’t touch d_array
        }

        $wording = [ordered]@{
            '\b[Aa]rray\b'  = 'vector'
            '\b[Aa]rrays\b' = 'vectors'
        }

        $testFrom = 'd_tests_sa_vector_insert_array'
        $testTo   = 'd_tests_sa_vector_insert_vector'

        function _ResolveOne([string] $p, [string] $base)
        {
            if ($base)
            {
                if ([System.IO.Path]::IsPathRooted($p)) { return (Resolve-Path -LiteralPath $p).Path }
                $combined = Join-Path -Path $base -ChildPath $p
                return (Resolve-Path -LiteralPath $combined).Path
            }
            else
            {
                return (Resolve-Path -LiteralPath $p).Path
            }
        }

        $targets = New-Object System.Collections.Generic.List[string]
    }

    process
    {
        if ($PSCmdlet.ParameterSetName -eq 'ByDirectory')
        {
            # Resolve directory (with optional Base)
            try {
                $dirResolved = _ResolveOne -p $Directory -base $Base
            } catch {
                Write-Error "Directory not found: $Directory"
                return
            }

            $gciParams = @{
                LiteralPath = $dirResolved
                File        = $true
                Recurse     = [bool]$Recurse
                ErrorAction = 'Stop'
            }
            if ($Include) { $gciParams['Include'] = $Include }
            if ($Exclude) { $gciParams['Exclude'] = $Exclude }

            try {
                $files = Get-ChildItem @gciParams
            } catch {
                Write-Error "Failed to enumerate directory '$Directory': $_"
                return
            }

            foreach ($f in $files) { $targets.Add($f.FullName) }
        }
        else
        {
            foreach ($p in $Path)
            {
                try {
                    $resolved = _ResolveOne -p $p -base $Base
                    if (Test-Path -LiteralPath $resolved -PathType Leaf) { $targets.Add($resolved) }
                    else { Write-Warning "Skipping non-file path: $p" }
                } catch {
                    Write-Warning "Skipping unresolved path: $p"
                }
            }
        }
    }

    end
    {
        foreach ($file in $targets)
        {
            $content = Get-Content -LiteralPath $file -Raw
            $changed = $false

            # 1) Test rename
            if ($content -match [regex]::Escape($testFrom))
            {
                $content = $content -replace $testFrom, $testTo
                $changed = $true
            }

            # 2) Identifier renames
            foreach ($k in $idents.Keys)
            {
                if ($content -match $k)
                {
                    $content = [regex]::Replace($content, $k, $idents[$k])
                    $changed = $true
                }
            }

            # 3) Wording (comments/strings)
            foreach ($k in $wording.Keys)
            {
                if ($content -match $k)
                {
                    $content = [regex]::Replace($content, $k, $wording[$k])
                    $changed = $true
                }
            }

            if ($changed -and $PSCmdlet.ShouldProcess($file, "Apply array→vector refactor"))
            {
                if ($Backup) { Copy-Item -LiteralPath $file -Destination "$file.bak" -Force }

                switch ($Encoding)
                {
                    'utf8'             { Set-Content -LiteralPath $file -Value $content -Encoding utf8 }
                    'utf8BOM'          { Set-Content -LiteralPath $file -Value $content -Encoding utf8BOM }
                    'ascii'            { Set-Content -LiteralPath $file -Value $content -Encoding ascii }
                    'unicode'          { Set-Content -LiteralPath $file -Value $content -Encoding unicode }
                    'utf7'             { Set-Content -LiteralPath $file -Value $content -Encoding utf7 }
                    'utf32'            { Set-Content -LiteralPath $file -Value $content -Encoding utf32 }
                    'bigendianunicode' { Set-Content -LiteralPath $file -Value $content -Encoding bigendianunicode }
                    'oem'              { Set-Content -LiteralPath $file -Value $content -Encoding oem }
                }

                Write-Verbose "Updated: $file"
            }
            elseif (-not $changed)
            {
                Write-Verbose "No changes needed: $file"
            }

            if ($ScanLeftovers)
			{
				# Re-scan the file on disk so LineNumber/Context are correct
				$scan = Select-String -Path $file -Pattern '\barray\b|\barrays\b'
				if ($scan)
				{
					Write-Host "`nLeftover whole-word 'array' mentions in ${file}:" -ForegroundColor Yellow
					foreach ($m in $scan)
					{
						"{0}:{1}: {2}" -f $file, $m.LineNumber, $m.Line.Trim() | Write-Host
					}
				}
			}
        }
    }
}
