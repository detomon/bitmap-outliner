import 'src/bitmap-outliner';

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
</svg>`
);
