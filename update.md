# Note for myself: Update procedure on this application

1. Bump version number in `Makefile`
2. Commit, push
3. Add `git tag` for new version and wait for pipeline
4. Use prepared pre-release, insert changelog as body, then remove pre-release and change to `latest`
