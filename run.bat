@echo off
setlocal

set SCRIPT_DIR=%~dp0
pushd "%SCRIPT_DIR%"

set BIN=build-win\Release\sudoku_solver.exe
if not exist "%BIN%" set BIN=build-win\sudoku_solver.exe

if not exist "%BIN%" (
  if exist "install.bat" (
    call install.bat
    if errorlevel 1 goto :fail
    set BIN=build-win\Release\sudoku_solver.exe
    if not exist "%BIN%" set BIN=build-win\sudoku_solver.exe
  )
)

if not exist "%BIN%" (
  echo ERROR: Solver binary not found.
  goto :fail
)

set INPUT=questions.json
if not exist "%INPUT%" set INPUT=question.json
set OUTPUT=answer.json

if not exist "%INPUT%" (
  echo ERROR: Input file not found. Expected questions.json or question.json in repository root.
  goto :fail
)

if exist "%OUTPUT%" del /q "%OUTPUT%"

powershell -NoProfile -ExecutionPolicy Bypass -Command "$sw=[System.Diagnostics.Stopwatch]::StartNew(); & '%BIN%' '%INPUT%' '%OUTPUT%'; $code=$LASTEXITCODE; $sw.Stop(); Write-Output ('Execution time (ms): ' + [Math]::Round($sw.Elapsed.TotalMilliseconds,3)); exit $code"
if errorlevel 1 goto :fail

if not exist "%OUTPUT%" (
  echo ERROR: answer.json was not generated.
  goto :fail
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "$in = Get-Content '%INPUT%' -Raw | ConvertFrom-Json; $out = Get-Content '%OUTPUT%' -Raw | ConvertFrom-Json; if ($null -eq $out.answers) { exit 2 }; $inCount = 1; if ($null -ne $in.puzzles) { $inCount = $in.puzzles.Count } elseif ($null -ne $in.questions) { $inCount = $in.questions.Count } elseif ($in -is [array]) { $inCount = $in.Count }; if ($null -ne $in._meta -and $null -ne $in._meta.total_puzzles) { $expected = [int]$in._meta.total_puzzles; if ($expected -ne $inCount) { Write-Output ('ERROR: Input metadata mismatch: _meta.total_puzzles=' + $expected + ' but puzzles in file=' + $inCount); exit 4 } }; $outCount = $out.answers.Count; Write-Output ('Input puzzles : ' + $inCount); Write-Output ('Output answers: ' + $outCount); if ($inCount -ne $outCount) { exit 3 }"
if errorlevel 1 (
  echo ERROR: Output format/count validation failed.
  goto :fail
)

echo Run completed successfully.
echo Input : %INPUT%
echo Output: %OUTPUT%
popd
exit /b 0

:fail
echo Run failed.
popd
exit /b 1
