:: If winget is not available on your Windows yet, open the "Microsoft Store"
:: and install "App Installer" (no account required)
:: For further help: https://learn.microsoft.com/en-us/windows/package-manager/winget/

@echo off
@title "Windows VST3 SDK Setup"

:: Use ```winget install -h``` for help about arguments and options
@echo on
winget install --id=Kitware.CMake -e
winget install --id=Microsoft.VisualStudioCode -e
winget install --id=Microsoft.VisualStudio.2022.Community -e --override "--add Microsoft.VisualStudio.Workload.NativeDesktop;includeRecommended --focusedUi --wait"
