## Package dependencies

These packages refer to those in `include/` and `src/`.

All libraries depend on `common`, which has no dependencies. In addition, the following dependencies exist:

- `ast`: *(none)*
- `asset`: *(none)*
- `geometry`: *(none)*
- `bytecode`: ast, asset
- `collision`: asset
- `interpreter`: asset, bytecode, collision
- `project`: ast, asset, bytecode
- `gig`: ast, asset, bytecode