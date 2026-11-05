#!/usr/bin/env bash
set -euo pipefail

source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
test_root=$(mktemp -d)
trap 'rm -rf -- "$test_root"' EXIT
export GIT_CONFIG_NOSYSTEM=1
export GIT_CONFIG_GLOBAL=/dev/null
export GIT_AUTHOR_NAME="Release Test"
export GIT_AUTHOR_EMAIL="release-test@example.invalid"
export GIT_COMMITTER_NAME="$GIT_AUTHOR_NAME"
export GIT_COMMITTER_EMAIL="$GIT_AUTHOR_EMAIL"

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

expect_failure() {
    local expected=$1
    shift
    if output=$("$@" 2>&1); then
        fail "Command unexpectedly succeeded: $*"
    fi
    [[ $output == *"$expected"* ]] || fail "Expected '$expected', got: $output"
}

mkdir "$test_root/repository"
cd "$test_root/repository"
git init --quiet -b main
cp "$source_root/release.sh" release.sh
printf '0.1.0\n' > VERSION
printf '# Changelog\n\n## [Unreleased]\n\n- Test release notes.\n' > CHANGELOG.md
git add release.sh VERSION CHANGELOG.md
git -c commit.gpgsign=false commit --quiet -m "Initial release fixture"
initial_commit=$(git rev-parse HEAD)

output=$(bash release.sh --help)
[[ $output == *"Usage:"* ]] || fail "Help missing."
expect_failure "Unknown argument" bash release.sh --unknown
expect_failure "at most one" bash release.sh --dry-run extra
output=$(bash release.sh --dry-run)
[[ $output == *"v0.1.0"* && $output == *"Test release notes."* ]] ||
    fail "Dry run did not show version and notes."
[[ -z $(git tag) && -z $(git status --porcelain) ]] || fail "Dry run modified repository."
printf 'dirty\n' > untracked.txt
expect_failure "Commit all changes" bash release.sh --dry-run
rm untracked.txt
git tag v0.1.0
expect_failure "already exists" bash release.sh --dry-run
git tag -d v0.1.0 >/dev/null

printf 'invalid\n' > VERSION
expect_failure "major.minor.patch" bash release.sh --dry-run
printf '0.1.0\n' > VERSION
printf '# Changelog\r\n\r\n## [0.1.0]\r\n\r\n- Version-specific notes.\r\n\r\n## [Unreleased]\r\n\r\n- Future notes.\r\n' > CHANGELOG.md
git add CHANGELOG.md
git -c commit.gpgsign=false commit --quiet -m "Add version notes"
output=$(bash release.sh --dry-run)
[[ $output == *"Version-specific notes."* && $output != *"Future notes."* ]] ||
    fail "Version notes did not take precedence."
printf '# Changelog\n\n## [Unreleased]\n' > CHANGELOG.md
git add CHANGELOG.md
git -c commit.gpgsign=false commit --quiet -m "Use commit subjects"
output=$(bash release.sh --dry-run)
[[ $output == *"Use commit subjects"* ]] || fail "Commit-note fallback failed."

git init --quiet --bare "$test_root/origin.git"
git remote add origin "$test_root/origin.git"
mkdir "$test_root/bin"
cat > "$test_root/bin/gh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
if [[ $* == "auth status" ]]; then
    exit 0
fi
if [[ ${1:-} == release && ${2:-} == create && ${3:-} == v* ]]; then
    [[ ${RELEASE_TEST_FAIL_CREATE:-0} == 0 ]] || exit 1
    [[ ${4:-} == --verify-tag && ${5:-} == --title && ${6:-} == "$3" &&
       ${7:-} == --notes-file && -s ${8:-} ]] || exit 1
    cp "$8" "$RELEASE_TEST_NOTES"
    exit 0
fi
printf 'Unexpected gh invocation: %s\n' "$*" >&2
exit 1
EOF
chmod +x "$test_root/bin/gh"
export PATH="$test_root/bin:$PATH"
export RELEASE_TEST_NOTES="$test_root/published-notes"
bash release.sh
[[ $(git cat-file -t v0.1.0) == tag ]] || fail "Release tag is not annotated."
[[ $(git --git-dir="$test_root/origin.git" rev-parse refs/heads/main) == "$(git rev-parse HEAD)" ]] ||
    fail "Branch was not pushed."
[[ $(git --git-dir="$test_root/origin.git" rev-parse 'refs/tags/v0.1.0^{}') == "$(git rev-parse HEAD)" ]] ||
    fail "Tag was not pushed."
[[ -s $RELEASE_TEST_NOTES && -z $(git status --porcelain) ]] ||
    fail "Release notes missing or worktree modified."
[[ $(git rev-list --count "$initial_commit..HEAD") == 2 ]] || fail "Release created unexpected commits."
git tag -d v0.1.0 >/dev/null
expect_failure "already exists on origin" bash release.sh

printf '0.2.0\n' > VERSION
git add VERSION
git -c commit.gpgsign=false commit --quiet -m "Prepare next version"
printf '#!/bin/sh\nexit 1\n' > "$test_root/origin.git/hooks/pre-receive"
chmod +x "$test_root/origin.git/hooks/pre-receive"
expect_failure "Local tag v0.2.0 was retained" bash release.sh
git show-ref --verify --quiet refs/tags/v0.2.0 || fail "Failed push lost the local tag."
if git --git-dir="$test_root/origin.git" show-ref --verify --quiet refs/tags/v0.2.0; then
    fail "Rejected atomic push published the tag."
fi
rm "$test_root/origin.git/hooks/pre-receive"
git tag -d v0.2.0 >/dev/null
export RELEASE_TEST_FAIL_CREATE=1
expect_failure "release creation failed" bash release.sh
git --git-dir="$test_root/origin.git" show-ref --verify --quiet refs/tags/v0.2.0 ||
    fail "Failed release did not retain the published tag."
git switch --quiet --detach HEAD
expect_failure "Check out a branch" bash release.sh --dry-run
printf 'PASS: release validation, dry-run, CRLF/commit notes, publishing and failure recovery.\n'
