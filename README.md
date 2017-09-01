# Bitmap Outliner

The algorithm converts a bitmap to vector paths enclosing the pixel groups.

![Conversion Diagram](conversion-diagram.svg)

*The outlined paths on the right side are slightly shifted to show their way around the pixels; they will, of course, be aligned with the pixel borders.*

## Example

Given the following bitmap from the above image:

```c
uint8_t const map[] = {
	0, 1, 1, 1 ,0, 0,
	1, 0, 1, 0 ,0, 1,
	1, 1, 0, 0 ,1, 1,
	1, 0, 0, 1 ,0, 1,
	0, 0, 1, 0 ,1, 1,
	1, 0, 1, 1 ,1, 0,
};
```

The generated SVG path will look like this:

```
# Pixel group at top left
M 1 0h3v1h-1v1h-1v-1h-1v1h1v1h-1v1h-1v-3h1z

# Pixel group at bottom right
M 5 1h1v4h-1v1h-3v-2h1v1h1v-1h1v-1h-1v1h-1v-1h1v-1h1z

# Single pixel at bottom left
M 0 5h1v1h-1z
```

Full SVG:

```xml
<svg viewBox="0 0 6 6" xmlns="http://www.w3.org/2000/svg">
	<path d="
		M 1 0h3v1h-1v1h-1v-1h-1v1h1v1h-1v1h-1v-3h1z
		M 5 1h1v4h-1v1h-3v-2h1v1h1v-1h1v-1h-1v1h-1v-1h1v-1h1z
		M 0 5h1v1h-1z"
		fill="#000" fill-rule="evenodd"/>
</svg>
```
