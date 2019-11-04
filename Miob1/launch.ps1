$INPUT_PATH = ""
$EXE_PATH = ""

$files = (ls $INPUT_PATH).FullName
$algos = @(1, 2, 3, 4)
$cores = (Get-WmiObject –class Win32_processor).NumberOfLogicalProcessors
$time = 10

$Block = {
    Param(
        [string] $exe_path,
        [string] $file_name,
        [int] $algo,
        [int] $time
    )

    $name = (Get-Item $file_name).Basename
    $path = (Get-Item $exe_path).Directory

    & "$exe_path" $file_name $algo $time >"$path\results\$name $algo $time.txt"
}

# get results path
$path = (Get-Item $EXE_PATH).Directory

# delete old results directory if exists
Remove-Item "$path\results" -Recurse -Force | Out-Null

# create results dir
New-Item -ItemType Directory -Path $path -Name "results" | Out-Null


foreach ($file in $files) {
        foreach ($algo in $algos) {
        $running = @(Get-Job | Where-Object { $_.State -eq 'Running' })
        if ($running.Count -ge $cores) {
            $running | Wait-Job -Any | Out-Null
        }

        Write-Host "Starting job for $file, algorithm: $algo, time: $time"
        Start-Job -Scriptblock $Block -ArgumentList $EXE_PATH, $file, $algo, $time | Out-Null
    }
}

# Wait for all jobs to complete and results ready to be received
Wait-Job * | Out-Null

# Process the results
foreach($job in Get-Job)
{
    $result = Receive-Job $job
    Write-Host $result
}

Remove-Job -State Completed
