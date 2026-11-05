#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: bash release.sh [--dry-run]

Publish the version in VERSION using an annotated tag and a GitHub release.
Requires a clean committed branch, origin, Git and authenticated GitHub CLI.
Release notes come from the matching CHANGELOG.md section, Unreleased, or
non-merge commit subjects since the previous version tag, in that order.
The CI workflow builds, tests and attaches the native SDK packages.

--dry-run  Preview without modifying files, accessing the network or publishing.
--help     Show this help.
EOF
}

fail() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

[[ $# -le 1 ]] || fail "Expected at most one argument."
dry_run=false
case "${1:-}" in
    --help) usage; exit 0 ;;
    --dry-run) dry_run=true ;;
    "") ;;
    *) fail "Unknown argument: $1" ;;
esac

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
git rev-parse --show-toplevel >/dev/null
version=$(tr -d '\r\n' < VERSION)
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] ||
    fail "VERSION must contain a major.minor.patch version."
tag="v$version"
[[ -z $(git status --porcelain --untracked-files=normal) ]] ||
    fail "Commit all changes, including untracked files, before releasing."
branch=$(git symbolic-ref --quiet --short HEAD) ||
    fail "Check out a branch before releasing."
git rev-parse --verify HEAD >/dev/null
if git show-ref --verify --quiet "refs/tags/$tag"; then
    fail "Tag $tag already exists."
fi

notes=$(awk -v heading="## [$version]" '
    { sub(/\r$/, "") }
    /^## / {
        if (active) exit
        active = ($0 == heading || index($0, heading " ") == 1)
        next
    }
    active { print }
' CHANGELOG.md)
if [[ -z ${notes//[[:space:]]/} ]]; then
    notes=$(awk '
        { sub(/\r$/, "") }
        /^## / {
            if (active) exit
            active = ($0 ~ /^## \[Unreleased\]/)
            next
        }
        active { print }
    ' CHANGELOG.md)
fi
if [[ -z ${notes//[[:space:]]/} ]]; then
    previous_tag=$(git describe --tags --abbrev=0 --match 'v[0-9]*' HEAD 2>/dev/null) ||
        previous_tag=''
    if [[ -n $previous_tag ]]; then
        notes=$(git log --no-merges --format='- %s' "$previous_tag..HEAD")
    else
        notes=$(git log --no-merges --format='- %s' HEAD)
    fi
fi
[[ -n ${notes//[[:space:]]/} ]] || fail "No release notes or commits to publish."
printf 'Release %s from %s\n\n%s\n' "$tag" "$branch" "$notes"
if [[ $dry_run == true ]]; then
    printf '\nDry run: would push branch and annotated tag atomically, then create the GitHub release.\n'
    exit 0
fi

command -v gh >/dev/null || fail "GitHub CLI (gh) is required."
gh auth status
git remote get-url origin >/dev/null
remote_tag=$(git ls-remote --tags origin "refs/tags/$tag")
[[ -z $remote_tag ]] || fail "Tag $tag already exists on origin."
notes_file=$(mktemp)
trap 'rm -f -- "$notes_file"' EXIT
printf '%s\n' "$notes" > "$notes_file"
git tag -a "$tag" -m "Release $tag"
if ! git push --atomic origin "HEAD:refs/heads/$branch" "refs/tags/$tag"; then
    fail "Push failed. Local tag $tag was retained; inspect and recover manually."
fi
if ! gh release create "$tag" --verify-tag --title "$tag" --notes-file "$notes_file"; then
    fail "Branch and tag were pushed, but release creation failed. Create the release for $tag manually."
fi
printf 'Released %s. CI will attach the tested SDK packages.\n' "$tag"
