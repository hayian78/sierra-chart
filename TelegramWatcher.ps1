<# 
.SYNOPSIS
    Telegram Chart Snapshot Watcher (Resilient Version)
    Monitors Sierra Chart exports and sends/updates images in Telegram
    
.DESCRIPTION
    This script watches for manifest.json updates from ChartSnapshotEngine
    and sends chart images to Telegram. On first send, it posts a new message.
    On subsequent sends, it updates the existing message to avoid chat spam.
    
.USAGE
    1. Configure config.json with your Telegram bot token and chat ID
    2. Run: powershell -ExecutionPolicy Bypass -File TelegramWatcher.ps1
#>

param(
    [string]$ConfigPath = ".\config.json",
    [switch]$Verbose
)

$ErrorActionPreference = "Continue"
$script:Running = $true

# --- Configuration ---

function Load-Config {
    param([string]$Path)
    
    if (-not (Test-Path $Path)) {
        Write-Host "[ERROR] Config file not found: $Path" -ForegroundColor Red
        Write-Host "Create config.json with: { 'telegramBotToken': 'YOUR_TOKEN', 'telegramChatId': 'YOUR_CHAT_ID', 'manifestPath': 'C:\\SierraChart\\Exports\\manifest.json' }" -ForegroundColor Yellow
        exit 1
    }
    
    $config = Get-Content $Path -Raw | ConvertFrom-Json
    
    if (-not $config.telegramBotToken -or $config.telegramBotToken -eq "YOUR_BOT_TOKEN_HERE") {
        Write-Host "[ERROR] Telegram bot token not configured in config.json" -ForegroundColor Red
        exit 1
    }
    
    if (-not $config.telegramChatId -or $config.telegramChatId -eq "YOUR_CHAT_ID_HERE") {
        Write-Host "[ERROR] Telegram chat ID not configured in config.json" -ForegroundColor Red
        exit 1
    }
    
    return $config
}

# --- State Management (Resilient) ---

function Get-StatePath {
    param([string]$ManifestPath)
    $dir = Split-Path $ManifestPath -Parent
    return Join-Path $dir "telegram_state.json"
}

function Load-State {
    param([string]$StatePath)
    
    # Always return a valid hashtable structure
    $defaultState = @{
        messageIds = @{}
    }
    
    if (-not (Test-Path $StatePath)) {
        Write-Host "[INFO] No state file found, starting fresh" -ForegroundColor Gray
        return $defaultState
    }
    
    try {
        $content = Get-Content $StatePath -Raw -ErrorAction Stop
        $loadedState = $content | ConvertFrom-Json -ErrorAction Stop
        
        # Validate structure
        if (-not $loadedState.messageIds) {
            Write-Host "[WARN] State file missing messageIds, resetting" -ForegroundColor Yellow
            return $defaultState
        }
        
        # Convert PSCustomObject messageIds to proper hashtable
        $messageIdsHash = @{}
        if ($loadedState.messageIds.PSObject.Properties) {
            foreach ($prop in $loadedState.messageIds.PSObject.Properties) {
                $messageIdsHash[$prop.Name] = [int]$prop.Value
            }
        }
        
        Write-Host "[INFO] Loaded $($messageIdsHash.Count) message ID(s) from state" -ForegroundColor Gray
        return @{ messageIds = $messageIdsHash }
        
    }
    catch {
        Write-Host "[WARN] Failed to load state file: $_" -ForegroundColor Yellow
        Write-Host "[INFO] Starting with fresh state" -ForegroundColor Gray
        
        # Backup corrupted file
        if (Test-Path $StatePath) {
            $backupPath = "$StatePath.backup"
            Copy-Item $StatePath $backupPath -Force
            Write-Host "[INFO] Backed up corrupted state to $backupPath" -ForegroundColor Gray
        }
        
        return $defaultState
    }
}

function Save-State {
    param(
        [string]$StatePath,
        [hashtable]$State
    )
    
    try {
        # Ensure messageIds exists and is a hashtable
        if (-not $State.messageIds) {
            $State.messageIds = @{}
        }
        
        # Convert to JSON and save
        $json = $State | ConvertTo-Json -Depth 10 -Compress:$false
        $json | Set-Content $StatePath -Encoding UTF8 -Force
        
        if ($Verbose) {
            Write-Host "[DEBUG] Saved state with $($State.messageIds.Count) message ID(s)" -ForegroundColor DarkGray
        }
        
        return $true
    }
    catch {
        Write-Host "[ERROR] Failed to save state: $_" -ForegroundColor Red
        return $false
    }
}

