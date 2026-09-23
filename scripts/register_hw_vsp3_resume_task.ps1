<#
.SYNOPSIS
    Registers a Scheduled Task that runs hw_vsp3_fix_on_resume.ps1 every time
    Windows wakes from sleep. Run this once, in an elevated PowerShell.

.DESCRIPTION
    Triggers on Event ID 1 from source "Microsoft-Windows-Power-Troubleshooter"
    in the System log -- the standard, documented signal for "resumed from
    sleep". Runs as SYSTEM with highest privileges, so it does not need the
    user's session to be unlocked and does not raise a UAC prompt.
#>

#Requires -RunAsAdministrator

$TaskName   = 'HW_VSP3_FixOnResume'
$ScriptPath = Join-Path $PSScriptRoot 'hw_vsp3_fix_on_resume.ps1'

if (-not (Test-Path $ScriptPath)) {
    throw "Companion script not found next to this one: $ScriptPath"
}

# Event-based trigger: Task Scheduler's own New-ScheduledTaskTrigger has no
# -OnEvent parameter in Windows PowerShell 5.1, so this goes through the CIM
# class directly. This is the same subscription the Task Scheduler GUI
# writes when you pick "Begin the task: On an event" -> Log=System,
# Source=Power-Troubleshooter, Event ID=1.
$eventClass = Get-CimClass -Namespace Root/Microsoft/Windows/TaskScheduler -ClassName MSFT_TaskEventTrigger
$trigger = New-CimInstance -CimClass $eventClass -ClientOnly
$trigger.Subscription = @'
<QueryList><Query Id="0" Path="System"><Select Path="System">*[System[Provider[@Name='Microsoft-Windows-Power-Troubleshooter'] and EventID=1]]</Select></Query></QueryList>
'@
$trigger.Enabled = $true
# Debounce: a resume can log more than one Event 1 in quick succession
# (display + full system), and the fix script itself is idempotent, but
# there's no reason to run it twice a second.
$trigger.Delay = 'PT5S'

$action    = New-ScheduledTaskAction -Execute 'powershell.exe' `
                 -Argument "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$ScriptPath`""
$principal = New-ScheduledTaskPrincipal -UserId 'SYSTEM' -LogonType ServiceAccount -RunLevel Highest
$settings  = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
                 -StartWhenAvailable -ExecutionTimeLimit (New-TimeSpan -Minutes 2)

Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false -ErrorAction SilentlyContinue

Register-ScheduledTask -TaskName $TaskName -Trigger $trigger -Action $action `
    -Principal $principal -Settings $settings `
    -Description 'Recovers the HW VSP3 bridge (ESP32 dashboard) after the PC wakes from sleep. See hw_vsp3_fix_on_resume.ps1.'

Write-Host "Tarefa '$TaskName' registrada. Log de cada execucao: C:\ProgramData\HW_VSP3_fix_on_resume.log"
Write-Host "Teste manual (sem esperar suspender de verdade):"
Write-Host "  Start-ScheduledTask -TaskName '$TaskName'"
