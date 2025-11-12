#!/bin/bash

generate_commit_message() {
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    local changed_files=$(git diff --cached --name-only | wc -l)
    
    cat << EOF
chore: auto-sync $timestamp

## Statistics
- Files changed: $changed_files
- Timestamp: $timestamp

## Purpose
Automated commit to track work progress and preserve state.

## Type
Scheduled backup commit
EOF
}

cd $HOME/Lunaria || exit 1
git add .

# Only commit if there are changes
if ! git diff-index --quiet HEAD --; then
    git commit -m "$(generate_commit_message)"
    git push
fi