# --- Telegram API ---

function Clear-TelegramChat {
    param(
        [string]$BotToken,
        [string]$ChatId
    )
    
    Write-Host "[INFO] Clearing old messages from Telegram chat..." -ForegroundColor Yellow
    
    try {
        $url = "https://api.telegram.org/bot$BotToken/getUpdates?limit=100"
        $updates = Invoke-RestMethod -Uri $url -Method Get
        
        if ($updates.ok -and $updates.result.Count -gt 0) {
            $messagesToDelete = @()
            
            foreach ($update in $updates.result) {
                if ($update.message -and $update.message.chat.id -eq $ChatId) {
                    $messagesToDelete += $update.message.message_id
                }
                if ($update.channel_post -and $update.channel_post.chat.id -eq $ChatId) {
                    $messagesToDelete += $update.channel_post.message_id
                }
            }
            
            $deletedCount = 0
            foreach ($msgId in $messagesToDelete) {
                try {
                    $deleteUrl = "https://api.telegram.org/bot$BotToken/deleteMessage"
                    $body = @{
                        chat_id    = $ChatId
                        message_id = $msgId
                    } | ConvertTo-Json
                    
                    $null = Invoke-RestMethod -Uri $deleteUrl -Method Post -Body $body -ContentType "application/json"
                    $deletedCount++
                }
                catch {
                    # Message might already be deleted or too old
                }
            }
            
            if ($deletedCount -gt 0) {
                Write-Host "[INFO] Deleted $deletedCount old message(s)" -ForegroundColor Green
            }
            else {
                Write-Host "[INFO] No messages to delete" -ForegroundColor Gray
            }
        }
    }
    catch {
        Write-Host "[WARN] Could not clear chat (this is normal for new chats): $_" -ForegroundColor Yellow
    }
}

function Send-TelegramPhoto {
    param(
        [string]$BotToken,
        [string]$ChatId,
        [string]$PhotoPath,
        [string]$Caption
    )
    
    $url = "https://api.telegram.org/bot$BotToken/sendPhoto"
    
    try {
        # Retry reading file with delays (Sierra Chart might still be writing)
        $photoBytes = $null
        $maxRetries = 5
        $retryDelay = 200
        
        for ($i = 0; $i -lt $maxRetries; $i++) {
            try {
                $photoBytes = [System.IO.File]::ReadAllBytes($PhotoPath)
                break
            }
            catch {
                if ($i -lt ($maxRetries - 1)) {
                    Start-Sleep -Milliseconds $retryDelay
                    $retryDelay *= 2
                }
                else {
                    throw
                }
            }
        }
        
        if (-not $photoBytes) {
            throw "Failed to read photo after $maxRetries attempts"
        }
        
        $boundary = [System.Guid]::NewGuid().ToString()
        $fileName = Split-Path $PhotoPath -Leaf
        
        $bodyLines = @(
            "--$boundary",
            "Content-Disposition: form-data; name=`"chat_id`"",
            "",
            $ChatId,
            "--$boundary",
            "Content-Disposition: form-data; name=`"caption`"",
            "",
            $Caption,
            "--$boundary",
            "Content-Disposition: form-data; name=`"photo`"; filename=`"$fileName`"",
            "Content-Type: image/png",
            ""
        ) -join "`r`n"
        
        $bodyEnd = "`r`n--$boundary--`r`n"
        
        $encoding = [System.Text.Encoding]::UTF8
        $bodyStart = $encoding.GetBytes($bodyLines + "`r`n")
        $bodyEndBytes = $encoding.GetBytes($bodyEnd)
        
        $memStream = New-Object System.IO.MemoryStream
        $memStream.Write($bodyStart, 0, $bodyStart.Length)
        $memStream.Write($photoBytes, 0, $photoBytes.Length)
        $memStream.Write($bodyEndBytes, 0, $bodyEndBytes.Length)
        
        $requestBody = $memStream.ToArray()
        $memStream.Close()
        
        $response = Invoke-RestMethod -Uri $url -Method Post -Body $requestBody -ContentType "multipart/form-data; boundary=$boundary"
        
        if ($response.ok) {
            return $response.result.message_id
        }
        else {
            Write-Host "[ERROR] Telegram API error: $($response | ConvertTo-Json)" -ForegroundColor Red
            return $null
        }
    }
    catch {
        Write-Host "[ERROR] Failed to send photo: $_" -ForegroundColor Red
        return $null
    }
}

