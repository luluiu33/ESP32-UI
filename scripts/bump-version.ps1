#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Bump firmware version (MAJOR.MINOR.PATCH) and create git tag.
.DESCRIPTION
    Usage:
      .\scripts\bump-version.ps1          # show current version
      .\scripts\bump-version.ps1 patch    # 1.0.0 -> 1.0.1
      .\scripts\bump-version.ps1 minor    # 1.0.1 -> 1.1.0
      .\scripts\bump-version.ps1 major    # 1.1.0 -> 2.0.0
#>

$versionFile = "$PSScriptRoot\..\main\version.h"

# --- Parse current version from version.h ---
$content = Get-Content -Path $versionFile -Raw

$major = [regex]::Match($content, '#define FW_MAJOR\s+(\d+)').Groups[1].Value
$minor = [regex]::Match($content, '#define FW_MINOR\s+(\d+)').Groups[1].Value
$patch = [regex]::Match($content, '#define FW_PATCH\s+(\d+)').Groups[1].Value

if (-not $major -or -not $minor -or -not $patch) {
    Write-Error "Cannot parse version from $versionFile"
    exit 1
}

$current = "$major.$minor.$patch"
$action = $args[0]

if (-not $action) {
    Write-Host "Current version: $current"
    Write-Host ""
    Write-Host "Usage:  .\scripts\bump-version.ps1 {major|minor|patch}"
    exit 0
}

# --- Bump ---
switch ($action.ToLower()) {
    "major" { $major = [int]$major + 1; $minor = 0; $patch = 0 }
    "minor" { $minor = [int]$minor + 1; $patch = 0 }
    "patch" { $patch = [int]$patch + 1 }
    default {
        Write-Error "Unknown action '$action'. Use: major, minor, or patch"
        exit 1
    }
}

$new = "$major.$minor.$patch"

# --- Update version.h ---
$content = $content -replace '#define FW_MAJOR\s+\d+', "#define FW_MAJOR $major"
$content = $content -replace '#define FW_MINOR\s+\d+', "#define FW_MINOR $minor"
$content = $content -replace '#define FW_PATCH\s+\d+', "#define FW_PATCH $patch"

Set-Content -Path $versionFile -Value $content -NoNewline

# --- Commit & tag ---
$tag = "v$new"
Write-Host "Bumped: $current -> $new"

$answer = Read-Host "Create git commit & tag '$tag'? (y/N)"
if ($answer -eq 'y') {
    git add -A
    git commit -m "chore: bump version to $new"
    git tag -a "$tag" -m "Release $tag"
    Write-Host "Tag $tag created. Push with: git push origin $tag"
}
else {
    Write-Host "Skipped git operations. Changes staged in $versionFile"
}
