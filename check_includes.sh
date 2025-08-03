#!/bin/bash

set -euo pipefail

echo "🔍 Scanning source files for include directives..."

# List of directories to search for headers
INCLUDE_DIRS=(. algorithm controllers door enum generator layout math model role store data vector)

# Build a searchable file list of headers
HEADER_DB=$(mktemp)
find "${INCLUDE_DIRS[@]}" -type f \( -name "*.hpp" -o -name "*.h" \) -print > "$HEADER_DB"

# Collect all quoted includes from source files
MISSING=()

grep -rhoE '#include\s+"[^"]+"' . \
    --include=\*.{cpp,hpp} \
    --exclude-dir={.git,build,extern} | \
    sed -E 's/#include\s+"([^"]+)"/\1/' | sort -u | while read -r HEADER; do
    FOUND=0
    while read -r CANDIDATE; do
        if [[ "$CANDIDATE" == */"$HEADER" ]]; then
            FOUND=1
            break
        fi
    done < "$HEADER_DB"
    if [[ "$FOUND" -eq 0 ]]; then
        MISSING+=("$HEADER")
    fi
done

rm -f "$HEADER_DB"

# Report results
if [[ ${#MISSING[@]} -eq 0 ]]; then
    echo "✅ All included headers are accounted for."
    exit 0
else
    echo "❌ Missing headers:"
    for FILE in "${MISSING[@]}"; do
        echo " - $FILE"
    done
    exit 1
fi
