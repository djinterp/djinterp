[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [Parameter(Mandatory)][string] $Root,
    [Parameter(Mandatory)][string] $Find,
    [Parameter(Mandatory)][string] $Replace
)
Write-Host "It works! Root=$Root Find=$Find Replace=$Replace"
