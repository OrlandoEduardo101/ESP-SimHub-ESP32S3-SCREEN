<#
.SYNOPSIS
    Recovers the HW VSP3 virtual serial bridge after the PC wakes from sleep.

.DESCRIPTION
    HW VSP3 (COM15 -> 192.168.0.5:10002, the ESP32 dashboard's raw transport)
    does not survive a sleep/resume cycle on its own: the TCP session to the
    board dies while the PC is asleep, the service does not notice and does
    not redial, and -- separately -- HW_VSP3s.ini has been observed reverting
    its IP= line back to the old ARQ port (:10001) instead of the raw one
    (:10002) across a restart. Neither half fixes itself; both need doing.

    This is the automated form of the manual recovery the user was running
    by hand after every sleep, roughly:
        Stop-Process HW_VSP3s_client; Stop-Service HW_VSP3s_Service
        fix the ini; Start-Service HW_VSP3s_Service

    Intended to run unattended (as SYSTEM, via Task Scheduler triggered on
    Power-Troubleshooter Event ID 1 -- "resumed from sleep"), so it logs what
    it did instead of printing to a console nobody is watching.

.NOTES
    Registered by scripts/register_hw_vsp3_resume_task.ps1 (run elevated).
#>

$ErrorActionPreference = 'Stop'
$IniPath     = 'C:\Program Files (x86)\HW group\HW VSP3s\HW_VSP3s.ini'
$ServiceName = 'HW_VSP3s_Service'
$ClientProc  = 'HW_VSP3s_client'
$BoardPort   = ':10002'   # raw transport; ':10001' is the old ARQ port
$LogPath     = 'C:\ProgramData\HW_VSP3_fix_on_resume.log'

function Write-Log($msg) {
    $line = "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')  $msg"
    Add-Content -Path $LogPath -Value $line -Encoding utf8
}

Write-Log '--- resume detected, starting recovery ---'

try {
    # 1) Stop the tray client if it is running, so it is not holding the
    #    port or racing the service restart.
    $client = Get-Process -Name $ClientProc -ErrorAction SilentlyContinue
    if ($client) {
        Stop-Process -Name $ClientProc -Force
        Write-Log "stopped $ClientProc (was PID $($client.Id -join ','))"
    } else {
        Write-Log "$ClientProc was not running"
    }

    # 2) Stop the service before touching the ini it has open.
    $svc = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
    if (-not $svc) {
        Write-Log "ERROR: service '$ServiceName' not found - nothing to recover"
        return
    }
    if ($svc.Status -ne 'Stopped') {
        Stop-Service -Name $ServiceName -Force
        (Get-Service -Name $ServiceName).WaitForStatus('Stopped', '00:00:15')
    }
    Write-Log "service stopped"

    # 3) Fix the ini if it reverted, backing up the copy we are about to
    #    change so a bad edit is recoverable.
    if (-not (Test-Path $IniPath)) {
        Write-Log "ERROR: ini not found at $IniPath"
    } else {
        $content = Get-Content $IniPath -Raw
        if ($content -match ':10001') {
            Copy-Item $IniPath "$IniPath.bak" -Force
            $fixed = $content -replace ':10001', $BoardPort
            Set-Content -Path $IniPath -Value $fixed -NoNewline
            Write-Log "ini had reverted to :10001 - corrected to $BoardPort (backup saved)"
        } elseif ($content -match [regex]::Escape($BoardPort)) {
            Write-Log "ini already correct ($BoardPort) - no change needed"
        } else {
            Write-Log "ini has neither :10001 nor $BoardPort - left untouched, check manually"
        }
    }

    # 4) Bring the service back. ConnectIfPortClosed=1 in the ini means it
    #    redials on its own once something opens COM15 -- the tray client
    #    does not need to be relaunched separately for the bridge to work,
    #    only for the systray icon/UI to reappear.
    Start-Service -Name $ServiceName
    (Get-Service -Name $ServiceName).WaitForStatus('Running', '00:00:15')
    Write-Log "service started, status=$((Get-Service -Name $ServiceName).Status)"

    Start-Process -FilePath "C:\Program Files (x86)\HW group\HW VSP3s\$ClientProc.exe" -ErrorAction SilentlyContinue
    Write-Log "relaunched $ClientProc (tray UI)"

    Write-Log '--- recovery finished ---'
}
catch {
    Write-Log "ERROR: $($_.Exception.Message)"
    throw
}