function Update-TelegramPhoto {
    param(
        [string]$BotToken,
        [string]$ChatId,
        [int]$MessageId,
        [string]$PhotoPath,
        [string]$Caption
    )
    
    $url = "https://api.telegram.org/bot$BotToken/editMessageMedia"
    
    try {
        # Retry reading file with delays
        $photoBytes = $null
        $maxRetries = 5
        $retryDelay = 200
        
        for ($i = 0; $i -lt $maxRetries; $i++) {
            try {
                $photoBytes = [System.IO.File]::ReadAllBytes($PhotoPath)
                break
            }
            catch {
                if ($i -lt ($maxRetries - 1)) {
                    Start-Sleep -Milliseconds $retryDelay
                    $retryDelay *= 2
                }
                else {
                    throw
                }
            }
        }
        
        if (-not $photoBytes) {
            throw "Failed to read photo after $maxRetries attempts"
        }
        
        $boundary = [System.Guid]::NewGuid().ToString()
        $fileName = Split-Path $PhotoPath -Leaf
        
        $media = @{
            type    = "photo"
            media   = "attach://photo"
            caption = $Caption
        } | ConvertTo-Json -Compress
        
        $bodyLines = @(
            "--$boundary",
            "Content-Disposition: form-data; name=`"chat_id`"",
            "",
            $ChatId,
            "--$boundary",
            "Content-Disposition: form-data; name=`"message_id`"",
            "",
            $MessageId.ToString(),
            "--$boundary",
            "Content-Disposition: form-data; name=`"media`"",
            "",
            $media,
            "--$boundary",
            "Content-Disposition: form-data; name=`"photo`"; filename=`"$fileName`"",
            "Content-Type: image/png",
            ""
        ) -join "`r`n"
        
        $bodyEnd = "`r`n--$boundary--`r`n"
        
        $encoding = [System.Text.Encoding]::UTF8
        $bodyStart = $encoding.GetBytes($bodyLines + "`r`n")
        $bodyEndBytes = $encoding.GetBytes($bodyEnd)
        
        $memStream = New-Object System.IO.MemoryStream
        $memStream.Write($bodyStart, 0, $bodyStart.Length)
        $memStream.Write($photoBytes, 0, $photoBytes.Length)
        $memStream.Write($bodyEndBytes, 0, $bodyEndBytes.Length)
        
        $requestBody = $memStream.ToArray()
        $memStream.Close()
        
        $response = Invoke-RestMethod -Uri $url -Method Post -Body $requestBody -ContentType "multipart/form-data; boundary=$boundary"
        
        if ($response.ok) {
            return $true
        }
        else {
            Write-Host "[WARN] Failed to update message: $($response.description)" -ForegroundColor Yellow
            return $false
        }
    }
    catch {
        Write-Host "[WARN] Update failed (message may be deleted): $_" -ForegroundColor Yellow
        return $false
    }
}

# --- Main Processing ---

