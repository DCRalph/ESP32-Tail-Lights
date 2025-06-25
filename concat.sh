#!/usr/bin/env bash
set -Eeuo pipefail

# Default patterns to exclude (relative paths, shell globs):
default_excludes=(
  '*.log'           # log files
  '*~'              # editor backups
  '*.tmp'           # temp files
  '*.bak'           # backups
  '*.swp'           # vim swap files
  'node_modules/*'  # npm deps
  '.git/*'          # git metadata
  '.DS_Store'       # macOS finder
  '*.gif'       # Windows thumbnails
  '*.jpg'           # image files
  '*.jpeg'          # image files
  '*.png'           # image files
  '*.svg'           # image files
  '*.mp3'           # audio files
  '*.avif'          # image files
  '*package-lock.json' # npm lock files
)

# Usage: concat.sh [-e PATTERN]… <directory> [output_file]
#   -e PATTERN    add an exclude pattern (shell glob) against the
#                 relative path
# Defaults: ${default_excludes[*]}
# If output_file is omitted, writes to stdout.

# Start with defaults
excludes=( "${default_excludes[@]}" )

# Parse -e options
while getopts "e:" opt; do
  case $opt in
    e) excludes+=("$OPTARG") ;;
    *) echo "Usage: $0 [-e PATTERN] <dir> [output]" >&2; exit 1 ;;
  esac
done
shift $(( OPTIND - 1 ))

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 [-e PATTERN] <dir> [output]" >&2
  exit 1
fi

dir=${1%/}
out=${2:--}

# Redirect stdout if writing to a file
if [[ "$out" != "-" ]]; then
  exec >"$out"
fi

# Returns 0 if $1 (relative path) matches any exclude pattern
is_excluded() {
  local rel=$1 pat
  for pat in "${excludes[@]}"; do
    if [[ $rel == $pat ]]; then
      return 0
    fi
  done
  return 1
}

# Find & concat
find "$dir" -type f -print0 \
  | sort -z \
  | while IFS= read -r -d '' file; do
      rel=${file#"$dir"/}
      if is_excluded "$rel"; then
        continue
      fi
      printf '==== %s: ====\n' "$rel"
      cat "$file"
      printf '========\n'
    done