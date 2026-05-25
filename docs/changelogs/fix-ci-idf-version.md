# fix/ci-idf-version

## Ändrat

- `.github/workflows/build.yml` — ersatt `esp-idf-ci-action` med direkt installation av IDF 6.1.0 från GitHub
- Lagt till caching av `~/.espressif` och `~/esp-idf` per target för att hålla byggtiden nere
