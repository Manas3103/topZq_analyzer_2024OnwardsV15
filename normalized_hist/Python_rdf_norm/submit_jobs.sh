#!/bin/bash

# Minimum required proxy time in hours
MIN_HOURS=12

echo "Checking VOMS proxy status..."

# Get remaining time in seconds
TIME_LEFT=$(voms-proxy-info -timeleft 2>/dev/null)

if [ -z "$TIME_LEFT" ]; then
    echo "No valid proxy found. Creating new proxy..."
    voms-proxy-init -voms cms -valid 192:00
else
    # Convert required hours to seconds
    REQUIRED_SECONDS=$((MIN_HOURS * 3600))

    if [ "$TIME_LEFT" -lt "$REQUIRED_SECONDS" ]; then
        HOURS_LEFT=$((TIME_LEFT / 3600))
        echo "Proxy has only $HOURS_LEFT hours left."
        echo "Renewing proxy..."
        voms-proxy-init -voms cms -valid 192:00
    else
        HOURS_LEFT=$((TIME_LEFT / 3600))
        echo "Proxy is valid for $HOURS_LEFT hours. No renewal needed."
    fi
fi

# If proxy creation failed, exit
if [ $? -ne 0 ]; then
    echo "Proxy creation failed. Exiting."
    exit 1
fi

echo "Cleaning old logs..."
mkdir -p logs
rm -f logs/*

echo "Submitting Condor jobs..."
condor_submit job_hist.submit 
condor_submit job_hist_data.submit 

echo "Done."
