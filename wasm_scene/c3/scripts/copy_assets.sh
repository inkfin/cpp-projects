#!/usr/bin/env bash

set -euo pipefail

# Resolve project root (parent of scripts/)
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASSETS_SRC="${PROJECT_ROOT}/../assets"
ASSETS_DST="${PROJECT_ROOT}/build"

if [ ! -d "$ASSETS_SRC" ]; then
    echo "No assets directory found at $ASSETS_SRC, skipping."
    exit 0
fi

# rsync: only copy changed files, preserve timestamps
rsync -a --delete "$ASSETS_SRC/" "$ASSETS_DST/assets/"
echo "Assets synced to $ASSETS_DST/assets/"
