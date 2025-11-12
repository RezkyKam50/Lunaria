#!/bin/bash

generate_commit_message() {
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    local changed_files=$(git diff --cached --name-only | wc -l)
    
    cat << EOF
chore: auto-sync $timestamp

> Summary
- Files changed: $changed_files
- Timestamp: $timestamp

> Purpose
Preserve latest local state.

> Type
Scheduled commit every 1:00AM Jakarta Time.

EOF
}

cd $HOME/Lunaria || exit 1
git add .

# Only commit if there are changes
if ! git diff-index --quiet HEAD --; then
    git commit -m "$(generate_commit_message)"
    git push
fi