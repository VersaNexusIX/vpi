# Creating Packages

## Structure

A `.vers` file is a ZIP archive. The minimum required contents:

```
mypackage.vers (ZIP)
  vers.json
  <your files>
```

`vers.json` must be at the root of the archive or one directory level deep.

## Step 1: Prepare the directory

```sh
mkdir mypackage
cp -r build/output/* mypackage/
```

## Step 2: Create vers.json

Run `vpi init` inside the directory:

```sh
cd mypackage
vpi init
```

Or create it manually:

```json
{
  "nama": "mypackage",
  "versi": "1.0.0",
  "developer": "Your Name",
  "lisensi": "MIT",
  "description": "A short description.",
  "homepage": "https://example.com",
  "type": "binary",
  "entry": "bin/mypackage"
}
```

All four fields `nama`, `versi`, `developer`, and `lisensi` are required. Installation will fail if any are missing.

## Step 3: Create the archive

```sh
cd mypackage
zip -r ../mypackage.vers .
```

Using the helper script, which validates `vers.json` before packing:

```sh
chmod +x scripts/mkvers.sh
./scripts/mkvers.sh ./mypackage mypackage
```

## Step 4: Verify

```sh
unzip -l mypackage.vers
```

Confirm `vers.json` appears at root level, not inside a subdirectory.

## Step 5: Register and upload

Add an entry to the registry index (`versas.my.id/vers.json`):

```json
{
  "nama": "mypackage",
  "versi": "1.0.0",
  "developer": "Your Name",
  "lisensi": "MIT",
  "description": "A short description.",
  "_filename": "mypackage.vers",
  "_size": 12345,
  "_updatedAt": "2026-05-14T00:00:00.000Z"
}
```

Upload the `.vers` file to:

```
https://versas.my.id/package/mypackage.vers
```

Once registered, anyone can install the package with:

```sh
vpi install mypackage
```

## Versioning

Use semantic versioning: `MAJOR.MINOR.PATCH`

- Increment `PATCH` for bug fixes.
- Increment `MINOR` for backward-compatible new features.
- Increment `MAJOR` for breaking changes.

## Naming Rules

- Lowercase letters, numbers, and hyphens only.
- No spaces or underscores.
- The `nama` field in `vers.json` is the authoritative name. The `.vers` filename may differ, as VPI resolves it through the registry index.
