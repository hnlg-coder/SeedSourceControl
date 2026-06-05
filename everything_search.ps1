# Everything Search MCP Server
# PowerShell implementation for Trae MCP integration

param(
    [Parameter(Mandatory=$false)]
    [string]$query = "",
    
    [Parameter(Mandatory=$false)]
    [string]$filter = "",
    
    [Parameter(Mandatory=$false)]
    [int]$max_results = 50,
    
    [Parameter(Mandatory=$false)]
    [string]$sort_by = "name",
    
    [Parameter(Mandatory=$false)]
    [switch]$path_only
)

# Configuration
$EverythingPath = "C:\Program Files\Everything\Everything.exe"

# Function to search using Everything
function Search-Everything {
    param(
        [string]$Query,
        [string]$Filter,
        [int]$MaxResults,
        [string]$SortBy,
        [bool]$PathOnly
    )
    
    $results = @()
    
    # Build search query
    $searchQuery = $Query
    if ($Filter) {
        $searchQuery = "$Query $Filter"
    }
    
    if ($PathOnly) {
        $searchQuery = "$searchQuery path:"
    }
    
    try {
        # Try using Everything HTTP API first
        $uri = "http://localhost:25821/search?q=[$searchQuery]&n=$MaxResults"
        
        try {
            $response = Invoke-RestMethod -Uri $uri -TimeoutSec 5 -UseBasicParsing
            if ($response) {
                foreach ($item in $response) {
                    $results += [PSCustomObject]@{
                        name = $item.name
                        path = $item.path
                        size = $item.size
                        date_modified = $item.date_modified
                        date_created = $item.date_created
                    }
                }
            }
        } catch {
            # Fallback: Use Everything.exe with -search parameter
            Write-Verbose "HTTP API not available, trying CLI method"
            
            $tempFile = "$env:TEMP\everything_search_$PID.txt"
            
            $cliArgs = @(
                "-search", $searchQuery,
                "-output", "csv",
                "-save", $tempFile
            )
            
            $process = Start-Process -FilePath $EverythingPath -ArgumentList $cliArgs -NoNewWindow -Wait -PassThru
            
            Start-Sleep -Milliseconds 500
            
            if (Test-Path $tempFile) {
                $lines = Get-Content $tempFile | Select-Object -First $MaxResults
                
                foreach ($line in $lines) {
                    $parts = $line -split ","
                    if ($parts.Length -ge 2) {
                        $results += [PSCustomObject]@{
                            name = $parts[0]
                            path = $parts[1]
                            size = if ($parts.Length -ge 3) { $parts[2] } else { 0 }
                            date_modified = if ($parts.Length -ge 4) { $parts[3] } else { "" }
                            date_created = ""
                        }
                    }
                }
                
                Remove-Item $tempFile -ErrorAction SilentlyContinue
            }
        }
    } catch {
        Write-Error "Search failed: $_"
    }
    
    return $results
}

# Execute search
$result = Search-Everything -Query $query -Filter $filter -MaxResults $max_results -SortBy $sort_by -PathOnly $path_only

# Output as JSON
$result | ConvertTo-Json -Depth 3
