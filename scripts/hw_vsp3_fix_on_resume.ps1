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

# Win32 1061 (ERROR_SERVICE_CANNOT_ACCEPT_CTRL) means the service is mid
# transition -- start/stop/pause pending -- and can't take a new control
# right this instant. It is not a permissions problem (confirmed: SYSTEM has
# SERVICE_STOP in the service's own ACL) and it is not a recurring restart
# either (the service has no failure/recovery action configured), just a
# timing window that clears on its own. Retrying a few times with a short
# wait is the standard, correct handling for it -- failing on the first
# attempt, which is what this script originally did, is what actually
# produced the "cannot be stopped" error the user hit.
function Invoke-ServiceControlWithRetry {
    param(
        [string]$Name,
        [ValidateSet('Stop', 'Start')][string]$Action,
        [int]$MaxAttempts = 6,
        [int]$DelaySeconds = 3
    )
    for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++) {
        try {
            if ($Action -eq 'Stop') { Stop-Service -Name $Name -Force -ErrorAction Stop }
            else { Start-Service -Name $Name -ErrorAction Stop }
            $target = if ($Action -eq 'Stop') { 'Stopped' } else { 'Running' }
            (Get-Service -Name $Name).WaitForStatus($target, '00:00:15')
            return $true
        } catch {
            Write-Log "  $Action attempt $attempt/$MaxAttempts failed: $($_.Exception.Message)"
            if ($attempt -eq $MaxAttempts) { throw }
            Start-Sleep -Seconds $DelaySeconds
        }
    }
}

Write-Log '--- resume detected, starting recovery ---'

try {
    # 1) Stop the service before touching the ini it has open. The tray
    #    client is handled last, after the service is settled -- it is only
    #    a UI on top of the service, not required for the bridge itself.
    $svc = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
    if (-not $svc) {
        Write-Log "ERROR: service '$ServiceName' not found - nothing to recover"
        return
    }
    if ($svc.Status -ne 'Stopped') {
        Invoke-ServiceControlWithRetry -Name $ServiceName -Action Stop
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
    #    redials on its own once something opens COM15.
    Invoke-ServiceControlWithRetry -Name $ServiceName -Action Start
    Write-Log "service started, status=$((Get-Service -Name $ServiceName).Status)"

    # 5) Tray client last: kill any stale instance and relaunch fresh, now
    #    that the service it talks to is up and stable.
    $client = Get-Process -Name $ClientProc -ErrorAction SilentlyContinue
    if ($client) {
        Stop-Process -Name $ClientProc -Force
        Write-Log "stopped stale $ClientProc (was PID $($client.Id -join ','))"
        Start-Sleep -Seconds 1
    }
    Start-Process -FilePath "C:\Program Files (x86)\HW group\HW VSP3s\$ClientProc.exe" -ErrorAction SilentlyContinue
    Write-Log "relaunched $ClientProc (tray UI)"

    Write-Log '--- recovery finished ---'
}
catch {
    Write-Log "ERROR: $($_.Exception.Message)"
    throw
}
