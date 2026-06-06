# feature/readme-ci-linting

## Ändrat

- `.github/workflows/build.yml` — lagt till nytt CI-jobb `cppcheck` som kör statisk kodanalys på `firmware/cam/main` och `firmware/screen/main` vid varje push och PR mot `main`

## Varför

Tidigare CI kontrollerade bara att koden kompilerade. Det fångar syntaxfel men inte logikfel som null-pointer-avläsningar, potentiella buffer overflows, oanvända variabler eller portabilitetsproblem. Statisk analys hittar den typen av fel utan att hårdvaran behöver vara inkopplad.

## Nytta

- Fel flaggas automatiskt i PR:en innan någon mergar — ingen behöver granska koden manuellt för uppenbara misstag
- `--suppress=missingIncludeSystem` gör att ESP-IDF-headers som saknas på CI-servern inte genererar falska varningar
- `--inline-suppr` låter utvecklare tysta enskilda varningar med en kommentar direkt i koden när de är medvetna om en false positive
- Jobbet är fristående från byggstegen och körs parallellt, så det bromsar inte CI-tiden
