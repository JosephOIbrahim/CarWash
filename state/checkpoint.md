LAST_TASK:        t021,t023,t025,t050 (structural layer)
LAST_STEP:        verify (self-review; review-only — no compiler in container)
NEXT_TASK:        PAUSED for human review (per session decision). Resume order: t022 (template
                  workflows, needs q009) -> t024 (networking, needs lib decision) -> Phase C
                  (t030/t031/t032 wiring + clip cache) -> Phase D (t040/t041 tests).
OPEN_ISSUES:      Edits to comfyClient.cpp are NOT compiled here (no Houdini/USD); verify build on
                  a Houdini machine. q009 (depth-seq ingestion + clip lengths) gates t022/t032.
                  Networking-lib dependency decision still deferred (gates t024).
EXIT_STATUS:
  - crit1 (engine mapped >=0.8):                 met (c001-c003,c005, + fixes c016-c019)
  - crit2 (V questions closed/parked):           partial (q001,q002,q005 closed; q003/q004 decided;
                                                 q006,q007 resolved by decision; q008,q009 open)
  - crit3 (comfyClient in build, configures):    met (t020); compile pending Houdini machine (c018)
  - crit4 (JSON parser + template workflows+tests): partial — JSON parser DONE (t021); templates (t022)
                                                 + unit tests (t040) PAUSED
  - crit5 (determinism consistent + test):       partial — seed fix DONE (t023/c016); regression test (t041) PAUSED
  - crit6 (delegate instantiates client, full pipeline): pending (t030/t031/t032, Phase C, PAUSED)
  - crit7 (docs honest):                         met (t050/c005)
TOKENS_USED:      ~540k cumulative (survey + research + structural layer + orchestration)
TOKENS_BUDGET:    null (not set by human)
UPDATED:          2026-05-25
