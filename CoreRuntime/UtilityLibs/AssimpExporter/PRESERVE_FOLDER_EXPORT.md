# Model batch output layout

`-all` exports model FBX files into one shared output directory by default.
Generated or copied model textures use that same directory.

For example:

```powershell
AssimpExporter `
  -nif_folder "C:\Game\models" `
  -texture_folder "C:\Game\textures" `
  -output "C:\Export\models" `
  -all
```

With these inputs:

```text
C:\Game\models\monster\M001.nif
C:\Game\models\monster\boss\M500.nif
C:\Game\models\npc\N001.nif
```

The default result is flat:

```text
C:\Export\models\M001.fbx
C:\Export\models\M500.fbx
C:\Export\models\N001.fbx
C:\Export\models\<exported textures>
```

Use `-preserve_folders` to create one folder for each exported FBX and its
textures:

```powershell
AssimpExporter `
  -nif_folder "C:\Game\models" `
  -texture_folder "C:\Game\textures" `
  -output "C:\Export\models" `
  -all `
  -preserve_folders
```

The result becomes:

```text
C:\Export\models\M001\M001.fbx
C:\Export\models\M001\<M001 textures>
C:\Export\models\M500\M500.fbx
C:\Export\models\M500\<M500 textures>
C:\Export\models\N001\N001.fbx
C:\Export\models\N001\<N001 textures>
```

`-per_fbx_folder` and `-mirror_folders` are accepted as compatibility aliases.
The option does not mirror source subdirectories and does not split top-level
objects inside a NIF. Each input asset still produces one FBX.

Explicit non-`-all` model exports keep the historical per-model folder layout.
Terrain export is unchanged: each terrain remains in its own folder and a
recursive `-terrain_folder` scan still mirrors its source directory tree.
