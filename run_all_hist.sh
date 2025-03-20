#!/bin/bash

input_file="histogram_data_whole.txt"  # Change if needed
output_dir="Analysed"

mkdir -p "$output_dir"  # Ensure output directory exists

while read -r rootfile num1 num2; do
    echo "Processing: $rootfile $num1 $num2"
    
    # Extract filename without path and extension
    base_name=$(basename "$rootfile" .root)
    
    # Run the job script
    ./runjob_hist.sh "$rootfile" "$num1" "$num2"
    
    # Check if the expected output file exists and move it to prevent overwriting
    output_file="${base_name}_hist.root"
    
    if [ -f "$output_file" ]; then
        mv "$output_file" "$output_dir/$output_file"
        echo "Saved: $output_dir/$output_file"
    else
        echo "Error: Output file not found for $rootfile"
    fi
    
done < "$input_file"

