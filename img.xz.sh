#!/bin/bash

# Check if the user provided a .img.xz file as an argument
if [ -z "$1" ]; then
  echo "Usage: $0 <path_to_img.xz_file>"
  exit 1
fi

# Assign the input file path to a variable
input_file="$1"

# Decompress the .img.xz file and write the output to /mnt/temp/temp.img
sudo xz -d -k -c --ignore-check "$input_file" > /mnt/temp/temp.img

# Check if the command was successful
if [ $? -eq 0 ]; then
  echo "Decompression successful. Output saved to /mnt/temp/temp.img"
else
  echo "Decompression failed."
  exit 1
fi
