[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function ConvertFrom-CStringLiteral {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    $decoded = New-Object System.Text.StringBuilder
    for ($index = 0; $index -lt $Value.Length; $index++) {
        $character = $Value[$index]
        if ($character -ne '\') {
            [void]$decoded.Append($character)
            continue
        }

        $index++
        if ($index -ge $Value.Length) {
            throw 'Unterminated escape sequence in the STID payload.'
        }

        switch ($Value[$index]) {
            '"' { [void]$decoded.Append('"') }
            'n' { [void]$decoded.Append("`n") }
            't' { [void]$decoded.Append("`t") }
            default {
                throw "Unsupported escape sequence \$($Value[$index]) in the STID payload."
            }
        }
    }

    return $decoded.ToString()
}

$inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
$outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
if (-not [System.IO.File]::Exists($inputFullPath)) {
    throw "ELX source file does not exist: $inputFullPath"
}

$source = [System.IO.File]::ReadAllText($inputFullPath)
$fprintfPattern = 'fprintf\s*\(\s*pfile\s*,\s*"(?<literal>(?:\\.|[^"\\])*)"\s*\)\s*;'
$fprintfMatches = [System.Text.RegularExpressions.Regex]::Matches(
    $source,
    $fprintfPattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline)

$stidBuilder = New-Object System.Text.StringBuilder
$collecting = $false
$startRawIndex = -1
$endRawIndex = -1
foreach ($match in $fprintfMatches) {
    $decoded = ConvertFrom-CStringLiteral $match.Groups['literal'].Value
    if (-not $collecting) {
        $startIndex = $decoded.IndexOf(
            "#ifdef STID`n",
            [System.StringComparison]::Ordinal)
        if ($startIndex -lt 0) {
            continue
        }

        $collecting = $true
        $startRawIndex = $match.Index
        [void]$stidBuilder.Append($decoded.Substring($startIndex))
    }
    else {
        [void]$stidBuilder.Append($decoded)
    }

    $current = $stidBuilder.ToString()
    $rgrgbIndex = $current.IndexOf(
        'rgrgb',
        [System.StringComparison]::Ordinal)
    if ($rgrgbIndex -lt 0) {
        continue
    }

    $endIndex = $current.IndexOf(
        '#endif',
        $rgrgbIndex,
        [System.StringComparison]::Ordinal)
    if ($endIndex -lt 0) {
        continue
    }

    $endLineIndex = $current.IndexOf(
        "`n",
        $endIndex,
        [System.StringComparison]::Ordinal)
    if ($endLineIndex -lt 0) {
        $endLineIndex = $current.Length - 1
    }

    $stidText = $current.Substring(0, $endLineIndex + 1)
    $endRawIndex = $match.Index + $match.Length
    break
}

if (-not $collecting -or $endRawIndex -lt 0) {
    throw 'Could not locate a complete #ifdef STID payload in the ELX source.'
}

$rawRegion = $source.Substring($startRawIndex, $endRawIndex - $startRawIndex)
$allFprintfCount = [System.Text.RegularExpressions.Regex]::Matches(
    $rawRegion,
    'fprintf\s*\(').Count
$literalFprintfCount = [System.Text.RegularExpressions.Regex]::Matches(
    $rawRegion,
    $fprintfPattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline).Count
if ($allFprintfCount -ne $literalFprintfCount) {
    throw 'The STID payload contains a formatted or otherwise unsupported fprintf statement.'
}

if ([System.Text.RegularExpressions.Regex]::Matches(
        $stidText,
        '(?m)^#ifdef STID\s*$').Count -ne 1 -or
    [System.Text.RegularExpressions.Regex]::Matches(
        $stidText,
        '(?m)^#endif\s*$').Count -ne 1) {
    throw 'The generated STID payload does not have exactly one conditional section.'
}

foreach ($symbol in @('rgksp', 'mpstiderc', 'rgstid', 'rgrgb')) {
    if ([System.Text.RegularExpressions.Regex]::Matches(
            $stidText,
            "\b$symbol\s*\[").Count -ne 1) {
        throw "The generated STID payload must define $symbol exactly once."
    }
}

$runtimeRgkspInitializer =
    'csconst char rgksp [] = StringMap("SUPO", 0, 1);'
if ([System.Text.RegularExpressions.Regex]::Matches(
        $stidText,
        [System.Text.RegularExpressions.Regex]::Escape(
            $runtimeRgkspInitializer)).Count -ne 1) {
    throw 'The STID payload does not contain the expected rgksp initializer.'
}

# StringMap returns its input unchanged when counted is zero, but a function
# call is not a valid global C initializer.  Emit the equivalent literal.
$stidText = $stidText.Replace(
    $runtimeRgkspInitializer,
    'csconst char rgksp [] = "SUPO";')

$neutralTables = @'
/* This file was created by GenerateElxStid.ps1.  Do Not Edit! */

#ifdef elkAppMac
typedef struct _OPUS_X64_EMPTY_ELDI
	{
	HID hid;
	CABI cabi;
	unsigned celfd;
	} OPUS_X64_EMPTY_ELDI;
static csconst OPUS_X64_EMPTY_ELDI vopusX64EmptyEldi = {0};
#define rgeldi ((ELDI *)&vopusX64EmptyEldi)
csconst unsigned rgichName[ibstElkAppMac] = {0};
csconst unsigned char rgchElkNames[1] = {0};
csconst unsigned mpelkistName[elkAppMac] = {0};
#endif

'@
$neutralTables = $neutralTables.Replace("`r`n", "`n")
$content = $neutralTables + $stidText

$outputDirectory = [System.IO.Path]::GetDirectoryName($outputFullPath)
if (-not [string]::IsNullOrEmpty($outputDirectory)) {
    [void][System.IO.Directory]::CreateDirectory($outputDirectory)
}

if ([System.IO.File]::Exists($outputFullPath) -and
    [System.IO.File]::ReadAllText($outputFullPath) -ceq $content) {
    return
}

$utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($outputFullPath, $content, $utf8WithoutBom)
