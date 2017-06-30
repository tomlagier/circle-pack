# Circle-pack

A C++ port of the [d3-heirarchy pack layout](https://github.com/d3/d3-hierarchy/tree/master/src/pack), intended for compilation into WebAssembly.

## Compiling to WASM
1. [Install and set up Emscripten](http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html)
2. Use `emcc` to compile `circle-pack.cpp`:
```
emcc circle-pack.cpp -O3 --bind -o index.html -s WASM=1 -s ALLOW_MEMORY_GROWTH=1
```
3. Include the output `index.js` on your page or in your javascript bundle.
4. `index.js` will request `index.wasm`, make sure it is available for download. If necessary, update the path to `index.wasm` in `index.js`.


## Usage
The exported Hierarchy class exposes an interface very similar to d3-hierarchy.

### `new Module.Hierarchy(options, hierarchy): Hierarchy`

`options`: {
    `size`: [float, float]. Bounding rectangle of circle
    `padding`: (optional, default 0) float or function, the   distance between nodes
    `radius`: (optional) function. Transform the radius of    each node
}

`hierarchy`: {
    value: relative size of node
    children: [...nodes]
}

### `Hierarchy.pack(): HNode`
Performs the circle packing layout on the hierarchy. Returns the root node of the hierarchy.

### `HNode`
```
class HNode {
    double x;
    double y;
    double r;
    HNodeVec children;
    leaves()
}
```

To get the length of `children` use, `node.children.size()`. To get a child by index, use `node.children.get(i)`. To get all leaf children, use `node.leaves()`.

## Example
```
const rootNode = {
    value: 100,
    children: [
        {
            value: 200,
            children: [
                { value: 50 },
                { value: 25 },
                { value: 700 }
            ]
        }
    ]
}

const root = new Module.Hierarchy({
    size: [width, width],
    padding: 1.5
}, rootNode);

const packedChildren = root.pack().leaves();

for (var i = 0; i < packedChildren.size(); i++) {
    canvas.drawCircle(packedChildren.get(i))
}
```