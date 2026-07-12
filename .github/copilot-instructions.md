# Copilot Instructions

## Project Guidelines
- The user prefers uninterrupted execution and does not want confirmation pauses during implementation. Implementation should continue uninterrupted through all steps once a plan is approved.
- For this workflow, proceed through planned implementation phases directly without pausing to ask for confirmation or validation prompts.
- User prefers not to be asked for validation during phased native bridge work and wants implementation to continue through the requested phase automatically.
- For this codebase analysis, do not use the Native project as evidence for runtime/export capabilities; it is only for exporting to C then C#.
- Prioritize NIFToolset-native API compatibility in the C bridge to improve C# compatibility.
- Use Build\x86-debug (local) for compiling the native bridge when a DLL output is desired.
- For this codebase, avoid wrapping NiApplication directly; expose lower-level rendering pieces so the application flow can be reproduced later in C#.
- For this codebase, LOD0 should be treated as the highest-quality LOD when diagnosing NiRangeLODData behavior.