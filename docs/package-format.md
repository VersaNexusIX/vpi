# Package Format

A `.vers` file is a standard ZIP archive with the `.vers` extension. The rename is a convention to identify Versa packages.

## Registry Index

Before downloading any package, VPI fetches the registry index:

```
GET https://versas.my.id/vers.json
```

This is a JSON array where each entry describes a package:

```json
[
  {
    "nama": "neodax",
    "versi": "1.1.1",
    "developer": "versanexusix",
    "lisensi": "Apache-2.0",
    "description": "mobile first binary analysis framework",
    "_filename": "neodax.vers",
    "_size": 609471,
    "_updatedAt": "2026-05-14T03:16:13.772Z"
  }
]
```

VPI searches this array by `nama`, reads `_filename`, and constructs the download URL:

```
https://versas.my.id/package/<_filename>
```

The installed package name always comes from the `nama` field in `vers.json`, not from the filename of the archive.

If the registry index is unreachable, VPI falls back to:

```
https://versas.my.id/package/<requested_name>.vers
```

## vers.json

Every `.vers` package must contain a `vers.json` file at the root of the archive, or one directory level deep. If this file is absent or missing required fields, installation fails immediately.

### Required Fields

| Field | Type | Description |
|---|---|---|
| `nama` | string | Package identifier, lowercase, no spaces |
| `versi` | string | Version in semver format (X.Y.Z) |
| `developer` | string | Author or organization name |
| `lisensi` | string | License identifier |

### Optional Fields

| Field | Type | Description |
|---|---|---|
| `description` | string | Short description |
| `homepage` | string | Project URL |
| `email` | string | Developer contact |
| `keywords` | string | Comma-separated keywords |
| `type` | string | library, binary, mixed, or script |
| `entry` | string | Entry point path |

### Example

```json
{
  "nama": "mypackage",
  "versi": "1.0.0",
  "developer": "Your Name",
  "lisensi": "MIT",
  "description": "A short description.",
  "homepage": "https://versas.my.id/package/mypackage",
  "type": "binary",
  "entry": "bin/mypackage"
}
```

## Validation

During installation, VPI performs these checks in order:

1. Registry index fetched and package entry located by name.
2. Archive downloaded successfully (HTTP 200).
3. Archive is a valid ZIP and extracts without error.
4. `vers.json` found at root or one level deep.
5. All four required fields are present and non-empty.

Any failure aborts installation and cleans up temporary files.

## Install Location

| Platform | Path |
|---|---|
| Linux | `~/.local/share/vpi/packages/<name>/` |
| macOS | `~/Library/Application Support/vpi/packages/<name>/` |
| Windows | `%APPDATA%\vpi\packages\<name>\` |

Package metadata is stored in a `.meta` subdirectory as a JSON file per package.