function Process-Manifest {
    param(
        [object]$Config,
        [hashtable]$State,
        [string]$StatePath
    )
    
    $manifestPath = $Config.manifestPath
    
    if (-not (Test-Path $manifestPath)) {
        if ($Verbose) { Write-Host "[DEBUG] Manifest not found: $manifestPath" }
        return $State
    }
    
    try {
        $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
    }
    catch {
        Write-Host "[WARN] Failed to parse manifest: $_" -ForegroundColor Yellow
        return $State
    }
    
    # Check if Telegram is enabled in Sierra Chart
    if ($manifest.telegramEnabled -eq $false) {
        if ($Verbose) { Write-Host "[DEBUG] Telegram disabled in Sierra Chart, skipping..." }
        return $State
    }
    
    # Handle empty charts array (from button clicks)
    if (-not $manifest.charts -or $manifest.charts.Count -eq 0) {
        if ($Verbose) { Write-Host "[DEBUG] No charts in manifest, skipping..." }
        return $State
    }
    
    $stateChanged = $false
    
    foreach ($chart in $manifest.charts) {
        $chartNum = $chart.chartNumber.ToString()
        $imagePath = $chart.imagePath
        $chartName = $chart.name
        
        if (-not (Test-Path $imagePath)) {
            Write-Host "[WARN] Image not found: $imagePath" -ForegroundColor Yellow
            continue
        }
        
        # Check if we have an existing message for this chart
        $existingMessageId = $null
        if ($State.messageIds.ContainsKey($chartNum)) {
            $existingMessageId = $State.messageIds[$chartNum]
            if ($Verbose) {
                Write-Host "[DEBUG] Found existing message ID $existingMessageId for Chart $chartNum" -ForegroundColor DarkGray
            }
        }
        
        $caption = "$chartName`n$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
        
        if ($existingMessageId) {
            # Try to update existing message
            Write-Host "[INFO] Updating Chart $chartNum..." -ForegroundColor Cyan
            $updated = Update-TelegramPhoto -BotToken $Config.telegramBotToken -ChatId $Config.telegramChatId -MessageId $existingMessageId -PhotoPath $imagePath -Caption $caption
            
            if (-not $updated) {
                # Update failed, send new message
                Write-Host "[INFO] Sending new message for Chart $chartNum..." -ForegroundColor Green
                $newMessageId = Send-TelegramPhoto -BotToken $Config.telegramBotToken -ChatId $Config.telegramChatId -PhotoPath $imagePath -Caption $caption
                if ($newMessageId) {
                    $State.messageIds[$chartNum] = $newMessageId
                    $stateChanged = $true
                    Write-Host "[INFO] Saved new message ID $newMessageId for Chart $chartNum" -ForegroundColor Gray
                }
            }
        }
        else {
            # First time - send new message
            Write-Host "[INFO] Sending first message for Chart $chartNum - $chartName" -ForegroundColor Green
            $newMessageId = Send-TelegramPhoto -BotToken $Config.telegramBotToken -ChatId $Config.telegramChatId -PhotoPath $imagePath -Caption $caption
            if ($newMessageId) {
                $State.messageIds[$chartNum] = $newMessageId
                $stateChanged = $true
                Write-Host "[INFO] Saved message ID $newMessageId for Chart $chartNum" -ForegroundColor Gray
            }
        }
    }
    
    # Only save if state actually changed
    if ($stateChanged) {
        [void](Save-State -StatePath $StatePath -State $State)
    }
    
    # Return raw hashtable (no comma needed if pipeline is clean)
    return $State
}

# --- Main Loop ---

Write-Host "======================================" -ForegroundColor Cyan
Write-Host "  Telegram Chart Snapshot Watcher" -ForegroundColor Cyan
Write-Host "  (Resilient Version)" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

$config = Load-Config -Path $ConfigPath
Write-Host "[OK] Config loaded" -ForegroundColor Green
Write-Host "[INFO] Watching: $($config.manifestPath)" -ForegroundColor Gray

$statePath = Get-StatePath -ManifestPath $config.manifestPath
$stateFileExists = Test-Path $statePath
$state = Load-State -StatePath $statePath

if ($state.messageIds.Count -gt 0) {
    Write-Host "[INFO] Loaded state with $($state.messageIds.Count) saved message(s)" -ForegroundColor Gray
}
else {
    # No state file - clear the chat to start fresh
    if (-not $stateFileExists) {
        Clear-TelegramChat -BotToken $config.telegramBotToken -ChatId $config.telegramChatId
    }
}

$lastModified = $null

Write-Host "[INFO] Press Ctrl+C to stop" -ForegroundColor Yellow
Write-Host ""

# Register Ctrl+C handler
$null = Register-EngineEvent -SourceIdentifier PowerShell.Exiting -Action {
    $script:Running = $false
}

try {
    while ($script:Running) {
        try {
            if (Test-Path $config.manifestPath) {
                $currentModified = (Get-Item $config.manifestPath).LastWriteTime
                
                if ($null -eq $lastModified -or $currentModified -gt $lastModified) {
                    if ($null -ne $lastModified) {
                        Write-Host "[INFO] Manifest updated at $currentModified" -ForegroundColor Cyan
                    }
                    $lastModified = $currentModified
                    
                    # Small delay to ensure file is fully written
                    Start-Sleep -Milliseconds 500
                    
                    $state = Process-Manifest -Config $config -State $state -StatePath $statePath
                }
            }
        }
        catch {
            Write-Host "[ERROR] Processing error: $_" -ForegroundColor Red
        }
        
        # Poll every 2 seconds
        Start-Sleep -Seconds 2
    }
}
finally {
    Write-Host "`n[INFO] Watcher stopped" -ForegroundColor Yellow
}
