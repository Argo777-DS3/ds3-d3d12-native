# I-0007 operator protocol - selected-candidate producer watch

This is the first bounded runtime discriminator of I-0007. It watches only the
three projected lanes at the canonical selected `A8 -> +0x40 -> +0x70`
candidate and records the exact instruction immediately after each write. It
does not patch the candidate, the final writer, or any motion value.

## Preconditions

- Exact I-0001 executable and the accepted fixed sprint anchor.
- Active Windows 280-Hz display mode for target 165.
- `I0007CandidateWatch.dll`, its injector, and the synthetic debug-register
  restoration test pass before DS3 is launched.
- No earlier probe DLL is loaded in the target process.

## Two strictly ordered passes

1. ChatGPT Work announces the one-sentence discriminator and waits.
2. Thomas launches DS3 and places the character at the accepted sprint anchor.
3. ChatGPT Work prepares target 60, injects the unchanged watch, and confirms
   `READY`.
4. Thomas presses `F4`, then holds AZERTY `Z+Space` with neutral controller
   through the one-delta-second warmup and three-delta-second measurement.
5. ChatGPT Work stops the probe and accepts the pass only if it records
   `COMPLETE`, exact timing and debug-register restoration, no overflow, one
   complete stimulus, and valid candidate addresses and writes.
6. Thomas closes DS3 cleanly.
7. Steps 1-6 are repeated at target 165 with the same DLL hash and stimulus.
8. Only then are writer identity, operands, and output values compared.

Any invalid chain, pointer change, overflow, missing write, cadence failure, or
restoration failure stops the acquisition without interpretation.
