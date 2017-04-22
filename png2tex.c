#include <png.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#pragma pack(0)
typedef struct _tex_hdr {
	uint32_t width, height;
	uint32_t format, type;
} tex_hdr;

void abort_(const char * s, ...) {
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

int main(int argc, char **argv) {
	int x, y;
	unsigned char header[8];
	int width, height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
	int number_of_passes;
	png_bytep *row_pointers;
	char *src_file_name, *dest_file_name;
	FILE *src_fp, *dst_fp;
	tex_hdr hdr;
	
	src_file_name = argv[1];
	dest_file_name = argv[2];
	
	/* open file and test for it being a png */
	src_fp = fopen(src_file_name, "rb");
	if (!src_fp) {
		abort_("[read_png_file] File %s could not be opened for reading", src_file_name);
	}
	fread(header, 1, 8, src_fp);
	if (png_sig_cmp(header, 0, 8)) {
		abort_("[read_png_file] File %s is not recognized as a PNG file", src_file_name);
	}
	
	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr) {
		abort_("[read_png_file] png_create_read_struct failed");
	}
	
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		abort_("[read_png_file] png_create_info_struct failed");
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) {
		abort_("[read_png_file] Error during init_io");
	}
	
	png_init_io(png_ptr, src_fp);
	png_set_sig_bytes(png_ptr, 8);
	
	png_read_info(png_ptr, info_ptr);
	
	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	
	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	
	
	/* read file */
	if (setjmp(png_jmpbuf(png_ptr))) {
		abort_("[read_png_file] Error during read_image");
	}
	
	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++) {
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
	}
	
	png_read_image(png_ptr, row_pointers);
	
	if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB) {
		hdr.format = 0x1907;
	} else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGBA) {
		hdr.format = 0x1908;
	} else {
		abort_("Invalid format: %d\n", hdr.format);
	}

	hdr.width = width;
	hdr.height = height;
	hdr.type = 0x1401;
	dst_fp = fopen(dest_file_name, "wb");
	fwrite(&hdr, sizeof(hdr), 1, dst_fp);
	for (y = 0; y < height; ++y) {
		png_byte *row = row_pointers[y];
		fwrite(row, sizeof(png_byte), width * (hdr.format == 0x1907 ? 3 : 4),
			   dst_fp);
	}
	
	fclose(src_fp);
	fclose(dst_fp);
}
