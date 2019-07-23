## Tasks

- Refactor
    - [ ] rename ogmi namespace to ogm::interpreter or something
    - [ ] move `interpreter` to `interpreter/`
- [x] Parsing -> AST
- [x] AST -> bytecode
- [ ] Optimizing bytecode compiler of any sort.
- [x] Execute bytecode
- [x] Short-circuit evaluations
- global variables
    - [x] bytecode compilation
    - [x] execution
    - [x] globalvar
- Finish collision detection
    - [x] Fix the current bugs
    - [ ] Diamond and ellipse collision
    - [ ] pixel-perfect collision detection
    - [x] `place_meeting`, `position_meeting`
    - [x] `place_free`, `position_free`
- Drawing
    - [x] origin offsets
    - [x] model transform
    - [x] transparency
    - [x] `sprite_draw_general`
    - [x] `sprite_draw_ext`, `sprite_draw_part`, etc.
    - [x] set sprite_index from object definition
    - [x] modular arithmetic image_index
    - [x] image_angle, width, height, etc.
- Rooms
    - [x] default instance number (non-reallocable)
    - [x] creation code compilation
    - [x] persistent instances
    - [x] first room
    - [x] background colours
    - [x] tiles
    - [ ] backgrounds
- [x] Tiles
- built-in instance variables
    - [x] direction
    - [x] gravity
    - [x] image_*
    - [x] friction
    - [x] xstart, ystart
    - [x] xprevious
- built-in global variables
    - [x] view_*
- [x] Constants
- [x] Spelling differences support: colour/color, grey/gray, randomise/randomize
- Secondary features
    - [x] alarms
    - [ ] surfaces
    - [ ] fonts
    - [ ] async
- Remaining library functions (`src/interpreter/library/fn_todo.h`)
    - [X] random_*
    - [ ] buffer_*
    - [ ] date_*
    - [ ] draw_*
    - [ ] file_*
    - [ ] gamepad_*
    - [ ] http_*
    - [ ] keyboard_*
    - [ ] asset (`room_*`, `sprite_*`, etc.)
    - [ ] social (`achievement_*`, `facebook_*`, `xboxlive_*`)
- `SparseContiguousMap`
    - [ ] Fix the unit tests for this
Data Structures
    - [ ] stack
    - [ ] queue
    - [x] list
    - [x] map
    - [ ] priority queue
    - [ ] grid
    - [ ] reading/writing/JSON
    - [ ] secure reading/writing
- More error reporting and error cases
- Debugger
    - [ ] breakpoints
    - [ ] profiling
    - [ ] stack trace on error
- Speed optimization
    - [ ] profile interpreting
- [ ] D&D programming support
- [ ] netsim 
- [ ] Audio
- [ ] Particles
- [ ] Physics

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