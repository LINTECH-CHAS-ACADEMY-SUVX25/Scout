# fix/ci-idf-version

## Ändrat

- `.github/workflows/build.yml` — ersatt `esp-idf-ci-action` med direkt installation av IDF v6.0.1 från GitHub (v6.1.0 saknar publik tag/Docker-image)
- Lagt till caching av `~/.espressif` och `~/esp-idf` per target för att hålla byggtiden nere
- `firmware/screen/main/video.c` — cast `out.width`/`out.height` till `uint32_t` i `ESP_LOGD`, fixar `-Werror=format=`
