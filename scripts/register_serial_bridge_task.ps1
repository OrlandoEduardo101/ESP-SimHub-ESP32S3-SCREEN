<#
.SYNOPSIS
    Registers a Scheduled Task that keeps scripts/serial_tcp_bridge.py running
    in the background, starting at logon and relaunching it if it ever dies.
    Run this once, in an elevated PowerShell.

.DESCRIPTION
    Replaces HW VSP3 for the WiFi dashboard path. HW VSP3 measured with
    gaps_over_100ms and multi_frame_chunks both nonzero every window on the
    real SimHub -> COM15 -> HW VSP3 -> TCP path -- it was batching frames
    instead of forwarding each one as it arrived, so ~21 frames/s landed on
    screen as ~4 visible updates/s. The same frames sent straight over TCP,
    with the exact same board and network, showed neither: one frame per TCP
    segment, no gap over 88ms. See docs/WIRELESS.md, section "Ponte serial-TCP
    (substitui a HW VSP3)".

    Trigger is "at log on" plus a repetition every 1 minute, indefinitely,
    with "do not start a new instance if one is already running". That
    repetition is what makes this self-healing: if the bridge process ever
    exits (crash, board power-cycled and never came back, etc.), the next
    minute's tick finds no running instance and starts a fresh one. If it's
    still running, the tick is a no-op.

.NOTES
    Needs com0com's COM20/COM21 pair and pyserial already installed -- see
    docs/WIRELESS.md for the from-scratch setup. Needs HW VSP3's service
    stopped (or at least not holding COM15/port 10002) since only one client
    may hold the board's raw port at a time.
#>

#Requires -RunAsAdministrator

$TaskName    = 'SimHub_SerialTcpBridge'
$ScriptPath  = Join-Path $PSScriptRoot 'serial_tcp_bridge.py'
$LogPath     = 'C:\ProgramData\simhub_serial_bridge.log'
$PythonPath  = (Get-Command python.exe -ErrorAction SilentlyContinue).Source
if (-not $PythonPath) { $PythonPath = 'C:\Program Files\Python313\python.exe' }

if (-not (Test-Path $ScriptPath)) {
    throw "Companion script not found next to this one: $ScriptPath"
}
if (-not (Test-Path $PythonPath)) {
    throw "python.exe not found at '$PythonPath' -- install Python first (see docs/WIRELESS.md) or edit `$PythonPath` above."
}

$action = New-ScheduledTaskAction -Execute 'cmd.exe' `
    -Argument "/c `"`"$PythonPath`" -u `"$ScriptPath`" >> `"$LogPath`" 2>&1`""

$trigger = New-ScheduledTaskTrigger -AtLogOn
# [TimeSpan]::MaxValue serializes to "P99999999DT23H59M59S", which the Task
# Scheduler XML schema rejects outright (Register-ScheduledTask fails with
# "O XML da tarefa contem um valor formatado incorretamente"). A whole
# number of days serializes cleanly (e.g. "P3650D"); 10 years is long enough
# that this never needs re-registering in practice, and an AtLogOn trigger
# fires again on every logon regardless, refreshing the window anyway.
$trigger.Repetition = (New-ScheduledTaskTrigger -Once -At (Get-Date) `
    -RepetitionInterval (New-TimeSpan -Minutes 1) `
    -RepetitionDuration (New-TimeSpan -Days 3650)).Repetition

$principal = New-ScheduledTaskPrincipal -UserId $env:USERNAME -LogonType Interactive -RunLevel Limited
$settings  = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
    -StartWhenAvailable -MultipleInstances IgnoreNew -ExecutionTimeLimit ([TimeSpan]::Zero)

Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false -ErrorAction SilentlyContinue

# -ErrorAction Stop: a failed registration must stop the script here, not
# fall through to "registrada" and then a confusing Start-ScheduledTask
# failure on a task that was never actually created -- which is exactly
# what happened with the MaxValue duration above.
Register-ScheduledTask -TaskName $TaskName -Trigger $trigger -Action $action `
    -Principal $principal -Settings $settings -ErrorAction Stop `
    -Description 'Keeps the SimHub-to-ESP32 serial-TCP bridge (scripts/serial_tcp_bridge.py) running, restarting it within a minute if it dies. Replaces HW VSP3 for the WiFi dashboard path -- see docs/WIRELESS.md.' `
    | Out-Null

Write-Host "Tarefa '$TaskName' registrada. Log: $LogPath"
Write-Host "Iniciando agora (nao precisa esperar o proximo logon):"
Start-ScheduledTask -TaskName $TaskName
Start-Sleep -Seconds 2
Write-Host "--- ultimas linhas do log ---"
Get-Content $LogPath -Tail 5 -ErrorAction SilentlyContinue
