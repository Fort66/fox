@echo off
rem Сборка foxc под Windows без WSL и пакетных менеджеров.
rem Пробует MSVC (cl), затем MinGW-w64 (gcc).
where cl >nul 2>nul
if %errorlevel%==0 (
    echo Сборка через MSVC (cl)...
    cl /nologo /O2 /std:c11 src\buf.c src\codegen.c src\expr.c src\func.c src\lexer.c src\main.c src\parser_core.c src\stmt.c src\symbols.c /Fefoxc.exe
    goto :done
)
where gcc >nul 2>nul
if %errorlevel%==0 (
    echo Сборка через MinGW-w64 (gcc)...
    gcc -O2 -std=c11 src\*.c -o foxc.exe -lm
    goto :done
)
echo Ошибка: не найден ни cl, ни gcc.
echo Поставь w64devkit (zip, распаковать и добавить bin в PATH) или Visual Studio Build Tools.
exit /b 1
:done
if %errorlevel%==0 (echo Готово: foxc.exe) else (echo Ошибка сборки)