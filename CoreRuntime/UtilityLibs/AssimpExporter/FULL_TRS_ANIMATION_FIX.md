# Complete local TRS animation compatibility

Assimp 6.0.x creates translation, rotation, and scaling curve nodes for every
`aiNodeAnim`. Omitting a component, reducing it to the model rest value, or
replacing it with a one-key fallback can cause the FBX writer/importer path to
use a decomposed world transform as a local default. Child bones then receive
incorrect local transforms and animations deform after import into Unity.

The exporter now writes complete local T/R/S arrays at every baked sample for
every animated node. Sequence-specific posed channels are preserved even when
they are constant, and non-authored components are sampled from the local NIF
rest transform for the entire clip. Quaternion keys remain hemisphere
continuous. NIF scalar scale is written exactly as `(s, s, s)`.

The post-write Assimp import remains a diagnostic check. A nonzero animation
count mismatch is reported but no longer deletes the FBX, because Assimp 6.0.x
can report a different AnimationStack count during its own round trip. A file
that reads back with zero animations, invalid numeric data, or unsafe scales is
still rejected.
