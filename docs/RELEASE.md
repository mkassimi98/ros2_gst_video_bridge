# Release Gate

## Required Gates

1. PR gate
- Build + unit tests + smoke tests must pass.

2. Nightly gate
- Transport/codec matrix script must run and generate report.

3. Release gate
- Soak run >= 30 minutes per target machine class (Jetson/x86/Raspi/generic as available).

## KPIs

Release candidate is blocked if any threshold regresses vs previous stable:
- reconnects/hour
- dropped frames ratio
- startup success ratio
- median and p95 latency estimate

## Rollback

- Keep previous release profile bundle and parameter files.
- Revert to last stable tag and documented config migration pair.
