# Bitmap Outliner

The algorithm converts a bitmap to vector paths enclosing the pixel groups.

![Conversion Diagram](assets/conversion-diagram.svg)

*The outlined paths on the right side are slightly shifted to show their way around the pixels; they will, of course, be aligned with the pixel borders.*

## Paths

Given the following bitmap from the above image:

```c
0, 1, 1, 1 ,0, 0,
1, 0, 1, 0 ,0, 1,
1, 1, 0, 0 ,1, 1,
1, 0, 0, 1 ,0, 1,
0, 0, 1, 0 ,1, 1,
1, 0, 1, 1 ,1, 0,
```

The generated SVG path will look like this (line breaks are added to separate the path loops):

```xml
<svg viewBox="0 0 6 6" xmlns="http://www.w3.org/2000/svg">
	<path d="
		M 1 0h3v1h-1v1h-1v-1h-1v1h1v1h-1v1h-1v-3h1z
		M 5 1h1v4h-1v1h-3v-2h1v1h1v-1h1v-1h-1v1h-1v-1h1v-1h1z
		M 0 5h1v1h-1z"
		fill="#000" fill-rule="evenodd"/>
</svg>
```

## C

```c
int const width = 6;
int const height = 6;
uint8_t const data[] = {
	0, 1, 1, 1 ,0, 0,
	1, 0, 1, 0 ,0, 1,
	1, 1, 0, 0 ,1, 1,
	1, 0, 0, 1 ,0, 1,
	0, 0, 1, 0 ,1, 1,
	1, 0, 1, 1 ,1, 0,
};

bmol_outliner* outliner = bmol_alloc(width, height, data);

bmol_outliner_find_paths(outliner, NULL);

size_t path_len = bmol_outliner_svg_path_len(outliner);
char path[path_len];

bmol_outliner_svg_path(outliner, path, path_len);

printf(
	"<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\">\n"
	"	<path d=\"%s\" fill=\"#000\" fill-rule=\"evenodd\"/>\n"
	"</svg>\n", width, height, path);

bmol_free(outliner);
```

## Javascript

```js
const width = 6;
const height = 6;
const data = new Uint8Array([
	0, 1, 1, 1 ,0, 0,
	1, 0, 1, 0 ,0, 1,
	1, 1, 0, 0 ,1, 1,
	1, 0, 0, 1 ,0, 1,
	0, 0, 1, 0 ,1, 1,
	1, 0, 1, 1 ,1, 0,
]);

let outliner = new BitmapOutliner(width, height, data);

let path = outliner.svgPath();

console.log(
`<svg viewBox="0 0 ${width} ${height}" xmlns="http://www.w3.org/2000/svg">
	<path d="${path}" fill="#000" fill-rule="evenodd"/>
</svg>`);
```