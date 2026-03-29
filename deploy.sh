#!/bin/bash

# Configuration
TARGET_DIR="/mnt/c/SierraChart/ACS_Source"
FILENAME="$1"

# Check if a filename was provided
if [ -z "$FILENAME" ]; then
    echo "Usage: ./deploy.sh <filename.cpp>"
    echo ""
    echo "Available studies to deploy:"
    ls -1 *.cpp 2>/dev/null | sed 's/^/  - /'
    exit 0
fi

# Check if the source file exists locally
if [ ! -f "$FILENAME" ]; then
    echo "Error: Source file '$FILENAME' not found in current directory."
    echo ""
    echo "Available studies:"
    ls -1 *.cpp 2>/dev/null | sed 's/^/  - /'
    exit 1
fi

TARGET_PATH="$TARGET_DIR/$FILENAME"
ORIG_PATH="$TARGET_PATH.orig"

# 1. Handle the 'original' backup
if [ -f "$TARGET_PATH" ]; then
    if [ -f "$ORIG_PATH" ]; then
        echo "Backup file '$ORIG_PATH' already exists."
        read -p "Overwrite existing backup? (y/N): " choice
        case "$choice" in 
          y|Y ) 
            echo "Overwriting backup: $ORIG_PATH"
            cp "$TARGET_PATH" "$ORIG_PATH"
            ;;
          * ) 
            echo "Skipping backup. Existing '$ORIG_PATH' preserved."
            ;;
        esac
    else
        echo "Creating one-time backup of original: $ORIG_PATH"
        cp "$TARGET_PATH" "$ORIG_PATH"
    fi
fi

# 2. Deploy the current file
echo "Deploying '$FILENAME' to '$TARGET_DIR'..."
cp "$FILENAME" "$TARGET_PATH"

if [ $? -eq 0 ]; then
    echo "Successfully deployed $FILENAME."
else
    echo "Error: Deployment failed. Check permissions for $TARGET_DIR."
fi
