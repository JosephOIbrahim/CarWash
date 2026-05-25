LAST_TASK:        t010,t011,t012,t013,t020 (Phase A research + build-graph fix)
LAST_STEP:        update_beliefs
NEXT_TASK:        blocked on human decisions: d009 output contract (q003/q004), d008 LTX version
                  (q007), d007 vendor stb_image.h. Then t025 -> t021 -> t022/t023/t024 -> Phase C/D.
OPEN_ISSUES:      LTX-2 hardcoded graph is largely wrong (c010); flagship moved to 2.3 (c012);
                  comfyClient.cpp won't compile until stb_image.h vendored (c015); WS default port
                  wrong (c007); single-image img2vid can't give coherent video (c013).
EXIT_STATUS:
  - crit1 (engine mapped in beliefs >=0.8):        met (c001-c005,c015)
  - crit2 (V questions closed/parked):             partial (q001,q002 closed; q003/q004 answered-
                                                   pending-decision; q007/q008/q009 open)
  - crit3 (comfyClient in build, configures):      met for CMake; compile blocked by t025 (stb)
  - crit4 (JSON parser + template workflows+tests): pending (t021,t022,t040)
  - crit5 (determinism consistent + test):         pending (t023,t041)
  - crit6 (delegate instantiates client, full pipeline): pending (t030,t031,t032)
  - crit7 (docs honest):                           pending (t050)
TOKENS_USED:      ~360k (t000 survey 110k + t010/t011/t012 research ~137k + orchestrator)
TOKENS_BUDGET:    null (not set by human)
UPDATED:          2026-05-25
