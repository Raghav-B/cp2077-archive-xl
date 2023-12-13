# ArchiveXL

ArchiveXL is a modding tool that allows you to load custom resources without touching original game files,
thus allowing multiple mods to expand same resources without conflicts.

With the mod you can:

- Load custom entity factories (necessary for item additions)
- Add localization texts that can be used in scripts, resources and TweakDB
- Edit existing localization texts without overwriting original resources
- Override submeshes visibility of entity parts
- Add visual tags to a clothing item
- Spawn widgets from any library without registering dependencies

## Getting Started

### Compatibility

- Cyberpunk 2077 2.1

### Installation

1. Install requirements:
   - [RED4ext](https://docs.red4ext.com/getting-started/installing-red4ext) 1.19.0+
2. Extract the release archive `ArchiveXL-x.x.x.zip` into the Cyberpunk 2077 directory.

## Documentation

- [Dynamic appearances](https://github.com/psiberx/cp2077-archive-xl/wiki#dynamic-appearances)
- [Body types](https://github.com/psiberx/cp2077-archive-xl/wiki#body-types)
- [Appearance suffixes](https://github.com/psiberx/cp2077-archive-xl/wiki#appearance-suffixes)
- [Components overrides](https://github.com/psiberx/cp2077-archive-xl/wiki#components-overrides)
- [Visual tags](https://github.com/psiberx/cp2077-archive-xl/wiki#visual-tags)
- [Extending resources](https://github.com/psiberx/cp2077-archive-xl/wiki#extending-resources)

## Building from Source

1. Install `xmake` using
   ```
   Invoke-Expression (Invoke-Webrequest 'https://xmake.io/psget.text' -UseBasicParsing).Content
   ```
2. Download all submodules using
   ```
   git submodule update --init --recursive
   ```
3. Run `xmake` in the root directory of the project. This will download and install dependencies, and compile the project.
4. Output is in `build/windows/x64/release/`