## Tasks
- [ ] Docker containers that can build x64, x86; win, linux
- [ ] Optimizing bytecode compiler of any sort.
- [ ] structs
  - [ ] struct inheritance
  - [ ] struct instanceof
- [ ] try/catch
- [ ] @-prefixed multiline strings
- [ ] draw_sprite(noone, ...) should not error.
- [ ] auto-infer and settable standards; --std=1.4, --std=2.3, etc. These set other settings in turn e.g. allowing function references.
- The following asset types:
    - [ ] Timelines
    - [ ] Paths
    - [ ] **Datafiles**
- [ ] **D&D programming support** (action\_)
- [ ] **Listener events** (keyboard presses, collision, etc.)
- [ ] Render batching

## Behaviour

These need to be checked to see what the intended behaviour is.

- after destroying an instance, can it be collided with? `place_meeting(x, y, destroyed_instance)`? `instance_position()` at the location of the dead instance?

## Iterative builds

- `.manifest.ogmc` containing the following:
    - list of all local, global, globalvar variables in order (the namespaces)
    - (\*) the list of files which contain globalvar, enum, or macro definitions (needed to check for a change, and also for ease of rebuilding)
- full rebuild occurs if any `identifier` (variable, function name, constant) node could be reinterpreted:
    - *any* changes to enums or macros
    - *any* changes to globalvars, asset indices, bytecode indices, or room instances (i.e. if any room file changed.)
    - these can be detected by checking (a) if any of the files (\*) have been updated, or if while preprocessing any of the new/changed files a global accumulation occurs (macro/enum/globalvar).
- minor rebuild:
    - expands namespaces from the ones in the manifest
    - updates manifest
    - may assign different variable->id mappings than a clean rebuild, but who cares?
    - what needs to be rebuilt?
        - all files which are modified since last time!
