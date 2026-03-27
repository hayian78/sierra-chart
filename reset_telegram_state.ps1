# Quick fix script - run this to reset state
$statePath = "C:\SierraChart\Exports\telegram_state.json"
if (Test-Path $statePath) {
    Remove-Item $statePath -Force
    Write-Host "Deleted old state file" -ForegroundColor Green
}

# Create fresh state
$freshState = @{
    messageIds = @{}
}
$freshState | ConvertTo-Json | Set-Content $statePath -Encoding UTF8
Write-Host "Created fresh state file at $statePath" -ForegroundColor Green
