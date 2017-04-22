# Hello Cube

This is a simple Android application I wrote to learn the basics of OpenGL ES 2.0. It renders a spinning cube with both texture and normal maps in a perspective projection.

![Screenshot](https://github.com/devint1/HelloCube/raw/master/Screenshot.png)

## Texture Format

I developed a simple, custom texture format that is simple to load via OpenGL. It is as follows:

```C
typedef struct _tex_hdr {
	uint32_t width, height;
	uint32_t format, type;
} tex_hdr;

typedef struct _tex {
	tex_hdr header;
	void *data;
} tex;
```

`data` is the image bitmap data, which is in the format specified in the header. E.g., RGBA, float. I use a simple conversion utility (png2tex) to convert PNG files to this format.
