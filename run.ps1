$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Binary = Join-Path $ScriptDir "build-win\Release\sudoku_solver.exe"
if (-not (Test-Path $Binary)) {
    $Binary = Join-Path $ScriptDir "build-win\sudoku_solver.exe"
}

if (-not (Test-Path $Binary)) {
    $Installer = Join-Path $ScriptDir "install.bat"
    if (-not (Test-Path $Installer)) {
        throw "Solver binary not found and install.bat is missing."
    }

    & $Installer
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed while running install.bat"
    }

    $Binary = Join-Path $ScriptDir "build-win\Release\sudoku_solver.exe"
    if (-not (Test-Path $Binary)) {
        $Binary = Join-Path $ScriptDir "build-win\sudoku_solver.exe"
    }

    if (-not (Test-Path $Binary)) {
        throw "Solver binary not found after build."
    }
}

$QuestionsPath = Join-Path $ScriptDir "questions.json"
if (-not (Test-Path $QuestionsPath)) {
    $QuestionsPath = Join-Path $ScriptDir "question.json"
}

if (-not (Test-Path $QuestionsPath)) {
    throw "Input file not found. Expected questions.json or question.json in $ScriptDir"
}

$AnswersPath = Join-Path $ScriptDir "answer.json"

if (Test-Path $AnswersPath) {
    Remove-Item $AnswersPath -Force
}

$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
& $Binary $QuestionsPath $AnswersPath
$Stopwatch.Stop()
if ($LASTEXITCODE -ne 0) {
    throw "Solver exited with code $LASTEXITCODE"
}

if (-not (Test-Path $AnswersPath)) {
    throw "answer.json was not generated."
}

$AnswerJson = Get-Content $AnswersPath -Raw | ConvertFrom-Json
if ($null -eq $AnswerJson.answers) {
    throw "Unexpected answer.json format: missing answers array."
}

$InputJson = Get-Content $QuestionsPath -Raw | ConvertFrom-Json
$InputCount = 1
if ($null -ne $InputJson.puzzles) {
    $InputCount = $InputJson.puzzles.Count
} elseif ($null -ne $InputJson.questions) {
    $InputCount = $InputJson.questions.Count
} elseif ($InputJson -is [array]) {
    $InputCount = $InputJson.Count
}

$OutputCount = $AnswerJson.answers.Count
if ($InputCount -ne $OutputCount) {
    throw "answers count mismatch: input=$InputCount output=$OutputCount"
}

if ($null -ne $InputJson._meta -and $null -ne $InputJson._meta.total_puzzles) {
    $ExpectedCount = [int]$InputJson._meta.total_puzzles
    if ($ExpectedCount -ne $InputCount) {
        throw "Input metadata mismatch: _meta.total_puzzles=$ExpectedCount but puzzles in file=$InputCount"
    }
}

Write-Host "Run completed successfully."
Write-Host "Input : $(Split-Path -Leaf $QuestionsPath)"
Write-Host "Output: $(Split-Path -Leaf $AnswersPath)"
Write-Host "Puzzles solved: $OutputCount"
Write-Host ("Execution time (ms): {0}" -f ([Math]::Round($Stopwatch.Elapsed.TotalMilliseconds, 3)))
