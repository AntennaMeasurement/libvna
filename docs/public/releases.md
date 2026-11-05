# Releases

`VERSION` is the source of the CMake, API, package, documentation and release
version. Use a three-component numeric version, such as `0.1.0`.

1. Update `VERSION` and add notes to the matching `## [0.1.0]` section in
   `CHANGELOG.md`, or to `## [Unreleased]`.
2. Commit changes on the branch to publish. The working tree must be clean,
   including untracked files.
3. Preview and publish with Bash (Git Bash on Windows):

```sh
bash release.sh --dry-run
bash release.sh
```

Publishing requires Git, an `origin` remote, an authenticated GitHub CLI and
permission to push the branch and create releases. The script creates an
annotated `v<version>` tag, atomically pushes branch and tag, and creates a
GitHub release. It does not update files or create commits automatically.
Dry runs do not contact the network or modify the repository.

Release notes use the matching version section first, then `Unreleased`,
then non-merge commit subjects since the previous version tag. Existing
local or remote tags are rejected. Move or clear `Unreleased` notes as part
of preparing the next release so they are not reused unintentionally.

If a push fails, the local tag is retained. If release creation fails after
the push, the branch and tag remain published. Inspect the reported state
and finish recovery manually; rerunning the script rejects the existing tag.

CI tests static/shared SDKs on Windows, Linux and macOS, tests library-only
builds, builds documentation and checks the release script. On a published
release, tested SDK archives are attached automatically. A documentation
deployment destination is not configured; preview locally with
`python -m mkdocs serve`.

Release-script integration tests use temporary local repositories and a
stub GitHub CLI; no real release is published:

```sh
bash tests/test_release.sh
```
