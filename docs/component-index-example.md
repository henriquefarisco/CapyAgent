# Component index example

This is a descriptive alpha example for GitHub tag-release distribution without certificates/signatures.

```json
{
  "schema": "capyos-component-index-v1",
  "channel": "testing",
  "components": [
    {
      "id": "org.capyos.codecs.image-basic",
      "name": "CapyCodecs Image Basic",
      "kind": "codec",
      "channel": "testing",
      "tag": "v0.0.1",
      "artifact": "capycodecs-image-basic-v0.0.1.capypkg",
      "sha256": "0000000000000000000000000000000000000000000000000000000000000000",
      "activation_class": "atomic",
      "required_abis": [
        { "name": "capyos-base", "minimum_version": 3 },
        { "name": "capy-codec-image", "minimum_version": 1 }
      ],
      "dependencies": [],
      "permissions": ["decode.image"]
    },
    {
      "id": "org.capyos.browser.core",
      "name": "CapyBrowser Core",
      "kind": "browser-core",
      "channel": "testing",
      "tag": "v0.0.1",
      "artifact": "capybrowser-core-v0.0.1.capypkg",
      "sha256": "0000000000000000000000000000000000000000000000000000000000000000",
      "activation_class": "atomic",
      "required_abis": [
        { "name": "capyos-base", "minimum_version": 3 },
        { "name": "capy-browser-core", "minimum_version": 1 }
      ],
      "dependencies": ["org.capyos.codecs.image-basic"],
      "permissions": ["network.client", "ui.window", "fs.cache"]
    }
  ]
}
```

The zero hashes above are placeholders. A release workflow should replace them with the lowercase sha256 of each release artifact.
