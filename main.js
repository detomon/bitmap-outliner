import 'src/bitmap-outliner';

// the bitmap size
const width = 6;
const height = 6;

// data can be any indexable array
const data = new Uint8Array([
	0, 1, 1, 1 ,0, 0,
	1, 0, 1, 0 ,0, 1,
	1, 1, 0, 0 ,1, 1,
	1, 0, 0, 1 ,0, 1,
	0, 0, 1, 0 ,1, 1,
	1, 0, 1, 1 ,1, 0,
]);

// create outliner
let outliner = new BitmapOutliner(width, height, data);

// get SVG path; implicitly calls `outliner.findPaths()`
let path = outliner.svgPath();

// output SVG
console.log(
`<svg viewBox="0 0 ${width} ${height}" xmlns="http://www.w3.org/2000/svg">
	<path d="${path}" fill="#000" fill-rule="evenodd"/>
</svg>`);
